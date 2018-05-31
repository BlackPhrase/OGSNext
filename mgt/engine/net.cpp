/*
*	This file is part of Magenta Engine
*
*	Copyright (C) 1996-1997 Id Software, Inc.
*	Copyright (C) 2018 BlackPhrase
*
*	Magenta Engine is free software: you can redistribute it and/or modify
*	it under the terms of the GNU General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	Magenta Engine is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with Magenta Engine. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file

// net_ws.c

#include "quakedef.h"
#include "network/INetwork.hpp"

#ifdef _WIN32
#include "winquake.h"
#endif

void *gpNetworkLib{ nullptr };
INetwork *gpNetwork{ nullptr };

netadr_t net_local_adr;

netadr_t net_from;
sizebuf_t net_message;

#ifdef _WIN32

int net_socket;

#define MAX_UDP_PACKET (MAX_MSGLEN * 2) // one more than msg + header

#elif __linux__

int net_socket;      // non blocking, for receives
int net_send_socket; // blocking, for sends

#define MAX_UDP_PACKET 8192

int gethostname(char *, int);
int close(int);

#endif

byte net_message_buffer[MAX_UDP_PACKET];

#ifdef _WIN32
WSADATA winsockdata;
#endif

//=============================================================================

bool InitNetwork()
{
	gpNetworkLib = Sys_LoadModule("network");

	if(!gpNetworkLib)
		return false;

	auto pfnNetworkFactory{ Sys_GetFactory(gpNetworkLib) };

	if(!pfnNetworkFactory)
		return false;

	gpNetwork = (INetwork*)pfnNetworkFactory(MGT_NETWORK_INTERFACE_VERSION, nullptr);

	if(!gpNetwork)
		return false;
};

void ShutdownNetwork()
{
	if(gpNetwork)
		gpNetwork->Shutdown();

	gpNetwork = nullptr;

	if(gpNetworkLib)
	{
		Sys_UnloadModule(gpNetworkLib);
		gpNetworkLib = nullptr;
	};
};

//=============================================================================

void NetadrToSockadr(netadr_t *a, struct sockaddr_in *s)
{
	memset(s, 0, sizeof(*s));
	s->sin_family = AF_INET;

	*(int *)&s->sin_addr = *(int *)&a->ip;
	s->sin_port = a->port;
};

void SockadrToNetadr(struct sockaddr_in *s, netadr_t *a)
{
	*(int *)&a->ip = *(int *)&s->sin_addr;
	a->port = s->sin_port;
};

bool NET_CompareBaseAdr(netadr_t a, netadr_t b)
{
	if(a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
		return true;
	return false;
};

bool NET_CompareAdr(netadr_t a, netadr_t b)
{
	if(a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
};

char *NET_AdrToString(netadr_t a)
{
	static char s[64];
	sprintf(s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));
	return s;
};

char *NET_BaseAdrToString(netadr_t a)
{
	static char s[64];
	sprintf(s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);
	return s;
};

/*
=============
NET_StringToAdr

idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
bool NET_StringToAdr(char *s, netadr_t *a)
{
	struct hostent *h;
	struct sockaddr_in sadr;
	char *colon;
	char copy[128];

	memset(&sadr, 0, sizeof(sadr));
	sadr.sin_family = AF_INET;

	sadr.sin_port = 0;

	strcpy(copy, s);
	// strip off a trailing :port if present
	for(colon = copy; *colon; colon++)
		if(*colon == ':')
		{
			*colon = 0;
			sadr.sin_port = htons((short)atoi(colon + 1)); // TODO: (short)?
		};

	if(copy[0] >= '0' && copy[0] <= '9')
	{
		*(int *)&sadr.sin_addr = inet_addr(copy);
	}
	else
	{
#ifdef _WIN32
		if((h = gethostbyname(copy)) == 0)
#else
		if(!(h = gethostbyname(copy)))
#endif
			return 0;
		*(int *)&sadr.sin_addr = *(int *)h->h_addr_list[0];
	};

	SockadrToNetadr(&sadr, a);

	return true;
};

//=============================================================================

bool NET_GetPacket()
{
	int ret;
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof(from);

#ifdef _WIN32
	ret = recvfrom(net_socket, (char *)net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);
	SockadrToNetadr(&from, &net_from);

	if(ret == -1)
	{
		int errno = WSAGetLastError();

		if(errno == WSAEWOULDBLOCK)
			return false;
		if(errno == WSAEMSGSIZE)
		{
			Con_Printf("Warning:  Oversize packet from %s\n", NET_AdrToString(net_from));
			return false;
		};

		Sys_Error("NET_GetPacket: %s", strerror(errno));
	};
#elif __linux__
	ret = recvfrom(net_socket, net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);
	if(ret == -1)
	{
		if(errno == EWOULDBLOCK)
			return false;
		if(errno == ECONNREFUSED)
			return false;
		Sys_Printf("NET_GetPacket: %s\n", strerror(errno));
		return false;
	}
#endif

	net_message.cursize = ret;

#ifdef _WIN32	
	if(ret == sizeof(net_message_buffer))
	{
		Con_Printf("Oversize packet from %s\n", NET_AdrToString(net_from));
		return false;
	};
#elif __linux__
	SockadrToNetadr(&from, &net_from);
#endif

	return ret;
};

//=============================================================================

void NET_SendPacket(int length, void *data, netadr_t to)
{
	int ret;
	struct sockaddr_in addr;

	NetadrToSockadr(&to, &addr);

	ret = sendto(net_socket, data, length, 0, (struct sockaddr *)&addr, sizeof(addr));

	if(ret == -1)
	{
#ifdef _WIN32
		int err = WSAGetLastError();

		// wouldblock is silent
		if(err == WSAEWOULDBLOCK)
			return;

#ifndef SERVERONLY
		if(err == WSAEADDRNOTAVAIL)
			Con_DPrintf("NET_SendPacket Warning: %i\n", err);
		else
#endif
			Con_Printf("NET_SendPacket ERROR: %i\n", errno);
#elif __linux__
		if(errno == EWOULDBLOCK)
			return;
		if(errno == ECONNREFUSED)
			return;
		Sys_Printf("NET_SendPacket: %s\n", strerror(errno));
#endif
	};
};

//=============================================================================

int UDP_OpenSocket(int port)
{
#ifdef _WIN32
	int newsocket;
	struct sockaddr_in address;
	unsigned long _true = true;
	int i;

	if((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Error("UDP_OpenSocket: socket:", strerror(errno));

	if(ioctlsocket(newsocket, FIONBIO, &_true) == -1)
		Sys_Error("UDP_OpenSocket: ioctl FIONBIO:", strerror(errno));

	address.sin_family = AF_INET;
	
	//ZOID -- check for interface binding option
	if((i = COM_CheckParm("-ip")) != 0 && i < com_argc)
	{
		address.sin_addr.s_addr = inet_addr(com_argv[i + 1]);
		Con_Printf("Binding to IP Interface Address of %s\n",
		           inet_ntoa(address.sin_addr));
	}
	else
		address.sin_addr.s_addr = INADDR_ANY;

	if(port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
		Sys_Error("UDP_OpenSocket: bind: %s", strerror(errno));

	return newsocket;
#endif
};

void NET_GetLocalAddress()
{
	char buff[512];
	struct sockaddr_in address;
	int namelen;

#ifdef _WIN32
	gethostname(buff, 512);
	buff[512 - 1] = 0;

	NET_StringToAdr(buff, &net_local_adr);

	namelen = sizeof(address);
	if(getsockname(net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error("NET_Init: getsockname:", strerror(errno));
#endif
	net_local_adr.port = address.sin_port;

	Con_Printf("IP address %s\n", NET_AdrToString(net_local_adr));
};

/*
====================
NET_Init
====================
*/
void NET_Init(/*int port*/)
{
	if(!InitNetwork())
		//gpNetwork = new CNetworkNull(); // TODO
		return;

	gpNetwork->Init();

#ifdef _WIN32
	WORD wVersionRequested;
	int r;

	wVersionRequested = MAKEWORD(1, 1);

	r = WSAStartup(MAKEWORD(1, 1), &winsockdata);

	if(r)
		Sys_Error("Winsock initialization failed.");
#endif

	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket(port);

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.data = net_message_buffer;

	//
	// determine my name & address
	//
	NET_GetLocalAddress();

	Con_Printf("UDP Initialized\n");
};

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	ShutdownNetwork();

#ifdef _WIN32
	closesocket(net_socket);
	WSACleanup();
#elif __linux__
	close(net_socket);
#endif
};

/*
// Returns true if we can't bind the address locally--in other words,
// the IP is NOT one of our interfaces.
bool NET_IsClientLegal(netadr_t *adr)
{
	struct sockaddr_in sadr;
	int newsocket;

#if 0
	if (adr->ip[0] == 127)
		return false; // no local connections period

	NetadrToSockadr (adr, &sadr);

	if ((newsocket = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		Sys_Error ("NET_IsClientLegal: socket:", strerror(errno));

	sadr.sin_port = 0;

	if( bind (newsocket, (void *)&sadr, sizeof(sadr)) == -1) 
	{
		// It is not a local address
		close(newsocket);
		return true;
	}
	close(newsocket);
	return false;
#else
	return true;
#endif
}
*/