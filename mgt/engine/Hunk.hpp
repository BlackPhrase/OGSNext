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
/// @brief hunk allocator

#pragma once

#include "CommonTypes.hpp"

/////////////////////////////////////////////////

extern byte *hunk_base;
extern int hunk_size;

extern int hunk_low_used;
extern int hunk_high_used;

/// @return 0 filled memory
void *Hunk_Alloc(int size);

///
void *Hunk_AllocName(int size, const char *name);

///
void *Hunk_HighAllocName(int size, const char *name);

///
int	Hunk_LowMark();

///
void Hunk_FreeToLowMark(int mark);

///
int	Hunk_HighMark();

///
void Hunk_FreeToHighMark(int mark);

///
void *Hunk_TempAlloc(int size);

///
void Hunk_Check();