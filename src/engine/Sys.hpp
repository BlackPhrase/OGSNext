/*
Copyright (C) 2019-2020 BlackPhrase

This program is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file

#pragma once

class CSys
{
public:
	//
	// memory protection
	//
	
	void MakeCodeWriteable(unsigned long startaddr, unsigned long length);

	//
	// system IO
	//
	
	void DebugLog(const char *file, char *fmt, ...);

	/// an error will cause the entire program to exit
	void Error(const char *error, ...);

	/// send text to the console
	void Printf(const char *fmt, ...);

	void Quit();

	double GetDoubleTime();

	char *GetConsoleInput();

	/// called to yield for a little bit so as
	/// not to hog cpu when paused or debugging
	void Sleep(int anMilliSecs);

	/// Perform Key_Event() callbacks until the input que is empty
	void SendKeyEvents();

	void LowFPPrecision();
	void HighFPPrecision();
	void SetFPCW();
};