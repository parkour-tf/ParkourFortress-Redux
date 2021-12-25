#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _OBJECTS_ZIPLINES_INCLUDED
	#endinput
#endif
#define _OBJECTS_ZIPLINES_INCLUDED

//#define DEBUGROPES

stock const char ROPE_REPLACEMENT[] = "cable/rope.vmt";
stock const char ROPE_REPLACEMENT_VTF[] = "cable/rope.vtf";

enum ePFRopeType
{
	ROPE_INVALID = -1,
	ROPE_HEAD = 0,
	ROPE_SEGMENT,
	RAIL_HEAD,
	RAIL_SEGMENT,
	RAIL_RADIAL,
	
	ROPETYPE_COUNT
}

enum ePFRopeProperties
{
	ROPE_TYPE,
	MOVEROPE_IDX,
	ROPE_IDX,
	KEYFRAMEROPE_IDX,
	ROPE_LENGTH,
	HEAD_SEGMENT,
	NEXT_SEGMENT,
	PREVIOUS_SEGMENT,
#if defined DEBUGROPES
	DEBUGSPRITE_START_IDX,
	DEBUGSPRITE_END_IDX,
#endif
	
	ROPEPROP_COUNT
};

methodmap CPFRope < ArrayList
{
	public CPFRope(int iEntRope, ArrayList hHeadRope = ARRAYLIST_INVALID, ArrayList hPrevRope = ARRAYLIST_INVALID)
	{
		if (!IsValidEntity(iEntRope))
			SetFailState("CPFRope::CPFRope --- Invalid entity passed to constructor! Entity: %d", iEntRope);
		
		if (!IsModelPrecached("materials/sprites/old_xfire.vmt"))
			PrecacheModel("materials/sprites/old_xfire.vmt");
		
		if (!IsModelPrecached("materials/sprites/laserbeam.spr"))
			PrecacheModel("materials/sprites/laserbeam.spr");
			
		if (!IsModelPrecached("materials/sprites/glow.vmt"))
			PrecacheModel("materials/sprites/glow.vmt");
		
		// Avoid cache corruption
		char strRopeMaterialModel[PLATFORM_MAX_PATH];
		GetEntPropString(iEntRope, Prop_Data, "m_strRopeMaterialModel", strRopeMaterialModel, sizeof(strRopeMaterialModel));
		if (StrEqual(strRopeMaterialModel, "cable/cable.vmt"))
			DispatchKeyValue(iEntRope, "RopeMaterial", ROPE_REPLACEMENT);
		
		int iKeyframe = GetEntPropEnt(iEntRope, Prop_Data, "m_hEndPoint");
		if (!IsValidEntity(iKeyframe) || iKeyframe == iEntRope)
			return CPFROPE_INVALID;
		
		// At this point, we have a zipline. Assign it an index:
		int iRopeIndex;
		if (g_hRopeControl == null)
			SetFailState("CPFRope::CPFRope --- Controller not initiated!");
		else
			iRopeIndex = g_hRopeControl.Length;
		
		ArrayList hRope = new ArrayList(1, ROPEPROP_COUNT);
		hRope.Set(ROPE_IDX, iRopeIndex);
		hRope.Set(MOVEROPE_IDX, EntIndexToEntRef(iEntRope));
		hRope.Set(KEYFRAMEROPE_IDX, EntIndexToEntRef(iKeyframe));
		
		float vecRopeOrigin[3], vecEndOrigin[3];
		GetEntPropVector(iEntRope, Prop_Data, "m_vecAbsOrigin", vecRopeOrigin);
		GetEntPropVector(iKeyframe, Prop_Data, "m_vecAbsOrigin", vecEndOrigin);
		hRope.Set(ROPE_LENGTH, GetVectorDistance(vecRopeOrigin, vecEndOrigin));
		
		// Set rope type to head if it's the first one in the linked list
		ePFRopeType eType = (hHeadRope != ARRAYLIST_INVALID) ? ROPE_SEGMENT : ROPE_HEAD;
		hRope.Set(ROPE_TYPE, eType);
		
#if defined DEBUGROPES
		int iSpriteStart = CreateSprite((eType == ROPE_HEAD) ? vecRopeOrigin : vecLaserOrigin);
		int iSpriteEnd = CreateSprite(vecEndOrigin);
		
		if (eType == ROPE_HEAD || eType == ROPE_SEGMENT)
		{
			AddEFlags(iSpriteStart, EF_NODRAW);
			AddEFlags(iSpriteEnd, EF_NODRAW);
		}
		else
		{
			AcceptEntityInput(iSpriteStart, "ShowSprite");
			AcceptEntityInput(iSpriteEnd, "ShowSprite");
		}
		
		hRope.Set(DEBUGSPRITE_START_IDX, iSpriteStart);
		hRope.Set(DEBUGSPRITE_END_IDX, iSpriteEnd);
#endif
		
		if (eType == ROPE_HEAD)
		{
			// Rope heads have no previous zipline, and have themselves as a head segment
			hRope.Set(PREVIOUS_SEGMENT, CPFROPE_INVALID);
			hRope.Set(HEAD_SEGMENT, hRope);
			hHeadRope = hRope;
			
			g_hRopeControl.Push(hRope);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRope.Get(MOVEROPE_IDX)), hRope.Get(ROPE_IDX));
		}
		else
		{
			hRope.Set(HEAD_SEGMENT, hHeadRope);
			hRope.Set(PREVIOUS_SEGMENT, hPrevRope);
			
			g_hRopeControl.Push(hRope);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRope.Get(MOVEROPE_IDX)), hRope.Get(ROPE_IDX));
			
			DebugOutput("CPFRope::CPFRope - Found Loop!");
			// If the keyframe is the initial move_rope, or already in the controller, we have a loop
			if (iKeyframe == EntRefToEntIndex(hHeadRope.Get(MOVEROPE_IDX)) || g_hMoveRopeToIndex.Get(iKeyframe))
			{
				hHeadRope.Set(PREVIOUS_SEGMENT, hRope);
				hRope.Set(NEXT_SEGMENT, hHeadRope);
				
				// We're done here
				return view_as<CPFRope>(hRope);
			}
		}
		
		hPrevRope = hRope;
		
		// Recursively set up the next ziplines, finishing them from tail to head.
		int iNextKeyframe = GetEntPropEnt(iKeyframe, Prop_Data, "m_hEndPoint");
		if (IsValidEntity(iNextKeyframe))
		{
			CPFRope hNext = MakeLinkedRope(iKeyframe, hHeadRope, hPrevRope);
			hRope.Set(PREVIOUS_SEGMENT, hNext);
		}
		else
		{
			// We're at the tail end of the chain
			hRope.Set(NEXT_SEGMENT, CPFROPE_INVALID);
		}
		
		return view_as<CPFRope>(hRope);
	}

	
	property int EntIndex
	{
		public get()
		{
			return this.Get(MOVEROPE_IDX);
		}
		
		public set(int idx)
		{
			this.Set(MOVEROPE_IDX, idx);
		}
	}
	
	property int RopeIndex
	{
		public get()
		{
			return this.Get(ROPE_IDX);
		}
	}
	
	property int KeyframeIndex
	{
		public get()
		{
			return this.Get(KEYFRAMEROPE_IDX);
		}
		
		public set(int idx)
		{
			this.Set(KEYFRAMEROPE_IDX, idx);
		}
	}
	
	property float RopeLength
	{
		public get()
		{
			return this.Get(ROPE_LENGTH);
		}
	}
	
#if defined DEBUGROPES
	property int DebugSpriteStart
	{
		public get()
		{
			return this.Get(DEBUGSPRITE_START_IDX);
		}
		
		public set(int idx)
		{
			this.Set(DEBUGSPRITE_START_IDX, idx);
		}
	}
	
	property int DebugSpriteEnd
	{
		public get()
		{
			return this.Get(DEBUGSPRITE_END_IDX);
		}
		
		public set(int idx)
		{
			this.Set(DEBUGSPRITE_END_IDX, idx);
		}
	}
#endif
	
	property ePFRopeType RopeType
	{
		public get()
		{
			return this.Get(ROPE_TYPE);
		}
		
		public set(ePFRopeType val)
		{
			this.Set(ROPE_TYPE, val);
		}
	}
	
	property CPFRope NextSegment
	{
		public get()
		{
			return this.Get(NEXT_SEGMENT);
		}
		
		public set(CPFRope hRope)
		{
			this.Set(NEXT_SEGMENT, hRope);
		}
	}
};

CPFRope MakeLinkedRope(int iKeyframe, ArrayList hHeadSegment, ArrayList hPrevSegment)
{
	return new CPFRope(iKeyframe, hHeadSegment, hPrevSegment);
}