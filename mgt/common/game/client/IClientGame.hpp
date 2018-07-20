/*
 * This file is part of Magenta Engine
 *
 * Copyright (C) 2015-2018 BlackPhrase
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
/// @brief client-side game interface (interface that client game module exposes to the engine)

#pragma once

#include "CommonTypes.hpp"
#include "Interface.hpp"

constexpr auto MGT_CLIENTGAME_INTERFACE_VERSION{"MGTClientGame001Alpha"};

interface IClientGame : public IBaseInterface
{
	///
	virtual bool Init(CreateInterfaceFn afnEngineFactory) = 0;
	
	///
	virtual void Shutdown() = 0;
	
	///
	virtual void Update() = 0;
	//virtual void Frame(double frametime) = 0;
};