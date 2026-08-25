#if defined _PFTUTORIAL_INCLUDED
	#endinput
#endif
#define _PFTUTORIAL_INCLUDED

Handle hTutorialTimer[MAXPLAYERS + 1] = { INVALID_HANDLE, ... };

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

		// #19246: mirror the stage into LoadoutSharp's cross-session store so tutorial progress follows the
		// player across sessions and servers. Uses the int value directly, not the single-digit Stage buffer
		// above. Best-effort - an absent bridge command just logs an unknown-command line and is a no-op.
		ServerCommand("compat_settutorialstage %d %d", GetClientUserId(iClient), view_as<int>(eStage));
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
	
	// #19246: read the player's tutorial stage back from LoadoutSharp's cross-session store via the
	// compat_gettutorialstage bridge command. Mirrors the ServerCommandEx capture pattern in weapons/stocks.sp:
	// the command prints a bare integer line to the console, which the redirect captures into strBuf. Returns
	// TUTORIAL_INVALID when the client is invalid, the capture is empty / non-numeric (e.g. an unknown-command
	// line when LoadoutSharp is absent), or the value is negative (the getter's -1 failure sentinel).
	public static TutorialStage FetchStageFromLoadoutSharp(int iClient)
	{
		if (!IsValidClient(iClient))
			return TUTORIAL_INVALID;

		char strBuf[16];
		ServerCommandEx(strBuf, sizeof(strBuf), "compat_gettutorialstage %d", GetClientUserId(iClient));
		TrimString(strBuf);

		int iStage;
		if (StringToIntEx(strBuf, iStage) == 0 || iStage < 0)
			return TUTORIAL_INVALID;

		return view_as<TutorialStage>(iStage);
	}

	public static void InitPlayer(int iClient)
	{
		if (hTutorialTimer[iClient] != INVALID_HANDLE)
			delete hTutorialTimer[iClient];

		// #19246 / #19272: reconcile the local tutorial-stage cookie with LoadoutSharp's cross-session store by
		// taking the higher of the two, so a player who advanced the tutorial in a prior session or on another
		// server resumes at the right stage and neither store can ever move them backwards.
		//
		// Seeding the bridged value unconditionally is what #19272 fixes: an absent LoadoutSharp cookie answers 0,
		// which is a valid stage and therefore is not caught by the TUTORIAL_INVALID guard, so a player who
		// finished the tutorial under the old cookie-only system had their progress reset and the 0 pushed back.
		// Writing the max also carries a local value LoadoutSharp is behind on across the bridge, which matters
		// because LoadoutSharp hides the player's HUD for as long as its own copy reads mid-tutorial.
		//
		// A failed or invalid fetch (TUTORIAL_INVALID) falls through to the existing cookie-only behaviour and
		// never blocks the player. Stores that already agree are left alone rather than rewritten.
		TutorialStage eBridged = CPFTutorialController.FetchStageFromLoadoutSharp(iClient);
		TutorialStage eLocal = CPFTutorialController.GetStage(iClient);

		if (eBridged != TUTORIAL_INVALID && eBridged != eLocal)
		{
			TutorialStage eResolved = eLocal;
			if (eBridged > eLocal)
				eResolved = eBridged;

			CPFTutorialController.SetStage(iClient, eResolved);
		}

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