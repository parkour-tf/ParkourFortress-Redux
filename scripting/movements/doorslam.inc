#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_DOORSLAM_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_DOORSLAM_INCLUDED

static bool g_bDoorBoostEligible[MAXPLAYERS + 1];
static int g_iActionCounter[MAXPLAYERS + 1];

const int DOORBOOST_TICKS = 20;
const float DOORBOOST_DISTANCE = 6000.0; // Squared value of actual door distance
stock const char DOOR_OPENER_TARGETNAME[6] = "closer";

methodmap CPFDoorHandler
{
	public static void Mount(const int iClient)
	{
		CPFViewController.Queue(iClient, AnimState_Doorsmash, 1.0, true);
		CPFSoundController.PlayBigDing(iClient);
		CPFSoundController.AddIntensity(iClient, 1.0);
		
		CPFStateController.Set(iClient, State_DoorBoost);
		CPFSpeedController.AddSpeed(iClient, 50.0);
		g_iActionCounter[iClient] = 0;
	}
	
	public static void Dismount(const int iClient)
	{
		CPFStateController.Set(iClient, State_None);
		CPFSpeedController.RestoreSpeed(iClient);
		g_iActionCounter[iClient] = 0;
	}
	
	public static void SetDoorBoostStatus(const int iClient, const bool bBoost)
	{
		if (!IsValidClient(iClient))
			g_bDoorBoostEligible[iClient] = bBoost;
	}
	
	public static bool GetDoorBoostStatus(const int iClient)
	{
		if (!IsValidClient(iClient))
			return false;
		
		return g_bDoorBoostEligible[iClient];
	}
	
	public static void DoorBoost(const int iClient)
	{
		if (IsOnGround(iClient) && g_iActionCounter[iClient] > TickModify(4) || g_iActionCounter[iClient] >= TickModify(DOORBOOST_TICKS))
		{
			CPFDoorHandler.Dismount(iClient);
		}
		
		g_iActionCounter[iClient]++;
	}
	
	public static void StartDoorBoost(const int iClient, float vecVelocity[3])
	{
		const float DOORBOOST_VELOCITY = 650.0;
		const float DOORBOOST_Z_OFFSET = 2.0;
		
		float vecOrigin[3], flCurrentSpeed;
		GetClientAbsOrigin(iClient, vecOrigin);
		vecOrigin[2] += DOORBOOST_Z_OFFSET;
		flCurrentSpeed = CPFSpeedController.GetSpeed(iClient);
		
		CPFDoorHandler.Mount(iClient);
		
		SetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed", DOORBOOST_VELOCITY);
		SetEntPropFloat(iClient, Prop_Data, "m_flSpeed", DOORBOOST_VELOCITY);
		
		NormalizeVector(vecVelocity, vecVelocity);
		ScaleVector(vecVelocity, DOORBOOST_VELOCITY);
		vecVelocity[2] += 50.0;
		
		TeleportEntity(iClient, (!CheckPointAgainstPlayerHull(iClient, vecOrigin)) ? vecOrigin : NULL_VECTOR, NULL_VECTOR, vecVelocity);
		
		CPFSpeedController.SetStoredSpeed(iClient, flCurrentSpeed);
	}
	
	public static void Slam(int iClient, CPFDoor hDoor)
	{
		const float DOORBOOST_MINIMUM_ALIGNMENT = 0.8;
	
		float vecEyeAngles[3], vecDoorAngles[3], vecEyeForward[3], vecVelocity[3], flAlignment;
		int iDoor = EntRefToEntIndex(hDoor.EntIndex);
		
		// Client eye angles and forward
		GetClientEyeAngles(iClient, vecEyeAngles);
		GetAngleVectors(vecEyeAngles, vecEyeForward, NULL_VECTOR, NULL_VECTOR);
		
		// Door angles and velocity set-up
		GetEntPropVector(iDoor, Prop_Send, "m_angRotation", vecDoorAngles);
		GetAngleVectors(vecDoorAngles, vecVelocity, NULL_VECTOR, NULL_VECTOR);
		
		flAlignment = FloatAbs(GetVectorDotProduct(vecEyeForward, vecVelocity) / (GetVectorLength(vecEyeForward) * GetVectorLength(vecVelocity)));
		if (flAlignment < DOORBOOST_MINIMUM_ALIGNMENT)
			return;
		
		// Store the player targetname and set it to something we can use in an input
		char strTargetname[255];
		GetEntPropString(iClient, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		SetEntPropString(iClient, Prop_Data, "m_iName", DOOR_OPENER_TARGETNAME);
		
		// Open the door
		SetVariantString(DOOR_OPENER_TARGETNAME);
		AcceptEntityInput(iDoor, "OpenAwayFrom");
		
		// Restore the player targetname
		SetEntPropString(iClient, Prop_Data, "m_iName", strTargetname);

		CPFDoorHandler.StartDoorBoost(iClient, vecEyeForward);
	}
	
	public static void Think(int iClient)
	{
		// Check if the player is rolling
		if (CPFStateController.Get(iClient) == State_Roll)
			return;
		
		// If not, get their aim target
		int iAimTarget = GetClientAimTarget(iClient, false);
		if (!IsValidEntity(iAimTarget))
			return;
		
		// Check if that target is a valid, closed door
		CPFDoor hDoor = CPFDoorController.GetDoorFromProp(iAimTarget);
		if (hDoor == null || !IsValidEntity(EntRefToEntIndex(hDoor.EntIndex)) || hDoor.State != DOOR_CLOSED_IDLE)
			return;
		
		// Measure distance from the door
		float vecOrigin[3], vecDoorOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		GetEntPropVector(EntRefToEntIndex(hDoor.EntIndex), Prop_Send, "m_vecOrigin", vecDoorOrigin);
		
		if (GetVectorDistance(vecOrigin, vecDoorOrigin, true) < DOORBOOST_DISTANCE)
			CPFDoorHandler.Slam(iClient, hDoor);
	}
};