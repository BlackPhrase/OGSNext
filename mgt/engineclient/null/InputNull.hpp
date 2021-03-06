/*
*	This file is part of Magenta Engine
*
*	Copyright (C) 2017-2018 BlackPhrase
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
/// @brief null input module implementation

#include "input/IInput.hpp"

class CInputNull final : public IInput
{
public:
	CInputNull() = default;
	~CInputNull() = default;

	bool Init() override { return true; }
	void Shutdown() override {}
	
	//void Frame() override {}
	
	void HandleOSMessage(const SOSMessage &apMsg) override {}
	
	void AddEventListener(IInputEventListener *apListener) override {}
	void RemoveEventListener(IInputEventListener *apListener) override {}
};