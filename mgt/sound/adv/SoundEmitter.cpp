/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/// @file

#include "../idlib/precompiled.h"
#include "snd_local.h"

idCVar s_singleEmitter("s_singleEmitter", "0", CVAR_INTEGER, "mute all sounds but this emitter");
idCVar s_showStartSound("s_showStartSound", "0", CVAR_BOOL, "print a message every time a sound starts/stops");
idCVar s_useOcclusion("s_useOcclusion", "1", CVAR_BOOL, "Attenuate sounds based on walls");
idCVar s_centerFractionVO("s_centerFractionVO", "0.75", CVAR_FLOAT, "Portion of VO sounds routed to the center channel");

extern idCVar s_noSound;

/*
================================================================================================

	CSoundEmitter

================================================================================================
*/

/*
========================
CSoundEmitter::CSoundEmitter
========================
*/
CSoundEmitter::CSoundEmitter()
{
	Init(0, NULL);
}

/*
========================
CSoundEmitter::~CSoundEmitter
========================
*/
CSoundEmitter::~CSoundEmitter()
{
	assert(channels.Num() == 0);
}

/*
========================
CSoundEmitter::Clear
========================
*/
void CSoundEmitter::Init(int i, idSoundWorldLocal *sw)
{
	index = i;
	soundWorld = sw;

	// Init should only be called on a freshly constructed sound emitter or in a Reset()
	assert(channels.Num() == 0);

	canFree = false;
	origin.Zero();
	emitterId = 0;

	directDistance = 0.0f;
	lastValidPortalArea = -1;
	spatializedDistance = 0.0f;
	spatializedOrigin.Zero();

	memset(&parms, 0, sizeof(parms));
}

/*
========================
CSoundEmitter::Reset
========================
*/
void CSoundEmitter::Reset()
{
	for(int i = 0; i < channels.Num(); i++)
	{
		soundWorld->FreeSoundChannel(channels[i]);
	}
	channels.Clear();
	Init(index, soundWorld);
}

/*
==================
CSoundEmitter::OverrideParms
==================
*/
void CSoundEmitter::OverrideParms(const soundShaderParms_t *base, const soundShaderParms_t *over, soundShaderParms_t *out)
{
	if(!over)
	{
		*out = *base;
		return;
	}
	if(over->minDistance)
	{
		out->minDistance = over->minDistance;
	}
	else
	{
		out->minDistance = base->minDistance;
	}
	if(over->maxDistance)
	{
		out->maxDistance = over->maxDistance;
	}
	else
	{
		out->maxDistance = base->maxDistance;
	}
	if(over->shakes)
	{
		out->shakes = over->shakes;
	}
	else
	{
		out->shakes = base->shakes;
	}
	if(over->volume)
	{
		out->volume = over->volume;
	}
	else
	{
		out->volume = base->volume;
	}
	if(over->soundClass)
	{
		out->soundClass = over->soundClass;
	}
	else
	{
		out->soundClass = base->soundClass;
	}
	out->soundShaderFlags = base->soundShaderFlags | over->soundShaderFlags;
}

/*
========================
CSoundEmitter::CheckForCompletion

Checks to see if any of the channels have completed, removing them as they do

This will also play any postSounds on the same channel as their owner.

Returns true if the emitter should be freed.
========================
*/
bool CSoundEmitter::CheckForCompletion(int currentTime)
{
	for(int i = channels.Num() - 1; i >= 0; i--)
	{
		idSoundChannel *chan = channels[i];

		if(chan->CheckForCompletion(currentTime))
		{
			channels.RemoveIndex(i);
			soundWorld->FreeSoundChannel(chan);
		}
	}
	return (canFree && channels.Num() == 0);
}

/*
========================
CSoundEmitter::Update
========================
*/
void CSoundEmitter::Update(int currentTime)
{
	if(channels.Num() == 0)
	{
		return;
	}

	directDistance = (soundWorld->listener.pos - origin).LengthFast() * DOOM_TO_METERS;

	spatializedDistance = directDistance;
	spatializedOrigin = origin;

	// Initialize all channels to silence
	for(int i = 0; i < channels.Num(); i++)
	{
		channels[i]->volumeDB = DB_SILENCE;
	}

	if(s_singleEmitter.GetInteger() > 0 && s_singleEmitter.GetInteger() != index)
	{
		return;
	}
	if(soundWorld->listener.area == -1)
	{
		// listener is outside the world
		return;
	}
	if(soundSystemLocal.muted || soundWorld != soundSystemLocal.currentSoundWorld)
	{
		return;
	}
	float maxDistance = 0.0f;
	bool maxDistanceValid = false;
	bool useOcclusion = false;
	if(emitterId != soundWorld->listener.id)
	{
		for(int i = 0; i < channels.Num(); i++)
		{
			idSoundChannel *chan = channels[i];
			if((chan->parms.soundShaderFlags & SSF_GLOBAL) != 0)
			{
				continue;
			}
			useOcclusion = useOcclusion || ((chan->parms.soundShaderFlags & SSF_NO_OCCLUSION) == 0);
			maxDistanceValid = true;
			if(maxDistance < channels[i]->parms.maxDistance)
			{
				maxDistance = channels[i]->parms.maxDistance;
			}
		}
	}
	if(maxDistanceValid && directDistance >= maxDistance)
	{
		// too far away to possibly hear it
		return;
	}
	if(useOcclusion && s_useOcclusion.GetBool())
	{
		// work out virtual origin and distance, which may be from a portal instead of the actual origin
		if(soundWorld->renderWorld != NULL)
		{
			// we have a valid renderWorld
			int soundInArea = soundWorld->renderWorld->PointInArea(origin);
			if(soundInArea == -1)
			{
				soundInArea = lastValidPortalArea;
			}
			else
			{
				lastValidPortalArea = soundInArea;
			}
			if(soundInArea != -1 && soundInArea != soundWorld->listener.area)
			{
				spatializedDistance = maxDistance * METERS_TO_DOOM;
				soundWorld->ResolveOrigin(0, NULL, soundInArea, 0.0f, origin, this);
				spatializedDistance *= DOOM_TO_METERS;
			}
		}
	}

	for(int j = 0; j < channels.Num(); j++)
	{
		channels[j]->UpdateVolume(currentTime);
	}

	return;
}

/*
========================
CSoundEmitter::Index
========================
*/
int CSoundEmitter::Index() const
{
	assert(soundWorld);
	assert(soundWorld->emitters[this->index] == this);

	return index;
}

/*
========================
CSoundEmitter::Free

Doesn't free it until the next update.
========================
*/
void CSoundEmitter::Free(bool immediate)
{
	assert(soundWorld);
	assert(soundWorld->emitters[this->index] == this);

	if(canFree)
	{
		// Double free
		return;
	}
	if(soundWorld && soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_FREE);
		soundWorld->writeDemo->WriteInt(index);
		soundWorld->writeDemo->WriteInt(immediate);
	}

	if(immediate)
	{
		Reset();
	}

	canFree = true;
}

/*
========================
CSoundEmitter::UpdateEmitter
========================
*/
void CSoundEmitter::UpdateEmitter(const idVec3 &origin, int listenerId, const soundShaderParms_t *parms)
{
	assert(soundWorld != NULL);
	assert(soundWorld->emitters[this->index] == this);

	if(soundWorld && soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_UPDATE);
		soundWorld->writeDemo->WriteInt(index);
		soundWorld->writeDemo->WriteVec3(origin);
		soundWorld->writeDemo->WriteInt(listenerId);
		soundWorld->writeDemo->WriteFloat(parms->minDistance);
		soundWorld->writeDemo->WriteFloat(parms->maxDistance);
		soundWorld->writeDemo->WriteFloat(parms->volume);
		soundWorld->writeDemo->WriteFloat(parms->shakes);
		soundWorld->writeDemo->WriteInt(parms->soundShaderFlags);
		soundWorld->writeDemo->WriteInt(parms->soundClass);
	}

	this->origin = origin;
	this->emitterId = listenerId;
	this->parms = *parms;
}

/*
========================
CSoundEmitter::StartSound

in most cases play sounds immediately, however
  intercept sounds using SSF_FINITE_SPEED_OF_SOUND
  and schedule them for playback later

return: int	- the length of the started sound in msec.
========================
*/
int CSoundEmitter::StartSound(const idSoundShader *shader, const s_channelType channel, float diversity, int shaderFlags, bool allowSlow)
{
	assert(soundWorld != NULL);
	assert(soundWorld->emitters[this->index] == this);

	if(shader == NULL)
	{
		return 0;
	}

	if(soundWorld && soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_START);
		soundWorld->writeDemo->WriteInt(index);

		soundWorld->writeDemo->WriteHashString(shader->GetName());

		soundWorld->writeDemo->WriteInt(channel);
		soundWorld->writeDemo->WriteFloat(diversity);
		soundWorld->writeDemo->WriteInt(shaderFlags);
	}

	if(s_noSound.GetBool())
	{
		return 0;
	}

	int currentTime = soundWorld->GetSoundTime();

	bool showStartSound = s_showStartSound.GetBool();
	if(showStartSound)
	{
		idLib::Printf("%dms: StartSound(%d:%d): %s: ", currentTime, index, channel, shader->GetName());
	}

	// build the channel parameters by taking the shader parms and optionally overriding
	soundShaderParms_t chanParms;
	chanParms = shader->parms;
	OverrideParms(&chanParms, &parms, &chanParms);
	chanParms.soundShaderFlags |= shaderFlags;

	if(shader->entries.Num() == 0)
	{
		if(showStartSound)
		{
			idLib::Printf(S_COLOR_RED "No Entries\n");
		}
		return 0;
	}

	// PLAY_ONCE sounds will never be restarted while they are running
	if(chanParms.soundShaderFlags & SSF_PLAY_ONCE)
	{
		for(int i = 0; i < channels.Num(); i++)
		{
			idSoundChannel *chan = channels[i];
			if(chan->soundShader == shader && !chan->CheckForCompletion(currentTime))
			{
				if(showStartSound)
				{
					idLib::Printf(S_COLOR_YELLOW "Not started because of playOnce\n");
				}
				return 0;
			}
		}
	}

	// never play the same sound twice with the same starting time, even
	// if they are on different channels
	for(int i = 0; i < channels.Num(); i++)
	{
		idSoundChannel *chan = channels[i];
		if(chan->soundShader == shader && chan->startTime == currentTime && chan->endTime != 1)
		{
			if(showStartSound)
			{
				idLib::Printf(S_COLOR_RED "Already started this frame\n");
			}
			return 0;
		}
	}

	// kill any sound that is currently playing on this channel
	if(channel != SCHANNEL_ANY)
	{
		for(int i = 0; i < channels.Num(); i++)
		{
			idSoundChannel *chan = channels[i];
			if(chan->soundShader && chan->logicalChannel == channel)
			{
				if(showStartSound)
				{
					idLib::Printf(S_COLOR_YELLOW "OVERRIDE %s: ", chan->soundShader->GetName());
				}
				channels.RemoveIndex(i);
				soundWorld->FreeSoundChannel(chan);
				break;
			}
		}
	}

	idSoundSample *leadinSample = NULL;
	idSoundSample *loopingSample = NULL;

	if(shader->leadin && (chanParms.soundShaderFlags & SSF_LOOPING))
	{
		leadinSample = shader->entries[0];
		loopingSample = shader->entries.Num() > 1 ? shader->entries[1] : NULL;
	}
	else
	{
		if(shader->entries.Num() == 1)
		{
			leadinSample = shader->entries[0];
		}
		else
		{
			int choice;
			if(chanParms.soundShaderFlags & SSF_NO_DUPS)
			{
				// Don't select the most recently played entry
				int mostRecentTime = 0;
				int mostRecent = 0;
				for(int i = 0; i < shader->entries.Num(); i++)
				{
					int entryTime = shader->entries[i]->GetLastPlayedTime();
					if(entryTime > mostRecentTime)
					{
						mostRecentTime = entryTime;
						mostRecent = i;
					}
				}
				choice = (int)(diversity * (shader->entries.Num() - 1));
				if(choice >= mostRecent)
				{
					choice++;
				}
			}
			else
			{
				// pick a sound from the list based on the passed diversity
				choice = (int)(diversity * shader->entries.Num());
			}
			choice = idMath::ClampInt(0, shader->entries.Num() - 1, choice);
			leadinSample = shader->entries[choice];
			leadinSample->SetLastPlayedTime(soundWorld->GetSoundTime());
		}
		if(chanParms.soundShaderFlags & SSF_LOOPING)
		{
			loopingSample = leadinSample;
		}
	}

	// set all the channel parameters here,
	// a hardware voice will be allocated next update if the volume is high enough to be audible
	if(channels.Num() == channels.Max())
	{
		CheckForCompletion(currentTime); // as a last chance try to release finished sounds here
		if(channels.Num() == channels.Max())
		{
			if(showStartSound)
			{
				idLib::Printf(S_COLOR_RED "No free emitter channels!\n");
			}
			return 0;
		}
	}
	idSoundChannel *chan = soundWorld->AllocSoundChannel();
	if(chan == NULL)
	{
		if(showStartSound)
		{
			idLib::Printf(S_COLOR_RED "No free global channels!\n");
		}
		return 0;
	}
	channels.Append(chan);
	chan->emitter = this;
	chan->parms = chanParms;
	chan->soundShader = shader;
	chan->logicalChannel = channel;
	chan->leadinSample = leadinSample;
	chan->loopingSample = loopingSample;
	chan->allowSlow = allowSlow;

	// return length of sound in milliseconds
	int length = chan->leadinSample->LengthInMsec();

	// adjust the start time based on diversity for looping sounds, so they don't all start at the same point
	int startOffset = 0;

	if(chan->IsLooping() && !shader->leadin)
	{
		// looping sounds start at a random point...
		startOffset = soundSystemLocal.random.RandomInt(length);
	}

	chan->startTime = currentTime - startOffset;

	if((chanParms.soundShaderFlags & SSF_LOOPING) != 0)
	{
		// This channel will never end!
		chan->endTime = 0;
	}
	else
	{
		// This channel will automatically end at this time
		chan->endTime = chan->startTime + length + 100;
	}
	if(showStartSound)
	{
		if(loopingSample == NULL || leadinSample == loopingSample)
		{
			idLib::Printf("Playing %s @ %d\n", leadinSample->GetName(), startOffset);
		}
		else
		{
			idLib::Printf("Playing %s then looping %s\n", leadinSample->GetName(), loopingSample->GetName());
		}
	}

	return length;
}

/*
========================
CSoundEmitter::OnReloadSound

This is a shortened version of StartSound, called whenever a sound shader is reloaded.
If the emitter is currently playing the given sound shader, restart it so
a change in the sound sample used for a given sound shader will be picked up.
========================
*/
void CSoundEmitter::OnReloadSound(const idDecl *decl)
{
}

/*
========================
CSoundEmitter::StopSound

Can pass SCHANNEL_ANY.
========================
*/
void CSoundEmitter::StopSound(const s_channelType channel)
{
	assert(soundWorld != NULL);
	assert(soundWorld->emitters[this->index] == this);

	if(soundWorld && soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_STOP);
		soundWorld->writeDemo->WriteInt(index);
		soundWorld->writeDemo->WriteInt(channel);
	}

	for(int i = 0; i < channels.Num(); i++)
	{
		idSoundChannel *chan = channels[i];

		if(channel != SCHANNEL_ANY && chan->logicalChannel != channel)
		{
			continue;
		}
		if(s_showStartSound.GetBool())
		{
			idLib::Printf("%dms: StopSound(%d:%d): %s\n", soundWorld->GetSoundTime(), index, channel, chan->soundShader->GetName());
		}

		// This forces CheckForCompletion to return true
		chan->endTime = 1;
	}
}

/*
========================
CSoundEmitter::ModifySound
========================
*/
void CSoundEmitter::ModifySound(const s_channelType channel, const soundShaderParms_t *parms)
{
	assert(soundWorld != NULL);
	assert(soundWorld->emitters[this->index] == this);

	if(soundWorld && soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_MODIFY);
		soundWorld->writeDemo->WriteInt(index);
		soundWorld->writeDemo->WriteInt(channel);
		soundWorld->writeDemo->WriteFloat(parms->minDistance);
		soundWorld->writeDemo->WriteFloat(parms->maxDistance);
		soundWorld->writeDemo->WriteFloat(parms->volume);
		soundWorld->writeDemo->WriteFloat(parms->shakes);
		soundWorld->writeDemo->WriteInt(parms->soundShaderFlags);
		soundWorld->writeDemo->WriteInt(parms->soundClass);
	}

	for(int i = channels.Num() - 1; i >= 0; i--)
	{
		idSoundChannel *chan = channels[i];
		if(channel != SCHANNEL_ANY && chan->logicalChannel != channel)
		{
			continue;
		}
		if(s_showStartSound.GetBool())
		{
			idLib::Printf("%dms: ModifySound(%d:%d): %s\n", soundWorld->GetSoundTime(), index, channel, chan->soundShader->GetName());
		}
		OverrideParms(&chan->parms, parms, &chan->parms);
	}
}

/*
========================
CSoundEmitter::FadeSound
========================
*/
void CSoundEmitter::FadeSound(const s_channelType channel, float to, float over)
{
	assert(soundWorld != NULL);
	assert(soundWorld->emitters[this->index] == this);

	if(soundWorld->writeDemo)
	{
		soundWorld->writeDemo->WriteInt(DS_SOUND);
		soundWorld->writeDemo->WriteInt(SCMD_FADE);
		soundWorld->writeDemo->WriteInt(index);
		soundWorld->writeDemo->WriteInt(channel);
		soundWorld->writeDemo->WriteFloat(to);
		soundWorld->writeDemo->WriteFloat(over);
	}

	int overMSec = SEC2MS(over);

	for(int i = 0; i < channels.Num(); i++)
	{
		idSoundChannel *chan = channels[i];

		if(channel != SCHANNEL_ANY && chan->logicalChannel != channel)
		{
			continue;
		}
		if(s_showStartSound.GetBool())
		{
			idLib::Printf("%dms: FadeSound(%d:%d): %s to %.2fdb over %.2f seconds\n", soundWorld->GetSoundTime(), index, channel, chan->soundShader->GetName(), to, over);
		}

		// fade it
		chan->volumeFade.Fade(to - chan->parms.volume, overMSec, soundWorld->GetSoundTime());
	}
}

/*
========================
CSoundEmitter::CurrentlyPlaying
========================
*/
bool CSoundEmitter::CurrentlyPlaying(const s_channelType channel) const
{
	if(channel == SCHANNEL_ANY)
		return (channels.Num() > 0);

	for(int i = 0; i < channels.Num(); ++i)
	{
		if(channels[i] != NULL && channels[i]->logicalChannel == channel)
		{
			if(channels[i]->endTime == 1)
				return false;
			else
				return true;
		};
	};

	return false;
};

/*
========================
CSoundEmitter::CurrentAmplitude
========================
*/
float CSoundEmitter::CurrentAmplitude()
{
	float amplitude = 0.0f;
	int currentTime = soundWorld->GetSoundTime();
	
	for(int i = 0; i < channels.Num(); i++)
	{
		idSoundChannel *chan = channels[i];
		
		if(chan == NULL || currentTime < chan->startTime || (chan->endTime > 0 && currentTime >= chan->endTime))
			continue;

		int relativeTime = currentTime - chan->startTime;
		int leadinLength = chan->leadinSample->LengthInMsec();
		
		if(relativeTime < leadinLength)
			amplitude = Max(amplitude, chan->leadinSample->GetAmplitude(relativeTime));
		else if(chan->loopingSample != NULL)
			amplitude = Max(amplitude, chan->loopingSample->GetAmplitude((relativeTime - leadinLength) % chan->loopingSample->LengthInMsec()));
	};
	return amplitude;
};