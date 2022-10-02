#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_ROLL_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_ROLL_INCLUDED

#if !defined _MOVEMENTS_LONGJUMP_INCLUDED
	#include "movements/longjump.inc"
	#warning Include longjumps before rolls!
#endif

enum struct RollData
{
	int ActionStart;
	
	int Camera;
	bool Queued;
	bool JustRolled;
}

static RollData g_RollData[MAXPLAYERS + 1];

methodmap CPFRollHandler
{
	public static void Queue(int iClient, bool bQueue = true)
	{
		g_RollData[iClient].Queued = bQueue;
		if (bQueue)
			DebugOutput("CPFRollHandler::Queue --- Queued roll for %N", iClient);
	}
	
	public static void ForceRemoveCamera(int iClient)
	{
		int iCamera = EntRefToEntIndex(g_RollData[iClient].Camera);
		if ((iCamera < 1) || !IsValidEntity(iCamera))
			return;

		AcceptEntityInput(iCamera, "Disable");
		RemoveEntity(iCamera);
		//PrintToChatAll("Killed %d", iCamera);
		g_RollData[iClient].Camera = 0;
	}

	public static void Disengage(int iClient)
	{
		if (!IsValidClient(iClient))
			return;
		
		const float SPEED_BOOST_ROLL = 95.0;
		
		CPFRollHandler.ForceRemoveCamera(iClient);
		
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		
		if (CPFStateController.Get(iClient) == State_Roll || CPFStateController.Get(iClient) == State_Locked)
		{
			CPFStateController.Set(iClient, State_None);
			CPFSpeedController.SetSpeed(iClient, CPFSpeedController.GetStoredSpeed(iClient) + SPEED_BOOST_ROLL);
			CPFSoundController.AddIntensity(iClient, 1.0);
			CPFSoundController.SwitchMusic(iClient);
		}
	}
	
	public static void Roll(int iClient)
	{
		RollData sPlayerRoll;
		sPlayerRoll = g_RollData[iClient];
		if ((GetGameTickCount() - sPlayerRoll.ActionStart) <= TickModify(45))
		{
			if (g_RollData[iClient].JustRolled)
			{
				SetEntProp(iClient, Prop_Data, "m_takedamage", view_as<int>(DAMAGE_YES));
				g_RollData[iClient].JustRolled = false;
			}
			
			if (!IsValidEntity(EntRefToEntIndex(sPlayerRoll.Camera)))
				return;
				
			if ((GetGameTickCount() - sPlayerRoll.ActionStart) > TickModify(43))
				AcceptEntityInput(EntRefToEntIndex(g_RollData[iClient].Camera), "Disable");
			
			float vecAngles[3];
			GetEntPropVector(EntRefToEntIndex(sPlayerRoll.Camera), Prop_Send, "m_angRotation", vecAngles);
			vecAngles[0] = ((TICKRATE_STANDARD_FLOAT/GetTickRate()) * float(GetGameTickCount() - sPlayerRoll.ActionStart)) * 8;
			TeleportEntity(EntRefToEntIndex(sPlayerRoll.Camera), NULL_VECTOR, vecAngles, NULL_VECTOR);
		}
		else
		{
			CPFRollHandler.Disengage(iClient);
		}
	}
	
	public static bool Try(int iClient)
	{
		float vecVelocity[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
		
		if (vecVelocity[2] < -1200)
		{
			if (!CPFSpeedController.GetFallDeathImmunity(iClient))
				SDKHooks_TakeDamage(iClient, iClient, iClient, 5000.0, DMG_FALL);
			
			CPFStateController.Set(iClient, State_None);
			return false;
		}
		
		if (vecVelocity[2] < -1050 && !CPFSpeedController.GetFallDeathImmunity(iClient) && CPFStateController.Get(iClient) != State_Roll && CPFStateController.Get(iClient) != State_Locked)
		{
			const float STUN_DURATION = 3.0;
			
			TF2_StunPlayer(iClient, STUN_DURATION, 1.0, TF_STUNFLAG_BONKSTUCK|TF_STUNFLAG_NOSOUNDOREFFECT);
			CPFStateController.Set(iClient, State_Locked);
			g_hUnlockPlayerTimer[iClient] = CreateTimer(STUN_DURATION + 1.0, UnlockPlayer, GetClientUserId(iClient));
			SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_ATCONTROLS);
			return true;
		}
		else if (g_RollData[iClient].Queued && IsOnGround(iClient) && CPFStateController.Get(iClient) != State_Locked && !TF2_IsPlayerInCondition(iClient, TFCond_Dazed))
		{
			if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
			{
				TF2_RemoveCondition(iClient, TFCond_Zoomed);
				TF2_RemoveCondition(iClient, TFCond_Slowed);
				FakeClientCommand(iClient, "-attack");
				CPFSpeedController.SetStoredSpeed(iClient, 250.0);
			}
			else
				CPFSpeedController.StoreSpeed(iClient);
			CPFSpeedController.SetSpeed(iClient, 1.0);
			
			CPFStateController.Set(iClient, State_Roll);
			CPFViewController.Queue(iClient, AnimState_Roll, 1.0, true);
			CPFViewController.SetDontInterrupt(iClient, true);
			CPFSoundController.PlaySmallDing(iClient);
			CPFStateController.RemoveFlags(iClient, SF_CAMEFROMSLIDE);
			CPFRollHandler.Queue(iClient, false);
			g_RollData[iClient].ActionStart = GetGameTickCount();
			
			SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_ATCONTROLS);
			
			float vecEyePosition[3], vecEyeAngles[3];
			GetClientEyePosition(iClient, vecEyePosition);
			GetClientEyeAngles(iClient, vecEyeAngles);
			
			g_RollData[iClient].Camera = EntIndexToEntRef(CreateEntityByName("point_viewcontrol"));
			vecEyePosition[2] -= 16.0;
			TeleportEntity(EntRefToEntIndex(g_RollData[iClient].Camera), vecEyePosition, vecEyeAngles, NULL_VECTOR);
			AcceptEntityInput(EntRefToEntIndex(g_RollData[iClient].Camera), "Enable", iClient);
			g_RollData[iClient].JustRolled = true;
			
			int iUserID = GetClientUserId(iClient);
			
			CreateTimer(0.7, RestorePlayer, iUserID);
			return false;
		}
		else
		{
			CPFSpeedController.SetBoost(iClient, false);
			CPFSpeedController.SetStoredSpeed(iClient, 250.0);
			CPFSpeedController.RestoreSpeed(iClient);
			
			return true;
		}
	}
	
	public static void Disconnect(int iClient)
	{
		CPFRollHandler.ForceRemoveCamera(iClient);
	}
};

public Action UnlockPlayer(Handle hTimer, int iUserID)
{
	int iClient = GetClientOfUserId(iUserID);
	
	CPFStateController.Set(iClient, State_None);
	SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
	SetEntityMoveType(iClient, MOVETYPE_DEFAULT);
	
	CPFSpeedController.SetSpeed(iClient, 250.0);
	CPFSpeedController.SetStoredSpeed(iClient, 250.0);
	CPFSpeedController.SetBoost(iClient, false);
	
	return Plugin_Handled;
}

public Action RestorePlayer(Handle hTimer, int iUserID)
{
	int iClient = GetClientOfUserId(iUserID);
	
	CPFRollHandler.Disengage(iClient);
}

public void ForceRemoveCamera(int iClient)
{
	CPFRollHandler.ForceRemoveCamera(iClient);
}
