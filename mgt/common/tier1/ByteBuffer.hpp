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
/// @brief (network) message I/O functions - handles byte ordering and avoids alignment errors

#pragma once

#include "qcommon.h"
#include "network/IReadBuffer.hpp"
#include "network/IWriteBuffer.hpp"

using sizebuf_t = struct sizebuf_s;

//extern struct usercmd_s nullcmd; // TODO: qw

struct ISystem;

class CByteBuffer : public IReadBuffer, public IWriteBuffer
{
public:
	CByteBuffer(sizebuf_t *apData);
#ifdef MGT_PARANOID
	CByteBuffer(sizebuf_t *apData, ISystem *apSystem = nullptr);
#endif
	~CByteBuffer();
	
	// Writing methods
	
	///
	void WriteByte(int nValue) override;

	///
	void WriteChar(int nValue) override;
	
	///
	void WriteShort(int nValue) override;

	///
	void WriteLong(int nValue) override;
	
	///
	void WriteFloat(float fValue) override;
	
	///
	void WriteAngle(float fValue) override;
	
	///
	void WriteCoord(float fValue) override;
	
	///
	void WriteString(const char *sValue) override;
	
	///
	void WriteEntity(int nValue) override;
	
	// Reading methods

	///
	int ReadByte() override;
	
	/// returns -1 and sets msg_badread if no more characters are available
	int ReadChar() override;

	///
	int ReadShort() override;

	///
	int ReadLong() override;
	
	///
	float ReadFloat() override;
	
	///
	float ReadCoord() override;
	
	///
	float ReadAngle() override;
	
	///
	const char *ReadString() override;
	
	///
	int GetReadCount() const {return msg_readcount;} // TODO: used by ServerUser
private:
	///
	void BeginReading(); // TODO: Reset?
	
	sizebuf_t *mpData{nullptr};

#ifdef MGT_PARANOID
	ISystem *mpSystem{nullptr};
#endif

	int msg_readcount{0};
	bool msg_badread{false}; ///< set if a read goes beyond end of message
};