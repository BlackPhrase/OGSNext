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

class CFileSystem
{
public:
	//
	// file IO
	//

	/// returns the file size
	/// return -1 if file is not present
	/// the file should be in BINARY mode for stupid OSs that care
	IFile *FileOpenRead(const char *path);

	IFile *FileOpenWrite(const char *path);
	
	void FileClose(IFile *handle);
	
	void FileSeek(IFile *handle, int position);
	
	int FileRead(IFile *handle, void *dest, int count);
	int FileWrite(IFile *handle, void *data, int count);
	
	int	FileTime(const char *path);
	void mkdir(const char *path);
};