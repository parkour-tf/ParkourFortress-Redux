#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFSPEED_INCLUDED
#endinput
#endif
#define _PFSPEED_INCLUDED

enum struct SpeedData
{
	bool Boosted;
	bool FallDeathImmune;
	float LastSpeed;
	float AirVelocity;
}

static SpeedData g_SpeedData[MAXPLAYERS + 1];

const float SPEED_GAIN_FWD = 0.35;
const float SPEED_DEPREC = 1.0;
const float SPEED_BASE = 250.0;
const float SPEED_MAX = 400.0;
const float SPEED_MAX_BOOST = 420.0;

methodmap CPFSpeedController
{
	public static void Debug(const char[] strDebug, any ...)
	{
		if (g_cvarDebugSpeed == null || !g_cvarDebugSpeed.IntValue) return;
		
		char strDebugFmt[255];
		VFormat(strDebugFmt, sizeof(strDebugFmt), strDebug, 2);
		
		PrintToChatAll(strDebugFmt);
	}
	
	public static void SetFallDeathImmunity(int iClient, bool bValue)
	{
		g_SpeedData[iClient].FallDeathImmune = bValue;
		DebugOutput("CPFSpeedController::SetFallDeathImmunity --- %N %d", iClient, bValue);
	}
	
	public static bool GetFallDeathImmunity(int iClient)
	{
		return g_SpeedData[iClient].FallDeathImmune;
	}
	
	public static float AbsoluteSpeed(int iClient)
	{
		float vecAbsVelocity[3], vecOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", vecAbsVelocity);
		return GetVectorDistance(vecOrigin, vecAbsVelocity);
	}
	
	public static void ValidateSpeed(int iClient, float &flSpeed, float flMaxClampAt = SPEED_MAX, float flMaxClampTo = SPEED_MAX_BOOST, float flMinClampAt = SPEED_BASE, float flMinClampTo = SPEED_BASE)
	{
		if (PlayerHasRifleActive(iClient))
		{
			flMaxClampTo = SPEED_MAX;
			if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
				flMinClampTo = 80.0;
		}
		if (flSpeed > flMaxClampAt)
			flSpeed = flMaxClampTo;
		else if (flSpeed < flMinClampAt)
			flSpeed = flMinClampTo;
	}
	
	public static float GetSpeed(int iClient, bool bValidate = false)
	{
		if (!IsValidClient(iClient)) return 0.0;
		
		if (bValidate)
		{
			float flSpeed = GetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed");
			CPFSpeedController.ValidateSpeed(iClient, flSpeed);
			return flSpeed;
		}
		else
			return GetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed");
	}
	
	public static void SetSpeed(int iClient, float flSpeed)
	{
		if (!IsValidClient(iClient)) return;
		
		if (PlayerHasRifleActive(iClient))
			flSpeed = fMin(flSpeed, (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed) ? 80.0 : SPEED_MAX));
		
		SetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed", flSpeed);
		SetEntPropFloat(iClient, Prop_Data, "m_flSpeed", flSpeed);
		
		if (flSpeed > 1.0) // Don't store speed pauses
			g_SpeedData[iClient].LastSpeed = flSpeed;
	}
	
	public static void AddSpeed(int iClient, float flSpeed)
	{
		if (!IsValidClient(iClient)) return;
		
		CPFSpeedController.Debug("CPFSpeedController::AddSpeed --- %N Speed: %.3f", iClient, CPFSpeedController.GetSpeed(iClient) + flSpeed);
		CPFSpeedController.SetSpeed(iClient, CPFSpeedController.GetSpeed(iClient) + flSpeed);
	}
	
	public static void RemoveSpeed(int iClient, float flSpeed)
	{
		if (!IsValidClient(iClient)) return;
		
		CPFSpeedController.Debug("CPFSpeedController::RemoveSpeed --- %N Speed: %.3f", iClient, CPFSpeedController.GetSpeed(iClient) - flSpeed);
		CPFSpeedController.SetSpeed(iClient, CPFSpeedController.GetSpeed(iClient) - flSpeed);
	}
	
	public static void RestoreSpeed(int iClient)
	{
		if (!IsValidClient(iClient)) return;
		
		CPFSpeedController.ValidateSpeed(iClient, g_SpeedData[iClient].LastSpeed);
		CPFSpeedController.SetSpeed(iClient, g_SpeedData[iClient].LastSpeed);
	}
	
	public static void CarrySpeedToNextTick(int iClient, float flSpeed)
	{
		if (!IsValidClient(iClient)) return;
		
		DataPack hData = new DataPack();
		hData.WriteCell(flSpeed);
		hData.WriteCell(iClient);
		RequestFrame(RestoreLastTickSpeed, hData);
	}
	
	public static float GetStoredSpeed(int iClient)
	{
		if (!IsValidClient(iClient)) return 0.0;
		
		return g_SpeedData[iClient].LastSpeed;
	}
	
	public static void SetStoredSpeed(int iClient, float flSpeed)
	{
		if (!IsValidClient(iClient)) return;
		//DebugOutput("Setting stored speed to %f via SetStoredSpeed", flSpeed);
		g_SpeedData[iClient].LastSpeed = flSpeed;
	}
	
	public static void StoreSpeed(int iClient)
	{
		if (!IsValidClient(iClient)) return;
		//DebugOutput("Setting stored speed to %f vis StoreSpeed", CPFSpeedController.GetSpeed(iClient));
		g_SpeedData[iClient].LastSpeed = CPFSpeedController.GetSpeed(iClient);
	}
	
	public static bool GetBoost(int iClient)
	{
		if (!IsValidClient(iClient)) return false;
		
		return g_SpeedData[iClient].Boosted;
	}
	
	public static void SetBoost(int iClient, bool bBoosted)
	{
		if (!IsValidClient(iClient)) return;
		
		if (bBoosted == g_SpeedData[iClient].Boosted) return;
		
		char sClassname[64];
		TF2_GetActiveWeaponClassname(iClient, sClassname, sizeof(sClassname));
		g_SpeedData[iClient].Boosted = (bBoosted && !(StrContains(sClassname, "tf_weapon_sniperrifle") == 0));
		
		if (g_SpeedData[iClient].Boosted)
		{
			CPFTutorialController.Stagepoint(iClient, TUTORIAL_SPRINT);
			
			TF2_AddCondition(iClient, TFCond_SpeedBuffAlly, TFCondDuration_Infinite, 0);
		}
		else
			TF2_RemoveCondition(iClient, TFCond_SpeedBuffAlly);
	}
	
	public static void StoreAirVel(int iClient)
	{
		float vecVelocity[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
		vecVelocity[2] = 0.0;
		g_SpeedData[iClient].AirVelocity = GetVectorLength(vecVelocity);
		
	}
	
	public static float GetAirVel(int iClient)
	{
		return g_SpeedData[iClient].AirVelocity;
	}
	
	public static void SetAirVel(int iClient, float flVelocity)
	{
		g_SpeedData[iClient].AirVelocity = flVelocity;
	}
	
	public static void Think(int iClient, int iButtons)
	{
		if (!IsValidClient(iClient)) return;
		
		float flNewIntensity = ((CPFSpeedController.GetSpeed(iClient) - 250.0)/170.0) * 0.02;
		CPFSoundController.AddIntensity(iClient, flNewIntensity);
		
		if (CPFStateController.Get(iClient) == State_None || CPFStateController.Get(iClient) == State_Locked)
		{
			if (((CPFSpeedController.GetSpeed(iClient) - CPFSpeedController.GetStoredSpeed(iClient)) > 45.0))
			{
				CPFSpeedController.RestoreSpeed(iClient);
			}
			else if (CPFSpeedController.GetSpeed(iClient) != CPFSpeedController.GetStoredSpeed(iClient))
				CPFSpeedController.SetStoredSpeed(iClient, CPFSpeedController.GetSpeed(iClient));
		}
		
		if (CPFStateController.Get(iClient) != State_None) return;
		
		float flClientAbsVel[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", flClientAbsVel);
		
		if (!IsOnGround(iClient))
		{
			CPFSpeedController.StoreAirVel(iClient);
			if (flClientAbsVel[2] < -1200 && !g_SpeedData[iClient].FallDeathImmune && !CPFStateController.HasFlags(iClient, SF_INFINITEJUMP))
			{
				CPFStateController.Set(iClient, State_Falling);
				CPFSoundController.PlayFallVO(iClient);
				CPFViewController.Queue(iClient, AnimState_OnlyDeathBelow, 1.0, true);
			}
			else if (flClientAbsVel[2] < -650 && !CPFStateController.HasFlags(iClient, SF_INFINITEJUMP))
			{
				SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
			}
			return;
		}
		
		float flClientSpeed = CPFSpeedController.GetSpeed(iClient);
		CPFSpeedController.SetAirVel(iClient, 0.0);
		//CPFSpeedController.Debug("CPFSpeedController::Think --- %N (%d) Speed: %.3f", iClient, iClient, flClientSpeed);
		
		
		
		// Button checks first, speed checks second
		if (iButtons & IN_FORWARD && !((iButtons & IN_DUCK) || IsFullyDucked(iClient)))
		{
			float vecEyePosition[3], vecEyeAngles[3], vecEndPos[3];
			GetClientEyePosition(iClient, vecEyePosition);
			GetClientEyeAngles(iClient, vecEyeAngles);
			
			vecEyeAngles[0] = 0.0;
			vecEyeAngles[2] = 0.0;
			ForwardVector(vecEyeAngles, 24.35, vecEndPos);
			AddVectors(vecEyePosition, vecEndPos, vecEndPos);
			
			TraceHullF.Start(vecEyePosition, vecEndPos, view_as<float>({-12.0, -12.0, -8.0 }), view_as<float>({ 12.0, 12.0, 0.0 }), MASK_PLAYERSOLID, TraceRayNoPlayers);
			if (flClientSpeed + (TICKRATE_STANDARD_FLOAT/GetTickRate() * SPEED_GAIN_FWD) > SPEED_MAX)
			{
				
				CPFSpeedController.SetSpeed(iClient, view_as<float>(SPEED_MAX_BOOST));
				CPFSpeedController.SetBoost(iClient, true);
			}
			else if (flClientSpeed < SPEED_MAX && !TRACE_GLOBAL.Hit)
			{
				if (!TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
					CPFSpeedController.AddSpeed(iClient, view_as<float>(TICKRATE_STANDARD_FLOAT/GetTickRate() * SPEED_GAIN_FWD));
				
				CPFSpeedController.SetBoost(iClient, false);
			}
		}
		
		if (iButtons & IN_DUCK)
		{
			CPFSpeedController.SetSpeed(iClient, view_as<float>(SPEED_BASE));
			CPFSpeedController.SetStoredSpeed(iClient, view_as<float>(SPEED_BASE));
			CPFSpeedController.SetBoost(iClient, false);
		}
		
		if (!(iButtons & IN_FORWARD))
		{
			if (flClientSpeed - (TICKRATE_STANDARD_FLOAT/GetTickRate() * SPEED_DEPREC) <= SPEED_BASE)
			{
				//CPFSpeedController.Debug("CPFSpeedController::Think --- Setting speed to base for %N (%d)", iClient, iClient);
				CPFSpeedController.SetSpeed(iClient, view_as<float>(SPEED_BASE));
			}
			/*else if (flClientSpeed + SPEED_DEPREC > SPEED_MAX && flClientSpeed + SPEED_DEPREC < SPEED_MAX_BOOST)
			{
				CPFSpeedController.SetSpeed(iClient, view_as<float>(SPEED_MAX + SPEED_DEPREC));
				CPFSpeedController.SetBoost(iClient, false);
			}*/
			else
			{
				CPFSpeedController.Debug("CPFSpeedController::Think --- Decrementing speed for %N (%d)", iClient, iClient);
				CPFSpeedController.RemoveSpeed(iClient, view_as<float>(TICKRATE_STANDARD_FLOAT/GetTickRate() * SPEED_DEPREC));
			}
			
			if (flClientSpeed - SPEED_DEPREC < SPEED_MAX && IsOnGround(iClient))
			{
				//CPFSpeedController.Debug("CPFSpeedController::Think --- Unboosting client %N (%d)", iClient, iClient);
				CPFSpeedController.SetBoost(iClient, false);
			}
			else
			{
				CPFSpeedController.SetBoost(iClient, true);
			}
			
			CPFSpeedController.SetAirVel(iClient, 0.0);
		}
	}
	
	public static void Init()
	{
		for (int i = 1; i < MaxClients; i++)
		{
			CPFSpeedController.SetBoost(i, false);
			CPFSpeedController.SetSpeed(i, SPEED_BASE);
			CPFSpeedController.SetAirVel(i, 0.0);
		}
	}
};

public void RestoreLastTickSpeed(DataPack hData)
{
	float flSpeed;
	int iClient;
	
	hData.Reset();
	flSpeed = hData.ReadCell();
	iClient = hData.ReadCell();
	hData.Close();
	
	CPFSpeedController.SetStoredSpeed(iClient, flSpeed);
	CPFSpeedController.RestoreSpeed(iClient);
}

public void TF2_OnConditionRemoved(int iClient, TFCond eCondition)
{
	if (eCondition == TFCond_Taunting && (!IsOnGround(iClient) || CPFStateController.Get(iClient) == State_Rail))
	{
		SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
		CPFStateController.Set(iClient, State_Falling);
		CPFSoundController.PlayFallVO(iClient);
		CPFViewController.Queue(iClient, AnimState_OnlyDeathBelow, 1.0, true);
	}
}
