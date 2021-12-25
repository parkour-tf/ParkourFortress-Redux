#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _MOVEMENTS_LONGJUMP_INCLUDED
	#endinput
#endif
#define _MOVEMENTS_LONGJUMP_INCLUDED

const int IN_LONGJUMP = IN_ATTACK2;

methodmap CPFLongjumpHandler
{
	public static void Longjump(int iClient)
	{
		if (!IsValidClient(iClient) || !IsOnGround(iClient))
			return;
		
		CPFStateController.AddFlags(iClient, SF_LONGJUMP);
		CPFTutorialController.Stagepoint(iClient, TUTORIAL_LONGJUMP);
		
		const float LONGJUMP_FWD_VELOCITY = 610.0;
		const float LONGJUMP_UP_VELOCITY = 510.0;
		
		SetEntProp(iClient, Prop_Send, "m_iAirDash", 1); 
		//SetEntityMoveType(iClient, MOVETYPE_FLYGRAVITY);
		
		float vecForward[3];
		GetForwardVector(iClient, vecForward);
		ScaleVector(vecForward, LONGJUMP_FWD_VELOCITY);
		vecForward[2] = LONGJUMP_UP_VELOCITY;
		TeleportEntity(iClient, NULL_VECTOR, NULL_VECTOR, vecForward);
		
		CPFSpeedController.SetStoredSpeed(iClient, SPEED_BASE);
		CPFSpeedController.RestoreSpeed(iClient);
		
		CPFSoundController.PlayLongjumpVO(iClient);
		CPFSoundController.PlayBigDing(iClient);
		CPFSoundController.AddIntensity(iClient, 1.0);
		
		DebugOutput("CPFLongjumpHandler::Longjump --- Longjump successful for %N", iClient);
		
		CPFViewController.Queue(iClient, AnimState_Leap, 1.0, true);
		CPFViewController.SetDontInterrupt(iClient, true);
		
		// TODO: Implement tutorial
		//CPFTutorialController.MarkPoint(iClient, Tutorial_Sprint);
	}
	
	public static void End(int iClient)
	{
		SetEntityMoveType(iClient, MOVETYPE_WALK);
		
		CPFSpeedController.SetStoredSpeed(iClient, SPEED_BASE);
		CPFSpeedController.RestoreSpeed(iClient);
		CPFStateController.RemoveFlags(iClient, SF_LONGJUMP);
	}
};