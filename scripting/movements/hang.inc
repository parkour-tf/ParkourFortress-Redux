#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_HANG_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_HANG_INCLUDED

static bool g_bHangEnabled[MAXPLAYERS + 1] = { true, true, ... };
int g_iHangElapsedTicks[MAXPLAYERS + 1];

enum eLedgeGrabDisengageSrc
{
	LEDGEGRAB_DISENGAGE_INVALID = 0,
	LEDGEGRAB_DISENGAGE_JUMP = 1,
	LEDGEGRAB_DISENGAGE_CROUCH,
	
	LGDS_COUNT
};

enum struct HangData
{
	float Yaw;
	float flEndZ;
}

static HangData g_HangData[MAXPLAYERS + 1];

methodmap CPFHangHandler
{
	
	public static void Mount(int iClient)
	{
		// 27.0 = 65.0 (Player Height) - 38.0 (Default Height Offset)
		CPFViewController.LockRotation(iClient, g_HangData[iClient].Yaw, 23.0);
		
		if (CPFStateController.HasFlags(iClient, SF_LONGJUMP))
			CPFLongjumpHandler.End(iClient);	
		
		float vecOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		vecOrigin[2] = g_HangData[iClient].flEndZ;

		CPFStateController.Set(iClient, State_Hang);
		CPFSpeedController.SetSpeed(iClient, 0.0);
		SetEntProp(iClient, Prop_Send, "m_iAirDash", 1); 
		SetEntityMoveType(iClient, MOVETYPE_NONE);
		TeleportEntity(iClient, vecOrigin, NULL_VECTOR, view_as<float>({0.0, 0.0, 0.0}));
		
		g_iHangElapsedTicks[iClient] = GetGameTickCount();
		
		DebugOutput("CPFHangHandler::Mount --- Stored Speed: %.3f", CPFSpeedController.GetStoredSpeed(iClient));
		
		CPFSoundController.PlayLedgegrab(iClient);
		CPFSoundController.PlaySmallDing(iClient);
		CPFSoundController.AddIntensity(iClient, 0.5);
		
		CPFViewController.Queue(iClient, AnimState_Ledgegrab, 1.0, true);
		CPFViewController.SetDefaultSequence(iClient, AnimState_LedgegrabIdle);
	}
	
	public static void Dismount(const int iClient)
	{
		SetEntityMoveType(iClient, MOVETYPE_DEFAULT);
		CPFStateController.Set(iClient, State_None);
		CPFSpeedController.RestoreSpeed(iClient);
		
		CPFViewController.UnlockRotation(iClient);
		
		CreateTimer(0.5, HangCooldown, iClient);
		g_bHangEnabled[iClient] = false;
		g_HangData[iClient].Yaw = 0.0;
		g_HangData[iClient].flEndZ = 0.0;
	}
	
	public static bool Think(int iClient, int iButtons)
	{
		PFState eState = CPFStateController.Get(iClient);
		if (eState == State_Roll || eState == State_Slide || IsOnGround(iClient) || !g_bHangEnabled[iClient] || CPFStateController.HasFlags(iClient, SF_STRIPHOOKSHOT))
			return false;
			
		float flClientAbsVel[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", flClientAbsVel);
		if (flClientAbsVel[2] < -800.0)
			return false;
			
		if (iButtons & IN_FORWARD && !(iButtons & IN_DUCK))
		{
			const float FORWARD_TRACE_OFFSET = 44.0;
			const float Z_OFFSET = 10.0;
		
			float vecTopTraceStart[3], vecTopTraceEnd[3];
			float vecBtmTraceStart[3], vecBtmTraceEnd[3];
			float vecForward[3], vecForward2[3], vecEyeAngles[3], vecEyePosition[3], vecOrigin[3], vecPlaneNormal[3], vecEndPos[3];
			float flEndZ;
			
			// Process Eye Position and Eye Angles
			GetClientAbsAngles(iClient, vecEyeAngles);
			GetClientEyePosition(iClient, vecEyePosition);
			GetClientAbsOrigin(iClient, vecOrigin);
			
			// Process Forward Vector
			vecEyeAngles[2] = 0.0;
			GetAngleVectors(vecEyeAngles, vecForward, NULL_VECTOR, NULL_VECTOR);
			NormalizeVector(vecForward, vecForward);
			vecForward[2] = 0.0;
			vecForward2 = vecForward;
			ScaleVector(vecForward, FORWARD_TRACE_OFFSET);
			ScaleVector(vecForward2, 4.0);
			
			// Process Bottom Trace Start and End
			vecBtmTraceStart = vecEyePosition;
			//vecBtmTraceStart[2] -= Z_OFFSET;
			AddVectors(vecBtmTraceStart, vecForward, vecBtmTraceEnd);
			
			// Run Bottom Trace
			// TODO: Grab the wall angle here so it can be passed into the mount function
			TraceRayF.Start(vecBtmTraceStart, vecBtmTraceEnd, MASK_PLAYERSOLID, RayType_EndPoint, TraceRayNoPlayers, iClient);
			DrawVectorPoints(vecBtmTraceStart, vecBtmTraceEnd, 5.0, {0, 255, 0, 255});
			bool bTraceHit = TRACE_GLOBAL.Hit;
			TRACE_GLOBAL.GetPlaneNormal(vecPlaneNormal);
			TRACE_GLOBAL.GetEndPosition(vecEndPos);
			if (!bTraceHit)
				return false;
				
			if (FloatAbs(vecPlaneNormal[2]) > 0.05)
				return false;
			
			AddVectors(vecEndPos, vecForward2, vecTopTraceStart);
			int attempts = 1;
			bool valid;
			do
			{
				vecTopTraceStart[2] += 1.0;
				vecEndPos[2] += 1.0;
				TraceRayF.Start(vecEndPos, vecTopTraceStart, MASK_PLAYERSOLID, RayType_EndPoint, TraceRayNoPlayers, iClient);
				bTraceHit = TRACE_GLOBAL.Hit;
				if (!bTraceHit)
				{
					valid = true;
				}
				attempts += 1;
			}
			while (bTraceHit && attempts <= 16);
			if (!valid)
				return false;
				
			DrawVectorPoints(vecEndPos, vecTopTraceStart, 5.0, {255, 0, 255, 255});
			flEndZ = vecEndPos[2] - 65.0;
			
			// Process Top Trace Start and End
			//AddVectors(vecEndPos, vecForward2, vecTopTraceStart);
			//vecTopTraceStart[2] += 1.0;
			vecTopTraceEnd = vecTopTraceStart;
			vecTopTraceEnd[2] += Z_OFFSET;
			
			// Run Top Trace
			TraceRayF.Start(vecTopTraceStart, vecTopTraceEnd, MASK_PLAYERSOLID, RayType_EndPoint, TraceRayNoPlayers, iClient);
			bTraceHit = TRACE_GLOBAL.Hit;
			DrawVectorPoints(vecTopTraceStart, vecTopTraceEnd, 5.0, {255, 0, 0, 255});
			if (bTraceHit)
				return false;			
			
			//DebugOutput("CPFHangHandler::Think --- Top Trace Missed for %N", iClient);
			
			flEndZ = float(RoundToFloor(flEndZ));
			
			float vecOriginLowered[3];
			vecOriginLowered = vecOrigin;
			vecOriginLowered[2] = flEndZ;
			if (!CheckPointAgainstPlayerHull(iClient, vecOriginLowered))
			{
				 g_HangData[iClient].Yaw = NormalToYaw(vecPlaneNormal, WALLRUN_NONE) + 180.0;
				 g_HangData[iClient].flEndZ = flEndZ;
				 return true;
			}
		}	
		return false;
	}

	public static void Disengage(int iClient, eLedgeGrabDisengageSrc eCause)
	{
		DebugOutput("CPFHangHandler::Disengage --- Disengaging %N, Cause: %d", iClient, view_as<int>(eCause));
		
		const float GRABJUMP_FWD_VELOCITY = 320.0;
		const float GRABJUMP_UP_VELOCITY = 400.0;
		
		if (!IsValidClient(iClient))
			return;
		
		if (CPFStateController.Get(iClient) == State_Locked)
			eCause = LEDGEGRAB_DISENGAGE_CROUCH;
		
		if (eCause == LEDGEGRAB_DISENGAGE_JUMP)
		{
			float vecForward[3];
			GetForwardVector(iClient, vecForward);
			ScaleVector(vecForward, GRABJUMP_FWD_VELOCITY);
			vecForward[2] = GRABJUMP_UP_VELOCITY;
			TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
			
			CPFViewController.Queue(iClient, AnimState_Ledgegrab, -1.0, true);
		}
		
		CPFHangHandler.Dismount(iClient);
	}

	public static void Hang(int iClient)
	{
		// Idea: Add a second trace check here to make this work with moving brushes
			
		if (!CPFSpeedController.GetBoost(iClient))
		{
			float flStoredSpeed = CPFSpeedController.GetStoredSpeed(iClient);
			if (flStoredSpeed > SPEED_BASE)
			{
				flStoredSpeed -= SPEED_DEPREC;
				CPFSpeedController.SetStoredSpeed(iClient, flStoredSpeed);
				DebugOutput("CPFHangHandler::Hang --- Stored Speed: %.3f", CPFSpeedController.GetStoredSpeed(iClient));
			}
			else
				CPFSpeedController.SetStoredSpeed(iClient, SPEED_BASE);
		}
		
		CPFViewController.Queue(iClient, AnimState_LedgegrabIdle, 1.0, true);
	}
};

public Action HangCooldown(Handle hTimer, int iClient)
{
	g_bHangEnabled[iClient] = true;
	return Plugin_Handled;
}