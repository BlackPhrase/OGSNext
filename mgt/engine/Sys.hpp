/*
 * This file is part of Magenta Engine
 *
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) 2018-2019 BlackPhrase
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
/// @brief non-portable functions

#pragma once

//
// memory protection
//
void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length);

//
// system IO
//
void Sys_DebugLog(const char *file, const char *fmt, ...);

void Sys_Init();

/// an error will cause the entire program to exit
void Sys_Error(const char *error, ...);

void Sys_Printf(const char *fmt, ...);
// send text to the console

void Sys_Quit();

double Sys_FloatTime();

char *Sys_ConsoleInput();

void Sys_Sleep();
/// called to yield for a little bit so as
/// not to hog cpu when paused or debugging

/// Perform Key_Event() callbacks until the input que is empty
void Sys_SendKeyEvents();

void Sys_LowFPPrecision();
void Sys_HighFPPrecision();

//int Sys_FileTime (char *path); // TODO: FS_FileTime

//void Sys_mkdir (char *path); // TODO: FS_mkdir