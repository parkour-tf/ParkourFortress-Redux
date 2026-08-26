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

	/**
	 * The live heal repeat timer, or INVALID_HANDLE when the heal cycle is not running.
	 *
	 * Kept beside Flags because it pairs with SF_BEINGHEALED: the flag says a heal cycle is
	 * notionally active, this handle is the thing actually driving it. Before AB#18984 only the
	 * flag existed, the handle was discarded at creation, and the cycle could only be ended by
	 * clearing the flag and waiting for the timer to notice - which let a stale timer survive a
	 * death long enough for a second one to be armed alongside it.
	 */
	Handle HealTimer;
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

	/**
	 * Heal repeat timer lifetime (AB#18984).
	 *
	 * There are deliberately TWO ways to end the cycle, and picking the wrong one is the original
	 * defect this API exists to prevent:
	 *
	 *   KillHealTimer  - an EXTERNAL end (death, disconnect). The timer is still live and nobody
	 *                    else will free it, so this deletes the handle.
	 *   ClearHealTimer - an INTERNAL end (the timer callback itself returning Plugin_Stop).
	 *                    SourceMod frees the handle when the callback returns Plugin_Stop, so this
	 *                    only drops our reference. Deleting here would be a double close.
	 *
	 * A timer callback must never delete its own handle. Call ClearHealTimer and return
	 * Plugin_Stop; never KillHealTimer from inside StartHealPlayerRepeat.
	 */
	public static Handle GetHealTimer(int iClient) { return g_eStateInfo[iClient].HealTimer; }
	public static bool HasHealTimer(int iClient) { return g_eStateInfo[iClient].HealTimer != INVALID_HANDLE; }
	public static void SetHealTimer(int iClient, Handle hTimer) { g_eStateInfo[iClient].HealTimer = hTimer; }

	/** Drops the reference without freeing it. For use from inside the heal callback only; see above. */
	public static void ClearHealTimer(int iClient)
	{
		g_eStateInfo[iClient].HealTimer = INVALID_HANDLE;
	}

	/** Ends a live heal cycle from outside the callback, freeing the timer. Safe to call when none is running. */
	public static void KillHealTimer(int iClient)
	{
		if (g_eStateInfo[iClient].HealTimer != INVALID_HANDLE)
		{
			delete g_eStateInfo[iClient].HealTimer;
			g_eStateInfo[iClient].HealTimer = INVALID_HANDLE;
		}
	}

	/**
	 * Resets per-client state for a connecting client. The heal handle is cleared rather than
	 * deleted: the slot may carry a stale value from a previous occupant whose timer is long gone,
	 * and deleting that would close a handle we do not own.
	 */
	public static void InitPlayer(int iClient)
	{
		g_eStateInfo[iClient].HealTimer = INVALID_HANDLE;
	}

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

	public static void SetCooldown(int iClient, PFState eState, float flLength)
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
