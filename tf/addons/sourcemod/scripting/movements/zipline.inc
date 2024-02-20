#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_ZIPLINE_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_ZIPLINE_INCLUDED

enum eZiplineDisengageSource
{
	ZIPLINE_DISENGAGE_INVALID = 0,
	ZIPLINE_DISENGAGE_CROUCH = 1,
	ZIPLINE_DISENGAGE_END,
	ZIPLINE_DISENGAGE_NOTROPE,
	ZIPLINE_DISENGAGE_TOOSHORT,
	ZIPLINE_DISENGAGE_TOOFAST,
	
	ZDS_COUNT
};

static bool g_bZiplineCooldown[MAXPLAYERS + 1];
static CPFRope g_hClientRopes[MAXPLAYERS + 1];
static int g_iZiplineStartTick[MAXPLAYERS + 1];
static float g_vecNearestZipline[MAXPLAYERS + 1][2][3];
static CPFRope g_hNearestZipline[MAXPLAYERS + 1];

methodmap CPFZiplineHandler
{
	public static void Mount(const int iClient)
	{
		CPFSpeedController.SetSpeed(iClient, 0.0);
		CPFStateController.Set(iClient, State_Zipline);
		
		SetEntityMoveType(iClient, MOVETYPE_ISOMETRIC);
		SetEntityFlags(iClient, GetEntityFlags(iClient)|FL_ATCONTROLS);
		
		CPFSoundController.PlayZipline(iClient);
		CPFSoundController.PlayBigDing(iClient);
		CPFSoundController.AddIntensity(iClient, 2.0);
		
		CPFViewController.Queue(iClient, AnimState_Zipline, 1.0, true);
		CPFViewController.SetDefaultSequence(iClient, AnimState_ZiplineIdle);
	}
	
	public static void Dismount(const int iClient)
	{
		CPFStateController.Set(iClient, State_None);
		CPFSoundController.StopZipline(iClient);
		
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		SetEntityMoveType(iClient, MOVETYPE_WALK);
		SetCollisionGroup(iClient, g_ePFCollisionGroup);
	}
	
	public static void Set(const int iClient, const CPFRope hRope)
	{
		if (!IsValidClient(iClient))
			return;
		
		g_hClientRopes[iClient] = hRope;
		DebugOutput("CPFZiplineController::Set --- Set %N's rope to %d", iClient, ((hRope != null) ? hRope.RopeIndex : 0));
	}
	
	public static CPFRope Get(const int iClient)
	{
		if (!IsValidClient(iClient))
			return view_as<CPFRope>(INVALID_HANDLE);
		
		return g_hClientRopes[iClient];
	}

	public static void Break(const int iClient, const eZiplineDisengageSource eCause)
	{
		DebugOutput("CPFZiplineHandler::Break --- %N %d", iClient, view_as<int>(eCause));
		
		if (eCause == ZIPLINE_DISENGAGE_END)
		{
			CPFRope hRope = CPFZiplineHandler.Get(iClient);
			if (hRope.NextSegment != null)
			{
				MountNextSegment(iClient, hRope.NextSegment);
				return;
			}
		}
		
		const float ZIP_COOLDOWN_TIME = 0.5;
		
		CPFZiplineHandler.Set(iClient, view_as<CPFRope>(INVALID_HANDLE));
		CPFZiplineHandler.Dismount(iClient);
		
		if (eCause == ZIPLINE_DISENGAGE_TOOSHORT || eCause == ZIPLINE_DISENGAGE_TOOFAST)
		{
			CPFSpeedController.RestoreSpeed(iClient);
			return;
		}
		
		float vecVelocity[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
		vecVelocity[2] *= 0.66;
		ScaleVector(vecVelocity, 0.75);
		TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecVelocity);
		
		g_bZiplineCooldown[iClient] = true;
		CreateTimer(ZIP_COOLDOWN_TIME, Zipline_RemoveCooldown, GetClientUserId(iClient));
		
		RequestFrame(StopZiplineTwice, GetClientUserId(iClient));
		
		if (eCause == ZIPLINE_DISENGAGE_CROUCH || eCause == ZIPLINE_DISENGAGE_END)
			SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);
		
		CPFSpeedController.SetStoredSpeed(iClient, (CPFSpeedController.GetStoredSpeed(iClient) + 50.0));
		CPFSpeedController.RestoreSpeed(iClient);
	}

	public static void StartThink(const int iClient, const bool bPolarityBypass, float vecClientOrigin[3], 
									float vecEndPoint[3], float vecRopeVelocity[3], float &flScale = 1.0, int &iTicksElapsed = 0)
	{
		const float ZIP_MAX_SCALE = 1855.0;
		const float ZIP_SCALE_MULT = 6.5;
		const float ZIP_SCALE_OFFSET = 230.0;
		const int ZIP_MAX_TICKS = 250;

		// The old code sets speed again here, I'm not adding that unless I find it necessary
	
		float vecRopeNormal[3], vecRopeVelScaled[3], vecKeyframeOrigin[3], vecPlayerToEnd[3];

		CPFRope hRope = CPFZiplineHandler.Get(iClient);
		GetClientAbsOrigin(iClient, vecClientOrigin);
		
		if (!iTicksElapsed)
			iTicksElapsed = GetGameTickCount() - g_iZiplineStartTick[iClient];
		
		if (flScale == 0.0)
			flScale = (!(iTicksElapsed > TickModify(ZIP_MAX_TICKS))) ? (ZIP_SCALE_MULT * ((TICKRATE_STANDARD_FLOAT/GetTickRate()) * float(iTicksElapsed)) + ZIP_SCALE_OFFSET) : ZIP_MAX_SCALE;
		
			//(!(iTicksElapsed > 350)) ? (6.3 * float(iTicksElapsed) + 230) : ZIP_MAX_SCALE;
		RopeNormal(hRope, vecRopeNormal);	
		
		vecRopeVelocity = vecRopeNormal;
		ScaleVector(vecRopeVelocity, flScale);
		
		vecRopeVelScaled = vecRopeVelocity;
		ScaleVector(vecRopeVelScaled, 0.01);
		AddVectors(vecRopeVelScaled, vecClientOrigin, vecEndPoint);
		
		GetEntPropVector(EntRefToEntIndex(hRope.KeyframeIndex), Prop_Send, "m_vecOrigin", vecKeyframeOrigin);
		MakeVectorFromPoints(vecClientOrigin, vecKeyframeOrigin, vecPlayerToEnd);
		NormalizeVector(vecPlayerToEnd, vecPlayerToEnd);
		
		DebugOutput("vecPlayerToEnd: %.1f, %.1f, %.1f", vecPlayerToEnd[0], vecPlayerToEnd[1], vecPlayerToEnd[2]);
		DebugOutput("vecRopeNormal: %.1f, %.1f, %.1f", vecRopeNormal[0], vecRopeNormal[1], vecRopeNormal[2]);
		
		if (!bPolarityBypass)
		{
			bool bPolarityToEnd[3], bPolarityFromRope[3];
			for (int i = 0; i < 3; i++)
			{
				bPolarityToEnd[i] = (vecPlayerToEnd[i] >= 0.0);
				bPolarityFromRope[i] = (vecRopeNormal[i] >= 0.0);
			}
			
			if (bPolarityFromRope[0] != bPolarityToEnd[0] || bPolarityFromRope[1] != bPolarityToEnd[1])
			{
				CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_END);
				return;
			}
		}
	}

	public static bool TryForward(const int iClient)
	{
		const int ZIPLINE_FORWARD_ATTEMPTS = 30;
		
		float vecClientOrigin[3], vecEndPoint[3], vecEndPosition[3], vecRopeVelocity[3];
		
		for (int i = 0; i < ZIPLINE_FORWARD_ATTEMPTS; i++)
		{
			CPFZiplineHandler.StartThink(iClient, true, vecClientOrigin, vecEndPoint, vecRopeVelocity, _, i);
			AddVectors(vecClientOrigin, vecRopeVelocity, vecEndPosition);
			
			if (!CheckPointAgainstPlayerHull(iClient, vecEndPosition))
			{
				TeleportEntity(iClient, vecEndPosition, NULL_VECTOR, NULL_VECTOR);
				return true;
			}
			else continue;
		}
		
		return false;
	}

	public static void MountZipline(const int iClient, const CPFRope hRope, const bool bContinued = false, 
									const float vecPersist[3] = ZERO_VECTOR)
	{
		const float MOUNT_Z_OFFSET = 55.0;
		const float ZIPLINE_MIN_DISTANCE = 400.0;
		
		CPFZiplineHandler.Set(iClient, hRope);
		g_iZiplineStartTick[iClient] = GetGameTickCount();
		
		int iMoveRope = EntRefToEntIndex(hRope.EntIndex);
		if (!CPFRopeController.IsRopeStart(iMoveRope))
		{
			CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_NOTROPE);
			return;
		}
		
		float vecEyePosition[3], vecRopeOrigin[3], vecRopeStartPosition[3], vecKeyframeOrigin[3], flDistance;
		// First, let's set up some base values
		GetClientEyePosition(iClient, vecEyePosition);
		GetEntPropVector(iMoveRope, Prop_Send, "m_vecOrigin", vecRopeOrigin);
		GetEntPropVector(hRope.KeyframeIndex, Prop_Send, "m_vecOrigin", vecKeyframeOrigin);
		
		if ((vecPersist[0] == 0.0 && vecPersist[1] == 0.0 && vecPersist[2] == 0.0) || CheckPointAgainstPlayerHull(iClient, vecPersist))
		{
			// Client's distance from the start of the rope
			flDistance = GetVectorDistance(vecEyePosition, vecRopeOrigin);
			
			// Get our scaled normal and teleport the player to their zipline starting position
			RopeNormal(hRope, vecRopeStartPosition, flDistance);
			
			AddVectors(vecRopeOrigin, vecRopeStartPosition, vecRopeStartPosition);
		}
		else
			vecRopeStartPosition = vecPersist;
		
		vecRopeStartPosition[2] -= MOUNT_Z_OFFSET;
		
		if (GetVectorDistance(vecEyePosition, vecKeyframeOrigin) < ZIPLINE_MIN_DISTANCE && hRope.RopeType == ROPE_HEAD)
		{
			CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_TOOSHORT);
			return;
		}
		
		float flClientAbsVel[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", flClientAbsVel);
		if (flClientAbsVel[2] < -1200.0)
		{
			CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_TOOFAST);
			return;
		}
		
		
		if (!CheckPointAgainstPlayerHull(iClient, vecRopeStartPosition))
			TeleportEntity(iClient, vecRopeStartPosition, NULL_VECTOR, NULL_VECTOR);
		else
		{
			if (!CPFZiplineHandler.TryForward(iClient))
			{
				DebugOutput("CPFZiplineHandler::TryForward --- Dismounting");
				CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_TOOSHORT);
				return;
			}
			else
				DebugOutput("CPFZiplineHandler::TryForward --- Mounting");
		}

		if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
			UnscopeRifle(iClient);

		CPFZiplineHandler.Mount(iClient);
	}
	
	public static void Think(int iClient)
	{
		float vecClientOrigin[3], vecEndPoint[3], vecEndPosition[3], vecRopeVelocity[3], flScale;
		int iTicksElapsed;
		CPFZiplineHandler.StartThink(iClient, false, vecClientOrigin, vecEndPoint, vecRopeVelocity, flScale, iTicksElapsed);
		
		/**** START TRACE ****/
		TraceHullF hZipline = new TraceHullF(vecClientOrigin, vecEndPoint, view_as<float>({-24.0, -24.0, 0.0}), view_as<float>({24.0, 24.0, 65.0}), MASK_PLAYERSOLID, TraceRayNoPlayers, iClient);
		if (hZipline.Hit)
		{
			// Safety trace hit something
			hZipline.GetEndPosition(vecEndPosition);
			
			if ((CBaseSDKTrace.GetPointContents(vecEndPosition)) || IsValidEntity(hZipline.EntityIndex) || iTicksElapsed < 10)
			{
				DebugOutput("CPFZiplineHandler::Think --- %N, Point Contents: %d", iClient, CBaseSDKTrace.GetPointContents(vecEndPosition));
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);
				CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_END);
			}
			else
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);
		}
		
		if (!hZipline.Hit || (!CBaseSDKTrace.GetPointContents(vecEndPosition)))
		{
			TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);
		}
		
		delete hZipline;
		/**** END TRACE ****/
	}
	
	public static void RemoveRopeCooldown(const int iClient)
	{
		g_bZiplineCooldown[iClient] = false;
	}
	
	public static float ProcessNearestZipline(const int iClient, float vecClosestPoint[3])
	{
		float vecOrigin[3], vecFirstPoint[3], vecSecondPoint[3], vecClosestPointBuffer[3], flDist, flLowestDist;

		for (int j = 0; j < CPFRopeController.Total(); j++)
		{
			CPFRope hRope = view_as<CPFRope>(CPFRopeController.GetRope(j));
			if (hRope == null)
				break;
			
			ePFRopeType eType = hRope.Get(ROPE_TYPE);
			
			if (eType == RAIL_HEAD || eType == RAIL_SEGMENT || eType == RAIL_RADIAL)
				continue;
			
			GetEntPropVector(hRope.EntIndex, Prop_Send, "m_vecOrigin", vecFirstPoint);
			GetEntPropVector(hRope.KeyframeIndex, Prop_Send, "m_vecOrigin", vecSecondPoint);
			GetClientAbsOrigin(iClient, vecOrigin);
			
			flDist = ShortestDistanceToLine(vecOrigin, vecFirstPoint, vecSecondPoint, vecClosestPointBuffer);
			
			if (flLowestDist == 0.0 || flDist < flLowestDist)
			{
				flLowestDist = flDist;
				g_hNearestZipline[iClient] = hRope;
				vecClosestPoint = vecClosestPointBuffer;
				g_vecNearestZipline[iClient][0] = vecFirstPoint;
				g_vecNearestZipline[iClient][1] = vecSecondPoint;
				continue;
			}
		}

		return flLowestDist;
	}
	
	public static void OnGameFrame()
	{
		const float CLOSEST_POINT_DECREMENT = 15.0;
		const float ORIGIN_HEIGHT_ADJUSTMENT = 41.0;
		const float DISTANCE_CHECK_MARGIN = 256.0;
		const float DISTANCE_CHECK_MOUNT = 32.0;
		
		float vecOrigin[3], vecClosestPoint[3], flDist;
		
		if (!CPFRopeController.Total())
			return;
		
		for (int i = 1; i < 33; i++) // 33 used rather than MaxClients because this is run every frame and MaxClients adds an extra check
		{
			if (!IsValidClient(i) || !IsPlayerAlive(i) || GetClientButtons(i) & IN_DUCK || CPFStateController.Get(i) != State_None || g_bZiplineCooldown[i])
				continue;
			
			GetClientAbsOrigin(i, vecOrigin);
			
			int iTick = GetGameTickCount();
			if (((iTick % 5) == 0 && (i % 2) == 0) || ((iTick % 5) == 1 && (i % 2) == 1))
				flDist = CPFZiplineHandler.ProcessNearestZipline(i, vecClosestPoint);

			vecOrigin[2] += ORIGIN_HEIGHT_ADJUSTMENT;
			flDist = ShortestDistanceToLine(vecOrigin, g_vecNearestZipline[i][0], g_vecNearestZipline[i][1], vecClosestPoint);
			vecOrigin[2] -= ORIGIN_HEIGHT_ADJUSTMENT;

			if (flDist < DISTANCE_CHECK_MARGIN)
			{
				if (flDist < DISTANCE_CHECK_MOUNT)
					CPFZiplineHandler.MountZipline(i, g_hNearestZipline[i], _, vecClosestPoint);
				
				if (Zipline_IsPointInClientBBox(vecOrigin, vecClosestPoint))
					CPFZiplineHandler.MountZipline(i, g_hNearestZipline[i], _, vecClosestPoint);
				else
				{
					vecClosestPoint[2] -= CLOSEST_POINT_DECREMENT;
					if (Zipline_IsPointInClientBBox(vecOrigin, vecClosestPoint))
						CPFZiplineHandler.MountZipline(i, g_hNearestZipline[i], _, vecClosestPoint);
				}
			}
		}
	}
};

void MountNextSegment(const int iClient, const CPFRope hRope)
{
	CPFZiplineHandler.MountZipline(iClient, hRope);
}

void RopeNormal(const CPFRope hRope, float vecResult[3], float flScale = 0.0)
{
	//DebugOutput("RopeNormal --- hRope: %X", hRope);

	float vecRopeOrigin[3], vecKeyframeOrigin[3];
	int iMoveRope = EntRefToEntIndex(hRope.EntIndex);
	int iKeyframeRope = EntRefToEntIndex(hRope.KeyframeIndex);
	
	GetEntPropVector(iMoveRope, Prop_Send, "m_vecOrigin", vecRopeOrigin);
	GetEntPropVector(iKeyframeRope, Prop_Send, "m_vecOrigin", vecKeyframeOrigin);
	
	MakeVectorFromPoints(vecRopeOrigin, vecKeyframeOrigin, vecResult);
	NormalizeVector(vecResult, vecResult);
	if (flScale > 0.0)
		ScaleVector(vecResult, flScale);
}

Action Zipline_RemoveCooldown(Handle hTimer, any aData)
{
	int iClient = GetClientOfUserId(aData);
	DebugOutput("Zipline_RemoveCooldown --- %N", iClient);
	g_bZiplineCooldown[iClient] = false;

	return Plugin_Continue;
}

float ShortestDistanceToLine(float vecOrigin[3], float vecFirstPoint[3], float vecSecondPoint[3], 
							float vecClosestPoint[3] = NULL_VECTOR)
{
	float vecOriginFirst[3], vecSecondFirst[3], flLinesDotProduct, flLengthSquared, flDotProductLength = -1.0;
	MakeVectorFromPoints(vecFirstPoint, vecOrigin, vecOriginFirst);
	MakeVectorFromPoints(vecFirstPoint, vecSecondPoint, vecSecondFirst);
	
	flLinesDotProduct = GetVectorDotProduct(vecOriginFirst, vecSecondFirst);
	flLengthSquared = GetVectorLength(vecSecondFirst, true);
	
	if (flLengthSquared == 0.0)
		return GetVectorDistance(vecOrigin, vecFirstPoint);
	
	flDotProductLength = flLinesDotProduct / flLengthSquared;
	
	float vecCompare[3], vecCompareFromOrigin[3];
	if (flDotProductLength < 0)
		vecCompare = vecFirstPoint;
	else if (flDotProductLength > 1)
		vecCompare = vecSecondPoint;
	else
	{
		for (int i = 0; i < 3; i++)
		{
			vecCompare[i] = vecFirstPoint[i] + (flDotProductLength * vecSecondFirst[i]);
		}
	}
	
	vecClosestPoint = vecCompare;
	
	MakeVectorFromPoints(vecCompare, vecOrigin, vecCompareFromOrigin);
	return SquareRoot((vecCompareFromOrigin[0] * vecCompareFromOrigin[0]) + (vecCompareFromOrigin[1] * vecCompareFromOrigin[1]) + (vecCompareFromOrigin[2] * vecCompareFromOrigin[2]));
}

bool Zipline_IsPointInClientBBox(float vecOrigin[3], float vecPoint[3])
{
	const float PLAYER_ZIPLINE_HEIGHT_OFFSET = 41.0;
	
	float vecMins[3] = {-24.0, -24.0, 0.0};
	float vecMaxs[3] = {24.0, 24.0, 82.0};
	
	DrawVectorPoints(vecOrigin, vecPoint, 0.25, {100, 0, 255, 255});
	
	vecMaxs[2] += PLAYER_ZIPLINE_HEIGHT_OFFSET;
	
	return (((vecPoint[0] >= (vecOrigin[0] + vecMins[0])) && (vecPoint[0] <= (vecOrigin[0] + vecMaxs[0]))) &&
			((vecPoint[1] >= (vecOrigin[1] + vecMins[1])) && (vecPoint[1] <= (vecOrigin[1] + vecMaxs[1]))) &&
			((vecPoint[2] >= (vecOrigin[2] + vecMins[2])) && (vecPoint[2] <= (vecOrigin[2] + vecMaxs[2]))));
}

public void StopZiplineTwice(int iUserID)
{
	CPFSoundController.StopZipline(GetClientOfUserId(iUserID));
}
