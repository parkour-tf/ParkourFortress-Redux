#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFTUTORIAL_INCLUDED
	#endinput
#endif
#define _PFTUTORIAL_INCLUDED

/* BIG TODO: Redo this with cookies. It was hastily done using external MySQL, so it won't work outside of parkour.tf as is. */

#if !defined _PF_INCLUDED
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
#endif

methodmap CPFTutorialController
{
	public static TutorialStage GetStage(int iClient)
	{
#if defined _PF_INCLUDED
		return TutorialStage.GetStage(iClient);
#else
		return 6;
#endif
	}
	
	public static void SetStage(int iClient, TutorialStage eStage)
	{
		eStage.SetStage(iClient);
	}
	
	public static void IncStage(int iClient)
	{
#if defined _PF_INCLUDED
		TutorialStage eStage = TutorialStage.GetStage(iClient);
		eStage++;
		CPFTutorialController.SetStage(iClient, eStage);
#endif
	}

	public static void SetOverlay(int iClient, const char[] strOverlay)
	{
		char strPath[PLATFORM_MAX_PATH];
		FormatEx(strPath, PLATFORM_MAX_PATH, "parkoursource/tutorialredux/%s", strOverlay);
		DisplayScreenImage(iClient, strPath, (CPFTutorialController.GetStage(iClient) == TUTORIAL_COMPLETE) ? 5.0 : 0.0);
	}
	
	public static void ClearOverlay(int iClient)
	{
		ClientCommand(iClient, "r_screenoverlay \"\"");
	}

	public static void Stagepoint(int iClient, TutorialStage eStage)
	{
#if defined _PF_INCLUDED
		if (CPFTutorialController.GetStage(iClient) != eStage)
			return;
		
		CPFTutorialController.IncStage(iClient);
		TutorialStage eCurrent = CPFTutorialController.GetStage(iClient);
		
		char strOverlay[16];
		Format(strOverlay, sizeof(strOverlay), "tutorial%d", view_as<int>(eCurrent));
		CPFTutorialController.SetOverlay(iClient, strOverlay);
		
		if (eCurrent == TUTORIAL_COMPLETE)
			CreateTimer(5.0, CompleteTutorial, iClient);
		else if (eCurrent < TUTORIAL_NONE || eCurrent > TUTORIAL_COMPLETE)
		{
			CPFTutorialController.ClearOverlay(iClient);
			CreateTimer(0.1, CompleteTutorial, iClient);
		}
#endif
	}
	
	public static void Restart(int iClient)
	{
#if defined _PF_INCLUDED
		CPFTutorialController.SetStage(iClient, TUTORIAL_NONE);
		FakeClientCommand(iClient, "sm_restart");
		CPFTutorialController.Stagepoint(iClient, TUTORIAL_NONE);
#endif
	}
	/*
	public static void Think(int iClient)
	{
		CPFTutorialController.SetOverlayDynamic(iClient);
	}
	*/
	public static void Complete(int iClient)
	{
#if defined _PF_INCLUDED
		CPFTutorialController.SetStage(iClient, TUTORIAL_COMPLETE);
		FakeClientCommand(iClient, "sm_restart");
#endif
	}
	
	public static void Init()
	{
#if defined _PF_INCLUDED
		for (int i = 1; i < view_as<int>(TS_COUNT); i++)
		{
			char strMaterial[PLATFORM_MAX_PATH];
			Format(strMaterial, sizeof(strMaterial), "parkoursource/tutorialredux/tutorial%d", i);
			SuperPrecacheMaterial(strMaterial, true);
		}
#endif
	}
};

public void DisplayScreenImage(int iClient, const char[] strFile, float flDuration)
{
	if (!IsValidClient(iClient)) return;
	
	ClientCommand(iClient, "r_screenoverlay \"%s.vtf\"", strFile);
	
	if (flDuration)
		CreateTimer(flDuration, ClearScreenImage, iClient);
}

public Action ClearScreenImage(Handle hTimer, int iClient)
{
	if (IsValidClient(iClient))
		ClientCommand(iClient, "r_screenoverlay \"\"");
	
	return Plugin_Handled;
}

public Action CompleteTutorial(Handle hTimer, int iClient)
{
	CPFTutorialController.Complete(iClient);
	return Plugin_Handled;
}