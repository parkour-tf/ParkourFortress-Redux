#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_VAULT_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_VAULT_INCLUDED

methodmap CPFVaultHandler
{
	public static void Mount(const int iClient)
	{
		CPFStateController.Set(iClient, State_Vault);
	}
	
	public static void Dismount(const int iClient)
	{
		CPFStateController.Set(iClient, State_None);
		CPFSpeedController.RestoreSpeed(iClient);
		CPFStateController.SetCooldown(iClient, State_Wallclimb, 0.1);
		RequestFrame(VaultPost, GetClientUserId(iClient));
	}
	
	public static void Vault(const int iClient, float vecForward[3])
	{
		const float VAULT_FORWARD_MAGNITUDE = 1.0;
		const float VAULT_DIRECTION_SCALE = 128.0;
		
		float vecDir[3], vecEyeAngles[3], vecOrigin[3];
		CPFVaultHandler.Mount(iClient);
		
		GetClientEyeAngles(iClient, vecEyeAngles);
		vecEyeAngles[0] = 0.0;
		vecEyeAngles[2] = 0.0;
		ForwardVector(vecEyeAngles, VAULT_FORWARD_MAGNITUDE, vecDir);
		AddVectors(vecForward, vecDir, vecForward);
		ScaleVector(vecDir, VAULT_DIRECTION_SCALE);
				
		float vecAbsVelocity[3];
		GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", vecAbsVelocity);
		
		AddVectors(vecAbsVelocity, vecDir, vecAbsVelocity);
		vecAbsVelocity[2] = 0.0;
		
		if (!CheckPointAgainstPlayerHull(iClient, vecForward))
			TeleportEntity(iClient, vecForward, NULL_VECTOR, vecAbsVelocity);
		else
		{
			GetClientAbsOrigin(iClient, vecOrigin);
			vecOrigin[2] = vecForward[2];
			TeleportEntity(iClient, vecOrigin, NULL_VECTOR, vecAbsVelocity);
		}
			
		SetEntityMoveType(iClient, MOVETYPE_DEFAULT);
		SetCollisionGroup(iClient, g_ePFCollisionGroup);
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
		
#if defined DEBUG
		DrawVectorPoints(vecForward, vecOrigin, 5.0, {255, 0, 255, 255});
#endif
		
		CPFVaultHandler.Dismount(iClient);
	}

	public static bool Try(const int iClient)
	{
		const float VAULT_MAX_TOLERANCE = 140.0;
		const float VAULT_TRY_FORWARD_MAGNITUDE = 8.0;
		const float VAULT_FORWARD_SHIFT = 40.0;
		const float VAULT_ENDPOS_SHIFT = 25.0;
		const float VAULT_ATTEMPT_INCREMENT = 1.0;
		
		float vecAngles[3], vecOrigin[3], vecForward[3], vecForwardDelta[3];
		GetClientAbsOrigin(iClient, vecOrigin);
		GetClientAbsAngles(iClient, vecAngles);
		
		ForwardVector(vecAngles, VAULT_TRY_FORWARD_MAGNITUDE, vecForward);
		vecForward[2] -= VAULT_FORWARD_SHIFT;
		AddVectors(vecForward, vecOrigin, vecForward);
		vecForwardDelta = vecForward;	

		bool bVaultTraceHit;
		float vecEndPosition[3];
		do
		{
			TraceHullF.Start(vecForwardDelta, vecForwardDelta, view_as<float>({-25.0, -25.0, 0.0}), view_as<float>({25.0, 25.0, 82.0}), MASK_PLAYERSOLID, TraceRayNoPlayers);
			bVaultTraceHit = TRACE_GLOBAL.Hit;
			
			if (!bVaultTraceHit)
			{
				DrawBoundingBox(view_as<float>({-25.0, -25.0, 0.0}), view_as<float>({25.0, 25.0, 82.0}), vecForwardDelta, _, {0, 255, 0, 255});
				
				TRACE_GLOBAL.GetEndPosition(vecEndPosition);
				vecEndPosition[2] += VAULT_ENDPOS_SHIFT;
				
				if (!CheckPointAgainstPlayerHull(iClient, vecEndPosition))
				{
					CPFVaultHandler.Vault(iClient, vecEndPosition);
					break;
				}
				else
				{
					SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_ATCONTROLS);
					break;
				}
			}
			else
			{
#if defined DEBUG
				int iColors[4];
				iColors[0] = 0;
				iColors[1] = RoundToFloor(FloatAbs(255.0 - vecForward[2]) * 2.0);
				iColors[2] = RoundToFloor(FloatAbs(vecForward[2]) * 2.0);
				iColors[3] = 255;
				DrawBoundingBox(view_as<float>({-25.0, -25.0, 0.0}), view_as<float>({25.0, 25.0, 82.0}), vecForwardDelta, _, iColors);
#endif
				
				vecForwardDelta[2] += VAULT_ATTEMPT_INCREMENT;
				
				DebugOutput("CPFVaultHandler::Try --- %N vecForward[2]: %.1f", iClient, vecForwardDelta[2] - vecForward[2]);
				continue;
			}
		
		}
		while ((vecForwardDelta[2] - vecForward[2]) <= VAULT_MAX_TOLERANCE);
		
		return bVaultTraceHit;
	}
};

public void VaultPost(int iUserID)
{
    const float VAULT_SPEED_BOOST = 25.0;
    
    int iClient = GetClientOfUserId(iUserID);
    CPFSpeedController.SetSpeed(iClient, CPFSpeedController.GetStoredSpeed(iClient) + VAULT_SPEED_BOOST);
}
