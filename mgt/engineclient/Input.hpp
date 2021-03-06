/*
 * This file is part of OGSNext Engine
 *
 * Copyright (C) 1996-1997 Id Software, Inc.
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
/// @brief external (non-keyboard) input devices

#pragma once

void IN_Init();

void IN_Shutdown();

void IN_Commands();
// oportunity for devices to stick commands on the script buffer

//void IN_Frame (); // Q2

void IN_Move(usercmd_t *cmd);
// add additional movement on top of the keyboard move cmd

void IN_ClearStates(); // not present in Q2
// restores all button and position states to defaults
