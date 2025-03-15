#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFSTATE_INCLUDED
	#endinput
#endif
#define _PFSTATE_INCLUDED

enum PFState
{
	State_Invalid = -1,
	State_None = 0,
	State_Climb,
	State_Zipline,
	State_Rail,
	State_Wallrun,
	State_Wallclimb,
	State_Slide,
	State_Roll,
	State_Hang,
	State_DoorBoost,
	State_Vault,
	State_Locked,
	State_Noclip,
	State_Falling,
	
	STATE_COUNT
};

enum ePFStateFlags
{
    SF_NONE				= 1<<0,
    SF_LONGJUMP			= 1<<1,
    SF_BEINGHEALED		= 1<<2,
    SF_CAMEFROMSLIDE 	= 1<<3,
    SF_STRIPHOOKSHOT	= 1<<4,
    SF_SPAWNING			= 1<<5,
    SF_INFINITEJUMP		= 1<<6,
    SF_INFINITEBOOST	= 1<<7,
};

enum struct StateInfo
{
	PFState State;
	PFState LastState;
	ePFStateFlags Flags;
	int ButtonsInterrupted;
}

static StateInfo g_eStateInfo[MAXPLAYERS + 1];

static bool g_bOnCooldown[STATE_COUNT][MAXPLAYERS + 1];

methodmap CPFStateController
{
	public static void Debug(const char[] strDebug, any ...)
	{
		if (g_cvarDebugState == null || !g_cvarDebugState.IntValue) return;
		
		char strDebugFmt[255];
		VFormat(strDebugFmt, sizeof(strDebugFmt), strDebug, 2);
		
		PrintToChatAll(strDebugFmt);
	}
	
	public static ePFStateFlags GetFlags(int iClient) { return g_eStateInfo[iClient].Flags; }
	public static bool HasFlags(int iClient, ePFStateFlags eFlags)  { return view_as<bool>(g_eStateInfo[iClient].Flags & eFlags); }
	public static void SetFlags(int iClient, ePFStateFlags eFlags) { g_eStateInfo[iClient].Flags = eFlags; }
	public static void AddFlags(int iClient, ePFStateFlags eFlags) { g_eStateInfo[iClient].Flags |= eFlags; }
	public static void RemoveFlags(int iClient, ePFStateFlags eFlags)  { g_eStateInfo[iClient].Flags &= ~eFlags; }
	
	public static PFState Get(int iClient)
	{
		if (!IsValidClient(iClient))
			return State_Invalid;
		
		return g_eStateInfo[iClient].State;
	}
	
	
	public static void Set(int iClient, PFState eState)
	{
		if (eState >= STATE_COUNT)
			SetFailState("CPFStateController::Set --- Invalid State! Client: %N (%d), State: %d", iClient, iClient, eState);
		else if (!IsValidClient(iClient))
			return;
		else
		{
			if (eState != State_Roll && g_eStateInfo[iClient].State == State_Roll)
			{
				ForceRemoveCamera(iClient);
			}
			
			CPFStateController.Debug("CPFStateController::Set --- Setting state %d for client %N (%d)", eState, iClient, iClient);
			g_eStateInfo[iClient].LastState = g_eStateInfo[iClient].State;
			g_eStateInfo[iClient].State = eState;
		}
	}
	
	public static PFState GetLast(int iClient)
	{
		if (!IsValidClient(iClient))
			return State_Invalid;
		else
			return g_eStateInfo[iClient].LastState;
	}
	
	public static int GetWaterLevel(int iClient)
	{
		return GetEntProp(iClient, Prop_Send, "m_nWaterLevel"); 
	}
	
	public static void ResetClient(int iClient)
	{
		if (!IsValidClient(iClient))
			return;
		
		SetCollisionGroup(iClient, g_ePFCollisionGroup);
		SetEntityMoveType(iClient, GetEntityMoveType(iClient) == MOVETYPE_NOCLIP ? MOVETYPE_NOCLIP : MOVETYPE_WALK);
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_FROZEN);
		SetEntityGravity(iClient, 1.0);
		SendConVarValue(iClient, FindConVar("sv_footsteps"), "1");
		SetEntProp(iClient, Prop_Data, "m_takedamage", view_as<int>(DAMAGE_YES));
		
		
		float vecAngles[3];
		GetClientEyeAngles(iClient, vecAngles);
		vecAngles[2] = 0.0;
		TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
		
		switch (CPFStateController.Get(iClient))
		{
			case State_Slide:
			{
				float vecViewOffset[3];
				GetEntPropVector(iClient, Prop_Data, "m_vecViewOffset", vecViewOffset);
				vecViewOffset[2] += 15.0;
				SetEntPropVector(iClient, Prop_Data, "m_vecViewOffset", vecViewOffset);
			}
		}
		
		CPFStateController.Set(iClient, State_None);
	}
	
	/**
	 * (Assumes clean client input)
	 *
	 * This function stores all the buttons that the user can potentially press. If a player is holding down 
	 * a key after an action ends, that key should not be counted.
	 */
	public static void UpdateButtons(int iClient, int iButtons)
	{
		g_eStateInfo[iClient].ButtonsInterrupted = ~iButtons;
		//DebugOutput("CPFStateController::UpdateButtons --- %b", g_iButtonsInterrupted[iClient]);
	}
	
	public static void RemoveCooldown(int iClient, PFState eState, bool bCooldown)
	{
		g_bOnCooldown[eState][iClient] = bCooldown;
	}
	
	public static int GetButtons(int iClient)
	{
		return g_eStateInfo[iClient].ButtonsInterrupted;
	}

	public static int SetCooldown(int iClient, PFState eState, float flLength)
	{
		g_bOnCooldown[eState][iClient] = true;
		DataPack hData = new DataPack();
		hData.WriteCell(eState);
		hData.WriteCell(GetClientUserId(iClient));
		CreateTimer(flLength, ResetCooldown, hData, TIMER_DATA_HNDL_CLOSE);
	}
	
	public static bool IsOnCooldown(int iClient, PFState eState)
	{
		return g_bOnCooldown[eState][iClient];
	}
};

Action ResetCooldown(Handle hTimer, DataPack hData)
{
	hData.Reset();
	g_bOnCooldown[hData.ReadCell()][GetClientOfUserId(hData.ReadCell())] = false;
	
	return Plugin_Continue;
}

public void RemoveWallclimbCooldown(any iClient)
{
	CPFStateController.RemoveCooldown(iClient, State_Wallclimb, false);
}
