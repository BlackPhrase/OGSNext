/*
 * This file is part of Magenta Engine
 *
 * Copyright (C) 2018 BlackPhrase
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
/// @brief entity interface

#pragma once

#include "CommonTypes.hpp"

interface IGameEntity;

interface IEntity
{
	///
	virtual void SetGameEntity(IGameEntity *apGameEntity) = 0;
	
	///
	virtual IGameEntity *GetGameEntity() const = 0;
	
	//TODO
	virtual struct entvars_s *GetVars() const = 0;
	
	///
	virtual void SetOrigin(const float *org) = 0;
	
	///
	virtual void SetSize(const float *mins, const float *maxs) = 0;
	
	///
	virtual void SetModel(const char *name) = 0;
	
	///
	virtual void *GetModelPtr() const = 0;
	
	///
	virtual void GetBonePosition(int nBone, float *vOrigin, float *vAngles) = 0;
	
	///
	virtual void GetAttachment(int nAttachment, float *vOrigin, float *vAngles) = 0;
	
	///
	virtual int GetIllum() const = 0;
	
	///
	virtual void SaveSpawnParms() = 0;
	
	///
	virtual void GetSpawnParms() = 0;
	
	///
	virtual void AnimationAutomove(float fTime) = 0;
	
	///
	virtual void MoveToOrigin(const float *vGoalPos, float fDist, int nMoveType) = 0;
	
	///
	virtual int WalkMove(float yaw, float dist, int nMode) = 0;
	
	///
	virtual void ChangeYaw() = 0;
	
	///
	virtual void ChangePitch() = 0;
	
	///
	virtual int IsOnFloor() const = 0;
	
	///
	virtual int DropToFloor() = 0;
	
	//
	virtual int GetIndex() const = 0;
	
	///
	//virtual void SetName(const char *asName) = 0;
	
	///
	//virtual const char *GetName() const = 0;
};