#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_WALLRUN_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_WALLRUN_INCLUDED

const int IN_WALLRUN_LEFT = IN_JUMP|IN_FORWARD|IN_MOVELEFT;
const int IN_WALLRUN_RIGHT = IN_JUMP|IN_FORWARD|IN_MOVERIGHT;

stock const float VECMINS_WALLRUN[3] = {-2.000000, -2.000000, 0.000000};
stock const float VECMAXS_WALLRUN[3] = {2.000000, 2.000000, 40.000000};
stock const float VECMINS_STANDING[3] = {-24.000000, -24.000000, 0.000000};
stock const float VECMAXS_STANDING[3] = {24.000000, 24.000000, 82.000000};

const float ACTIVE_TRACE_ANGLE = 60.0;

static const int DISMOUNT_HANGTIME = 2;

enum eWallrunSide
{
	WALLRUN_NONE = -1,
	WALLRUN_LEFT = 0,
	WALLRUN_RIGHT = 1
};

enum eWallrunDisengageSource
{
	WALLRUN_DISENGAGE_INVALID = 0,
	WALLRUN_DISENGAGE_CROUCH = 1,
	WALLRUN_DISENGAGE_JUMP,
	WALLRUN_DISENGAGE_END,
	WALLRUN_DISENGAGE_BADWALL,
	WALLRUN_DISENGAGE_BADWALL_ANGLE,
	WALLRUN_DISENGAGE_CEILING,
	
	WDS_COUNT
};

enum struct WallrunData
{
	eWallrunSide Side;
	float Angle;
	float AirAccelPre;
	bool BufferedJump;
	int ActionStart;
	int DismountTime;
}

static WallrunData g_WallrunData[MAXPLAYERS + 1];

methodmap CPFWallrunHandler
{
	public static void Mount(const int iClient, const eWallrunSide eSide, float vecEyeAngles[3], float vecEndPosition[3])
	{
		const float WALLRUN_AIRACCEL = 10.0;
		const float WALLRUN_GRAVITY = 0.0;
		const float WALLRUN_SPEED = 0.0;
		
		g_WallrunData[iClient].AirAccelPre = GetPlayerAirAccel(iClient);

		g_WallrunData[iClient].Side = eSide;
		g_WallrunData[iClient].DismountTime = GetGameTickCount() + DISMOUNT_HANGTIME;

		SetPlayerAirAccel(iClient, WALLRUN_AIRACCEL);

		SetEntityMoveType(iClient, MOVETYPE_ISOMETRIC);
		SetEntityGravity(iClient, WALLRUN_GRAVITY);
		CPFSpeedController.StoreSpeed(iClient);
		CPFSpeedController.SetSpeed(iClient, WALLRUN_SPEED);
		g_WallrunData[iClient].Side = eSide;
		g_WallrunData[iClient].DismountTime = GetGameTickCount() + DISMOUNT_HANGTIME;
		SetEntProp(iClient, Prop_Data, "m_CollisionGroup", view_as<int>(COLLISION_GROUP_NONE));
		TeleportEntity(iClient, vecEndPosition, vecEyeAngles, NULL_VECTOR);
		SetEntProp(iClient, Prop_Data, "m_CollisionGroup", view_as<int>(g_ePFCollisionGroup));
		DebugOutput("CPFWallrunHandler::Start --- Stored Speed: %.3f", CPFSpeedController.GetStoredSpeed(iClient));
		
		CPFStateController.Set(iClient, State_Wallrun);
		CPFTutorialController.Stagepoint(iClient, TUTORIAL_WALLRUN);
		CPFSoundController.PlayBigDing(iClient);
		CPFSoundController.AddIntensity(iClient, 1.0);
	}
	
	public static void Dismount(const int iClient)
	{
		const float SPEED_BOOST_WALLRUN = 20.0;
		
		CPFStateController.Set(iClient, State_None);
		
		SetCollisionGroup(iClient, g_ePFCollisionGroup);
		
		SetPlayerAirAccel(iClient, g_WallrunData[iClient].AirAccelPre);
		
		float flSpeed;
		flSpeed = CPFSpeedController.GetStoredSpeed(iClient);
		
		if (GetGameTickCount() - g_WallrunData[iClient].ActionStart >= 2)
			flSpeed += SPEED_BOOST_WALLRUN;
		
		CPFSpeedController.ValidateSpeed(iClient, flSpeed);
		CPFSpeedController.SetSpeed(iClient, flSpeed);
		CPFSpeedController.SetStoredSpeed(iClient, flSpeed);
		
		SetEntityMoveType(iClient, MOVETYPE_DEFAULT);
		SetEntityGravity(iClient, 1.0);
		
		CPFViewController.Queue(iClient, AnimState_Longfall, 1.0, true);
	}
	
	public static void Break(int iClient, eWallrunDisengageSource eCause)
	{
		const float WALLRUNJUMP_FWD_VELOCITY = 460.0;
		const float WALLRUNJUMP_UP_VELOCITY = 340.0;
	
		DebugOutput("CPFWallrunHandler::Break --- %N, Cause: %d", iClient, view_as<int>(eCause));
		
		CPFWallrunHandler.Dismount(iClient);
		
		float vecAngles[3];
		GetClientEyeAngles(iClient, vecAngles);
		vecAngles[2] = 0.0;
		TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
		
		switch (eCause)
		{
			case WALLRUN_DISENGAGE_JUMP:
			{
				float vecForward[3];
				GetForwardVector(iClient, vecForward);
				ScaleVector(vecForward, WALLRUNJUMP_FWD_VELOCITY);
				vecForward[2] = WALLRUNJUMP_UP_VELOCITY;
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
				SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
				CPFStateController.SetCooldown(iClient, State_Wallrun, 0.5);
			}
			
			case WALLRUN_DISENGAGE_BADWALL:
			{
				SetEntProp(iClient, Prop_Send, "m_iAirDash", (GetGameTickCount() - g_WallrunData[iClient].ActionStart >= 2) ? 1 : 0);
			}
			
			default:
			{
				SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
				//DebugOutput("CPFWallrunHandler::Break --- Caught non-jump break (%N) eCause: %d", iClient, view_as<int>(eCause));
			}
		}
		
	}
	
	public static void SetJump(int iClient, bool bSet)
	{
		g_WallrunData[iClient].BufferedJump = bSet;
	}
	
	public static bool GetJump(int iClient)
	{
		return g_WallrunData[iClient].BufferedJump;
	}
	
	public static bool CanBreak(int iClient)
	{
		return (g_WallrunData[iClient].DismountTime < GetGameTickCount());
	}
	
	public static bool PerformCeilingCheck(int iClient)
	{
		const float WALLRUN_CEILING_CHECK = 65.0;
		
		float vecOrigin[3], vecRaisedOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		
		vecRaisedOrigin = vecOrigin;
		vecRaisedOrigin[2] += WALLRUN_CEILING_CHECK;
		
		DrawBoundingBox(view_as<float>({-24.0, 24.0, 0.0}), view_as<float>({24.0, -24.0, 1.0}), vecRaisedOrigin);
		TraceHullF hCeilingCheck = new TraceHullF(vecOrigin, vecRaisedOrigin, view_as<float>({-24.0, 24.0, 0.0}), view_as<float>({24.0, -24.0, 1.0}), MASK_PLAYERSOLID, TraceRayNoPlayers);
		bool bCeilingHit = hCeilingCheck.Hit;
		delete hCeilingCheck;
		
		return bCeilingHit;
	}
	
	public static bool IsValidWall(int iClient, const CBaseSDKTrace hTrace, float vecPlaneNormal[3])
	{
		if (hTrace.Hit)
		{
			if (hTrace.SurfaceFlags & SURF_SKY || hTrace.SurfaceFlags & SURF_SKY2D)
			{
				DebugOutput("CPFWallrunHandler::IsValidWall --- Skybox wallrun rejected");
				return false;
			}
		
			float vecEyePosition[3], vecDrawBuffer[3], vecOrigin[3];
			GetClientEyePosition(iClient, vecEyePosition);
			GetClientAbsOrigin(iClient, vecOrigin);
			hTrace.GetPlaneNormal(vecPlaneNormal);
			
			vecDrawBuffer = vecPlaneNormal;
			ScaleVector(vecDrawBuffer, 50.0);
			DrawVector(vecOrigin, vecDrawBuffer, 10.0, {255, 100, 200, 255});
			
			//DebugOutput("CPFWallrunHandler::IsValidWall --- Trace Hit");
			DebugOutput("CPFWallrunHandler::IsValidWall --- vecPlaneNormal: %.3f, %.3f, %.3f ", vecPlaneNormal[0], vecPlaneNormal[1], vecPlaneNormal[2]);
			return (vecPlaneNormal[2] > -0.05 && vecPlaneNormal[2] < 0.05);
		}
		
		DebugOutput("CPFWallrunHandler::IsValidWall --- Trace Miss");
		return false;
	}
	
	public static void Wallrun(int iClient)
	{
		const float WALLRUN_HEIGHT_COEFFICIENT = -1.5;
		const int WALLRUN_HEIGHT_SHIFT = 120;
		const int WALLRUN_MAX_TIME = 200;
		const float WALLRUN_LERP_GOAL_ANGLE = 15.0;
		const int WALLRUN_LERP_LENGTH = 12;
		
		if (GetGameTickCount() - g_WallrunData[iClient].ActionStart > TickModify(WALLRUN_MAX_TIME))
		{
			CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_END);
			return;
		}
		
		float vecRayAngles[3], vecEndHullPos[3], vecOrigin[3], vecVelocity[3], vecPlaneNormal[3], vecWallAngles[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		
		if (CPFWallrunHandler.PerformCeilingCheck(iClient))
		{
			CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_CEILING);
			return;
		}
		
		vecWallAngles[1] = g_WallrunData[iClient].Angle;
		
		//DebugOutput("CPFWallrunHandler::Wallrun --- g_WallrunData[%N].Side[: %d", iClient, g_WallrunData[iClient].Side);
		vecRayAngles[1] = g_WallrunData[iClient].Angle + (g_WallrunData[iClient].Side == WALLRUN_LEFT ? ACTIVE_TRACE_ANGLE : -ACTIVE_TRACE_ANGLE);
		ForwardVector(vecRayAngles, 300.0, vecEndHullPos);
		
		AddVectors(vecOrigin, vecEndHullPos, vecEndHullPos);
		
		static float vecMins[3] = {-1.0, -1.0, 0.0};
		static float vecMaxs[3] = {1.0, 1.0, 82.0};
		
		//DebugOutput("CPFWallrunHandler::Wallrun --- g_flLastWallAngle[%N]: %.3f", iClient, g_WallrunData[iClient].Angle);
		DrawVectorPoints(vecOrigin, vecEndHullPos, 25.0, {0, 50, 255, 255});
		
		TraceHullF hTrace = new TraceHullF(vecOrigin, vecEndHullPos, vecMins, vecMaxs, MASK_PLAYERSOLID, TraceRayNoPlayers);
		if (CPFWallrunHandler.IsValidWall(iClient, hTrace, vecPlaneNormal))
		{
			float vecAngles[3], flNextWallYaw, flAngleDiff;
			eWallrunSide eSide = g_WallrunData[iClient].Side;
			
			flNextWallYaw = NormalToYaw(vecPlaneNormal, eSide);
			flAngleDiff = ANGLEDIFF(g_WallrunData[iClient].Angle, flNextWallYaw);
			
			if (flAngleDiff >= 360)
				flAngleDiff -= 360;
			if (flAngleDiff > 60.0)
			{
				DebugOutput("CPFWallrunHandler::Wallrun --- flAngleDiff: %.3f", flAngleDiff);
				CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_BADWALL_ANGLE);
				delete hTrace;
				return;
			}
			
			g_WallrunData[iClient].Angle = flNextWallYaw;
			
			ForwardVector(vecWallAngles, 420.0, vecVelocity);
			vecVelocity[2] = (WALLRUN_HEIGHT_COEFFICIENT * (((TICKRATE_STANDARD_FLOAT/GetTickRate()) * float(GetGameTickCount() - g_WallrunData[iClient].ActionStart)) - WALLRUN_HEIGHT_SHIFT));
			
			if (!GetCookieInt(g_cookieLerp, iClient))
			{
				GetClientAbsAngles(iClient, vecAngles);
				if (GetGameTickCount() - g_WallrunData[iClient].ActionStart < WALLRUN_LERP_LENGTH + 5.0 && GetGameTickCount() - g_WallrunData[iClient].ActionStart > 5.0)
				{
					float flLerpAngle = ((float(GetGameTickCount() - g_WallrunData[iClient].ActionStart - 5) / float(WALLRUN_LERP_LENGTH)) * WALLRUN_LERP_GOAL_ANGLE);
					vecAngles[2] += (eSide == WALLRUN_LEFT) ? flLerpAngle : -flLerpAngle;
					TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
				}
				else if (GetGameTickCount() - g_WallrunData[iClient].ActionStart == WALLRUN_LERP_LENGTH + 5.0)
				{
					vecAngles[2] = (eSide == WALLRUN_LEFT) ? WALLRUN_LERP_GOAL_ANGLE : -WALLRUN_LERP_GOAL_ANGLE;
					TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
				}
			}
			
			PFTeleportPlayer(iClient, NULL_VECTOR, NULL_VECTOR, vecVelocity);
			
			CPFSpeedController.SetSpeed(iClient, 1.0);
			CPFSoundController.PlayWallrun(iClient, GetGameTickCount() - g_WallrunData[iClient].ActionStart);
		}
		else
		{
			CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_BADWALL);
		}
		
		delete hTrace;
	}
	
	public static void SetSide(const int iClient, const eWallrunSide eSide, const float flAngleDifference, float vecEyeAngles[3])
	{
		if (eSide == WALLRUN_LEFT && flAngleDifference >= -45.0 && flAngleDifference <= 60.0)
		{
			DebugOutput("CPFWallrunHandler::SetSide --- Left Wallrun (%N)", iClient);
			CPFViewController.Queue(iClient, AnimState_HandslideLeft, 1.0, true);
			CPFViewController.SetDefaultSequence(iClient, AnimState_HandslideLeft);

		}
		else if (eSide == WALLRUN_RIGHT && flAngleDifference >= -45.0 && flAngleDifference <= 60.0)
		{
			DebugOutput("CPFWallrunHandler::Try --- Right Wallrun (%N)", iClient);
			CPFViewController.Queue(iClient, AnimState_HandslideRight, 1.0, true);
			CPFViewController.SetDefaultSequence(iClient, AnimState_HandslideRight);
		}
	}
	
	public static void RunPositionCorrection(float vecEndPosition[3])
	{
		float vecGoDownPlease[3];
		vecGoDownPlease = vecEndPosition;
		vecGoDownPlease[2] -= 24.0;
		TraceRayF hPositionCorrect = new TraceRayF(vecEndPosition, vecGoDownPlease, MASK_PLAYERSOLID, RayType_EndPoint, TraceRayNoPlayers);
		float vecNewEndPos[3];
		if (hPositionCorrect.Hit)
		{
			hPositionCorrect.GetEndPosition(vecNewEndPos);
			DrawVectorPoints(vecEndPosition, vecNewEndPos, 10.0, {0, 255, 255, 255});
			
			vecEndPosition[2] = vecNewEndPos[2] + 24.0;
			//DrawVectorPoints(vecOrigin, vecEndPosition, 10.0, {255, 255, 255, 255});
		}
		delete hPositionCorrect;
	}
	
	public static void Try(const int iClient, const eWallrunSide eSide)
	{
		if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed)) return;
		
		g_WallrunData[iClient].ActionStart = GetGameTickCount();
		float vecOrigin[3], vecEyePosition[3], vecEyeAngles[3], vecPlaneNormal[3], vecEndPosition[3], vecEndHullPos[3], vecEndHullRotPos[3], vecStartHullPos[3];
		float flClientYaw, flWallYaw, flDegrees;
		float flRotation = 90.0;
		int iTraces = g_cvarWallrunTraces.IntValue;
		int iAttempts = 1;
		flDegrees = flRotation/(iTraces - 1);
		
		// Set up the values
		GetClientAbsOrigin(iClient, vecOrigin);
		GetClientEyeAngles(iClient, vecEyeAngles);
		GetClientEyePosition(iClient, vecEyePosition);
		
		// Client Yaw and Initial Forward
		flClientYaw = vecEyeAngles[1];
		vecEyeAngles[0] = 0.0;
		vecEyeAngles[2] = 0.0;
		ForwardVector(vecEyeAngles, 1.0, vecEndHullPos);
		flRotation = eSide ? -1 * (flRotation) : flRotation;
		
		
		// TraceHull Start Position Setup
		vecStartHullPos = vecEndHullPos;
		vecOrigin[2] += 45.0;
		ScaleVector(vecStartHullPos, 24.0);
		AddVectors(vecStartHullPos, vecOrigin, vecStartHullPos);
		AddVectors(vecStartHullPos, view_as<float>({0.0, 0.0, 2.0}), vecStartHullPos);
		
		ScaleVector(vecEndHullPos, 244.0);
		AddVectors(vecEndHullPos, vecOrigin, vecEndHullPos);
		DrawVectorPoints(vecOrigin, vecEndHullPos, 10.0, {255, 0, 0, 255});
		//SubtractVectors(vecEndHullPos, vecRotBuffer, vecEndHullRotPos);
		
		vecEndHullRotPos = vecEndHullPos;
		VectorAxisRotate (vecEndHullRotPos, vecOrigin, vecEndHullRotPos, flRotation, 0.0);
		float flRotationDelta = ((flRotation / FloatAbs(flRotation)));
		
		// Now, the initial trace
		do
		{	
			/**** START TRACE ****/
			TraceHullF.Start(vecStartHullPos, vecEndHullRotPos, VECMINS_WALLRUN, VECMAXS_WALLRUN, MASK_PLAYERSOLID, TraceRayNoPlayers);
			/**** END TRACE ****/
			TRACE_GLOBAL.GetEndPosition(vecEndPosition);
			DrawVectorPoints(vecOrigin, vecEndPosition, 10.0, {0, 255, 255, 255});
			if (TRACE_GLOBAL.Hit)
			{
				DebugOutput("CPFWallrunHandler::Try --- Attempt (%i)", iAttempts);
				
				/**** START TRACE ****/
				TraceRayF hCheckNormalStart = new TraceRayF(vecStartHullPos, vecEndHullRotPos, MASK_PLAYERSOLID, RayType_EndPoint, TraceRayNoPlayers);
				if (hCheckNormalStart.Hit && CPFWallrunHandler.IsValidWall(iClient, hCheckNormalStart, vecPlaneNormal))
				{
					flWallYaw = NormalToYaw(vecPlaneNormal, eSide);
					delete hCheckNormalStart;
					
					float flAngleDifference = ANGLEDIFF(flWallYaw, flClientYaw);
					DebugOutput("CPFWallrunHandler::Try --- Client (%N)f, lWallYaw (%f), flCientYaw (%f), flAngleDifference (%f)", iClient, flWallYaw, flClientYaw, flAngleDifference);
					if (flAngleDifference >= -45.0 && flAngleDifference <= 60.0)
					{
						ScaleVector(vecPlaneNormal, 32.0);
						
						AddVectors(vecEndPosition, vecPlaneNormal, vecEndPosition);
						DrawVectorPoints(vecOrigin, vecEndPosition, 10.0, {255, 255, 0, 255});
						vecEndPosition[2] -= 24.0;
						
						
						float vecHullStartHit[3];
						TraceHullF hCheckHullStart = new TraceHullF(vecEyePosition, vecEndPosition, VECMINS_WALLRUN, VECMAXS_WALLRUN, MASK_PLAYERSOLID, TraceRayNoPlayers);
						bool bCheckHullHit = hCheckHullStart.Hit;
						if (bCheckHullHit)
						{
							hCheckHullStart.GetEndPosition(vecHullStartHit);
						}
						
						delete hCheckHullStart;
						
						if(!CheckPointAgainstPlayerHull(iClient, vecEndPosition) && (!bCheckHullHit || GetVectorDistance(vecHullStartHit, vecEyePosition) == GetVectorDistance(vecEndPosition, vecEyePosition)))
						{
							CPFWallrunHandler.RunPositionCorrection(vecEndPosition);
							
							DebugOutput("CPFWallrunHandler::Try --- Valid Wall (%N)", iClient);
							vecEndHullPos = vecEndHullRotPos;
							g_WallrunData[iClient].Angle = flWallYaw;
							
							DrawVectorPoints(vecOrigin, vecEndPosition, 10.0, {0, 255, 0, 255});
							DrawBoundingBox(VECMINS_WALLRUN, VECMAXS_WALLRUN, vecEndPosition, _, {0, 255, 0, 255});
							
							CPFWallrunHandler.SetSide(iClient, eSide, flAngleDifference, vecEyeAngles);
							CPFWallrunHandler.Mount(iClient, eSide, vecEyeAngles, vecEndPosition);
							return;
						}
					}
				}
				delete hCheckNormalStart;
				/**** END TRACE ****/
			}
			vecEndHullRotPos = vecEndHullPos;
			VectorAxisRotate(vecEndHullRotPos, vecOrigin, vecEndHullRotPos, -1 * (-flRotation + (flRotationDelta * flDegrees * iAttempts)), ((flDegrees/100) * iAttempts));
			iAttempts += 1;
			
		} while (iAttempts <= iTraces);
		
		CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_BADWALL);
		return;
	}
};

float ANGLEDIFF(float x, float y) { return FloatAbs( (x - y) + (( (x - y) > 180.0 ) ? -360.0 : ( ( (x - y) < -180.0) ? 360.0 : 0.0) )); }

stock void VectorAxisRotate(float vec[3], float vecOrigin[3], float vecResult[3], float angle, float scale) //renamed to not conflict with new .inc
{
	SubtractVectors (vec, vecOrigin, vec);
	float theta, cs, sn, px, py, x, y;
	theta = DegToRad(angle);

	cs = Cosine(theta);
	sn = Sine(theta);
	
	x = vec[0];
	y = vec[1];
	
	px = (x * cs) - (y * sn); 
	py = (x * sn) + (y * cs);
	
	vec[0] = px;
	vec[1] = py;
	vec[2] = 0.0;
	
	NormalizeVector(vec, vec);
	ScaleVector(vec, (244.0 * (0.1 + scale)));
	AddVectors (vec, vecOrigin, vec);
	vecResult = vec;
}

stock void DebugVector(int iClient, float origin[3], float vec[3], float scale, float lifetime, int iColor[4])
{
    float vecBuffer[3], vecAngles[3];
    vecBuffer = vec;
    DrawVectorPoints(origin, vecBuffer, lifetime, iColor);
    GetVectorAngles(vecBuffer, vecAngles);
}
