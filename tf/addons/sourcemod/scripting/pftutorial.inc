#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFTUTORIAL_INCLUDED
	#endinput
#endif
#define _PFTUTORIAL_INCLUDED

Handle hTutorialTimer[TF_MAXPLAYERS + 1] = { INVALID_HANDLE, ... };

enum TutorialStage
{
	TUTORIAL_INVALID = -1,
	TUTORIAL_NONE = 0,
	TUTORIAL_SPRINT = 1,
	TUTORIAL_LONGJUMP,
	TUTORIAL_CLIMB,
	TUTORIAL_KICKOFF,
	TUTORIAL_WALLRUN,
	TUTORIAL_COMPLETE,
	
	TS_COUNT
};

methodmap CPFTutorialController
{
	public static TutorialStage GetStage(int iClient)
	{
		return view_as<TutorialStage>(GetCookieInt(g_cookieTutorialStage, iClient));
	}
	
	public static void SetStage(int iClient, TutorialStage eStage)
	{
		char Stage[2];
		IntToString(view_as<int>(eStage), Stage, sizeof(Stage));
		g_cookieTutorialStage.Set(iClient, Stage);
	}

	public static void IncStage(int iClient)
	{
		CPFTutorialController.SetStage(iClient, view_as<TutorialStage>(GetCookieInt(g_cookieTutorialStage, iClient) + 1));
	}
	
	public static void ClearOverlay(int iClient)
	{
		ClientCommand(iClient, "r_screenoverlay \"\"");
	}

	public static void Stagepoint(int iClient, TutorialStage eStage)
	{
		if (CPFTutorialController.GetStage(iClient) != eStage)
			return;
		
		CPFTutorialController.IncStage(iClient);
		TutorialStage eCurrent = CPFTutorialController.GetStage(iClient);
		
		char strOverlay[16];
		Format(strOverlay, sizeof(strOverlay), "tutorial%d", view_as<int>(eCurrent));

		if (hTutorialTimer[iClient] != INVALID_HANDLE)
			TriggerTimer(hTutorialTimer[iClient]);
		
		if (eCurrent < TUTORIAL_NONE || eCurrent > TUTORIAL_COMPLETE) {
			CPFTutorialController.ClearOverlay(iClient);
			CreateTimer(0.1, CompleteTutorial, iClient);
		}
		else if (eCurrent == TUTORIAL_COMPLETE)
			CreateTimer(5.0, CompleteTutorial, iClient);
	}

	public static void Restart(int iClient)
	{
		CPFTutorialController.SetStage(iClient, TUTORIAL_NONE);
#if defined _PFTIMER_INCLUDED
		FakeClientCommand(iClient, "sm_restart");
#endif
		CPFTutorialController.Stagepoint(iClient, TUTORIAL_NONE);
	}

	public static void Complete(int iClient)
	{
		CPFTutorialController.SetStage(iClient, TUTORIAL_COMPLETE);
#if defined _PFTIMER_INCLUDED
		FakeClientCommand(iClient, "sm_restart");
#endif
	}
	
	public static void InitPlayer(int iClient) {
		if (hTutorialTimer[iClient] != INVALID_HANDLE)
			delete hTutorialTimer[iClient];

		if (CPFTutorialController.GetStage(iClient) < TUTORIAL_COMPLETE)
			hTutorialTimer[iClient] = CreateTimer(1.0, DisplayTutorialScreen, iClient, TIMER_REPEAT);
	}

	public static void Init()
	{
		for (int i = 1; i < view_as<int>(TS_COUNT); i++)
		{
			char strMaterial[PLATFORM_MAX_PATH];
			Format(strMaterial, sizeof(strMaterial), "parkoursource/tutorialredux/tutorial%d", i);
			SuperPrecacheMaterial(strMaterial, true);
		}
	}
};

Action DisplayTutorialScreen(Handle hTimer, int iClient) 
{
	if (!IsValidClient(iClient)) {
		hTutorialTimer[iClient] = INVALID_HANDLE;
		return Plugin_Stop;
	}

	if (!IsPlayerAlive(iClient))
		return Plugin_Continue;
	
	TutorialStage eCurrent = CPFTutorialController.GetStage(iClient);
	char strPath[PLATFORM_MAX_PATH];
	FormatEx(strPath, PLATFORM_MAX_PATH, "parkoursource/tutorialredux/tutorial%i", view_as<int>(eCurrent)); //Get stage number
	ClientCommand(iClient, "r_screenoverlay \"%s.vtf\"", strPath);  //Set and overlay based on the player's stage
	
	if (CPFTutorialController.GetStage(iClient) == TUTORIAL_COMPLETE) { //Stop the timer and clean up the handle
		hTutorialTimer[iClient] = INVALID_HANDLE;
		CreateTimer(5.0, ClearScreenImage, iClient);
		return Plugin_Stop;
	}

	return Plugin_Continue;
}

Action ClearScreenImage(Handle hTimer, int iClient)
{
	if (IsValidClient(iClient))
		ClientCommand(iClient, "r_screenoverlay \"\"");
	
	return Plugin_Handled;
}

Action CompleteTutorial(Handle hTimer, int iClient)
{
	CPFTutorialController.Complete(iClient);
	return Plugin_Handled;
}