#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_WALLCLIMB_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_WALLCLIMB_INCLUDED

const int IN_WALLCLIMB = IN_FORWARD|IN_JUMP;

const int WALLCLIMB_MAX_TICKS = 125;

enum eWallclimbDisengageSource
{
	WALLCLIMB_DISENGAGE_INVALID = 0,
	WALLCLIMB_DISENGAGE_END = 1,
	WALLCLIMB_DISENGAGE_JUMP,
	WALLCLIMB_DISENGAGE_HANG,
	WALLCLIMB_DISENGAGE_VAULT,
	WALLCLIMB_DISENGAGE_TIMEOUT,
	WALLCLIMB_DISENGAGE_DUCK,
	
	WCBS_COUNT
};

enum struct WallclimbData
{
	float Angle;
	int ActionStart;
}

static WallclimbData g_WallclimbData[MAXPLAYERS + 1];


methodmap CPFWallclimbHandler
{
	public static void Mount(const int iClient, const float vecEndPosition[3])
	{
		CPFStateController.Set(iClient, State_Wallclimb);
		CPFTutorialController.Stagepoint(iClient, TUTORIAL_CLIMB);
		CPFSpeedController.SetSpeed(iClient, 1.0);
		
		SetEntityMoveType(iClient, MOVETYPE_ISOMETRIC);
		g_WallclimbData[iClient].ActionStart = GetGameTickCount();
		SetEntityFlags(iClient, GetEntityFlags(iClient)|FL_ATCONTROLS);
		TeleportEntity(iClient, vecEndPosition, NULL_VECTOR, ZERO_VECTOR);
		
		CPFSoundController.PlaySmallDing(iClient);
		CPFSoundController.AddIntensity(iClient, 0.25);
		CPFViewController.Queue(iClient, AnimState_VerticalClimb, 1.0, true);
	}
	
	public static void Dismount(const int iClient)
	{
		CPFViewController.Queue(iClient, AnimState_Idle, 1.0, true);
		
		g_WallclimbData[iClient].ActionStart = 0;
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		SetCollisionGroup(iClient, g_ePFCollisionGroup);
		
		CPFSpeedController.RestoreSpeed(iClient);
	}
	
	public static int StartTick(int iClient)
	{
		return g_WallclimbData[iClient].ActionStart;
	}

	public static void Break(int iClient, eWallclimbDisengageSource eCause)
	{
		DebugOutput("CPFWallclimbHandler::Disengage --- Disengaging %N, Cause: %d", iClient, view_as<int>(eCause));
		
		const float WALLCLIMBJUMP_FWD_VELOCITY = 330.0;
		const float WALLCLIMBJUMP_UP_VELOCITY = 355.0;
		
		if (eCause != WALLCLIMB_DISENGAGE_HANG)
		{
			SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
			SetEntityMoveType(iClient, MOVETYPE_WALK);
			CPFStateController.Set(iClient, State_None);
		}
		
		switch (eCause)
		{
			case WALLCLIMB_DISENGAGE_JUMP:
			{
				CPFTutorialController.Stagepoint(iClient, TUTORIAL_KICKOFF);
				float vecForward[3];
				GetForwardVector(iClient, vecForward, WALLCLIMBJUMP_FWD_VELOCITY);
				vecForward[2] = WALLCLIMBJUMP_UP_VELOCITY;
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
			}
			
			default:
			{
				DebugOutput("CPFWallclimbHandler::Break --- Disengage cause defaulted: %d", view_as<int>(eCause));
			}
			
		}
		
		CPFWallclimbHandler.Dismount(iClient);
	}
	
	public static bool IsValidWall(int iClient, TraceHullF hClimbTrace)
	{
		const float WALLCLIMB_CHECK_DISTANCE = 10.0;
	
		float vecAngles[3], vecEyePosition[3], vecEndPosition[3], vecWallNormal[3], vecWallAngles[3], flPhi;
		GetClientAbsAngles(iClient, vecAngles);
		GetClientEyePosition(iClient, vecEyePosition);
		
		vecEndPosition = vecEyePosition;
		vecEndPosition[2] += WALLCLIMB_CHECK_DISTANCE;
		
		/**** START TRACE ****/
		TraceHullF hTrace = new TraceHullF(vecEyePosition, vecEndPosition, view_as<float>({-20.0, -20.0, 0.0}), view_as<float>({20.0, 20.0, 74.0}), MASK_PLAYERSOLID, TraceRayNoPlayers);
		bool bHit = hTrace.Hit;
		delete hTrace;
		/**** END TRACE ****/
		
		if (bHit)
			return false;
		
		GetAngleVectors(vecAngles, vecAngles, NULL_VECTOR, NULL_VECTOR);
		hClimbTrace.GetPlaneNormal(vecWallNormal);
		flPhi = GetVectorDotProduct(vecAngles, vecWallNormal);
		
		if (-0.05 < vecWallNormal[2] && vecWallNormal[2] < 0.05 && flPhi <= -0.5)
		{
			NegateVector(vecWallNormal);
			GetVectorAngles(vecWallNormal, vecWallAngles);
			g_WallclimbData[iClient].Angle = vecWallAngles[1];
			
			return true;
		}

		return false;
	}
	
	public static void Try(int iClient)
	{
		if (CPFStateController.IsOnCooldown(iClient, State_Wallclimb) || TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
			return;
		
		float vecEyePosition[3], vecAngles[3], vecEndPosition[3], vecEndHullPos[3], vecForward[3], vecNegate[3];
		GetClientEyePosition(iClient, vecEyePosition);
		GetClientAbsAngles(iClient, vecAngles);
		
		vecAngles[0] = 0.0;
		ForwardVector(vecAngles, 64.0, vecForward);
		ForwardVector(vecAngles, 8.0, vecNegate);
		NegateVector(vecNegate);
		AddVectors(vecEyePosition, vecForward, vecEndHullPos);
		
		/**** START TRACE ****/
		TraceHullF hClimbTrace = new TraceHullF(vecEyePosition, vecEndHullPos, view_as<float>({-24.0, -24.0, -8.0}), view_as<float>({24.0, 24.0, 8.0}), MASK_PLAYERSOLID, TraceRayNoPlayers);
		hClimbTrace.GetEndPosition(vecEndPosition);
		
		float vecBuffer[3];
		vecBuffer = vecEndPosition;
		AddVectors(vecEndPosition, vecNegate, vecEndPosition);
		
		DrawVectorPoints(vecBuffer, vecEndPosition, 10.0, view_as<int>({64, 128, 255, 255}));
		vecEndPosition[2] -= 24.0;
		
		if (!hClimbTrace.Hit || !CPFWallclimbHandler.IsValidWall(iClient, hClimbTrace) || CheckPointAgainstPlayerHull(iClient, vecEndPosition))
		{
			delete hClimbTrace;
			return;
		}
		
		delete hClimbTrace;
		/**** END TRACE ****/
		
		CPFWallclimbHandler.Mount(iClient, vecEndPosition);
	}
	
	public static void Wallclimb(int iClient)
	{
		const float WALLCLIMB_HEIGHT_COEFFICIENT = -2.25;
		const int WALLCLIMB_HEIGHT_SHIFT = 100;
		
		float vecOrigin[3], vecTraceAngle[3], vecTraceEndPos[3], vecTraceBuffer[3], vecVelocity[3];
		GetClientEyePosition(iClient, vecOrigin);
		
		vecTraceAngle[1] = g_WallclimbData[iClient].Angle;
		
		ForwardVector(vecTraceAngle, 48.0, vecTraceBuffer);
		AddVectors(vecOrigin, vecTraceBuffer, vecTraceBuffer);
		
		TraceRayF.Start(vecOrigin, vecTraceAngle, MASK_PLAYERSOLID, RayType_Infinite, TraceRayNoPlayers, iClient);
		TRACE_GLOBAL.GetEndPosition(vecTraceEndPos);
		
		if (GetVectorDistance(vecOrigin, vecTraceEndPos, true) < 40000)
		{
			int iTicksElapsed = GetGameTickCount() - g_WallclimbData[iClient].ActionStart;
			if (iTicksElapsed < TickModify(WALLCLIMB_MAX_TICKS))
				CPFSpeedController.SetSpeed(iClient, 1.0);
			
			vecVelocity[2] = WALLCLIMB_HEIGHT_COEFFICIENT * (((TICKRATE_STANDARD_FLOAT/GetTickRate()) * float(iTicksElapsed)) - float(WALLCLIMB_HEIGHT_SHIFT));
			TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecVelocity);
			
			if (iTicksElapsed >= TickModify(WALLCLIMB_MAX_TICKS))
			{
				CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_TIMEOUT);
			}
		}
		else
		{
			CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_END);
		}	
	}
};
