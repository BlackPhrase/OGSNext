/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/// @file

#include "BaseWeapon.hpp"

/*
======================================================================

SHOTGUN

======================================================================
*/

class CWeaponSPAS12 : public CBaseWeapon
{
public:
	void Spawn() override;
	
	void PrimaryAttack() override;
};

/*QUAKED weapon_spas12 (0 .5 .8) (-16 -16 0) (16 16 32)
*/
C_EXPORT void weapon_spas12(entvars_t *self)
{
	CWeaponSPAS12::Spawn();
};

void CWeaponSPAS12::Spawn()
{
	//if(deathmatch <= 3)
	{
		gpEngine->pfnPrecacheModel("models/weapons/v_spas12.mdl");
		gpEngine->pfnPrecacheModel("models/weapons/w_spas12.mdl");
		
		SetModel("models/weapons/v_spas12.mdl");

		self->mnID = WEAPON_SPAS12;
		self->netname = "SPAS-12";
		SetTouchCallback(weapon_touch);

		SetSize('-16 -16 0', '16 16 56');

		StartItem(self);
	};
};

void CWeaponSPAS12::PrimaryAttack(edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	int damage = 4;
	int kick = 8;

	if(mpOwner->ps.gunframe == 9)
	{
		mpOwner->ps.gunframe++;
		return;
	};

	AngleVectors(mpOwner->v_angle, forward, right, nullptr);

	VectorScale(forward, -2, mpOwner->kick_origin);
	mpOwner->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(mpOwner, ent->GetOrigin(), offset, forward, right, start);

	if(is_quad)
	{
		damage *= 4;
		kick *= 4;
	};

	if(deathmatch->value)
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
	else
		fire_shotgun(ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->GetOrigin(), MULTICAST_PVS);

	mpOwner->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if(!((int)dmflags->value & DF_INFINITE_AMMO))
		mpOwner->pers.inventory[mpOwner->ammo_index]--;
};

void Weapon_Shotgun(edict_t *ent)
{
	static int pause_frames[] = { 22, 28, 34, 0 };
	static int fire_frames[] = { 8, 9, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}