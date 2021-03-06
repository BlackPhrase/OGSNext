/*
 * This file is part of Magenta Engine
 *
 * Copyright (C) 2017-2018 BlackPhrase
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

#include <memory>
#include "game/client/IClientGame.hpp"

struct IMemory;
struct IConsole;
struct IEngineSound;
struct IInput;
class CInputEventListener;

class CClientGame : public IClientGame
{
public:
	CClientGame();
	~CClientGame();
	
	bool Init(CreateInterfaceFn afnEngineFactory) override;
	void Shutdown() override;
	
	void Frame() override;
private:
	std::unique_ptr<CInputEventListener> mpInputEventListener;
	
	IMemory *mpMemory{nullptr};
	IConsole *mpConsole{nullptr};
	IEngineSound *mpSound{nullptr};
	IInput *mpInput{nullptr};
};