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

#include "quakedef.h"
#include "Sound.hpp"
#include "soundsystem/ISoundSystem.hpp"

void CSound::Init()
{
	LoadSoundSystemModule();
	
	mpSoundSystem->Init();
};

void CSound::Shutdown()
{
	UnloadSoundSystemModule();
};

void CSound::Update()
{
	mpSoundSystem->Update();
};

void CSound::LoadSoundSystemModule()
{
	mpSoundSystemModule = Sys_LoadModule("soundsystem");
	
	if(!mpSoundSystemModule)
		mpSystem->Error("");
	
	auto fnSoundSystemFactory{Sys_GetFactory(mpSoundSystemModule)};
	
	if(!fnSoundSystemFactory)
		mpSystem->Error("");
	
	mpSoundSystem = reinterpret_cast<ISoundSystem*>(fnSoundSystemFactory(OGS_SOUNDSYSTEM_INTERFACE_VERSION, nullptr));
	
	if(!mpSoundSystem)
		mpSystem->Error("");
};

void CSound::UnloadSoundSystemModule()
{
	if(mpSoundSystem)
	{
		mpSoundSystem->Shutdown();
		mpSoundSystem = nullptr;
	};
	
	if(mpSoundSystemModule)
	{
		Sys_UnloadModule(mpSoundSystemModule);
		mpSoundSystemModule = nullptr;
	};
};