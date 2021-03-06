/*
 * This file is part of Magenta Engine
 *
 * Copyright (C) 1996-2005 Id Software, Inc.
 * Copyright (C) 2017-2020 BlackPhrase
 *
 * Magenta Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Magenta Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Magenta Engine. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file

#include "quakedef.h"

#ifdef _WIN32
	#include "winquake.h"
#endif

#include "EngineClient.hpp"
#include "r_local.h"
#include "vgui_int.h"

#include "engine/ISystem.hpp"
#include "filesystem/IFileSystem.hpp"
#include "engine/IConsole.hpp"

#include "engine/ICmdArgs.hpp"

#define PAUSE_SLEEP 50     ///< sleep time on pause or minimization (in ms)
#define NOT_FOCUS_SLEEP 20 ///< sleep time when not focus (in ms)

ISystem *gpSystem{nullptr};
IFileSystem *gpFileSystem{nullptr};
IMemory *gpMemory{nullptr};

byte *host_basepal; // TODO: unsigned short
byte *host_colormap;

extern void Con_Init();
extern void Con_Print(const char *msg);

#ifdef _WIN32
bool ActiveApp{true}, Minimized{false};

void SleepUntilInput(int time)
{
	//MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT); // TODO
};
#endif

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer_f(const ICmdArgs &apArgs)
{
	if(cls.state != ca_connected)
	{
		Con_Printf("Can't \"%s\", not connected\n", apArgs.GetByIndex(0));
		return;
	};

	// not really connected
	if(cls.demoplayback)
		return;

	MSG_WriteByte(&cls.netchan.message, clc_stringcmd);

	if(Q_strcasecmp(apArgs.GetByIndex(0), "cmd") != 0)
	{
		SZ_Print(&cls.netchan.message, apArgs.GetByIndex(0));
		SZ_Print(&cls.netchan.message, " ");
	};

	if(apArgs.GetCount() > 1)
		SZ_Print(&cls.netchan.message, apArgs.ToString());
	else
		SZ_Print(&cls.netchan.message, "\n");
};

CEngineClient::CEngineClient() = default;
CEngineClient::~CEngineClient() = default;

bool CEngineClient::Init(CreateInterfaceFn afnEngineFactory /*, tWinHandle ahWindow*/)
{
	if(!afnEngineFactory)
		return false;

	mpSystem = (ISystem *)afnEngineFactory(MGT_SYSTEM_INTERFACE_VERSION, nullptr);
	mpFileSystem = (IFileSystem *)afnEngineFactory(MGT_FILESYSTEM_INTERFACE_VERSION, nullptr);
	mpConsole = (IConsole*)afnEngineFactory(MGT_CONSOLE_INTERFACE_VERSION, nullptr);

	if(!mpSystem || !mpFileSystem || !mpConsole)
		return false;

	// TODO: temp to support legacy code
	gpSystem = mpSystem;
	gpFileSystem = mpFileSystem;
	gpConsole = mpConsole;
	
	cls.state = ca_disconnected;

	host_basepal = (byte *)COM_LoadHunkFile("gfx/palette.lmp");
	
	if(!host_basepal)
		gpSystem->Error("Couldn't load gfx/palette.lmp");
	
	host_colormap = (byte *)COM_LoadHunkFile("gfx/colormap.lmp");
	
	if(!host_colormap)
		gpSystem->Error("Couldn't load gfx/colormap.lmp");
	
	W_LoadWadFile("gfx.wad");
	W_LoadWadFile("fonts.wad");
	
	Key_Init();

	V_Init();
	Chase_Init();

#ifndef _WIN32 // on non win32, mouse comes before video for security reasons
	IN_Init();
#endif

	VID_Init(host_basepal);
	// TODO: GL_Init() here + no VID_Shutdown in GS (at least for hw)

	Draw_Init();
	SCR_Init();
	R_Init();

#ifndef _WIN32
	// on Win32, sound initialization has to come before video initialization, so we
	// can put up a popup if the sound hardware is in use
	S_Init();
#else

#ifdef GLQUAKE
	// FIXME: doesn't use the new one-window approach yet
	S_Init();
#endif

#endif // _WIN32

	CDAudio_Init();
	Sbar_Init();
	CL_Init();

#ifdef _WIN32 // on non win32, mouse comes before video for security reasons
	IN_Init();
#endif

	R_InitTextures();
	
	// GUI should be initialized before the client dll start to use it
	VGui_Startup();

	// Initialize the GameUI module here (TODO: move to vgui init?)
	M_Init();

	// Initialize the client dll now
	ClientDLL_Init();

	LocalInit();
	return true;
};

void CEngineClient::Shutdown()
{
	// keep Con_Printf from trying to update the screen
	scr_disabled_for_loading = true; // TODO: revisit

	WriteConfig();

	ClientDLL_Shutdown();
	VGui_Shutdown();

	CDAudio_Shutdown();

	S_Shutdown();
	IN_Shutdown();

	VID_Shutdown(); // TODO: Not present in GS
};

void CEngineClient::ClearMemory()
{
	D_FlushCaches();

	cls.signon = 0;
	memset(&cl, 0, sizeof(cl));
};

bool CEngineClient::FilterTime(double frametime) const
{
	//inline float GetTimeStep(float fps){return 1.0/fps;}
	if(!cls.timedemo && frametime < 1.0 / 72.0) // TODO: use "fps_max" cvar value here
		return false; // framerate is too high
	
	return true;
};

bool CEngineClient::PreFrame()
{
	// TODO

#ifdef _WIN32
	// yield the CPU for a little while when paused, minimized, or not the focus
	if((cl.paused && (!ActiveApp && !DDActive)) || Minimized || block_drawing)
	{
		SleepUntilInput(PAUSE_SLEEP);
		scr_skipupdate = 1; // no point in bothering to draw
	}
	else if(!ActiveApp && !DDActive)
		SleepUntilInput(NOT_FOCUS_SLEEP);
#endif // _WIN32
	
	return true;
};

void CEngineClient::Frame()
{
	//-------------------
	//
	// client operations
	//
	//-------------------
	
	// get new key events
	SendKeyEvents();
	
	// allow mice or other external controllers to add commands
	IN_Commands();

	// process console commands again for client-side
	Cbuf_Execute();

	// fetch results from server
	//if(cls.state == ca_connected) // TODO
		CL_ReadPackets();

	if(cls.state == ca_disconnected)
		CL_CheckForResend();
	else
		CL_SendCmd();

	//host_time += host_frametime; // TODO

	// fetch results from server
	//if(cls.state == ca_connected) // TODO
		//CL_ReadPackets(); // TODO

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(false);

	// do client side motion prediction
	CL_PredictMove();

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(true);

	// build a renderable entity list
	CL_EmitEntities();

	// TODO: here were the screen update

	// update audio
	if(cls.signon == SIGNONS)
	{
		S_Update(r_origin, vpn, vright, vup);
		CL_DecayLights();
	}
	else
		S_Update(vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();
};

void CEngineClient::ForwardCmdToServer(const ICmdArgs &apArgs)
{
	Cmd_ForwardToServer_f(apArgs);
};

void CEngineClient::HostEndGame()
{
	if(cls.demonum != -1)
		CL_NextDemo();
	else
		CL_Disconnect();
};

void CEngineClient::HostError()
{
	SCR_EndLoadingPlaque(); // reenable screen updates
	
	CL_Disconnect();
	cls.demonum = -1;
};

void CEngineClient::HostServerShutdown()
{
	// stop all client sounds immediately
	if(cls.state == ca_connected)
		CL_Disconnect();
};

// TODO
/*
void CEngineClient::Disconnect(bool abForce)
{
	if(!abForce)
	{
		if(cls.state != ca_connected)
			return;
	};

	CL_Disconnect();
};
*/

void CEngineClient::ConInit()
{
	Con_Init();
};

void CEngineClient::ConPrint(const char *msg)
{
	Con_Print(msg);
};

extern void M_Menu_Quit_f(const ICmdArgs &apArgs);

bool CEngineClient::OnQuit() // TODO: CanQuit?
{
	if(key_dest != key_console && cls.state != ca_dedicated)
	{
		//M_Menu_Quit_f(apArgs); // TODO
		return false;
	};
	
	//CL_Disconnect(); // TODO: will be handled by Host_ServerShutdown which will call the HostServerShutdown method
	return true;
};

void CEngineClient::OnMap(const ICmdArgs &apArgs)
{
	cls.demonum = -1; // stop demo loop in case this fails
	
	//CL_Disconnect(); // TODO: will be handled by Host_ServerShutdown which will call the HostServerShutdown method
	
	key_dest = key_game; // remove console or menu
	SCR_BeginLoadingPlaque();
	
	cls.mapstring[0] = 0;
	
	for(int i = 0; i < apArgs.GetCount(); i++)
	{
		strcat(cls.mapstring, apArgs.GetByIndex(i));
		strcat(cls.mapstring, " ");
	};
	
	strcat(cls.mapstring, "\n");
};

void CEngineClient::ConnectToLocalServer(const ICmdArgs &apArgs)
{
	strcpy(cls.spawnparms, "");

	for(int i = 2; i < apArgs.GetCount(); i++)
	{
		strcat(cls.spawnparms, apArgs.GetByIndex(i));
		strcat(cls.spawnparms, " ");
	};

	Cmd_ExecuteString("connect local", src_command);
};

bool CEngineClient::IsDemoPlayback()
{
	if(cls.demoplayback)
		return true;
	
	return false;
};

bool CEngineClient::CanSaveGame()
{
	if(cl.intermission)
	{
		Con_Printf("Can't save in intermission.\n");
		return false;
	};
	
	return true;
};

/*
void CEngineClient::OnReconnect()
{
};
*/

void CEngineClient::LocalInit()
{
	//
	// register our commands
	//
	Cmd_AddCommand("cmd", Cmd_ForwardToServer_f);
};

/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CEngineClient::WriteConfig()
{
	if(!host_initialized)
		return;

	auto f{mpFileSystem->OpenPathID(va("%s/config.cfg", com_gamedir), "w")};

	if(!f)
	{
		Con_Printf("Couldn't write config.cfg.\n");
		return;
	};

	Key_WriteBindings(f);
	Cvar_WriteVariables(f);

	mpFileSystem->CloseFile(f);
};

void CEngineClient::UpdateScreen()
{
	// TODO: something else?

	SCR_UpdateScreen();

	// TODO: something else?
};

void CEngineClient::SendKeyEvents()
{
#ifdef _WIN32
	static MSG Msg{};

	while(PeekMessage(&Msg, nullptr, 0, 0, PM_NOREMOVE))
	{
		// we always update if there are any event, even if we're paused
		scr_skipupdate = 0;

		if(!GetMessage(&Msg, nullptr, 0, 0))
			gpSystem->Quit();

		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	};
#endif
};