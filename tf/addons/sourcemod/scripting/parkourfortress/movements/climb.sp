#if defined DONOTDEFINE
	#include <sourcemod>
	#include <tf2>
	#include <tf2_stocks>
	#include <cookies>
	#include <tracerayex>
	#include <parkourfortress>
#endif

#if defined _MOVEMENTS_CLIMB_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_CLIMB_INCLUDED

stock const float CLIMB_BBOX_MAXS[3] = {1.0, 1.0, 0.0};
stock const float CLIMB_BBOX_MINS[3] = {-1.0, -1.0, 0.0};

enum eClimbDisengageSource
{
	CLIMB_DISENGAGE_INVALID = 0,
	CLIMB_DISENGAGE_CROUCH = 1,
	CLIMB_DISENGAGE_LEAVETRIGGER,
	CLIMB_DISENGAGE_GROUNDTRACE,
	CLIMB_DISENGAGE_JUMP,
	
	CDS_COUNT
};

static bool g_bGoingDownPipe[MAXPLAYERS + 1];

methodmap CPFClimbHandler
{
	/*public static bool CheckInBounds(int iClient, CPFPipe hPipe)
	{
		float vecMins[3], vecMaxs[3], vecOrigin[3], vecPipeOrigin[3], vecPipeOriginMins[3], vecPipeOriginMaxs[3];
		
		GetEntPropVector(hPipe.EntIndex, Prop_Data, "m_vecMaxs", vecMaxs);
		GetEntPropVector(hPipe.EntIndex, Prop_Data, "m_vecMins", vecMins);
		GetEntPropVector(hPipe.EntIndex, Prop_Send, "m_vecOrigin", vecPipeOrigin);
		GetClientAbsOrigin(iClient, vecOrigin);
		
		DrawBoundingBox(vecMins, vecMaxs, vecPipeOrigin);
		
		AddVectors(vecPipeOrigin, vecMins, vecPipeOriginMins);
		AddVectors(vecPipeOrigin, vecMaxs, vecPipeOriginMaxs);
	
		if ((vecOrigin[0] >= vecPipeOriginMins[0] && vecOrigin[0] <= vecPipeOriginMaxs[0] &&
			vecOrigin[1] >= vecPipeOriginMins[1] && vecOrigin[1] <= vecPipeOriginMaxs[1] &&
			vecOrigin[2] >= vecPipeOriginMins[2] && vecOrigin[2] <= vecPipeOriginMaxs[2] ))
			return true;
		else
			return false;
	}*/
	
	public static void Mount(int iClient, CPFPipe hPipe)
    {
        float flClientAbsVel[3];
        GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", flClientAbsVel);
        if (flClientAbsVel[2] > -1200.0 && GetEntityMoveType(iClient) != MOVETYPE_NOCLIP)
        {
            float vecAngles[3];
            GetClientEyeAngles(iClient, vecAngles);
            vecAngles[2] = 0.0;
            TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
            
            if (!IsValidClient(iClient) || hPipe == null)
                return;
                
            if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
                UnscopeRifle(iClient);
            
            SetEntityMoveType(iClient, MOVETYPE_FLY);
            
            CPFPipeController.SetClientPipe(iClient, hPipe);
            CPFSpeedController.SetSpeed(iClient, 1.0);
            SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_ATCONTROLS);
            
            if (CPFStateController.HasFlags(iClient, SF_LONGJUMP))
                CPFSpeedController.SetStoredSpeed(iClient, SPEED_BASE);
            
            CPFStateController.Set(iClient, State_Climb);
            
            int iClimbable = hPipe.EntIndex;
            if (!IsValidEntity(iClimbable))
            {
                DebugOutput("CPFClimbHandler::Mount --- Pipe %d has invalid entindex %d! Client: %N", hPipe.PipeIndex, iClimbable, iClient);
                return;
            }
            
            float vecOrigin[3], vecDestination[3], vecMaxs[3], vecPipeOriginMaxs[3];
            GetEntPropVector(iClimbable, Prop_Send, "m_vecOrigin", vecDestination);
            GetEntPropVector(iClimbable, Prop_Data, "m_vecMaxs", vecMaxs);
            GetClientAbsOrigin(iClient, vecOrigin);
                
            AddVectors(vecDestination, vecMaxs, vecPipeOriginMaxs);
            
            if ((vecOrigin[2] + 5) <= vecPipeOriginMaxs[2])
                vecDestination[2] = vecOrigin[2] + 5.0; // Add 5 to keep player from auto-dismounting
            else
                vecDestination[2] = vecPipeOriginMaxs[2] - 5.0;
                
            if (CheckPointAgainstPlayerHull(iClient, vecDestination))
                return;
            
            TeleportEntity(iClient, vecDestination, NULL_VECTOR, ZERO_VECTOR);
            
            CPFViewController.SetDefaultSequence(iClient, AnimState_ClimbIdle);
            
            CPFSoundController.PlaySmallDing(iClient);
            
            CPFSoundController.AddIntensity(iClient, 0.5);

            //Teleport the player's viewmodel angles towards the pipe if possible
            float vAngles[3] = { 0.0, 180.0, 0.0 }, vEndPos[3], flBestYaw = 180.0, flShortestDist = 128.000001;
            
            for(int i; i < 4; i++, vAngles[1] -= 90.0) {
                TR_TraceRayFilter(vecDestination, vAngles, MASK_PLAYERSOLID, RayType_Infinite, TraceRayNoPlayers, iClient);
                if(!TR_DidHit())
                    continue;
                
                bool bFoundShorterDist;
                TR_GetEndPosition(vEndPos);
                float flDist = GetVectorDistance(vecDestination, vEndPos);
                if(flDist < flShortestDist) {
                    flBestYaw = vAngles[1];
                    flShortestDist = flDist;
                    bFoundShorterDist = true;
                }
                
                if(g_cvarDebugBeams.BoolValue)
                    DrawVectorPoints(vecDestination, vEndPos, 5.0, bFoundShorterDist ? { 0, 255, 0, 255 } : { 255, 0, 255, 255 });
            }
            
            CPFViewController.LockRotation(iClient, flBestYaw, 12.0);
            
            DebugOutput("CPFClimbHandler::Mount --- Mount successful for %N on pipe %d with estimated yaw %f", iClient, hPipe.PipeIndex, flBestYaw);
        }
        else return;
    }
	
	public static void Disengage(int iClient, eClimbDisengageSource eCause)
	{
		const float LEAVETRIGGER_COOLDOWN = 0.33;
		
		CPFPipe hPipe = CPFPipeController.GetClientPipe(iClient);
		CPFPipeController.SetClientPipe(iClient, CPFPIPE_INVALID);
		CPFPipeController.SetClientLastPipe(iClient, hPipe);
		CPFStateController.Set(iClient, State_None);
		CPFSoundController.StopPipeSlide(iClient);
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		
		SetEntityMoveType(iClient, MOVETYPE_WALK);
		
		switch (eCause)
		{
			case CLIMB_DISENGAGE_JUMP:
			{
				const float PIPEJUMP_FWD_VELOCITY = 320.0;
				const float PIPEJUMP_UP_VELOCITY = 256.0;
				
				SetEntProp(iClient, Prop_Send, "m_iAirDash", 1); 
				
				float vecForward[3];
				GetForwardVector(iClient, vecForward);
				ScaleVector(vecForward, PIPEJUMP_FWD_VELOCITY);
				vecForward[2] = PIPEJUMP_UP_VELOCITY;
				TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
				
				DebugOutput("CPFClimbHandler::Disengage --- Pipejump successful for %N from pipe %d", iClient, hPipe.PipeIndex);
				
				SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_DONTTOUCH);
				CreateTimer(LEAVETRIGGER_COOLDOWN, CheckPlayerTrigger, iClient);
			}
			
			case CLIMB_DISENGAGE_CROUCH:
			{
				DebugOutput("CPFClimbHandler::Disengage --- Crouch disengage successful for %N from pipe %d", iClient, hPipe.PipeIndex);
				SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_DONTTOUCH);
				CreateTimer(LEAVETRIGGER_COOLDOWN, CheckPlayerTrigger, iClient);
			}
				
			
			// If EndTouch doesn't work, let's check player origins to make sure they're in the trigger
			// Original PF likely used EndTouch
			case CLIMB_DISENGAGE_LEAVETRIGGER:
			{
				DebugOutput("CPFClimbHandler::Disengage --- Trigger disengage successful for %N from pipe %d", iClient, hPipe.PipeIndex);
				
				if (GetClientButtons(iClient) & IN_FORWARD)
				{
					float vecForward[3];
					GetForwardVector(iClient, vecForward);
					ScaleVector(vecForward, 320.0);
					vecForward[2] = 256.0;
					TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
				}
			}
			
			case CLIMB_DISENGAGE_GROUNDTRACE:
			{
				DebugOutput("CPFClimbHandler::Disengage --- Ground trace disengage successful for %N from pipe %d", iClient, hPipe.PipeIndex);
			}
			
			default:
			{
				DebugOutput("CPFClimbHandler::Disengage --- Disengage cause defaulted: %d", view_as<int>(eCause));
			}
		}
		
		
		CPFStateController.SetCooldown(iClient, State_Climb, LEAVETRIGGER_COOLDOWN);
		
		CPFSpeedController.RestoreSpeed(iClient);
		
		CPFViewController.UnlockRotation(iClient);
	}
	
	public static void Climb(int iClient, CPFPipe hPipe)
	{
		if (!IsValidClient(iClient) || hPipe == null)
			return;
		
		float vecOrigin[3], vecClimb[3];
		GetEntPropVector(hPipe.EntIndex, Prop_Send, "m_vecOrigin", vecClimb);
		GetClientAbsOrigin(iClient, vecOrigin);
		if ((vecOrigin[0] != vecClimb[0] || vecOrigin[1] != vecClimb[1]) && CPFStateController.Get(iClient) == State_Climb)
		{
			vecClimb[2] = vecOrigin[2];
			if (!CheckPointAgainstPlayerHull(iClient, vecClimb))
				TeleportEntity(iClient, vecClimb, NULL_VECTOR, NULL_VECTOR);
		}
		
		//SetEntityMoveType(iClient, MOVETYPE_FLY);
		CPFSpeedController.SetSpeed(iClient, 1.0);
		
		//SetEntPropVector(iClient, Prop_Data, "m_vecMins", CLIMB_BBOX_MINS);
		//SetEntPropVector(iClient, Prop_Data, "m_vecMins", CLIMB_BBOX_MAXS);
		
		float vecVelocity[3];
		int iButtons = GetClientButtons(iClient);
		
		if (iButtons & IN_FORWARD)
		{
			if (g_bGoingDownPipe[iClient])
			{
				CPFSoundController.StopPipeSlide(iClient);
				g_bGoingDownPipe[iClient] = false;
			}
			
			CPFSoundController.PlayPipeclimb(iClient);
			
			vecVelocity[2] = 250.0;
			DebugOutput("CPFClimbHandler --- Moving %N forward: %.3f %.3f %.3f", iClient, vecVelocity[0], vecVelocity[1], vecVelocity[2]);
			
			CPFViewController.Queue(iClient, AnimState_Climb, 1.0);
		}
		else if (iButtons & IN_BACK)
		{
			if (IsOnGroundTrace(iClient))
			{
				CPFClimbHandler.Disengage(iClient, CLIMB_DISENGAGE_GROUNDTRACE);
				return;
			}
			else
			{
				if (!g_bGoingDownPipe[iClient])
				{
					CPFSoundController.PlayPipeSlide(iClient);
					g_bGoingDownPipe[iClient] = true;
				}
				
				vecVelocity[2] = -350.0;
				
				CPFViewController.Queue(iClient, AnimState_ClimbIdle, 1.0);
			}
		}
		else
		{
			if (g_bGoingDownPipe[iClient])
			{
				CPFSoundController.StopPipeSlide(iClient);
				g_bGoingDownPipe[iClient] = false;
			}
			
			CPFViewController.Queue(iClient, AnimState_ClimbIdle, -1.0);
		}
		
		/*float vecOrigin[3], vecTriggerOrigin[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		GetEntPropVector(hPipe.EntIndex, Prop_Send, "m_vecOrigin", vecTriggerOrigin);
		vecOrigin[0] = vecTriggerOrigin[0];
		vecOrigin[1] = vecTriggerOrigin[1];
		
		if(!CheckPointAgainstPlayerHull(iClient, vecOrigin))
			TeleportEntity(iClient, vecOrigin, NULL_VECTOR, NULL_VECTOR);*/
		
		TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecVelocity);
	}
};