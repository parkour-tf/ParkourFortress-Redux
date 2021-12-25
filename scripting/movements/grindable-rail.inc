#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_GRINDABLERAIL_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_GRINDABLERAIL_INCLUDED

enum eRailDisengageSource
{
	RAIL_DISENGAGE_INVALID = 0,
	RAIL_DISENGAGE_JUMP = 1,
	RAIL_DISENGAGE_JUMP_TRYLEFT,
	RAIL_DISENGAGE_JUMP_TRYRIGHT,
	RAIL_DISENGAGE_END,
	RAIL_DISENGAGE_ENDPOLARITY,
	RAIL_DISENGAGE_NOTROPE,
	RAIL_DISENGAGE_TOOSHORT,
	RAIL_DISENGAGE_TOOFAST,
	RAIL_DISENGAGE_NEXTSEGMENT,
	RAIL_DISENGAGE_INVALIDROTATION,
	RAIL_DISENGAGE_TELEPORT,

	RDS_COUNT
};

enum eRailSide
{
	RAILSIDE_NONE = 0,
	RAILSIDE_LEFT = 1,
	RAILSIDE_RIGHT = 2,

	RS_COUNT
};

// MAJOR TODO: Replace this garbage with an enum-struct.
static bool g_bRailCooldown[MAXPLAYERS + 1];
static bool g_bRailDirection[MAXPLAYERS + 1];
static CPFRail g_hClientRopes[MAXPLAYERS + 1];
static CPFRail g_hNearestRail[MAXPLAYERS + 1];
static int g_iRailStartTick[MAXPLAYERS + 1];
static float g_vecNearestRail[MAXPLAYERS + 1][2][3];
static float g_flCurrentSpeed[MAXPLAYERS + 1];
static float g_flCurrentTheta[MAXPLAYERS + 1];
static eRailSide g_eSide[MAXPLAYERS + 1];

static const float RAIL_COOLDOWN_TIME = 0.5;
static const float RAIL_MOUNT_Z_OFFSET = 0.0;
static const float RAIL_EYE_TILT = 15.0;
static const float RAIL_MIN_SPEED = 1.0;

// Value to add to the velocity scale while player is holding forward/backward.
static const float RAIL_SPEED_ADD_FORWARD = 2.5;
static const float RAIL_SPEED_ADD_IDLE = -1.0;
static const float RAIL_SPEED_ADD_BACKWARD = -5.0;

// Maximum speed for each button.
static const float RAIL_MAX_SPEED_FORWARD = 750.0;

methodmap CPFRailHandler
{
	public static void Mount(const int iClient)
	{
		CPFSpeedController.SetSpeed(iClient, 0.0);
		CPFStateController.Set(iClient, State_Rail);

		// Use isometric movetype for smoother camera movement while on rail.
		SetEntityMoveType(iClient, MOVETYPE_ISOMETRIC);
		SetEntityFlags(iClient, GetEntityFlags(iClient)|FL_ATCONTROLS);
	}
	
	public static void Dismount(const int iClient)
	{
		CPFStateController.Set(iClient, State_None);
		CPFSoundController.StopRailGrind(iClient);
		CPFSoundController.PlayRailEnd(iClient);
		CPFViewController.Unhide(iClient);

		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		SetEntityMoveType(iClient, MOVETYPE_WALK);
		SetCollisionGroup(iClient, COLLISION_GROUP_DEBRIS_TRIGGER);
	}
	
	/**
	 * CPFRailHandler::Get(const int, const CPFRail)
	 * Description: Set the client's current rope.
	 */
	public static void Set(const int iClient, const CPFRail hRail)
	{
		if (!IsValidClient(iClient))
			return;

		g_hClientRopes[iClient] = hRail;
		DebugOutput("CPFRailHandler::Set --- Set %N's rope to %d", iClient, ((hRail != null) ? hRail.RopeIndex : 0));
	}

	/**
	 * CPFRailHandler::Get(const int)
	 * Description: Get the client's current rope.
	 */
	public static CPFRail Get(const int iClient)
	{
		if (!IsValidClient(iClient))
			return CPFRAIL_INVALID;

		return g_hClientRopes[iClient];
	}

	/**
	 * CPFRailHandler::SetSide(int, const int)
	 * Description: Set the client's eye tilt.
	 */
	public static void SetSide(const int iClient, const eRailSide eSide)
	{
		if (GetCookieInt(g_cookieLerp, iClient))
			return;
		
		float vecAngles[3];
		GetClientEyeAngles(iClient, vecAngles);

		switch (eSide)
		{
			case RAILSIDE_LEFT:
			{
				vecAngles[2] = -RAIL_EYE_TILT;
				g_eSide[iClient] = RAILSIDE_LEFT;
			}
			case RAILSIDE_RIGHT:
			{
				vecAngles[2] = RAIL_EYE_TILT;
				g_eSide[iClient] = RAILSIDE_RIGHT;
			}
			default:
			{
				if (vecAngles[2] != 0.0)
				{
					vecAngles[2] = 0.0;
					g_eSide[iClient] = RAILSIDE_NONE;
				}
				else
					return;
			}
		}

		TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
	}

	/**
	 * CPFRailHandler::Disengage(const int, eRailDisengageSource, const float[3])
	 * Input condition:		Client should have met one of several dismounting conditions.
	 * Output condition:	Client should no longer be mounted to the current rail segment.
	 */
	public static void Disengage(const int iClient, eRailDisengageSource eCause, const float vecPersist[3] = ZERO_VECTOR)
	{
		const float RAIL_DISENGAGE_JUMP_CONSTANT = 50.0;
		const float RAIL_DISENGAGE_JUMP_SCALAR = 2.00;
		const float RAIL_DISENGAGE_JUMP_HEIGHT_CAP = 375.0;

		DebugOutput("CPFRailHandler::Disengage --- %N %d", iClient, view_as<int>(eCause));

		if (eCause == RAIL_DISENGAGE_NEXTSEGMENT)
		{
			CPFRail hRail = CPFRailHandler.Get(iClient);
			if (((g_bRailDirection[iClient]) ? hRail.NextSegment : hRail.PreviousSegment) != null)
			{
				Rail_MountNextSegment(iClient, ((g_bRailDirection[iClient]) ? hRail.NextSegment : hRail.PreviousSegment), vecPersist);
				return;
			}
		}

		CPFRailHandler.SetSide(iClient, RAILSIDE_NONE);
		CPFRailHandler.Set(iClient, CPFRAIL_INVALID);
		CPFRailHandler.Dismount(iClient);

		if (eCause == RAIL_DISENGAGE_TOOSHORT || eCause == RAIL_DISENGAGE_TOOFAST)
		{
			CPFSpeedController.RestoreSpeed(iClient);
			return;
		}

		SetEntProp(iClient, Prop_Send, "m_iAirDash", 1);

		float vecVelocity[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);

		if (eCause == RAIL_DISENGAGE_JUMP_TRYLEFT || eCause == RAIL_DISENGAGE_JUMP_TRYRIGHT)
		{
			float vecOrigin[3], vecAngles[3], vecForward[3], vecDirection[3], vecStart[3];

			GetClientAbsOrigin(iClient, vecOrigin);
			GetClientAbsAngles(iClient, vecAngles);
			// Get client right vector, store it in vecDirection
			GetAngleVectors(vecAngles, vecForward, vecDirection, NULL_VECTOR);

			// Depending on the client speed, send them more or less to the right/left
			float flDirectionAngle = (g_flCurrentSpeed[iClient] / RAIL_MAX_SPEED_FORWARD) * (FLOAT_PI/2);

			if (eCause == RAIL_DISENGAGE_JUMP_TRYLEFT)
			{
				for (int i = 0; i < 3; i++)
					vecDirection[i] = -vecDirection[i];

				flDirectionAngle = -flDirectionAngle;
			}
			
			AddVectors(vecDirection, vecOrigin, vecStart);
			// Rotate the direction within the quadrant to send the player in a diagonal direction
			vecDirection[0] = vecOrigin[0] + ((vecStart[0] - vecOrigin[0]) * Cosine(flDirectionAngle)) + ((vecOrigin[1] - vecStart[1]) * Sine(flDirectionAngle));
			vecDirection[1] = vecOrigin[1] + ((vecStart[1] - vecOrigin[1]) * Cosine(flDirectionAngle)) + ((vecStart[0] - vecOrigin[0]) * Sine(flDirectionAngle));
			
			SubtractVectors(vecDirection, vecOrigin, vecDirection);
			vecDirection[2] = 0.0;
			NormalizeVector(vecDirection, vecDirection);

			ScaleVector(vecDirection, (g_flCurrentSpeed[iClient] < RAIL_DISENGAGE_JUMP_CONSTANT) ? RAIL_DISENGAGE_JUMP_CONSTANT + (g_flCurrentSpeed[iClient] * RAIL_DISENGAGE_JUMP_SCALAR) : g_flCurrentSpeed[iClient]);
			DrawVector(vecOrigin, vecDirection, 5.0, {255, 255, 0, 255});
			vecVelocity[0] = vecDirection[0];
			vecVelocity[1] = vecDirection[1];
			vecVelocity[2] =  (g_flCurrentSpeed[iClient] > RAIL_DISENGAGE_JUMP_HEIGHT_CAP) ? RAIL_DISENGAGE_JUMP_HEIGHT_CAP : g_flCurrentSpeed[iClient];
			DebugOutput("CPFRailHandler::Disengage --- %N Hopping In Direction, Scale: %.2f", iClient, ((g_flCurrentSpeed[iClient] < RAIL_DISENGAGE_JUMP_CONSTANT) ? RAIL_DISENGAGE_JUMP_CONSTANT + (g_flCurrentSpeed[iClient] * RAIL_DISENGAGE_JUMP_SCALAR) : g_flCurrentSpeed[iClient]));
		}
		else if (eCause == RAIL_DISENGAGE_JUMP)
		{
			float vecForward[3];
			GetForwardVector(iClient, vecForward, (g_flCurrentSpeed[iClient] < RAIL_DISENGAGE_JUMP_CONSTANT) ? RAIL_DISENGAGE_JUMP_CONSTANT + (g_flCurrentSpeed[iClient] * RAIL_DISENGAGE_JUMP_SCALAR) : g_flCurrentSpeed[iClient]);
			vecVelocity[0] = vecForward[0];
			vecVelocity[1] = vecForward[1];
			vecVelocity[2] = (g_flCurrentSpeed[iClient] > RAIL_DISENGAGE_JUMP_HEIGHT_CAP) ? RAIL_DISENGAGE_JUMP_HEIGHT_CAP : g_flCurrentSpeed[iClient];
		}
		else
		{
			ScaleVector(vecVelocity, 0.75);
		}

		if (eCause != RAIL_DISENGAGE_TELEPORT)
			TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecVelocity);

		CPFSpeedController.SetStoredSpeed(iClient, SPEED_BASE + (g_flCurrentSpeed[iClient] / RAIL_MAX_SPEED_FORWARD) * (SPEED_MAX_BOOST - SPEED_BASE));

		g_flCurrentSpeed[iClient] = 0.0;
		g_flCurrentTheta[iClient] = 0.0;

		g_bRailCooldown[iClient] = true;
		CreateTimer(RAIL_COOLDOWN_TIME, Rail_RemoveCooldown, GetClientUserId(iClient));

		if (eCause != RAIL_DISENGAGE_TELEPORT)
			CPFSpeedController.RestoreSpeed(iClient);
	}

	/**
	 * CPFRailHandler::StartThink(const int, const bool, float[3], float[3], float[3], &int)
	 * Input condition:		Client should be mounted (or mounting?) a rail.
	 * Output condition:	Values should be set up for the client to continue along the rail.
	 * Description:			Using the player's buttons, create a speed value and calculate the next velocity
	 *						for the client. If the client's next velocity would land on the next segment, dismount
	 *						current segment and mount the next with a persisting velocity. If there is no next
	 *						segment, dismount the client entirely.
	 */
	public static void StartThink(const int iClient, const bool bPolarityBypass, float vecClientOrigin[3], float vecEndPoint[3], float vecRopeVelocity[3], int &iTicksElapsed = 0)
	{
		float vecRailNormal[3], vecRopeVelScaled[3], vecKeyframeOrigin[3], vecPlayerToEnd[3], vecCenter[3], vecStart[3], vecEnd[3];

		CPFRail hRail = CPFRailHandler.Get(iClient);
		GetClientAbsOrigin(iClient, vecClientOrigin);

		iTicksElapsed = GetGameTickCount() - g_iRailStartTick[iClient];

		int iButtons = GetClientButtons(iClient);

		if (iButtons & IN_FORWARD && !(iButtons & IN_BACK) && !(iButtons & IN_JUMP))
		{
			
			if (g_flCurrentSpeed[iClient] < RAIL_MAX_SPEED_FORWARD)
			{
				if (g_flCurrentSpeed[iClient] == RAIL_MIN_SPEED)
					CPFSoundController.PlayRailGrind(iClient);
				
				g_flCurrentSpeed[iClient] += RAIL_SPEED_ADD_FORWARD;
				
				if (g_flCurrentSpeed[iClient] >= RAIL_MAX_SPEED_FORWARD)
				{
					g_flCurrentSpeed[iClient] = RAIL_MAX_SPEED_FORWARD;
					CPFSoundController.PlayRailGrind(iClient, SNDPITCH_LOW);
				}
				
				DebugOutput("CPFRailController::StartThink --- %N: Adding speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
			else
			{
				g_flCurrentSpeed[iClient] = RAIL_MAX_SPEED_FORWARD;
				DebugOutput("CPFRailController::StartThink --- %N: Max Capping Speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
		}
		else if (iButtons & IN_BACK && !(iButtons & IN_FORWARD) && !(iButtons & IN_JUMP))
		{
			if (g_flCurrentSpeed[iClient] > RAIL_MIN_SPEED)
			{
				if (g_flCurrentSpeed[iClient] == RAIL_MAX_SPEED_FORWARD)
					CPFSoundController.PlayRailGrind(iClient);
				
				g_flCurrentSpeed[iClient] += RAIL_SPEED_ADD_BACKWARD;
				
				if (g_flCurrentSpeed[iClient] <= RAIL_MIN_SPEED)
				{
					g_flCurrentSpeed[iClient] = RAIL_MIN_SPEED;
					CPFSoundController.StopRailGrind(iClient);
				}
				DebugOutput("CPFRailController::StartThink --- %N: Subtracting speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
			else
			{
				g_flCurrentSpeed[iClient] = RAIL_MIN_SPEED;
				DebugOutput("CPFRailController::StartThink --- %N: Min Capping speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
		}
		else if (!(iButtons & IN_FORWARD))
		{
			if (g_flCurrentSpeed[iClient] > RAIL_MIN_SPEED)
			{
				if (g_flCurrentSpeed[iClient] == RAIL_MAX_SPEED_FORWARD)
					CPFSoundController.PlayRailGrind(iClient);
					
				g_flCurrentSpeed[iClient] += RAIL_SPEED_ADD_IDLE;
				
				if (g_flCurrentSpeed[iClient] <= RAIL_MIN_SPEED)
				{
					g_flCurrentSpeed[iClient] = RAIL_MIN_SPEED;
					CPFSoundController.StopRailGrind(iClient);
				}
				DebugOutput("CPFRailController::StartThink --- %N: Subtracting idle speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
			else
			{
				g_flCurrentSpeed[iClient] = RAIL_MIN_SPEED;
				DebugOutput("CPFRailController::StartThink --- %N: Min Capping speed, %.2f", iClient, g_flCurrentSpeed[iClient]);
			}
		}

		if (hRail.RopeType != RAIL_RADIAL)
		{
			// Linear rails:
			// Start off with our rope's unit vector
			RailNormal(hRail, vecRailNormal, _, !g_bRailDirection[iClient]);

			// If the client is going in reverse, reverse the unit vector.
			/*if (!g_bRailDirection[iClient])
			{
				for (int i = 0; i < 3; i++)
					vecRailNormal[i] = -vecRailNormal[i];
			}*/

			// Scale the unit vector based off of our calculated speed.
			vecRopeVelocity = vecRailNormal;
			ScaleVector(vecRopeVelocity, g_flCurrentSpeed[iClient]);

			// Account for tickrate to find our end position.
			vecRopeVelScaled = vecRopeVelocity;
			ScaleVector(vecRopeVelScaled, 1.0 / GetTickRate());
			AddVectors(vecRopeVelScaled, vecClientOrigin, vecEndPoint);
			
			float vecOriginDebug[3], vecEndPointDebug[3];
			vecOriginDebug = vecClientOrigin;
			vecOriginDebug[2] += 84.0;
			vecEndPointDebug = vecEndPoint;
			vecEndPointDebug[2] += 84.0;
			DrawVectorPoints(vecOriginDebug, vecEndPointDebug, 5.0, {255, 0, 255, 255});
		}
		else
		{
			// Circular rails:
			// Set up our values based on whether the client is going backwards or not.
			GetEntPropVector(hRail.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecCenter);
			GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", (g_bRailDirection[iClient]) ? vecStart : vecEnd);
			GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", (g_bRailDirection[iClient]) ? vecEnd : vecStart);

			// Apply the Law of Cosines to get the next degree along the arc that we should be at.
			float r = GetVectorDistance(vecCenter, vecStart), theta = RadToDeg(ArcCosine((2.0 * Pow(r, 2.0) - Pow(g_flCurrentSpeed[iClient] / GetTickRate(), 2.0)) / (2.0 * Pow(r, 2.0))));

			g_flCurrentTheta[iClient] = (g_bRailDirection[iClient]) ? g_flCurrentTheta[iClient] + theta : g_flCurrentTheta[iClient] - theta;

			// With our theta, calculate the next point along the arc.
			GetPointAtDegree(hRail, g_flCurrentTheta[iClient], vecEndPoint);
			SubtractVectors(vecEndPoint, vecClientOrigin, vecRopeVelocity);
			
			// Scale by the tickrate to get our velocity.
			ScaleVector(vecRopeVelocity, GetTickRate());
		}

		// Get a unit vector from the player's position to the end of the current segment.
		// TODO: Better variable naming here.
		GetEntPropVector(EntRefToEntIndex((g_bRailDirection[iClient]) ? hRail.KeyframeIndex : hRail.EntIndex), Prop_Data, "m_vecAbsOrigin", vecKeyframeOrigin);
		MakeVectorFromPoints(vecClientOrigin, vecKeyframeOrigin, vecPlayerToEnd);
		NormalizeVector(vecPlayerToEnd, vecPlayerToEnd);

		// TODO: Find out if polarity bypass is necessary now that we're checking the line segment
		if (!bPolarityBypass && !CheckPointAgainstPlayerHull(iClient, vecEndPoint))
		{
			float vecCurrentSegmentStart[3], vecCurrentSegmentEnd[3];
			GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", (g_bRailDirection[iClient]) ? vecCurrentSegmentStart : vecCurrentSegmentEnd);
			GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", (g_bRailDirection[iClient]) ? vecCurrentSegmentEnd : vecCurrentSegmentStart);

			// Check if our next movement would put us outside the current segment
			if ((!Rail_IsPointInLineSegment(vecCurrentSegmentStart, vecCurrentSegmentEnd, vecEndPoint) && hRail.RopeType != RAIL_RADIAL) ||
				((hRail.RopeType == RAIL_RADIAL) && ((g_flCurrentTheta[iClient] > 90.0 && g_bRailDirection[iClient]) || (g_flCurrentTheta[iClient] < 0.0 && !g_bRailDirection[iClient]))))
			{				
				// Dismount if there's no next segment
				if (hRail.NextSegment == null && g_bRailDirection[iClient] || hRail.PreviousSegment == null && !g_bRailDirection[iClient])
				{
					CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_ENDPOLARITY);
				}
				else
				{
					CPFRail hNext = ((g_bRailDirection[iClient]) ? hRail.NextSegment : hRail.PreviousSegment);
					float vecPersist[3];

					if (hNext.RopeType != RAIL_RADIAL)
					{
						float vecNormal[3], flDist;

						if (hRail.RopeType != RAIL_RADIAL)
						{
							// Going from a linear rail to a linear rail
							flDist = GetVectorDistance(vecCurrentSegmentEnd, vecEndPoint);
						}
						else
						{
							// Going from a radial rail to a linear rail
							g_flCurrentTheta[iClient] = (g_bRailDirection[iClient]) ? g_flCurrentTheta[iClient] - 90.0 : FloatAbs(g_flCurrentTheta[iClient]);
							if (g_flCurrentTheta[iClient] < 0.0)
								g_flCurrentTheta[iClient] = 0.0;

							float r = GetVectorDistance(vecCenter, vecStart);

							// Reverse the law of cosines to get a distance from our spare theta
							flDist = (SquareRoot(-2.0 * Pow(r, 2.0) * Cosine(g_flCurrentTheta[iClient]) + (2.0 * Pow(r, 2.0)))) / GetTickRate();
						}
						// Use our spare velocity from the end of the segment to our endpoint to calculate the next start position
						RailNormal(hNext, vecNormal, flDist, !g_bRailDirection[iClient]);
						AddVectors(vecCurrentSegmentEnd, vecNormal, vecPersist);
					}
					else
					{
						// TODO: We need another case here for moving from a linear segment to a radial segment.
						// Use our leftover theta value to calculate the next start position.

						if (hRail.RopeType == RAIL_RADIAL)
						{
							// Going from a radial rail to a radial rail
							g_flCurrentTheta[iClient] = (g_bRailDirection[iClient]) ? g_flCurrentTheta[iClient] - 90.0 : g_flCurrentTheta[iClient] + 90.0;
							if (g_flCurrentTheta[iClient] < 0.0)
								g_flCurrentTheta[iClient] = 0.0;
						}
						else
						{
							// Going from a linear rail to a radial rail
							float vecNextCenter[3], r, theta;
							GetEntPropVector(hNext.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecNextCenter);

							// These are the same calculations used to get position earlier in the function.
							r = GetVectorDistance(vecCenter, vecStart);
							theta = RadToDeg(ArcCosine((2.0 * Pow(r, 2.0) - Pow(float(PointDistance(vecCurrentSegmentEnd, vecEndPoint)) / GetTickRate(), 2.0)) / (2.0 * Pow(r, 2.0))));
							g_flCurrentTheta[iClient] = (g_bRailDirection[iClient]) ? theta : 90.0 - theta;
						}

						// Write the result into vecPersist so that Mount can read it.
						GetPointAtDegree(hNext, g_flCurrentTheta[iClient], vecPersist);
					}

					// Move on to the next segment.
					CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_NEXTSEGMENT, vecPersist);
				}
			}
		}
	}

	/**
	 * CPFRailHandler::Think(int)
	 * Input condition:		Client should be mounted to a rail.
	 * Output condition:	Client should be riding the rail.
	 * Description:			After all the values are set up, run collision checks and teleport the player.
	 */
	public static void Think(int iClient)
	{
		float vecClientOrigin[3], vecEndPoint[3], vecEndPosition[3], vecRopeVelocity[3];
		int iTicksElapsed;
		CPFRailHandler.StartThink(iClient, false, vecClientOrigin, vecEndPoint, vecRopeVelocity, iTicksElapsed);

		/**** START TRACE ****/
		TraceHullF hRail = new TraceHullF(vecClientOrigin, vecEndPoint, view_as<float>({-24.0, -24.0, 0.0}), view_as<float>({24.0, 24.0, 65.0}), MASK_PLAYERSOLID, TraceRayNoPlayers);
		if (hRail.Hit)
		{
			hRail.GetEndPosition(vecEndPosition);

#if defined DEBUG
			CreateSprite(vecEndPosition);
#endif

			if ((CBaseSDKTrace.GetPointContents(vecEndPosition)) || !IsValidEntity(hRail.EntityIndex) /*|| (iTicksElapsed < 10 && (CPFRailHandler.Get(iClient)).NextSegment == null)*/)
			{
				DebugOutput("CPFRailHandler::Think --- %N, Point Contents: %d", iClient, CBaseSDKTrace.GetPointContents(vecEndPosition));
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);
				CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_END);
			}
			else
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);
		}

		if (!hRail.Hit || (!CBaseSDKTrace.GetPointContents(vecEndPosition)))
			TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecRopeVelocity);

		delete hRail;
		/**** END TRACE ****/
	}

	/**
	 * CPFRailHandler::MountRail(const int, const CPFRail, const bool, const float[3])
	 * Input condition:		Client should be valid, and should be within the bounds of the rail's segment.
	 *					If bContinued is true, preserve some values from last segment.
	 * Output condition:	Client should be mounted on top of rail segment with their state set. The main
	 *					controller will handle it from there.
	 */
	public static void MountRail(const int iClient, const CPFRail hRail, const bool bContinued = false, const float vecPersist[3] = ZERO_VECTOR)
	{
		const float RAIL_MIN_DISTANCE = 1.0;

		DebugOutput("CPFRailHandler::Mount --- Called");

		CPFRailHandler.Set(iClient, hRail);

		if (!bContinued)
		{
			g_iRailStartTick[iClient] = GetGameTickCount();
			CPFSoundController.PlayRailGrind(iClient);
			CPFSoundController.PlayRailMount(iClient);
			CPFViewController.Hide(iClient);
		}

		int iMoveRope = EntRefToEntIndex(hRail.EntIndex);
		if (!IsValidEntity(iMoveRope))
		{
			CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_NOTROPE);
			return;
		}

		if (hRail.RopeType == RAIL_RADIAL && hRail.Direction == Rot_Invalid)
		{
			CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_INVALIDROTATION);
			return;
		}

		float vecOrigin[3], vecRopeOrigin[3], vecRopeStartingPosition[3], vecKeyframeOrigin[3], flDistance;
		GetClientAbsOrigin(iClient, vecOrigin);
		GetEntPropVector(iMoveRope, Prop_Data, "m_vecAbsOrigin", vecRopeOrigin);
		GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecKeyframeOrigin);

		float vecForward[3], vecToKeyframe[3], flAngle;
		GetForwardVector(iClient, vecForward);
		MakeVectorFromPoints(vecOrigin, vecKeyframeOrigin, vecToKeyframe);

		if (!bContinued)
		{
			// This determines if the player is mounting backwards or not
			if (!hRail.ForwardMountOnly)
			{
				flAngle = RadToDeg(ArcCosine(GetVectorDotProduct(vecForward, vecToKeyframe) / (FloatAbs(GetVectorLength(vecForward)) * FloatAbs(GetVectorLength(vecToKeyframe)))));
				g_bRailDirection[iClient] = !(flAngle >= 90.0);
			}
			else
				g_bRailDirection[iClient] = true;

			/* Optimization: Use DistanceAtDegree's value as our starting position. If it's
			 *				empty or not suitable, recalculate.
			 */
			if ((vecPersist[0] == 0.0 && vecPersist[1] == 0.0 && vecPersist[2] == 0.0) || CheckPointAgainstPlayerHull(iClient, vecPersist))
			{
				// Client's distance from the start of the rope
				flDistance = GetVectorDistance(vecOrigin, vecRopeOrigin);

				// Get our scaled normal and teleport the player to their rail starting position
				RailNormal(hRail, vecRopeStartingPosition, flDistance, !g_bRailDirection[iClient]);

				AddVectors(vecRopeOrigin, vecRopeStartingPosition, vecRopeStartingPosition);
			}
			else
				vecRopeStartingPosition = vecPersist;

			vecRopeStartingPosition[2] += RAIL_MOUNT_Z_OFFSET;

			if (GetVectorDistance(vecOrigin, (g_bRailDirection[iClient]) ? vecRopeOrigin : vecKeyframeOrigin) < RAIL_MIN_DISTANCE)
			{
				CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_TOOSHORT);
				return;
			}

			float flClientAbsVel[3];
			GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", flClientAbsVel);
			if (flClientAbsVel[2] < -1200.0)
			{
				CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_TOOFAST);
				return;
			}

			if (hRail.RopeType == RAIL_RADIAL)
				CalculatePlayerTheta(iClient, hRail);

			if (!CheckPointAgainstPlayerHull(iClient, vecRopeStartingPosition))
				TeleportEntity(iClient, vecRopeStartingPosition, NULL_VECTOR, NULL_VECTOR);

			g_flCurrentSpeed[iClient] = ClientHorizontalSpeed(iClient);
		}
		else
		{
			float vecEyeAngles[3];
			GetClientEyeAngles(iClient, vecEyeAngles);

#if defined RAIL_CHANGE_PLAYER_ANGLES
			// This code is only if we want to rotate the player's view angles as they move from segment to segment
			float float vecLastRopeOrigin[3], vecLastRope[3], flDelta;
			CPFRail hLastRope = ((!g_bRailDirection[iClient]) ? hRail.NextSegment : hRail.PreviousSegment);
			GetEntPropVector(((!g_bRailDirection[iClient]) ? (hLastRope.KeyframeIndex) : (hLastRope.EntIndex)), Prop_Data, "m_vecAbsOrigin", vecLastRopeOrigin);
			MakeVectorFromPoints(vecOrigin, vecLastRopeOrigin, vecLastRope);

			flAngle = (RadToDeg(ArcCosine(GetVectorDotProduct(vecLastRope, vecToKeyframe) / (FloatAbs(GetVectorLength(vecLastRope)) * FloatAbs(GetVectorLength(vecToKeyframe))))));
			flDelta = (vecLastRope[0] * vecToKeyframe[1]) - (vecLastRope[1] * vecToKeyframe[0]);
			vecEyeAngles[1] += (flDelta >= 0.0) ? flAngle : -flAngle;
#endif

			// If we're continuing, trust the vecPersist and use it as a starting point.
			CPFRail hLast = (g_bRailDirection[iClient]) ? hRail.PreviousSegment : hRail.NextSegment;
			if (hLast.RopeType != RAIL_RADIAL)
				g_flCurrentTheta[iClient] = (g_bRailDirection[iClient]) ? 0.0 : 90.0; // TODO: Calculations for proper starting point based on velocity

			if (!CheckPointAgainstPlayerHull(iClient, vecPersist))
				TeleportEntity(iClient, vecPersist, vecEyeAngles, NULL_VECTOR);
		}

		// At this point we're good to go.
		CPFRailHandler.Mount(iClient);
	}

	/**
	 * CPFRailHandler::RemoveRopeCooldown(int)
	 * Input condition:		x amount of time has passed since client has dismounted.
	 * Output condition:	Client will no longer be on cooldown.
	 */
	public static void RemoveRopeCooldown(int iClient)
	{
		g_bRailCooldown[iClient] = false;
	}

	/**
	 * CPFRailHandler::ProcessNearestRail(int, float[3])
	 * Input condition:		Client should not be on a rail or zipline and should not be locked or noclipping.
	 * Output condition:	The closest point on a rail will be written to vecClosestPoint, and the
	 *					distance to it will be returned.
	 */
	public static float ProcessNearestRail(int iClient, float vecClosestPoint[3])
	{
		float vecOrigin[3], vecFirstPoint[3], vecSecondPoint[3], vecClosestPointBuffer[3], flDist, flLowestDist, theta;

		for (int j = 0; j < CPFRopeController.Total(true); j++)
		{
			CPFRail hRail = view_as<CPFRail>(CPFRopeController.GetRope(j, true));
			if (hRail == null)
				break;

			ePFRopeType eType = hRail.Get(ROPE_TYPE);

			if (eType == ROPE_HEAD || eType == ROPE_SEGMENT)
				continue;

			GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", vecFirstPoint);
			GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecSecondPoint);
			GetClientAbsOrigin(iClient, vecOrigin);

			if (eType == RAIL_RADIAL)
			{
				float vecCenter[3];
				GetEntPropVector(hRail.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecCenter);
				flDist = ShortestDistanceToCurve(hRail.Direction, vecOrigin, vecFirstPoint, vecSecondPoint, vecCenter, vecClosestPointBuffer, theta);
			}
			else
				flDist = ShortestDistanceToLine(vecOrigin, vecFirstPoint, vecSecondPoint, vecClosestPointBuffer);

			if (flLowestDist == 0.0 || flDist < flLowestDist)
			{
				if (eType == RAIL_RADIAL)
					g_flCurrentTheta[iClient] = theta;

				flLowestDist = flDist;
				g_hNearestRail[iClient] = hRail;
				vecClosestPoint = vecClosestPointBuffer;
				g_vecNearestRail[iClient][0] = vecFirstPoint;
				g_vecNearestRail[iClient][1] = vecSecondPoint;
				continue;
			}
		}

		return flLowestDist;
	}

	/**
	 * CPFRailHandler::OnGameFrame()
	 * Input: Runs on every frame.
	 * Output: Players who meet the criteria for it will be mounted to rails.
	 */
	public static CPFRail OnGameFrame()
	{
		const float CLOSEST_POINT_DECREMENT = 15.0;
		const float ORIGIN_HEIGHT_ADJUSTMENT = 41.0;
		const float DISTANCE_CHECK_MARGIN = 256.0;
		const float DISTANCE_CHECK_MOUNT = 32.0;

		float vecOrigin[3], vecClosestPoint[3], flDist;

		if (!CPFRopeController.Total(true))
			return;

		for (int i = 1; i < 33; i++) // 33 used rather than MaxClients because this is run every frame and MaxClients adds an extra check
		{
			if (!IsValidClient(i) || !IsPlayerAlive(i) || GetClientButtons(i) & IN_JUMP || CPFStateController.Get(i) != State_None || g_bRailCooldown[i])
				continue;

			GetClientAbsOrigin(i, vecOrigin);

			flDist = CPFRailHandler.ProcessNearestRail(i, vecClosestPoint); // TODO: We change flDist anyways; it's probably pointless to set it here

			vecOrigin[2] += ORIGIN_HEIGHT_ADJUSTMENT; // TODO: Fiddle with this value for better detection.
			if (g_hNearestRail[i].RopeType == RAIL_RADIAL)
			{
				float vecCenter[3];
				GetEntPropVector(EntRefToEntIndex(g_hNearestRail[i].RotatorIndex), Prop_Data, "m_vecAbsOrigin", vecCenter);
				flDist = ShortestDistanceToCurve(g_hNearestRail[i].Direction, vecOrigin, g_vecNearestRail[i][0], g_vecNearestRail[i][1], vecCenter, vecClosestPoint);
			}
			else
				flDist = ShortestDistanceToLine(vecOrigin, g_vecNearestRail[i][0], g_vecNearestRail[i][1], vecClosestPoint);

			vecOrigin[2] -= ORIGIN_HEIGHT_ADJUSTMENT;

			// Use two different margins, a wide margin and a thin margin. If only the wide one is hit, try decrementing the closest point.
			if (flDist < DISTANCE_CHECK_MARGIN)
			{
				if (flDist < DISTANCE_CHECK_MOUNT || Rail_IsPointInClientBBox(vecOrigin, vecClosestPoint))
					CPFRailHandler.MountRail(i, g_hNearestRail[i], false, vecClosestPoint);
				else
				{
					vecClosestPoint[2] -= CLOSEST_POINT_DECREMENT;
					if (Rail_IsPointInClientBBox(vecOrigin, vecClosestPoint))
						CPFRailHandler.MountRail(i, g_hNearestRail[i], false, vecClosestPoint);
				}

			}
		}
	}
}

/**
 * Rail_IsPointInClientBBox(float[3], float[3])
 * Description: Return whether a point in space is within a client standing bounding
 *				box at an origin point.
 */
bool Rail_IsPointInClientBBox(float vecOrigin[3], float vecPoint[3])
{
	const float PLAYER_RAIL_HEIGHT_OFFSET = 41.0;

	float vecMins[3] = {-24.0, -24.0, 0.0};
	float vecMaxs[3] = {24.0, 24.0, 82.0};

	//DrawVectorPoints(vecOrigin, vecPoint, 0.25, {100, 0, 255, 255});

	vecMaxs[2] -= PLAYER_RAIL_HEIGHT_OFFSET;

	return (((vecPoint[0] >= (vecOrigin[0] + vecMins[0])) && (vecPoint[0] <= (vecOrigin[0] + vecMaxs[0]))) &&
			((vecPoint[1] >= (vecOrigin[1] + vecMins[1])) && (vecPoint[1] <= (vecOrigin[1] + vecMaxs[1]))) &&
			((vecPoint[2] >= (vecOrigin[2] + vecMins[2])) && (vecPoint[2] <= (vecOrigin[2] + vecMaxs[2]))));
}

/**
 * Rail_RemoveCooldown(Handle, any)
 * Description: Pass from timer to CPFRailHandler::RemoveRopeCooldown.
 */
public Action Rail_RemoveCooldown(Handle hTimer, any aData)
{
	CPFRailHandler.RemoveRopeCooldown(GetClientOfUserId(aData));
}

/**
 * Rail_MountNextSegment(const int, const CPFRail, const float[3])
 * Description: Workaround for sourcepawn's lack of prototyping.
 */
void Rail_MountNextSegment(const int iClient, const CPFRail hRail, const float vecPersist[3])
{
	DebugOutput("Rail_MountNextSegment");
	CPFRailHandler.Set(iClient, hRail);
	CPFRailHandler.MountRail(iClient, hRail, true, vecPersist);
}

/**
 * PointDistance(const float[3], const float[3])
 * Description: I don't know why I don't just use GetVectorDistance here. TODO: Remember why.
 */
int PointDistance(const float vecA[3], const float vecB[3])
{
	float flSum;
	for (int i = 0; i < 2; i++)
		flSum += Pow(vecA[i] - vecB[i], 2.0);

	return RoundToFloor(SquareRoot(flSum));
}

/**
 * Rail_IsPointInLineSegment(const float[3], const float[3], const float[3])
 * Description: Return whether a given point, vecC, is within the line segment from vecA to vecB.
 */
bool Rail_IsPointInLineSegment(const float vecA[3], const float vecB[3], const float vecC[3])
{
	int iDistAB = PointDistance(vecA, vecB), iDistAC = PointDistance(vecA, vecC), iDistCB = PointDistance(vecC, vecB);
	return ((iDistAC + iDistCB) == iDistAB || (iDistAC + iDistCB) == (iDistAB + 1) || (iDistAC + iDistCB) == (iDistAB - 1));
}

/**
 * Rail_IsPointInLinearRail(const CPFRail, const float[3])
 * Description: Set up values for Rail_IsPointInLineSegment and use it.
 */
stock bool Rail_IsPointInLinearRail(const CPFRail hRail, const float vecC[3])
{
	if (hRail == CPFROPE_INVALID)
		return false;

	float vecA[3], vecB[3];
	GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", vecA);
	GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecB);
	
	return Rail_IsPointInLineSegment(vecA, vecB, vecC);
}

/**
 * Rail_IsPointInArcSegment(const CPFRail, float[3], const float)
 * Description: Return whether a given point, vecPoint, is within the arc at theta.
 *					Currently unused.
 */
stock bool Rail_IsPointInArcSegment(const CPFRail hRail, float vecPoint[3], const float theta)
{
	float vecStart[3], vecEnd[3], vecCenter[3], r_c;
	GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", vecStart);
	GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecEnd);
	if (hRail.RotatorIndex != -1)
		GetEntPropVector(hRail.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecCenter);
	else
		return false;

	vecPoint[2] = vecCenter[2];
	r_c = GetVectorDistance(vecCenter, vecStart);

	/* Convert to Polar Coordinates */
	const float epsilon = 5.0;
	float r = GetVectorDistance(vecCenter, vecPoint);
	// TODO: Account for off by one issues with RoundToFloor

	return (theta > 0 && theta < 90) && (r == r_c || (r < (r_c + epsilon) && r > (r_c - epsilon)));
}

/**
 * DistanceAtDegree(const ePFRotDirection, const float[3], const float[3], const float[3], const float[3], float[3], float)
 * Description: Given a degree value theta, rotate 90 degrees on an arc starting at
 *				vecStart with a center at vecCenter and write the result to vecRotated.
 *				The height should be an average depending on the degree from vecStart,
 *				to the ending point at vecEnd. Return the distance from the player
 *				origin to the calculated point.
 */
float DistanceAtDegree(const ePFRotDirection eDirection, const float vecOrigin[3], const float vecStart[3], const float vecEnd[3], const float vecCenter[3], float vecRotated[3], float theta)
{
	float phi;
	theta = DegToRad(theta);
	phi = (eDirection == Rot_CCW) ? theta : (FLOAT_PI - theta) + FLOAT_PI;

	vecRotated[0] = vecCenter[0] + ((vecStart[0] - vecCenter[0]) * Cosine(phi)) + ((vecCenter[1] - vecStart[1]) * Sine(phi));
	vecRotated[1] = vecCenter[1] + ((vecStart[1] - vecCenter[1]) * Cosine(phi)) + ((vecStart[0] - vecCenter[0]) * Sine(phi));
	vecRotated[2] = vecCenter[2] + ((vecEnd[2] - vecCenter[2]) * (2.0*theta)/FLOAT_PI);

	return GetVectorDistance(vecOrigin, vecRotated);
}

/**
 * ShortestDistanceToCurve(const ePFRotDirection, const float[3], const float[3], const float[3], const float[3], float[3], &float)
 * Description: Implement a binary search algorithm to find the nearest point on an arc,
 *				the distance to that point, and the degree on the arc at which it was
 *				found. Can have variable accuracy depending on epsilon.
 */
float ShortestDistanceToCurve(const ePFRotDirection eDirection, const float vecOrigin[3], const float vecStart[3], const float vecEnd[3], const float vecCenter[3], float vecClosestPointBuffer[3], float &thetaWrite = 0.0)
{
	const float epsilon = 1.0;
	float lower = 0.0, upper = 89.0, theta = 0.0;

	while (lower <= upper)
	{
		float middle = lower + (upper - lower) / 2;
		theta = lower + (upper - lower) / 2;

		float ldist = DistanceAtDegree(eDirection, vecOrigin, vecStart, vecEnd, vecCenter, vecClosestPointBuffer, lower);
		float udist = DistanceAtDegree(eDirection, vecOrigin, vecStart, vecEnd, vecCenter, vecClosestPointBuffer, upper);

		if (ldist > udist)
			lower = middle + epsilon;
		else
			upper = middle - epsilon;
	}

	thetaWrite = theta;
	return DistanceAtDegree(eDirection, vecOrigin, vecStart, vecEnd, vecCenter, vecClosestPointBuffer, theta);
}

/**
 * GetPointAtDegree(const CPFRail, float, float[3])
 * Description: Given a CPFRail, run DistanceAtDegree to get a point at a given
 *				degree theta.
 */
void GetPointAtDegree(const CPFRail hRail, float theta, float vecResult[3])
{
	float vecStart[3], vecEnd[3], vecCenter[3];
	GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", vecStart);
	GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecEnd);
	GetEntPropVector(hRail.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecCenter);

	DistanceAtDegree(hRail.Direction, ZERO_VECTOR, vecStart, vecEnd, vecCenter, vecResult, theta);
	vecResult[2] += RAIL_MOUNT_Z_OFFSET;
}

/**
 * CalculatePlayerTheta(const int, const CPFRail)
 * Description: Force a player theta to prevent a bug with mounting at the exact frame
 * 				that the cooldown ends.
 */
void CalculatePlayerTheta(const int iClient, const CPFRail hRail)
{
	ePFRopeType eType = hRail.RopeType;
	if (eType != RAIL_RADIAL)
		return;

	float vecOrigin[3], vecStart[3], vecEnd[3], vecCenter[3], vecClosestPointBuffer[3], theta;
	GetClientAbsOrigin(iClient, vecOrigin);
	GetEntPropVector(hRail.EntIndex, Prop_Data, "m_vecAbsOrigin", vecStart);
	GetEntPropVector(hRail.KeyframeIndex, Prop_Data, "m_vecAbsOrigin", vecEnd);
	GetEntPropVector(hRail.RotatorIndex, Prop_Data, "m_vecAbsOrigin", vecCenter);

	ShortestDistanceToCurve(hRail.Direction, vecOrigin, vecStart, vecEnd, vecCenter, vecClosestPointBuffer, theta);
	g_flCurrentTheta[iClient] = theta;
}

void RailNormal(CPFRail hRail, float vecResult[3], float flScale = 0.0, bool bReverse = false)
{
	float vecRailOrigin[3], vecKeyframeOrigin[3];
	GetEntPropVector(EntRefToEntIndex(hRail.EntIndex), Prop_Send, "m_vecOrigin", vecRailOrigin);
	GetEntPropVector(EntRefToEntIndex(hRail.KeyframeIndex), Prop_Send, "m_vecOrigin", vecKeyframeOrigin);
	if (!bReverse)
		MakeVectorFromPoints(vecRailOrigin, vecKeyframeOrigin, vecResult);
	else
		MakeVectorFromPoints(vecKeyframeOrigin, vecRailOrigin, vecResult);
	
	NormalizeVector(vecResult, vecResult);
	if (flScale > 0.0)
		ScaleVector(vecResult, flScale);
}