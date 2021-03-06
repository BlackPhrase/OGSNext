/*
*	This file is part of Magenta Engine
*
*	Copyright (C) 1996-1997 Id Software, Inc.
*	Copyright (C) 2018-2019 BlackPhrase
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

#include <cstdio>
#include "NetworkSystem.hpp"
#include "engine/ISystem.hpp"
#include "engine/ICvarRegistry.hpp"
#include "engine/IConVarController.hpp"
#include "cvardef.h"

#ifdef _WIN32
#include "winquake.h"
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#if defined(sun)
#include <unistd.h>
#endif

#ifdef sun
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

//=============================================================================

netadr_t net_local_adr{};

netadr_t net_from{};
sizebuf_t net_message{};

cvar_t net_showpackets = { "net_showpackets", "0" };
cvar_t net_showdrop = { "net_showdrop", "0" };
cvar_t qport = { "qport", "0" };

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

//=============================================================================

EXPOSE_SINGLE_INTERFACE(CNetworkSystem, INetworkSystem, MGT_NETWORKSYSTEM_INTERFACE_VERSION);

CNetworkSystem::CNetworkSystem() = default;
CNetworkSystem::~CNetworkSystem() = default;

/*
====================
NET_Init
====================
*/
bool CNetworkSystem::Init(CreateInterfaceFn afnEngineFactory /*, int port*/)
{
	mpSystem = (ISystem*)afnEngineFactory(MGT_SYSTEM_INTERFACE_VERSION, nullptr);
	
	if(!mpSystem)
	{
		printf("ISystem query failed!\n");
		return false;
	};
	
	mpCvarRegistry = (ICvarRegistry*)afnEngineFactory(MGT_CVARREGISTRY_INTERFACE_VERSION, nullptr);
	
	if(!mpCvarRegistry)
	{
		printf("ICvarRegistry query failed!\n");
		return false;
	};
	
	mpCvarController = (IConVarController*)afnEngineFactory(MGT_CONVARCONTROLLER_INTERFACE_VERSION, nullptr);
	
	if(!mpCvarController)
	{
		printf("IConVarController query failed!\n");
		return false;
	};
	
#ifdef _WIN32
	auto wVersionRequested{MAKEWORD(1, 1)};

	int r{WSAStartup(wVersionRequested, &winsockdata)};

	if(r)
		mpSystem->Error("Winsock initialization failed.");
#endif

	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket(PORT_ANY); // TODO: PORT_CLIENT/PORT_SERVER

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message.data = net_message_buffer;

	//
	// determine my name & address
	//
	GetLocalAddress();

	int port;

// pick a port value that should be nice and random
#ifdef _WIN32
	port = ((int)(timeGetTime() * 1000) * time(nullptr)) & 0xffff;
#else
	port = ((int)(getpid() + getuid() * 1000) * time(nullptr)) & 0xffff;
#endif

	mpCvarRegistry->Register(&net_showpackets);
	mpCvarRegistry->Register(&net_showdrop);
	mpCvarRegistry->Register(&qport);
	
	mpCvarController->SetFloat("qport", port);
	
	mpSystem->Printf("UDP Initialized\n");
	return true;
};

/*
====================
NET_Shutdown
====================
*/
void CNetworkSystem::Shutdown()
{
#ifdef _WIN32
	closesocket(net_socket);
	WSACleanup();
#elif __linux__
	close(net_socket);
#endif
};

//=============================================================================

bool CNetworkSystem::GetPacket(netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int ret;
	struct sockaddr_in from;
	int fromlen;

	fromlen = sizeof(from);

#ifdef _WIN32
	ret = recvfrom(net_socket, (char *)net_message->data, sizeof(net_message->data), 0, (struct sockaddr *)&from, &fromlen);
	SockadrToNetadr(&from, net_from);

	if(ret == -1)
	{
		int err = WSAGetLastError();

		if(err == WSAEWOULDBLOCK)
			return false;
		if(err == WSAEMSGSIZE)
		{
			mpSystem->Printf("Warning:  Oversize packet from %s\n", AdrToString(net_from));
			return false;
		};

		mpSystem->Error("NET_GetPacket: %s", strerror(err));
	};
#elif __linux__
	ret = recvfrom(net_socket, net_message->data, sizeof(net_message->data), 0, (struct sockaddr *)&from, &fromlen);
	if(ret == -1)
	{
		if(errno == EWOULDBLOCK)
			return false;
		if(errno == ECONNREFUSED)
			return false;
		mpSystem->Printf("NET_GetPacket: %s\n", strerror(errno));
		return false;
	}
#endif

	net_message->cursize = ret;

#ifdef _WIN32	
	if(ret == sizeof(net_message_buffer))
	{
		mpSystem->Printf("Oversize packet from %s\n", AdrToString(net_from));
		return false;
	};
#elif __linux__
	SockadrToNetadr(&from, &net_from);
#endif

	return ret;
};

//=============================================================================

void CNetworkSystem::SendPacket(netsrc_t sock, int length, void *data, netadr_t to)
{
	int ret;
	struct sockaddr_in addr;

	NetadrToSockadr(&to, &addr);

	ret = sendto(net_socket, (const char*)data, length, 0, (struct sockaddr *)&addr, sizeof(addr));

	if(ret == -1)
	{
#ifdef _WIN32
		int err = WSAGetLastError();

		// wouldblock is silent
		if(err == WSAEWOULDBLOCK)
			return;

#ifndef SWDS
		if(err == WSAEADDRNOTAVAIL)
			mpSystem->DevPrintf("NET_SendPacket Warning: %i\n", err);
		else
#endif
			mpSystem->Printf("NET_SendPacket ERROR: %i\n", errno);
#elif __linux__
		if(errno == EWOULDBLOCK)
			return;
		if(errno == ECONNREFUSED)
			return;
		mpSystem->Printf("NET_SendPacket: %s\n", strerror(errno));
#endif
	};
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
bool CNetworkSystem::StringToAdr(const char *s, netadr_t *a)
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

int CNetworkSystem::UDP_OpenSocket(int port)
{
	int newsocket;
	struct sockaddr_in address;
#ifdef _WIN32
	unsigned long _true = true;
#else
	bool _true = true;
#endif
	int i;

	if((newsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		mpSystem->Error("UDP_OpenSocket: socket:", strerror(errno));

#ifdef _WIN32
	if(ioctlsocket(newsocket, FIONBIO, &_true) == -1)
#else
	if(ioctl(newsocket, FIONBIO, (char *)&_true) == -1)
#endif
		mpSystem->Error("UDP_OpenSocket: ioctl FIONBIO:", strerror(errno));

	address.sin_family = AF_INET;
	
	// TODO: command line interface
	/*
	//ZOID -- check for interface binding option
	if((i = COM_CheckParm("-ip")) != 0 && i < com_argc)
	{
		address.sin_addr.s_addr = inet_addr(com_argv[i + 1]);
		mpSystem->Printf("Binding to IP Interface Address of %s\n", inet_ntoa(address.sin_addr));
	}
	else
	*/
		address.sin_addr.s_addr = INADDR_ANY;

	if(port == PORT_ANY)
		address.sin_port = 0;
	else
		address.sin_port = htons((short)port);
	
#ifdef _WIN32
	if(bind(newsocket, (struct sockaddr *)&address, sizeof(address)) == -1)
#else
	if(bind(newsocket, (void *)&address, sizeof(address)) == -1)
#endif
		mpSystem->Error("UDP_OpenSocket: bind: %s", strerror(errno));

	return newsocket;
#endif
};

void CNetworkSystem::GetLocalAddress()
{
#ifdef _WIN32
	char buff[512];
#else
	char buff[MAXHOSTNAMELEN];
#endif
	struct sockaddr_in address;
	int namelen;

#ifdef _WIN32
	gethostname(buff, 512);
	buff[512 - 1] = 0;
#else
	gethostname(buff, MAXHOSTNAMELEN);
	buff[MAXHOSTNAMELEN-1] = 0;
#endif

	StringToAdr(buff, &net_local_adr);

	namelen = sizeof(address);
	
	if(getsockname(net_socket, (struct sockaddr *)&address, &namelen) == -1)
		mpSystem->Error("NET_Init: getsockname:", strerror(errno));

	net_local_adr.port = address.sin_port;

	mpSystem->Printf("IP address %s\n", AdrToString(&net_local_adr));
};