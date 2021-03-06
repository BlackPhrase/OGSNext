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

#include "DedicatedExports.hpp"

#ifdef _WIN32
#include <windows.h>
extern HANDLE houtput;
#endif

EXPOSE_SINGLE_INTERFACE(CDedicatedExports, IDedicatedExports, VENGINE_DEDICATEDEXPORTS_API_VERSION);

CDedicatedExports::CDedicatedExports() = default;
CDedicatedExports::~CDedicatedExports() = default;

void CDedicatedExports::Sys_Printf(const char *text)
{
	if(!text || !*text)
		return;
	
#ifdef _WIN32
	DWORD dummy{0};
	WriteFile(houtput, text, strlen(text), &dummy, nullptr);
#else
	printf("%s", text);
#endif
};