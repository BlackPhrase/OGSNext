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

#include "sound/ISound.hpp"

class CSoundNull final : public ISound
{
public:
	CSoundNull() = default;
	~CSoundNull() = default;

	bool Init(CreateInterfaceFn afnEngineFactory, void *apWindow) override { return true; }
	void Shutdown() override {}
	
	void Startup() override {}
	
	void Update(float*, float*, float*, float*) override {}
	
	void ExtraUpdate() override {}
	
	void ClearBuffer() override {}
	
	void BeginPrecaching() override {}
	void EndPrecaching() override {}
	
	sfx_t *PrecacheSound(const char *sample) override {return nullptr;}
	void TouchSound(const char *sample) override {}
	
	void LocalSound(const char *sound) override {}
	
	void StartStaticSound(sfx_t *sfx, vec3_t origin, float vol, float attenuation) override {}
	void StartDynamicSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation) override {}
	
	void StopSound(int entnum, int entchannel) override {}
	void StopAllSounds(bool clear) override {}
};