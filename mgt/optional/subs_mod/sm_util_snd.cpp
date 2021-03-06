//=============================================================//
//	Half-Life Update MOD
//	https://github.com/Fograin/hl-subsmod-ex
//	
//	Written by: Vit_amiN
//  Edits by: Fograin92
//
//	Before using any parts of this code, read licence.txt file 
//=============================================================//
#ifndef DISABLE_ENGINE_HOOKS

#ifdef CLIENT_DLL
    #include "..\cl_dll\hud.h"
    #include "..\cl_dll\cl_util.h"
    #include "..\cl_dll\vgui_TeamFortressViewport.h"
#else
    #include "extdll.h"
    #include "util.h"
    #include "cbase.h"
#endif

#include "sm_consts.h"
#include "sm_defines.h"
#include "sm_util_sp.h"
#include "sm_util_snd.h"
#include "sm_gamespec.h"


// 'Vector' class has operator 'float *' returns vec_t *
extern "C" float Distance(const float *, const float *);
static client_textmessage_sndprops_t snd_props;

static const char * const SM_GetSoundMsgTokenName( const char * const pString )
{
    if (pString && pString[0] != '!')
    {
        const char * pWavName = pString;
        if (pWavName[0] == '*') pWavName++;

        for (int grp_index = 0; grp_index < ARRAYSIZE(pSoundGroups); grp_index++)
        {
            if (!strnicmp(pWavName, pSoundGroups[grp_index].pGroupName, pSoundGroups[grp_index].iGrNameLen))
            {
                pWavName += pSoundGroups[grp_index].iGrNameLen;
        
                for (int snt_index = 0; snt_index < pSoundGroups[grp_index].iGroupSize; snt_index++)
                {
                    if (!stricmp(pWavName, pSoundGroups[grp_index].pSentences[snt_index].pFileName))
                    {
                        return pSoundGroups[grp_index].pSentences[snt_index].pSentence;
                    }
                }
                //Con_DevMessage("Unable to find sentence for \"%s\" (sound group is \"%s\")\n", pString, pSoundGroups[grp_index].pGroupName);
                return NULL;
            }
        }
        //Con_DevMessage("Unable to find sentence for \"%s\" (sound group is unrecognized)\n", pString);
        return NULL;
    }
    return pString;
}

static float SM_PlaySound_Hook_SENDFUNC__FIXME_(
    const int & entIndex,
    const char * const pString,
    const float * const sndOrigin,
    const int & sndChannel,
    const float & sndVolume,
    const float & sndAttenuation,
    const int & sndFlags,
    const int & sndPitch
)
{
    if (sndPitch >= SND_PITCH_MIN_VALUE && sndPitch <= SND_PITCH_MAX_VALUE)
    {
        const char * sentence = SM_GetSoundMsgTokenName(pString);
        if (sentence)
        {
            snd_props.sndEntity  = entIndex;
            snd_props.hasOrigin  = sndOrigin != NULL;
            if (snd_props.hasOrigin)
            {
                snd_props.sndOrigin[0] = sndOrigin[0];
                snd_props.sndOrigin[1] = sndOrigin[1];
                snd_props.sndOrigin[2] = sndOrigin[2];
            } else {
                snd_props.sndOrigin[0] = 0.0f;
                snd_props.sndOrigin[1] = 0.0f;
                snd_props.sndOrigin[2] = 0.0f;
            }
            snd_props.sndChannel = sndChannel;
            snd_props.sndFlags   = sndFlags;
            snd_props.sndPitch   = sndPitch;
            snd_props.sndVolume  = sndVolume;
            snd_props.sndAtten   = sndAttenuation;  // when it is 0 (e.g. tride), distance doesn't change the volume
        
        #ifdef CLIENT_DLL
        
            if (gViewPort)
            {
                const client_textmessage_bundle_t * msg_bundle = gViewPort->GetSMClientMessage(sentence);

                // If we couldn't find or create a corresponding message, do nothing here
                if (msg_bundle)
                {
                    const cl_entity_t * pPlayer = gEngfuncs.GetLocalPlayer();  // FIXME: may we make use of gHUD origin?
                    if (pPlayer && pPlayer->player)
                    {
                        //FIXME: check for StopSound!!! if ((sndFlags & SND_STOP) == SND_STOP || sndVolume == 0)
                        float distance = snd_props.hasOrigin ? Distance(snd_props.sndOrigin, pPlayer->origin) : 0.0f;
                    
                        if (SND_VOL_THRESHOLD < sndVolume * (1.0f - sndAttenuation * distance / SND_CLIP_DISTANCE))
                        {
                            gViewPort->AddSMTextMessage(MSG_TYPE_SUBTITLE, msg_bundle, &snd_props, gHUD.m_flTime);
                        } else {
                            //Con_DevMessage("CLIENT: sentence '%s' is inaudible to the local player: volume = %.2f, attenuation = %.2f, pitch = %i, distance = %.2f\n", sentence, sndVolume, sndAttenuation, sndPitch, distance );
                        }
                    }
                }
            }
            
        #else
            
            // FIXME: server may be isn't activated yet... use g_serveractive ?
            const char * sub_message = SM_SUBTITLE_TO_STR( sentence, &snd_props );

            // loop through all players
            for ( int i = 1; i <= gpGlobals->maxClients; i++ )
            {
                CBaseEntity * pPlayer = UTIL_PlayerByIndex( i );
                if (pPlayer && pPlayer->IsNetClient())
                {
                    //FIXME: if ((sndFlags & SND_STOP) == SND_STOP || sndVolume == 0), do not check distance (or check?????) -- transmit further (StopSound)
                
                    float distance = sndOrigin ? Distance(sndOrigin, pPlayer->EarPosition()) : 0.0f;

                    if (SND_VOL_THRESHOLD < sndVolume * (1.0f - sndAttenuation * distance / SND_CLIP_DISTANCE))
                    {
                        UTIL_ShowMessage( sub_message, pPlayer );
                    } else {
                        //Con_DevMessage("SERVER: Sentence '%s' is inaudible to the player [id %i]: volume = %.2f, attenuation = %.2f, pitch = %i, distance = %.2f\n", sentence, i, sndVolume, sndAttenuation, sndPitch, distance );
                    }
                }
            }

        #endif // CLIENT_DLL
        }
    }
    return sndVolume;
}

float SM_Hook_Shared_PM_PlaySound( const char * const pString, const int sndChannel, const float sndVolume, const float sndAttenuation, const int sndFlags, const int sndPitch )
{
	char szFootStepName[256];

	// Fograin92: Check if it's OF
	if (CVAR_GET_FLOAT("sm_hud") == 2.0 )
	{
		if( strncmp( pString, "player/pl_step1", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_step1_of.wav");
		else if( strncmp( pString, "player/pl_step2", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_step2_of.wav");
		else if( strncmp( pString, "player/pl_step3", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_step3_of.wav");
		else if( strncmp( pString, "player/pl_step4", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_step4_of.wav");
		else if( strncmp( pString, "player/pl_tile1", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_tile1_of.wav");
		else if( strncmp( pString, "player/pl_tile2", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_tile2_of.wav");
		else if( strncmp( pString, "player/pl_tile3", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_tile3_of.wav");
		else if( strncmp( pString, "player/pl_tile4", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_tile4_of.wav");
		else if( strncmp( pString, "player/pl_tile5", 15 ) == 0 )
			sprintf(szFootStepName, "player/pl_tile5_of.wav");
		else
			sprintf(szFootStepName, "%s", pString);
	}
	else
		sprintf(szFootStepName, "%s", pString);



	// Fograin92: Hook sounds from shared code into new audio engine
#ifndef CLIENT_DLL
	EXEmitSound( FIND_ENTITY_BY_CLASSNAME(NULL, "player"), sndChannel, szFootStepName, sndVolume*SM_VOLUME_FOOTSTEPS, sndAttenuation, sndFlags, sndPitch );
#endif

    return SM_PlaySound_Hook_SENDFUNC__FIXME_(0 /* FIXME: "clgame.pmove->player_index + 1" on client, "svgame.pmove->player_index + 1" on server; check if it needs +1 or exact index */, szFootStepName, NULL /* FIXME: NULL on client, ENTINDEX(first_param) (? -- check) on server */, sndChannel, sndVolume, sndAttenuation, sndFlags, sndPitch);
}

#ifdef CLIENT_DLL

float SM_Hook_Client_EV_PlaySound( const int entIndex, const char * const pString, const float * const vecOrigin, const int sndChannel, const float sndVolume, const float sndAttenuation, const int sndFlags, const int sndPitch )
{
    return SM_PlaySound_Hook_SENDFUNC__FIXME_(entIndex, pString, vecOrigin, sndChannel, sndVolume, sndAttenuation, sndFlags, sndPitch);
}

float SM_Hook_Client_pfnPlaySoundByName( const char * const pString, const float sndVolume )
{
    const cl_entity_t * pPlayer = gEngfuncs.GetLocalPlayer();
    return SM_PlaySound_Hook_SENDFUNC__FIXME_(pPlayer ? pPlayer->index : 0 /* FIXME: cl.refdef.viewentity -- index подходит??? */, pString, NULL, CHAN_AUTO, sndVolume, ATTN_NORM, 0, PITCH_NORM);
}

float SM_Hook_Client_pfnPlaySoundByIndex( const int sndIndex, const float sndVolume )
{
    return sndVolume;   // Placeholder
}

/*
=============
pfnPlaySoundByIndex

=============

static void pfnPlaySoundByIndex( int sndIndex, float sndVolume )
{
    int hSound;

    // make sure what we in-bounds
    sndIndex = bound( 0, sndIndex, MAX_SOUNDS );
    hSound = cl.sound_index[sndIndex];

    if( !hSound )
    {
        MsgDev( D_ERROR, "CL_PlaySoundByIndex: invalid sound handle %i\n", sndIndex );
        return;
    }
    S_StartSound( NULL, cl.refdef.viewentity, CHAN_AUTO, hSound, sndVolume, ATTN_NORM, PITCH_NORM, 0 );
}
*/


float SM_Hook_Client_pfnPlaySoundByNameAtPitch( const char * const pString, const float sndVolume, const int sndPitch )
{
    return SM_PlaySound_Hook_SENDFUNC__FIXME_(0, pString, NULL, CHAN_AUTO, sndVolume, ATTN_NORM, 0, sndPitch); //FIXME: bogus entity ID
}


float SM_Hook_Client_pfnPlaySoundByNameAtLocation( const char * const pString, const float * const vecOrigin, const float sndVolume )
{
    return SM_PlaySound_Hook_SENDFUNC__FIXME_(0, pString, vecOrigin, CHAN_AUTO, sndVolume, ATTN_NORM, 0, PITCH_NORM);
    /*
        FIXME:
        hooked func calls:    S_StartSound( vecOrigin, 0, CHAN_AUTO, S_RegisterSound(pString), sndVolume, ATTN_NORM, PITCH_NORM, 0 );
    */
}


float SM_Hook_Client_pfnPlaySoundVoiceByName( const char * const pString, const float sndVolume, const int sndPitch )
{
    return SM_PlaySound_Hook_SENDFUNC__FIXME_(0, pString, NULL, CHAN_AUTO, sndVolume, ATTN_NORM, 0, sndPitch); //FIXME: bogus entity ID
}

#else

void SM_Hook_Server_EMIT_SOUND_DYN2( edict_t * const entity, const char * const pString, const int sndChannel, const float sndVolume, const float sndAttenuation, const int sndFlags, const int sndPitch )
{
	// Fograin92
	//char xszPath[256];
	//sprintf(xszPath, "%s\n", pString);
	//ALERT ( at_console, xszPath );

	// Fograin92: Pass sound dir, name or sentence ID to the new sound engine
	if( pString[0] == '!' )
	{
		char name[32];
		int iSentenceNum = SENTENCEG_Lookup(pString, name);

		// Fograin92: Check if this is HEV sentence
		if( strncmp( pString, "!HEV", 4 ) == 0 )
		{
			// Fograin92: Adjust volume using HEV Suit volume
			EXEmitSound(entity, sndChannel, name, sndVolume*SM_VOLUME_HEV, sndAttenuation, sndFlags, sndPitch);
		}

		// Fograin92: Check if this is random world (ambient) sentence OR creature sentence (e.g. SLV_IDLE)
		else if( 
			(strncmp( pString, "!WILD", 5 ) == 0)
			|| (strncmp( pString, "!ROCKET", 7 ) == 0)
			|| (strncmp( pString, "!FAR_WAR", 8 ) == 0)
			|| (strncmp( pString, "!NEAR_WAR", 9 ) == 0)
			|| (strncmp( pString, "!SLV_", 5 ) == 0)
			|| (strncmp( pString, "!ST_", 4 ) == 0) )
		{
			// Fograin92: All world-ambient sentences AND creature voices are controlled by SFX Volume
			EXEmitSound(entity, sndChannel, name, sndVolume*SM_VOLUME_SFX, sndAttenuation, sndFlags, sndPitch);
		}

		else
		{
			// Fograin92: It's a spoken (NPC) sentence, pass sentenceID and adjust volume with Voice Volume CVAR
			EXEmitSound(entity, sndChannel, name, sndVolume*SM_VOLUME_VOICE, sndAttenuation, sndFlags, sndPitch);
		}
	} // End if( pString[0] == '!' )


	// Fograin92: Check if this is non-sentenced voice emit
	else if( 
			(strncmp( pString, "barney/", 7 ) == 0)
			|| (strncmp( pString, "drill/", 6 ) == 0)
			|| (strncmp( pString, "rosenberg/", 10 ) == 0)
			|| (strncmp( pString, "scientist/", 10 ) == 0)
			|| (strncmp( pString, "gman/", 5 ) == 0)
			|| (strncmp( pString, "hgrunt/", 7 ) == 0)
			|| (strncmp( pString, "holo/", 5 ) == 0)
			|| (strncmp( pString, "ba_holo/", 8 ) == 0)
			|| (strncmp( pString, "otis/", 5 ) == 0)
			|| (strncmp( pString, "fgrunt/", 7 ) == 0)
			|| (strncmp( pString, "intro/", 6 ) == 0)
			|| (strncmp( pString, "ops/", 4 ) == 0)
			)
	{
		EXEmitSound(entity, sndChannel, pString, sndVolume*SM_VOLUME_VOICE, sndAttenuation, sndFlags, sndPitch);
	}


	// Fograin92: SFX voice
	else
	{
		// Fograin92: It's non-sentenced sound, pass dir + name
		EXEmitSound(entity, sndChannel, pString, sndVolume*SM_VOLUME_SFX, sndAttenuation, sndFlags, sndPitch);
	}

    SM_PlaySound_Hook_SENDFUNC__FIXME_(ENTINDEX(entity), pString, (entity->v.absmin + entity->v.absmax) * 0.5f, sndChannel, sndVolume, sndAttenuation, sndFlags, sndPitch);
}

void SM_Hook_Server_EMIT_AMBIENT_SOUND( edict_t * const entity, const char * const pString, const float * const vecOrigin, const float sndVolume, const float sndAttenuation, const int sndFlags, const int sndPitch )
{
	// Fograin92: Hook new audio system (Channel 6 for static sound)
	//EXEmitSound(entity, 6, pString, sndVolume, sndAttenuation, sndFlags, sndPitch);


	// Fograin92: Pass sound dir, name or sentence ID to the new sound engine
	if( pString[0] == '!' )
	{
		char name[32];
		int iSentenceNum = SENTENCEG_Lookup(pString, name);

		// Fograin92: Check if this is HEV sentence
		if( strncmp( pString, "!HEV", 4 ) == 0 )
		{
			// Fograin92: Adjust volume using HEV Suit volume
			EXEmitSound(entity, 6, name, sndVolume*SM_VOLUME_HEV, sndAttenuation, sndFlags, sndPitch);
		}

		// Fograin92: Check if this is random world (ambient) sentence OR creature sentence (e.g. SLV_IDLE)
		else if( 
			(strncmp( pString, "!WILD", 5 ) == 0)
			|| (strncmp( pString, "!ROCKET", 7 ) == 0)
			|| (strncmp( pString, "!FAR_WAR", 8 ) == 0)
			|| (strncmp( pString, "!NEAR_WAR", 9 ) == 0)
			|| (strncmp( pString, "!SLV_", 5 ) == 0)
			|| (strncmp( pString, "!ST_", 4 ) == 0) )
		{
			// Fograin92: All world-ambient sentences AND creature voices are controlled by SFX Volume
			EXEmitSound(entity, 6, name, sndVolume*SM_VOLUME_SFX, sndAttenuation, sndFlags, sndPitch);
		}

		else
		{
			// Fograin92: It's a spoken (NPC) sentence, pass sentenceID and adjust volume with Voice Volume CVAR
			EXEmitSound(entity, 6, name, sndVolume*SM_VOLUME_VOICE, sndAttenuation, sndFlags, sndPitch);
		}
	} // End if( pString[0] == '!' )


	// Fograin92: Check if this is non-sentenced voice emit
	else if( 
			(strncmp( pString, "barney/", 7 ) == 0)
			|| (strncmp( pString, "drill/", 6 ) == 0)
			|| (strncmp( pString, "rosenberg/", 10 ) == 0)
			|| (strncmp( pString, "scientist/", 10 ) == 0)
			|| (strncmp( pString, "gman/", 5 ) == 0)
			|| (strncmp( pString, "hgrunt/", 7 ) == 0)
			|| (strncmp( pString, "holo/", 5 ) == 0)
			|| (strncmp( pString, "ba_holo/", 8 ) == 0)
			|| (strncmp( pString, "otis/", 5 ) == 0)
			|| (strncmp( pString, "fgrunt/", 7 ) == 0)
			|| (strncmp( pString, "intro/", 6 ) == 0)
			|| (strncmp( pString, "ops/", 4 ) == 0)
			)
	{
		EXEmitSound(entity, 6, pString, sndVolume*SM_VOLUME_VOICE, sndAttenuation, sndFlags, sndPitch);
	}


	// Fograin92: SFX voice
	else
	{
		// Fograin92: It's non-sentenced sound, pass dir + name
		EXEmitSound(entity, 6, pString, sndVolume*SM_VOLUME_SFX, sndAttenuation, sndFlags, sndPitch);
	}


    SM_PlaySound_Hook_SENDFUNC__FIXME_(ENTINDEX(entity), pString, vecOrigin, CHAN_AUTO, sndVolume, sndAttenuation, sndFlags, sndPitch);
}

#endif  // CLIENT_DLL

#endif  // DISABLE_ENGINE_HOOKS
