#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _OBJECTS_ROPES_INCLUDED
	#endinput
#endif
#define _OBJECTS_ROPES_INCLUDED

methodmap CPFRopeController
{
	public static bool IsRopeStart(int iMoveRope)
	{
		if (!IsValidEntity(iMoveRope)) return false;
		
		char strClassname[128];
		if (!GetEntityClassname(iMoveRope, strClassname, sizeof(strClassname))) return false;
		if (!StrEqual("move_rope", strClassname)) return false;
		
		char strTargetname[128];
		GetEntPropString(iMoveRope, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrContains(strTargetname, "ornament", false) != -1 || StrContains(strTargetname, "rail", false) != -1) return false;
		
		return true;
	}
	
	public static bool IsRailStart(int iMoveRope)
	{
		if (!IsValidEntity(iMoveRope)) return false;
		
		char strClassname[128];
		if (!GetEntityClassname(iMoveRope, strClassname, sizeof(strClassname))) return false;
		if (!StrEqual("move_rope", strClassname)) return false;
		
		char strTargetname[128];
		GetEntPropString(iMoveRope, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrContains(strTargetname, "ornament", false) != -1 || StrContains(strTargetname, "rail", false) == -1) return false;
		
		return true;
	}
	
	public static void AddRope(CPFRope hRope)
	{
		if (hRope != null)
		{
			g_hRopeControl.Push(hRope);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRope.EntIndex), hRope.RopeIndex);
		}
		else
			SetFailState("CPFRopeController::AddRope --- Rope intended for index %d is null!", (g_hRopeControl.Length) ? g_hRopeControl.Length - 1 : 0);
	}
	
	public static void AddRail(CPFRail hRail)
	{
		if (hRail != null)
		{
			g_hRailControl.Push(hRail);
			g_hMoveRopeToIndex.Set(EntRefToEntIndex(hRail.EntIndex), hRail.RopeIndex);
		}
		else
			SetFailState("CPFRopeController::AddRail --- Rail intended for index %d is null!", (g_hRailControl.Length) ? g_hRailControl.Length - 1 : 0);
	}
	
	public static void RemoveRope(CPFRope hRope, bool bRails = false)
	{
		ArrayList hController = (bRails) ? g_hRailControl : g_hRopeControl;
		
		if (hRope != null)
		{
			hController.Erase(hRope.RopeIndex);
			delete hRope;
		}
		else
			SetFailState("CPFRopeController::RemoveRope --- Attempted to remove null %s!", (bRails) ? "rail" : "rope");
	}
	
	public static void SetRope(CPFRope hRope, int idx, bool bRails = false)
	{
		ArrayList hController = (bRails) ? g_hRailControl : g_hRopeControl;
		
		if (hRope != null)
			hController.Set(idx, hRope);
		else
			SetFailState("CPFRopeController::SetRope --- Attempted to set null %s at index %d!", (bRails) ? "rail" : "rope", idx);
	}
	
	public static CPFRope GetRope(int idx, bool bRails = false)
	{
		ArrayList hController = (bRails) ? g_hRailControl : g_hRopeControl;
		
		if (idx < hController.Length)
			return hController.Get(idx);
		else
			SetFailState("CPFRopeController::GetRope --- Attempted to fetch %s at invalid index %d!", (bRails) ? "rail" : "rope", idx);
		
		return null;
	}
	
	public static int Total(bool bRails = false)
	{
		ArrayList hController = (bRails) ? g_hRailControl : g_hRopeControl;
		
		if (hController == null)
			return 0;
		
		return hController.Length;
	}
	
	public static CPFRope FindRope(int iRope, bool bRails = false)
	{
		if (g_hMoveRopeToIndex == null || !CPFRopeController.Total(bRails))
			return CPFROPE_INVALID;
		
		return CPFRopeController.GetRope(g_hMoveRopeToIndex.Get(iRope), bRails);
	}
	
	public static void Init(bool bLate = false)
	{
		// Precache our rope replacement texture
		PrecacheGeneric(ROPE_REPLACEMENT, true); 
		PrecacheGeneric(ROPE_REPLACEMENT_VTF, true);
		
		// Precache the rail sprite material
		PrecacheModel(RAIL_SPRITE_MATERIAL, true);
		PrecacheGeneric(RAIL_SPRITE_MATERIAL_VTF, true);
		
		if (bLate)
		{
			g_hRopeControl = new ArrayList();
			g_hRailControl = new ArrayList();
			g_hMoveRopeToIndex = new ArrayList(_,2049);
			
			// Zerofill to avoid garbage in arraylist
			for (int i = 1; i < 2048; i++)
			{
				g_hMoveRopeToIndex.Set(i, 0);
			}
			
			for (int i = 1; i < 2048; i++)
			{
				if (CPFRopeController.IsRopeStart(i))
				{
					CPFRope hRope = new CPFRope(i);
					if (hRope != null)
					{
						CPFRopeController.AddRope(hRope);
						DebugOutput("CPFRopeController::Init --- Late-loaded rope %d", hRope.RopeIndex);
						DebugOutput("CPFRopeController::Init --- Length: %.3f", hRope.RopeLength);
					}
					else
						DebugOutput("CPFRopeController::Init --- move_rope %d is not qualified to be a rope", i);
				}
				else if (CPFRopeController.IsRailStart(i))
				{
					CPFRail hRail = new CPFRail(i);
					if (hRail != null)
					{
						CPFRopeController.AddRail(hRail);
						DebugOutput("CPFRopeController::Init --- Late-loaded rail %d", hRail.RopeIndex);
						DebugOutput("CPFRopeController::Init --- Length: %.3f", hRail.RailLength);
					}
					else
						DebugOutput("CPFRopeController::Init --- move_rope %d is not qualified to be a rail", i);
				}
			}
			
			ProcessRadials();
		}
	}
	
	public static bool HasRails()
	{
		return g_bHasRails;
	}
	
	public static void SpawnRopeBeams(int iClient)
	{
		for (int i = 0; i < CPFRopeController.Total(); i++)
		{
			CPFRope hRope = CPFRopeController.GetRope(i);
			
			float vecStart[3], vecEnd[3];
			GetEntPropVector(EntRefToEntIndex(hRope.EntIndex), Prop_Data, "m_vecAbsOrigin", vecStart);
			GetEntPropVector(EntRefToEntIndex(hRope.KeyframeIndex), Prop_Data, "m_vecAbsOrigin", vecEnd);
			
			vecStart[2] -= 10.0;
			vecEnd[2] -= 10.0;
			
			DrawVectorPoints(vecStart, vecEnd, 0.0, {25, 25, 25, 255}, 6.0, false);
		}
	}
};

#if defined DEBUGROPES
public void OnDebugRopes(ConVar cvarRopes, const char[] strOldValue, const char[] strNewValue)
{
	if (StringToInt(strNewValue) && !StringToInt(strOldValue))
	{
		for (int i = 0; i < CPFRopeController.Total(); i++)
		{
			CPFRope hRope = CPFRopeController.GetRope(i);
			if (hRope == null) continue;
			
			AcceptEntityInput(EntRefToEntIndex(hRope.DebugSpriteStart), "HideSprite");
			AcceptEntityInput(EntRefToEntIndex(hRope.DebugSpriteEnd), "HideSprite");
		}
	}
	else
	{
		for (int i = 0; i < CPFRopeController.Total(); i++)
		{
			CPFRope hRope = CPFRopeController.GetRope(i);
			if (hRope == null) continue;
			
			AcceptEntityInput(EntRefToEntIndex(hRope.DebugSpriteStart), "ShowSprite");
			AcceptEntityInput(EntRefToEntIndex(hRope.DebugSpriteEnd), "ShowSprite");
		}
	}
}
#endif

void ProcessRadials()
{
	int i = -1;
	while ((i = FindEntityByClassname(i, "keyframe_rope")) != -1)
	{
		char strTargetname[128];
		GetEntPropString(i, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrContains(strTargetname, "rotator", false) != -1)
		{
			DebugOutput("ProcessRadials --- %d Targetname: %s", i, strTargetname);
			CPFRail hSegmentToMark = view_as<CPFRail>(CPFRopeController.FindRope(GetEntPropEnt(i, Prop_Data, "m_hEndPoint"), true));
			hSegmentToMark.RopeType = RAIL_RADIAL;
			hSegmentToMark.RotatorIndex = EntIndexToEntRef(i);
			
			DebugOutput("ProcessRadials --- Rope %d is radial", hSegmentToMark.RopeIndex);
			float vecCenter[3], vecStart[3], vecEnd[3], flRadius;
			int iMoveRope = GetEntPropEnt(i, Prop_Data, "m_hEndPoint");
			
			GetEntPropVector(i, Prop_Data, "m_vecAbsOrigin", vecCenter);
			GetEntPropVector(iMoveRope, Prop_Data, "m_vecAbsOrigin", vecStart);
			GetEntPropVector(GetEntPropEnt(iMoveRope, Prop_Data, "m_hEndPoint"), Prop_Data, "m_vecAbsOrigin", vecEnd);

			flRadius = GetVectorDistance(vecCenter, vecStart);
			DebugOutput("ProcessRadials --- Radius: %.2f", flRadius);
			
			#if defined DEBUGVECS
			DebugBeamRing(vecCenter, flRadius * 2.0, -1.0);
			#endif
			
			hSegmentToMark.Direction = FindRotDirection(vecCenter, vecStart, vecEnd);
			if (hSegmentToMark.Direction == Rot_Invalid)
				DebugOutput("ProcessRadials --- Invalid Radial Segment!");
		}
	}
}

public CPFRope CPFRopeController_FindRope(int iRope)
{
	return CPFRopeController.FindRope(iRope);
}