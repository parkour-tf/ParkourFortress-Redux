#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif
#if defined _MOVEMENTS_SLIDE_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_SLIDE_INCLUDED

const int IN_SLIDE = IN_FORWARD|IN_DUCK;

static float g_vecForward[MAXPLAYERS + 1][3];
static float g_vecLastSlope[MAXPLAYERS + 1][3];
static float g_vecLastSlope2[MAXPLAYERS + 1][3];
static float g_vecLastPos[MAXPLAYERS + 1][3];
static float g_vecAverageSlope[MAXPLAYERS + 1][3];

static float g_flSlideSpeed[MAXPLAYERS + 1];

static int g_iSlideTime[MAXPLAYERS + 1];

static ArrayStack g_hStackAverage[MAXPLAYERS + 1];

const float SLIDE_Z_OFFSET = 25.0;	
const float LAST_SLOPE_VERTICAL = 1.0;
const float LAST_SLOPE_UPWARD_STEEP_LIMIT = 0.5;
const float LAST_SLOPE_DOWNWARD_STEEP_LIMIT = -0.75;
const float LAST_SLOPE_DOWNWARD_LIMIT = -0.05;
const float NO_SLOPE_CUTOFF_SPEED = 600.0;

methodmap CPFSlideHandler
{
	public static void Mount(const int iClient)
	{
		SendConVarValue(iClient, FindConVar("sv_footsteps"), "0");
		CPFSoundController.PlaySlide(iClient);
		CPFSoundController.PlaySmallDing(iClient);
		
		CPFSpeedController.StoreSpeed(iClient);
		SetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed", 2600.0);
		CPFStateController.Set(iClient, State_Slide);
		
		CPFSoundController.AddIntensity(iClient, 0.5);
		CPFViewController.Queue(iClient, AnimState_Waterslide, 1.0, true);
		CPFViewController.SetDefaultSequence(iClient, AnimState_Waterslide);
	}
	
	public static void Dismount(const int iClient, const float flSpeed)
	{
		SendConVarValue(iClient, FindConVar("sv_footsteps"), "1");
		SetEntityMoveType(iClient, MOVETYPE_WALK);
		
		CPFSoundController.StopSlide(iClient);
		CPFStateController.Set(iClient, State_None);
		CPFSpeedController.SetStoredSpeed(iClient, flSpeed);
		CPFSpeedController.RestoreSpeed(iClient);
		
		g_vecAverageSlope[iClient] = view_as<float>({0.0, 0.0, 0.0});
		g_vecLastSlope[iClient] = view_as<float>({0.0, 0.0, 0.0});
		g_vecLastSlope2[iClient] = view_as<float>({0.0, 0.0, 0.0});
		g_vecLastPos[iClient] = view_as<float>({0.0, 0.0, 0.0});
		g_vecForward[iClient] = view_as<float>({0.0, 0.0, 0.0});
		g_flSlideSpeed[iClient] = 0.0;
	}
	
	public static void End(int iClient, bool bVelocityBonus = false)
	{
		const float SLIDE_OFF_LEDGE_BONUS = 90.0;
		const float SLIDE_SPEED_GAIN_STATIC = 65.0;
		const int SLIDE_MINIMUM_FOR_SPEED = 20; // This should kill scripted sliding
		
		float flSpeed = CPFSpeedController.GetStoredSpeed(iClient);
		
		float flAverage = ArrayStackAverage(g_hStackAverage[iClient]);
		delete g_hStackAverage[iClient];
		
		if (g_iSlideTime[iClient] >= TickModify(SLIDE_MINIMUM_FOR_SPEED) && flAverage < -0.05)
		{
			if (flSpeed > SPEED_MAX)
				flSpeed = SPEED_MAX_BOOST;
			else if (flSpeed < SPEED_BASE)
				flSpeed = SPEED_BASE;
			
			flSpeed += (SLIDE_SPEED_GAIN_STATIC * Logarithm(float(1 + RoundToFloor((TICKRATE_STANDARD_FLOAT/GetTickRate()) * float(g_iSlideTime[iClient]))) * (1 + FloatAbs(flAverage / 2.0))));
		}
		
		if (flSpeed > SPEED_MAX)
			flSpeed = SPEED_MAX_BOOST;
		else if (flSpeed < SPEED_BASE)
			flSpeed = SPEED_BASE;
		
		DebugOutput("CPFSlideHandler::End --- Ending slide for %N at speed %.3f (Time: %d)", iClient, flSpeed, g_iSlideTime[iClient]);
		g_iSlideTime[iClient] = 0;
		
		if (IsOnGround(iClient))
		{
			CPFStateController.RemoveFlags(iClient, SF_CAMEFROMSLIDE);
			CPFViewController.Queue(iClient, AnimState_Idle, 1.0, true);
			
		}
		else
		{
			CPFStateController.AddFlags(iClient, SF_CAMEFROMSLIDE);
			CPFViewController.Queue(iClient, AnimState_Longfall, 1.0, true);
		}
		
		if (bVelocityBonus)
		{
			float flSpeedBoost;
			
			NormalizeVector(g_vecLastSlope[iClient], g_vecLastSlope[iClient]);
			ScaleVector(g_vecLastSlope[iClient], g_flSlideSpeed[iClient] + SLIDE_OFF_LEDGE_BONUS);
			g_vecLastSlope[iClient][2] *= 0.75;
			
			DebugOutput("CPFSlideHandler::End --- Giving velocity bonus");
			
			flSpeedBoost = GetVectorLength(g_vecLastSlope[iClient]);
			
			SetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", g_vecLastSlope[iClient]);
			SetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed", flSpeedBoost);
			SetEntPropFloat(iClient, Prop_Data, "m_flSpeed", flSpeedBoost);
			
			CPFSpeedController.CarrySpeedToNextTick(iClient, flSpeed);
		}
		
		CPFSlideHandler.Dismount(iClient, flSpeed);
	}
	
	public static bool TraceSlide(int iClient)
	{
		float vecTraceEnd[3], vecSlopeNormal[3], vecEyePosition[3], vecEyeAngles[3], vecForwardInternal[3], vecOrigin[3];
		
		GetClientAbsOrigin(iClient, vecOrigin);
		GetClientEyePosition(iClient, vecEyePosition);
		GetClientEyeAngles(iClient, vecEyeAngles);
		
		vecTraceEnd = vecOrigin;
		vecTraceEnd[2] -= SLIDE_Z_OFFSET;
		
		/**** START TRACE ****/
		TraceHullF.Start(vecEyePosition, vecTraceEnd, view_as<float>({-25.0, -25.0, 0.0 }), view_as<float>({ 25.0, 25.0, 2.0 }), MASK_PLAYERSOLID, TraceRayNoPlayers, iClient);
		/**** END TRACE ****/
		if (!TRACE_GLOBAL.Hit)
		{
			DebugOutput("CPFSlideHandler::TraceSlide --- Trace missed for %N", iClient);
			
			NormalizeVector(g_vecLastSlope[iClient], g_vecLastSlope[iClient]);
			CPFSlideHandler.End(iClient, true);
			return false;
		}
		
		TRACE_GLOBAL.GetEndPosition(vecTraceEnd);
		TRACE_GLOBAL.GetPlaneNormal(vecSlopeNormal);
		
		float vecLastPosInternal[3];
		vecLastPosInternal = g_vecLastPos[iClient];
		DebugOutput("CPFSlideHandler::TraceSlide --- Slope normal %f, %f, %f", iClient, vecSlopeNormal[0], vecSlopeNormal[1], vecSlopeNormal[2]);
		DebugOutput("CPFSlideHandler::TraceSlide --- Internal position for %N: %f, %f, %f", iClient, vecLastPosInternal[0], vecLastPosInternal[1], vecLastPosInternal[2]);
		DebugOutput("CPFSlideHandler::TraceSlide --- Current position for %N: %f, %f, %f", iClient, vecEyePosition[0], vecEyePosition[1], vecOrigin[2]);
		if ((FloatAbs(vecSlopeNormal[0]) < 0.01 && FloatAbs(vecSlopeNormal[1]) < 0.01 || FloatAbs(vecSlopeNormal[2]) < 0.01) && !VectorIsZero(vecLastPosInternal) && ((vecLastPosInternal[2] - vecOrigin[2] <= 16.0) && (vecLastPosInternal[2] - vecOrigin[2] >= 4.0)))
		{
			DebugOutput("CPFSlideHandler::TraceSlide --- Trying fake slope", iClient);
			float vecFakeSlopeBuffer[3];
			
			if (GetVectorDistance(vecLastPosInternal, vecOrigin) <= 64.0)
			{
				MakeVectorFromPoints(vecLastPosInternal, vecOrigin, vecFakeSlopeBuffer);
				NormalizeVector(vecFakeSlopeBuffer, vecFakeSlopeBuffer);
				if (vecFakeSlopeBuffer[2] >= -0.75)
					vecSlopeNormal = vecFakeSlopeBuffer;
			}
		}
		
		
#if defined DEBUG
		float vecSlopeNormalScaled[3];
		AddVectors(vecSlopeNormalScaled, vecSlopeNormal, vecSlopeNormalScaled);
		ScaleVector(vecSlopeNormalScaled, 32.0);
		DrawVector(vecTraceEnd, vecSlopeNormalScaled, 10.0, {0,255,0,255});
		DrawVectorPoints(vecEyePosition, vecTraceEnd, 10.0, {255,0,0,255});
#endif
		
		if (vecSlopeNormal[2] >= 0.2)
		{
			vecForwardInternal[0] = g_vecForward[iClient][1];
			vecForwardInternal[1] = -g_vecForward[iClient][0];
			NormalizeVector(vecForwardInternal, vecForwardInternal);
			GetVectorCrossProduct(vecSlopeNormal, vecForwardInternal, vecSlopeNormal);
		}
		
		NormalizeVector(vecSlopeNormal, vecSlopeNormal);
		
		// Sloping upward, steep
		if (vecSlopeNormal[2] >= LAST_SLOPE_UPWARD_STEEP_LIMIT && vecSlopeNormal[2] != LAST_SLOPE_VERTICAL)
		{
			if (CPFStateController.Get(iClient) == State_None)
			{
				return false;
			}
			else
			{
				DebugOutput("CPFSlideHandler::Slide --- Sloping upward, steep", iClient);
				CPFSlideHandler.End(iClient);
				return false;
			}
		}
		// Sloping downward, steep
		else if (vecSlopeNormal[2] <= LAST_SLOPE_DOWNWARD_STEEP_LIMIT)
		{
			if (CPFStateController.Get(iClient) == State_None)
			{
				return false;
			}
			else
			{
				DebugOutput("CPFSlideHandler::Slide --- Sloping downward, steep", iClient);
				CPFSlideHandler.End(iClient, true);
				return false;
			}
		}
		
		g_vecLastSlope[iClient] = vecSlopeNormal;
		g_vecLastPos[iClient] = vecOrigin;
		
		return true;
	}
	
	public static void Try(int iClient, int iButtons, bool fromPriorSlide = false)
	{
		if (!IsValidClient(iClient))
			return;
		
		if (!IsOnGround(iClient))
			return;
		
		PFState eState = CPFStateController.Get(iClient);
		if (eState == State_Locked)
			return;
		
		// Get the sliding velocity
		float vecVelocity[3];
		
		GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
		vecVelocity[2] = 0.0;
		
		eState = CPFStateController.GetLast(iClient);
		
		if (CPFSpeedController.GetAirVel(iClient) > 420.0)
		{
			DebugOutput("Trying Air Velocity");
			g_flSlideSpeed[iClient] = CPFSpeedController.GetAirVel(iClient);
		}
		else
			g_flSlideSpeed[iClient] = GetVectorLength(vecVelocity);

		DebugOutput("CPFSlideHandler::Try --- %N, g_flSlideSpeed[iClient]: %.1f", iClient, g_flSlideSpeed[iClient]);
		
		if (g_flSlideSpeed[iClient] < 83.0)
			return;
		else if (g_flSlideSpeed[iClient] < 356.0)
			g_flSlideSpeed[iClient] += 64.0;
		else if ((g_flSlideSpeed[iClient] > 356.0) && (g_flSlideSpeed[iClient] < 420.0))
			g_flSlideSpeed[iClient] = 420.0;
		
		SetEntProp(iClient, Prop_Send, "m_bDucked", 1);
		SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_DUCKING);
		
		NormalizeVector(vecVelocity, vecVelocity);
		vecVelocity[2] = 0.0;
		
		g_vecForward[iClient] = vecVelocity;
		
		float vecOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		
		if (!fromPriorSlide)
		{
			float vecViewOffs[3];
			GetEntPropVector(iClient, Prop_Data, "m_vecViewOffset", vecViewOffs);
			vecViewOffs[2] -= 15.0;
			SetEntPropVector(iClient, Prop_Data, "m_vecViewOffset", vecViewOffs);
		}
		
		g_hStackAverage[iClient] = new ArrayStack();
		
		CPFSlideHandler.Mount(iClient);
		CPFSlideHandler.TraceSlide(iClient);
	}
	
	public static void Slide(int iClient, int iButtons)
	{
		g_iSlideTime[iClient] += TickModify(2);
		
		if (!(iButtons & IN_DUCK))
		{
			if (!(CPFStateController.Get(iClient) == State_None))
				CPFSlideHandler.End(iClient);
			else
				return;
		}
		
		const float SLIDE_VELOCITY_END = 83.0;
		float SLIDE_VELOCITY_DELTA = (TICKRATE_STANDARD_FLOAT/GetTickRate()) * 9.0;
		const float SLIDE_VELOCITY_MAXIMUM = 650.0;
		
		// As with before, make sure we're still ducked
		// Quack
		SetEntProp(iClient, Prop_Send, "m_bDucked", 1);
		SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_DUCKING);
		
		// If sliding velocity is too low, end the slide
		if (g_flSlideSpeed[iClient] <= SLIDE_VELOCITY_END)
		{
			if (!(CPFStateController.Get(iClient) == State_None))
				CPFSlideHandler.End(iClient);
			else
				return;
		}
		
		if ((GetGameTickCount() + iClient) % 2 == 0)
		{
			if (!CPFSlideHandler.TraceSlide(iClient))
				return;
		}
		else
			NormalizeVector(g_vecLastSlope[iClient], g_vecLastSlope[iClient]);
		
		if (g_vecAverageSlope[iClient][0] != 0.0 && g_vecAverageSlope[iClient][1] != 0.0 && g_vecAverageSlope[iClient][2] != 0.0)
		{
			AddVectors(g_vecAverageSlope[iClient], g_vecLastSlope[iClient], g_vecAverageSlope[iClient]);
			ScaleVector(g_vecAverageSlope[iClient], 0.5);
			
			if (g_hStackAverage[iClient] != null)
				g_hStackAverage[iClient].Push(g_vecLastSlope[iClient][2]);
		}
		else
			g_vecAverageSlope[iClient] = g_vecLastSlope[iClient];
		
		float vecDebugAngles[3];
		GetVectorAngles(g_vecAverageSlope[iClient], vecDebugAngles);
		
		if (CPFStateController.Get(iClient) == State_None)
			return;
		
		// Sloping downward
		else if (g_vecLastSlope[iClient][2] <= LAST_SLOPE_DOWNWARD_LIMIT)
		{
			if (g_flSlideSpeed[iClient] <= SLIDE_VELOCITY_MAXIMUM)
				g_flSlideSpeed[iClient] += SLIDE_VELOCITY_DELTA * 0.45;
		}
		// Minor or no slope
		else
		{
			if (g_flSlideSpeed[iClient] > (NO_SLOPE_CUTOFF_SPEED))
				g_flSlideSpeed[iClient] = (NO_SLOPE_CUTOFF_SPEED);
			else
				g_flSlideSpeed[iClient] -= SLIDE_VELOCITY_DELTA * 0.4;
		}
		
		ScaleVector(g_vecLastSlope[iClient], g_flSlideSpeed[iClient]);
		
		DebugOutput("CPFSlideHandler::Slide --- Current Speed %f", g_flSlideSpeed[iClient]);
		
		float vecSlopeNormal[3], flMaxSpeedDelta, flMaxSpeedExponent, flMaxSpeedSlideModifier, flFinalMaxSpeed;
		const float flMaxSpeedExpModifier = 1.0;
		const float flBaseMaxSpeed = 520.0;
		
		flMaxSpeedSlideModifier = g_flSlideSpeed[iClient] / flBaseMaxSpeed;
		
		NormalizeVector(g_vecLastSlope[iClient], vecSlopeNormal);
		
		flMaxSpeedDelta = (1 + vecSlopeNormal[2]);
		flMaxSpeedExponent = (2 - flMaxSpeedDelta) * ((2 - flMaxSpeedDelta) * flMaxSpeedExpModifier);
		flFinalMaxSpeed = (((flBaseMaxSpeed * 4) - (flBaseMaxSpeed * 3) * flMaxSpeedDelta) * flMaxSpeedExponent) * flMaxSpeedSlideModifier;
		
		if (flFinalMaxSpeed < 250.0)
			flFinalMaxSpeed = 250.0;
		
		DebugOutput("CPFSlideHandler::Slide --- Setting maxspeed to %.3f", flFinalMaxSpeed);
		
		SetEntPropFloat(iClient, Prop_Data, "m_flMaxspeed", flFinalMaxSpeed);
		PFTeleportPlayer(iClient, NULL_VECTOR, NULL_VECTOR, g_vecLastSlope[iClient]);
	}
};