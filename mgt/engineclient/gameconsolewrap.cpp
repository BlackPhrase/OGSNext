/*
 * This file is part of OGSNext Engine
 *
 * Copyright (C) 2018, 2020 BlackPhrase
 *
 * OGSNext Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OGSNext Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OGSNext Engine. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file

#include "quakedef.h"
#include "GameUI/IGameConsole.hpp"

IGameConsole *gpGameConsole{nullptr};

//void GameConsole_Initialize();

/*
void GameConsole_Activate()
{
	if(gpGameConsole)
		gpGameConsole->SetVisible(true);
};

void GameConsole_Hide()
{
	if(gpGameConsole)
		gpGameConsole->SetVisible(false);
};
*/

void GameConsole_Clear()
{
	if(gpGameConsole)
		gpGameConsole->Clear();
};

bool GameConsole_IsVisible()
{
	if(gpGameConsole)
		return gpGameConsole->IsVisible();
	
	return false;
};

void GameConsole_Printf(const char *format, ...)
{
	char msg[256]{};
	va_list argptr;
	
	va_start(argptr, format);
	vsprintf(msg, format, argptr);
	va_end(argptr);
	
	if(gpGameConsole)
		gpGameConsole->Printf(msg);
};

void GameConsole_DPrintf(const char *format, ...)
{
	char msg[256]{};
	va_list argptr;
	
	va_start(argptr, format);
	vsprintf(msg, format, argptr);
	va_end(argptr);
	
	if(gpGameConsole)
		gpGameConsole->DevPrintf(msg);
};