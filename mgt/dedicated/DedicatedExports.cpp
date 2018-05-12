/*
*	This file is part of Magenta Engine
*
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

#include "DedicatedExports.hpp"

EXPOSE_SINGLE_INTERFACE(CDedicatedExports, IDedicatedExports, MGT_DEDICATEDEXPORTS_API_VERSION);

CDedicatedExports::CDedicatedExports() = default;
CDedicatedExports::~CDedicatedExports() = default;

void CDedicatedExports::Sys_Printf(const char *text)
{
	if(!text || !*text)
		return;
	
	// TODO
};