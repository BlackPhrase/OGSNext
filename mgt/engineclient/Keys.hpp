/*
 * This file is part of OGSNext Engine
 *
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) 2018-2020 BlackPhrase
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

#pragma once

#include "public/keydefs.h"

struct IFile;

typedef enum { key_game,
	           key_console,
	           key_message,
	           key_menu } keydest_t;

extern keydest_t key_dest;
extern char *keybindings[256];
extern int key_repeats[256];
extern int key_count; // incremented every key event
extern int key_lastpress;

void Key_Event(int key, qboolean down);
void Key_Init();
void Key_WriteBindings(IFile *f);
void Key_SetBinding(int keynum, const char *binding);
void Key_ClearStates();
const char *Key_KeynumToString(int keynum);
//int Key_GetKey (); // Q2