/*
 * This file is part of OGSNext Engine
 *
 * Copyright (C) 2019-2020 BlackPhrase
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

using sizebuf_t = struct sizebuf_s;

class CClient
{
public:
	void Printf(const char *asMsg, ...);
	void SendCmd(const char *asCmd, ...);
public:
	sizebuf_t message{};
	
	bool active{false};
	bool spawned{false};
};