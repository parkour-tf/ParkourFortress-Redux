#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _OBJECTS_RAILS_INCLUDED
	#endinput
#endif
#define _OBJECTS_RAILS_INCLUDED

#define RAIL_SPRITE_MATERIAL "materials/sprites/light_glow03.vmt"
#define RAIL_SPRITE_MATERIAL_VTF "materials/sprites/light_glow03.vtf"

bool g_bHasRails;

enum ePFRailProperties
{
	RAIL_TYPE,
	MOVEROPE_IDX_RAIL,
	ROPE_IDX_RAIL,
	KEYFRAMEROPE_IDX_RAIL,
	ROTATOR_IDX,
	RADIAL_DIR,
	RAIL_LENGTH_TOTAL,
	HEAD_SEGMENT_RAIL,
	NEXT_SEGMENT_RAIL,
	PREVIOUS_SEGMENT_RAIL,
	FORWARD_MOUNT_ONLY,
	
	RAILPROP_COUNT
};

methodmap CPFRail < ArrayList
{
	public CPFRail(int iEntRope, ArrayList hHeadRail = ARRAYLIST_INVALID, ArrayList hPrevRail = ARRAYLIST_INVALID)
	{
		if (!IsValidEntity(iEntRope))
			SetFailState("CPFRail::CPFRail --- Invalid entity passed to constructor! Entity: %d", iEntRope);
		
		if (!IsModelPrecached("materials/sprites/old_xfire.vmt"))
			PrecacheModel("materials/sprites/old_xfire.vmt");
		
		if (!IsModelPrecached("materials/sprites/laserbeam.spr"))
			PrecacheModel("materials/sprites/laserbeam.spr");
		
		if (!IsModelPrecached("RAIL_SPRITE_MATERIAL"))
			PrecacheModel("RAIL_SPRITE_MATERIALt");
		
		// Avoid cache corruption
		char strRopeMaterialModel[PLATFORM_MAX_PATH];
		GetEntPropString(iEntRope, Prop_Data, "m_strRopeMaterialModel", strRopeMaterialModel, sizeof(strRopeMaterialModel));
		if (StrEqual(strRopeMaterialModel, "cable/cable.vmt"))
			DispatchKeyValue(iEntRope, "RopeMaterial", ROPE_REPLACEMENT);
		
		// Verify targetname has "rail_" in it
		char strTargetname[128];
		GetEntPropString(iEntRope, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrContains(strTargetname, "rail", false) == -1)
			return CPFRAIL_INVALID;
		
		int iKeyframe = GetEntPropEnt(iEntRope, Prop_Data, "m_hEndPoint");
		if (!IsValidEntity(iKeyframe) || iKeyframe == iEntRope)
			return CPFRAIL_INVALID;
		
		// At this point, we have a rail. Assign it an index:
		int iRopeIndex;
		if (g_hRailControl == null)
			SetFailState("CPFRail::CPFRail --- Controller not initiated!");
		else
			iRopeIndex = g_hRailControl.Length;
		
		g_bHasRails = true;
		ArrayList hRail = new ArrayList(1, RAILPROP_COUNT);
		hRail.Set(ROPE_IDX_RAIL, iRopeIndex);
		hRail.Set(MOVEROPE_IDX_RAIL, EntIndexToEntRef(iEntRope));
		hRail.Set(KEYFRAMEROPE_IDX_RAIL, EntIndexToEntRef(iKeyframe));
		hRail.Set(ROTATOR_IDX, -1);
		
		// Get the rail segment's total length
		float vecRailOrigin[3], vecEndOrigin[3];
		GetEntPropVector(iEntRope, Prop_Data, "m_vecAbsOrigin", vecRailOrigin);
		GetEntPropVector(iKeyframe, Prop_Data, "m_vecAbsOrigin", vecEndOrigin);
		hRail.Set(RAIL_LENGTH_TOTAL, GetVectorDistance(vecRailOrigin, vecEndOrigin));
		
		// Check the targetname to find out if it blocks 2-way mounting
		if (StrContains(strTargetname, "fwd", false) != -1)
			hRail.Set(FORWARD_MOUNT_ONLY, true);
		else
			hRail.Set(FORWARD_MOUNT_ONLY, false);
		
		// Set rail type to head if it's the first one in the linked list
		ePFRopeType eType = (hHeadRail != ARRAYLIST_INVALID) ? RAIL_SEGMENT : RAIL_HEAD;
		hRail.Set(RAIL_TYPE, eType);
		
		if (eType == RAIL_HEAD)
		{
			// Rail heads have no previous rail, and have themselves as a head segment
			hRail.Set(PREVIOUS_SEGMENT_RAIL, CPFRAIL_INVALID);
			hRail.Set(HEAD_SEGMENT_RAIL, hRail);
			hHeadRail = hRail;
			
			g_hRailControl.Push(hRail);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRail.Get(MOVEROPE_IDX_RAIL)), hRail.Get(ROPE_IDX_RAIL));
			
			// Spawn a blue sprite at the mounting point
			if (StrContains(strTargetname, "nosprite", false) == -1)
				CreateRailSprites(vecRailOrigin, {0, 128, 255});
		}
		else
		{
			hRail.Set(HEAD_SEGMENT_RAIL, hHeadRail);
			hRail.Set(PREVIOUS_SEGMENT_RAIL, hPrevRail);
			
			g_hRailControl.Push(hRail);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRail.Get(MOVEROPE_IDX_RAIL)), hRail.Get(ROPE_IDX_RAIL));
			
			// If the keyframe is the initial move_rope, or already in the controller, we have a loop
			if (iKeyframe == EntRefToEntIndex(hHeadRail.Get(MOVEROPE_IDX_RAIL)) || g_hMoveRopeToIndex.Get(iKeyframe))
			{
				hHeadRail.Set(PREVIOUS_SEGMENT_RAIL, hRail);
				hRail.Set(NEXT_SEGMENT_RAIL, hHeadRail);
				
				// We're done here
				return view_as<CPFRail>(hRail);
			}
		}
		
		hPrevRail = hRail;
		
		// Recursively set up the next rails, finishing them from tail to head.
		int iNextKeyframe = GetEntPropEnt(iKeyframe, Prop_Data, "m_hEndPoint");
		if (IsValidEntity(iNextKeyframe))
		{
			CPFRail hNext = MakeLinkedRail(iKeyframe, hHeadRail, hPrevRail);
			hRail.Set(NEXT_SEGMENT_RAIL, hNext);
		}
		else
		{
			// We're at the tail end of the chain
			hRail.Set(NEXT_SEGMENT_RAIL, CPFRAIL_INVALID);
			
			// Spawn the ending sprite, blue if you can mount in reverse, red otherwise
			if (StrContains(strTargetname, "nosprite", false) == -1)
				CreateRailSprites(vecEndOrigin, (hRail.Get(FORWARD_MOUNT_ONLY)) ? {255, 0, 0} : {0, 128, 255});
		}
		return view_as<CPFRail>(hRail);
	}

	property int EntIndex
	{
		public get()
		{
			return this.Get(MOVEROPE_IDX_RAIL);
		}
		
		public set(int idx)
		{
			this.Set(MOVEROPE_IDX_RAIL, idx);
		}
	}
	
	property int RopeIndex
	{
		public get()
		{
			return this.Get(ROPE_IDX_RAIL);
		}
	}
	
	property int KeyframeIndex
	{
		public get()
		{
			return this.Get(KEYFRAMEROPE_IDX_RAIL);
		}
		
		public set(int idx)
		{
			this.Set(KEYFRAMEROPE_IDX_RAIL, idx);
		}
	}
	
	property int RotatorIndex
	{
		public get()
		{
			return this.Get(ROTATOR_IDX);
		}
		
		public set(int idx)
		{
			this.Set(ROTATOR_IDX, idx);
		}
	}
	
	property ePFRotDirection Direction
	{
		public get()
		{
			return this.Get(RADIAL_DIR);
		}
		
		public set(ePFRotDirection eDir)
		{
			this.Set(RADIAL_DIR, eDir);
		}
	}
	
	property float RailLength
	{
		public get()
		{
			return this.Get(RAIL_LENGTH_TOTAL);
		}
	}
	
	property ePFRopeType RopeType
	{
		public get()
		{
			return this.Get(RAIL_TYPE);
		}
		
		public set(ePFRopeType val)
		{
			this.Set(RAIL_TYPE, val);
		}
	}
	
	property CPFRail HeadSegment
	{
		public get()
		{
			return this.Get(HEAD_SEGMENT_RAIL);
		}
		
		public set(CPFRail hRail)
		{
			this.Set(HEAD_SEGMENT_RAIL, hRail);
		}
	}
	
	property CPFRail NextSegment
	{
		public get()
		{
			return this.Get(NEXT_SEGMENT_RAIL);
		}
		
		public set(CPFRail hRail)
		{
			this.Set(NEXT_SEGMENT_RAIL, hRail);
		}
	}
	
	property CPFRail PreviousSegment
	{
		public get()
		{
			return this.Get(PREVIOUS_SEGMENT_RAIL);
		}
		
		public set(CPFRail hRail)
		{
			this.Set(PREVIOUS_SEGMENT_RAIL, hRail);
		}
	}
	
	property bool ForwardMountOnly
	{
		public get()
		{
			return view_as<bool>(this.Get(FORWARD_MOUNT_ONLY));
		}
		
		public set(bool bFwd)
		{
			this.Set(FORWARD_MOUNT_ONLY, bFwd);
		}
	}
}

CPFRail MakeLinkedRail(int iEntRope, ArrayList hHeadRail, ArrayList hPrevRail)
{
	return new CPFRail(iEntRope, hHeadRail, hPrevRail);
}

void CreateRailSprites(float vecOrigin[3], int iColor[3] = {255, 0, 0})
{ 
	int iEntity = CreateEntityByName( "env_sprite" );
	
	SetEntityModel(iEntity, "materials/sprites/light_glow03.vmt");
	SetEntityRenderColor(iEntity, iColor[0], iColor[1], iColor[2]);

	SetEdictFlags(iEntity, FL_EDICT_ALWAYS);

	SetEntityRenderMode(iEntity, RENDER_WORLDGLOW);  
	DispatchKeyValue(iEntity, "GlowProxySize", "5000");
	DispatchKeyValue(iEntity, "renderamt", "255"); 
	DispatchKeyValue(iEntity, "framerate", "10.0"); 
	DispatchKeyValue(iEntity, "scale", "100.0");
	DispatchKeyValue(iEntity, "spawnflags", "1");

	SetEntProp(iEntity, Prop_Data, "m_bWorldSpaceScale", 1); 
	DispatchSpawn(iEntity); 

	CreateTimer(0.1, CreateRailSpriteTimer, EntIndexToEntRef(iEntity));
 
	TeleportEntity(iEntity, vecOrigin, NULL_VECTOR, NULL_VECTOR);
}

public Action CreateRailSpriteTimer(Handle hTimer, int iEntity)
{
	AcceptEntityInput(EntRefToEntIndex(iEntity), "ShowSprite");
}