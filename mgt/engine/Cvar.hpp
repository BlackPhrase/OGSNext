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

#pragma once

/**

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

it is sufficient to initialize a cvar_t with just the first two fields, or
you can add a flags bitsum for variables that you want to be applied on a cvar:

cvar_t	r_draworder = {"r_draworder","1"};
cvar_t	scr_screensize = {"screensize","1",FCVAR_ARCHIVE};

Cvars must be registered before use, or they will have a 0 value instead of the float interpretation of the string.  Generally, all cvar_t declarations should be registered in the apropriate init function before any console commands are executed:
Cvar_RegisterVariable (&host_framerate);

C code usually just references a cvar in place:
if ( r_draworder.value )

It could optionally ask for the value to be looked up for a string name:
if (Cvar_VariableValue ("r_draworder"))

Interpreted prog code can access cvars with the cvar(name) or
cvar_set (name, value) internal functions:
teamplay = cvar("teamplay");
cvar_set ("gl_ztrick", "1");

The user can access cvars from the console in two ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

#include "cvardef.h"

class CConVar
{
public:
	CConVar(const char *asName, const char *asValue, int anFlags = 0, const char *asDesc = "");
	~CConVar();
	
	const char *GetName() const {return mpData->name;}
	const char *GetDesc() const {return msDesc;}
	
	int GetFlags() const {return mpData->flags;}
	
	void SetValue(float afValue){mpData->value = afValue;}
	float GetValue() const {return mpData->value;}
	
	const char *GetString() const {return mpData->string;}
	
	cvar_t *internal() const {return mpData;} // TODO: temp
private:
	cvar_t *mpData{nullptr};
	
	const char *msDefValue{""};
	const char *msDesc{""};
};

extern cvar_t *cvar_vars;

/// registers a cvar that already has the name, string, and optionally the
/// archive elements set.
void Cvar_RegisterVariable(cvar_t *variable);

/// equivelant to "<name> <variable>" typed at the console
void Cvar_Set(const char *var_name, const char *value);

/// expands value to a string and calls Cvar_Set
void Cvar_SetValue(const char *var_name, float value);

/// returns 0 if not defined or non numeric
float Cvar_VariableValue(const char *var_name);

/// returns an empty string if not defined
const char *Cvar_VariableString(const char *var_name);

/// attempts to match a partial variable name for command line completion
/// returns nullptr if nothing fits
char *Cvar_CompleteVariable(const char *partial);

/// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
/// command.  Returns true if the command was a variable reference that
/// was handled. (print or change)
bool Cvar_Command();

/// Writes lines containing "set variable value" for all variables
/// with the archive flag set to true.
void Cvar_WriteVariables(IFile *f);

cvar_t *Cvar_FindVar(const char *var_name);