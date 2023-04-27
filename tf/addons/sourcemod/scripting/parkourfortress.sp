/*
**** Parkour Fortress: Redux ****

Code - ScrewdriverHyena (Jon S.), Nami
Early Alpha Code - NotPaddy (Patric O.), Muffinoota (Ian M.)
Original Parkour Fortress - MekuCube (Jonas K.)

Assets by various artists, including but not limited to:

Tutorial - Ben Sadfleck
Viewmodels - Kirillian
Original PF Assets - Werbad, MekuCube
Remade PF Assets - Beepin (Nate B.), ScrewdriverHyena

All code is licensed under the GNU General Public License, version 3.
*/

#define PLUGIN_VERSION		"1.01p Needle Release Edition"

#define TF_MAXPLAYERS		34	//32 clients + 1 for 0/world/console + 1 for replay/SourceTV
#define WEAPON_FISTS 		5

#include <sourcemod>
#include <sdkhooks>
#include <tf2>
#include <tf2attributes>
#include <tf2_stocks>
#include <clientprefs>
#include <tracerayex>
#include <dhooks>
#include <morecolors>
#include <tf_econ_data>

#pragma semicolon 1
#pragma newdecls required

#include "parkourfortress.inc"

#include "pftutorial.inc"
#include "pfclient.inc"
#include "pfstate.inc"
#include "pfsound.inc"
#include "pfviewmodel.inc"
#include "pfspeed.inc"
#include "objects/ziplines.inc"
#include "objects/rails.inc"
#include "objects/ropes.inc"
#include "objects/pipes.inc"
#include "objects/doors.inc"
#include "movements/longjump.inc"
#include "movements/roll.inc"
#include "movements/climb.inc"
#include "movements/slide.inc"
#include "movements/wallrun.inc"
#include "movements/hang.inc"
#include "movements/zipline.inc"
#include "movements/doorslam.inc"
#include "movements/vault.inc"
#include "movements/wallclimb.inc"
#include "movements/grindable-rail.inc"
#include "weapons/weapons.sp"
#include "weapons/stocks.sp"
#include "weapons/pickupweapons.sp"
#include "weapons/config.sp"
#include "smmem.inc"

public Plugin myinfo =
{
    name = "Parkour Fortress: Redux",
    author = "Screwdriver (Jon S.), Nami (Nami), NotPaddy (Patric O.)",
    description = "Let's restart from scratch, shall we?",
    version = PLUGIN_VERSION,
    url = "https://github.com/NotPaddy/pf-redux"
};

public APLRes AskPluginLoad2(Handle hSelf, bool bLate, char[] strError, int iErr_max)
{
	g_hForwardWeaponPickup = new GlobalForward("OnPickupWeapon", ET_Ignore, Param_Cell, Param_Cell, Param_Cell);
	
	g_bLate = bLate;
	return APLRes_Success;
}

public void OnPluginStart()
{
	if (GetEngineVersion() != Engine_TF2)
		return;
	
	PrintToServer("Parkour Fortress Reloading...");
	
	PrecacheModels();
	
	HookEvent("player_spawn", OnPlayerSpawn, EventHookMode_Post);
	HookEvent("post_inventory_application", OnInventoryPost);
	HookEvent("teamplay_round_start", OnRoundStart);
	HookEvent("teamplay_round_win", OnRoundEnd);
	HookEvent("teamplay_round_stalemate", OnRoundEnd);
	HookEvent("player_death", OnPlayerDeath, EventHookMode_Post);
	
	HookUserMessage(GetUserMessageId("VoiceSubtitle"), UserMsg_VoiceSubtitle, true);
	
	CreateConVar("pf_version", PLUGIN_VERSION, "Plugin Version", FCVAR_ARCHIVE);
	g_cvarPvP = CreateConVar("pf_pvp", "0", "Enable PvP", 0, true, 0.0);
	g_cvarDebugSpeed = CreateConVar("pf_speedlog", "0", "Enable speed log in chat", 0, true, 0.0, true, 1.0);
	g_cvarDebugState = CreateConVar("pf_statelog", "0", "Enable state log in chat", 0, true, 0.0, true, 1.0);
	g_cvarDebugExtra = CreateConVar("pf_debuglog", "0", "Enable certain debug messages in chat", 0, true, 0.0, true, 1.0);
	g_cvarDebugBeams = CreateConVar("pf_debugbeams", "0", "Enable certain debug messages in chat", 0, true, 0.0, true, 1.0);
	g_cvarMusicEnabled = CreateConVar("pf_enablemusic", "1", "Enable background music on map", 0, true, 0.0, true, 1.0);
	g_cvarViewmodels = CreateConVar("pf_viewmodels", "1", "Enable viewmodels", 0, true, 0.0, true, 1.0);
	g_cvarWallrunTraces = CreateConVar("pf_wallrun_traces", "10", "How many traces should wallruns cast?", 0, true, 8.0, true, 90.0);
	
	g_cvarAirAcceleration = FindConVar("sv_airaccelerate");
	g_cvarAcceleration = FindConVar("sv_accelerate");
	
	g_cvarWeaponRespawn = CreateConVar("pf_weapon_respawntime", "60.0", "Max time for weapons to respawn if respawntime_random is on, otherwise hard value for respawn period", 0, true, 1.0, false);
	g_cvarWeaponRespawnMin = CreateConVar("pf_weapon_respawntime_min", "10.0", "If respawntime_random is on, minimum time for weapons to respawn after being picked up", 0, true, 1.0, false);
	g_cvarWeaponRespawnRandom = CreateConVar("pf_weapon_respawntime_random", "1", "Weapons respawn in a time period between respawntime_min and respawntime", 0, true, 0.0, true, 1.0);
	g_cvarWeaponMaxRare = CreateConVar("pf_weapon_rare_max", "99999", "Max number of rare weapons spawned per round", 0, true, 0.0, false);
	g_cvarWeaponRareChance = CreateConVar("pf_weapon_rare_chance", "5", "1 in value chance to spawn rare weapon when spawning random rarity", 0, true, 1.0, false);
	g_cvarWeaponGrabDistance = CreateConVar("pf_weapon_grab_distance", "100.0", "How far away in units a player can pick up a weapon from", 0, true, 48.0, false);
	
	g_cvarPvP.AddChangeHook(OnChangePvP);
	g_cvarWeaponRespawn.AddChangeHook(OnWeaponRespawnSet);
	g_cvarWeaponRespawnMin.AddChangeHook(OnWeaponRespawnMinSet);
	g_cvarAirAcceleration.AddChangeHook(OnChangeAirAccel);
	g_cvarAcceleration.AddChangeHook(OnChangeAccel);

	g_cookieTutorialStage = new Cookie("tutorialprogress", "How far along you are in the tutorial", CookieAccess_Protected);
	g_cookieLerp = new Cookie("parkourlerp", "Enable camera tilt", CookieAccess_Protected);
	g_cookieMusicVolume = new Cookie("musicvolume", "Background music volume", CookieAccess_Protected);
	g_cookieSelfAmbientSound = new Cookie("fluwee", "Enable self ambient sounds", CookieAccess_Protected);
	CPFViewController.Init();

	FindConVar("tf_avoidteammates_pushaway").SetBool(false);
	FindConVar("tf_grapplinghook_los_force_detach_time").SetFloat(6.0);
	FindConVar("tf_grapplinghook_acceleration").SetFloat(5000.0);
	FindConVar("tf_grapplinghook_max_distance").SetFloat(8192.0);
	FindConVar("tf_grapplinghook_projectile_speed").SetFloat(4200.0);
	FindConVar("tf_grapplinghook_use_acceleration").SetBool(true);
	
	
#if defined DRAWVECS
	InitSprite();
#endif

	if (g_bLate)
	{
		InitObjects(true);
		InitSDK();
		InitWeapons();
		InitOther();
	}
	
	InitMovements();
	
	RegAdminCmd("sm_debugreach", DebugReach, ADMFLAG_ROOT);
	RegAdminCmd("sm_reloadweapon", WeaponReload, ADMFLAG_ROOT);
	RegAdminCmd("sm_debugbbox", DebugBBox, ADMFLAG_ROOT);
	RegAdminCmd("sm_debugcoords", DebugCoords, ADMFLAG_ROOT);
	
	RegConsoleCmd("sm_tutorial", RestartTutorial, "Restart a player's tutorial");
	
	RegConsoleCmd("sm_skip", SkipTutorial, "Skip the tutorial");
	RegConsoleCmd("sm_skiptutorial", SkipTutorial, "Skip the tutorial");
	
	RegConsoleCmd("pf_music", EnableMusic, "Enable background music for the player");
	RegConsoleCmd("parkour_music", EnableMusic, "Enable background music for the player");
	RegConsoleCmd("sm_pf_music", EnableMusic, "Enable background music for the player");
	RegConsoleCmd("sm_parkour_music", EnableMusic, "Enable background music for the player");
	
	RegConsoleCmd("pf_musicvolume", ChangeMusicVolume, "Change background music volume for the player");
	RegConsoleCmd("pf_music_volume", ChangeMusicVolume, "Change background music volume for the player");
	RegConsoleCmd("parkour_musicvolume", ChangeMusicVolume, "Change background music volume for the player");
	RegConsoleCmd("parkour_music_volume", ChangeMusicVolume, "Change background music volume for the player");
	
	RegConsoleCmd("pf_sound", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("pf_sfx", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("parkour_sound", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("parkour_sfx", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("sm_pf_sound", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("sm_pf_sfx", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("sm_parkour_sound", EnableSound, "Enable ambient sounds for the player");
	RegConsoleCmd("sm_parkour_sfx", EnableSound, "Enable ambient sounds for the player");
	
	RegConsoleCmd("pf_fluwee", EnableSelfAmbientSound, "Why would you want this?");
	
	RegConsoleCmd("parkour_viewmodel", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("pf_viewmodel", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("parkour_animations", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("pf_animations", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("sm_parkour_viewmodel", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("sm_pf_viewmodel", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("sm_parkour_animations", EnableViewmodel, "Enable firstperson animations for the player");
	RegConsoleCmd("sm_pf_animations", EnableViewmodel, "Enable firstperson animations for the player");
	
	RegConsoleCmd("sm_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_cameratilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_camera_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("pf_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_pf_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("pf_cameratilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_pf_cameratilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("pf_camera_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_pf_camera_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("parkour_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("parkour_camera_tilt", EnableLerp, "Enable camera tilt animation for the player");
	RegConsoleCmd("sm_parkour_tilt", EnableLerp, "Enable camera tilt animation for the player");
}

public void OnClientCookiesCached(int iClient)
{
	InitClientCookie(g_cookieMusic, iClient, "1");
	InitClientCookie(g_cookieMusicVolume, iClient, "1");
	InitClientCookie(g_cookieTutorialStage, iClient, "1");
	InitClientCookie(g_cookieSound, iClient, "1");
	InitClientCookie(g_cookieSelfAmbientSound, iClient, "0");
	InitClientCookie(g_cookieViewmodel, iClient, "1");
	InitClientCookie(g_cookieLerp, iClient, "1");
}

void InitClientCookie(Cookie cookie, int iClient, char[] default_value) {
	char Value[8];
	cookie.Get(iClient, Value, sizeof(Value));
	if (Value[0] == '\0')
		cookie.Set(iClient, default_value);
}

public void OnAllPluginsLoaded()
{
	AddCommandListener(BlockCYOA, "cyoa_pda_open");
	AddCommandListener(Command_VoiceMenu, "voicemenu");

	FindConVar("tf_maxspeed_limit").SetFloat(5200.00);
}

public void OnWeaponRespawnSet(ConVar cvarTime, const char[] strOldValue, const char[] strNewValue)
{
	if (StringToFloat(strNewValue) <= g_cvarWeaponRespawnMin.FloatValue)
		cvarTime.FloatValue = g_cvarWeaponRespawnMin.FloatValue;
}

public void OnWeaponRespawnMinSet(ConVar cvarMin, const char[] strOldValue, const char[] strNewValue)
{
	if (StringToFloat(strNewValue) <= g_cvarWeaponRespawn.FloatValue)
		cvarMin.FloatValue = g_cvarWeaponRespawn.FloatValue;
}

public Action BlockCYOA(int client, const char[] command, int argc)
{
	return Plugin_Handled;
}

void ResetAirAccel()
{
	g_flStockAirAccel = g_cvarAirAcceleration.FloatValue;
	for (int i = 1; i < MaxClients; i++)
		g_flAirAccel[i] = g_flStockAirAccel;
	
	g_flStockAccel = g_cvarAcceleration.FloatValue;
	for (int i = 1; i < MaxClients; i++)
		g_flAccel[i] = g_flStockAccel;
}

public void OnPluginEnd()
{
#if defined _PFVIEWMODEL_INCLUDED
	CPFViewController.KillAll();
#endif
	for (int i = 1; i < MaxClients; i++)
	{
		if (!IsValidClient(i)) continue;
		SDKUnhook(i, SDKHook_PreThink, OnPreThink);
		SDKUnhook(i, SDKHook_PostThink, OnPostThink);
		SDKUnhook(i, SDKHook_WeaponSwitch, OnWeaponSwitch);
		CPFSoundController.StopCurrentMusic(i);
	}
	
	RemoveMaxSpeedPatch();
}

public void Mapvote_OnMapsLoaded()
{
	g_ePFCollisionGroup = (g_cvarPvP.BoolValue) ? COLLISION_GROUP_PLAYER : COLLISION_GROUP_DEBRIS_TRIGGER;
	
	for (int i = 1; i < MaxClients; i++)
	{
		if (CPFStateController.Get(i) == State_None)
			SetCollisionGroup(i, g_ePFCollisionGroup);
	}
	if (!g_cvarPvP.BoolValue && !IsDevServer())
		return;
	
	// Start PvP Setup
	const int PVP_RESPAWNWAVETIME = 5;
	FindConVar("mp_respawnwavetime").SetInt(PVP_RESPAWNWAVETIME); //
}

void CheckClientRopeCvars(int iClient)
{
	for (int i = 0; i < ROPE_TOTAL; i++)
		QueryClientConVar(iClient, g_strRopeCommands[i], ProcessClientRopeCvars, i);
}

void ProcessClientRopeCvars(QueryCookie cookie, int iClient, ConVarQueryResult result, const char[] cvarName, const char[] cvarValue, any iData)
{
	if (!StrEqual(cvarValue, "1"))
	{
		CPFRopeController.SpawnRopeBeams(iClient);
		CPrintToChat(iClient, "{fullred}Please type {green}%s 1{fullred} in console in order to see ropes correctly! Add {green}%s 1{fullred} at the very end of your cfg/autoexec.cfg and restart your game.", cvarName, cvarName);
	}
}

void CheckClientDownloadCvar(int iClient)
{
	QueryClientConVar(iClient, "cl_downloadfilter", ProcessClientDownloadCvar);
}

void ProcessClientDownloadCvar(QueryCookie cookie, int iClient, ConVarQueryResult result, const char[] cvarName, const char[] cvarValue)
{
	if (!StrEqual(cvarValue, "all"))
		CPrintToChat(iClient, "{fullred}Please allow all downloads in the multiplayer options menu!", cvarName);
}

/**
 * Description
 * 
 * @param cookie            The preference cookie to change
 * @param iClient           The client that wants their preference changed
 * @param iDesiredValue     The value to set the cookie. Leave null to toggle the preference
 * @return                  Returns whether or not the value was modified
 */
bool SetClientCookiePreference(Cookie cookie, int iClient, char[] DesiredValue = { '\0' }) {
	bool Enabled = !!GetCookieInt(cookie, iClient);
	if(DesiredValue[0] != '\0') {
		bool CurrValue = !!GetCookieInt(cookie, iClient),
        NewValue = !!StringToInt(DesiredValue);

		if(CurrValue == NewValue)
			return false;

		cookie.Set(iClient, DesiredValue);
	}
	else
		cookie.Set(iClient, Enabled ? "0" : "1");

	return true;
}

public Action EnableMusic(int iClient, int iArgs)
{
	char Arg1[2];
	GetCmdArg(1, Arg1, sizeof(Arg1));
	if(SetClientCookiePreference(g_cookieMusic, iClient, Arg1)) {
		bool Value = !!GetCookieInt(g_cookieMusic, iClient);
		Value ? CPFSoundController.SwitchMusic(iClient, true) : CPFSoundController.StopAllMusic(iClient);
		ReplyToCommand(iClient, "%s Parkour Music", Value ? "Enabled" : "Disabled");
	}
	
	return Plugin_Handled;
}

public Action ChangeMusicVolume(int iClient, int iArgs)
{
	char MusicVol[8];
	bool bDecimal;
	
	if (GetCmdArg(1, MusicVol, sizeof(MusicVol)) == 0)
	{
		char MusicVolumeCookie[8];
		g_cookieMusicVolume.Get(iClient, MusicVolumeCookie, sizeof(MusicVolumeCookie));
		ReplyToCommand(iClient, "Music Volume: %s", MusicVolumeCookie);
		return Plugin_Handled;
	}
	
	for(int i; i < strlen(MusicVol); i++)
	{
		if(MusicVol[i] == '.' && !bDecimal)
		{
			bDecimal = true;
			continue;
		}
		
		if (IsCharNumeric(MusicVol[i]))
			continue;

		ReplyToCommand(iClient, "Invalid music volume input");
		return Plugin_Handled;
	}
	
	if (StringToFloat(MusicVol) > 1.0)
		MusicVol = "1.0";
		
	g_cookieMusicVolume.Set(iClient, MusicVol);
	ReplyToCommand(iClient, "Music Volume: %s", MusicVol);
	return Plugin_Handled;
}

public Action EnableSound(int iClient, int iArgs)
{
	char Arg1[2];
	GetCmdArg(1, Arg1, sizeof(Arg1));
	if(SetClientCookiePreference(g_cookieSound, iClient, Arg1))
		ReplyToCommand(iClient, "%s Parkour Sounds", GetCookieInt(g_cookieSound, iClient) ? "Enabled" : "Disabled");
	
	return Plugin_Handled;
}

public Action EnableSelfAmbientSound(int iClient, int iArgs)
{
	char Arg1[2];
	GetCmdArg(1, Arg1, sizeof(Arg1));
	if(SetClientCookiePreference(g_cookieSelfAmbientSound, iClient, Arg1))
		ReplyToCommand(iClient, "%s Self Parkour Sounds", GetCookieInt(g_cookieSelfAmbientSound, iClient) ? "Enabled" : "Disabled");
	
	return Plugin_Handled;
}

public Action EnableViewmodel(int iClient, int iArgs)
{
	char Arg1[2];
	GetCmdArg(1, Arg1, sizeof(Arg1));
	if(SetClientCookiePreference(g_cookieViewmodel, iClient, Arg1))
		ReplyToCommand(iClient, "%s Parkour Viewmodel", GetCookieInt(g_cookieViewmodel, iClient) ? "Enabled" : "Disabled");
	
	return Plugin_Handled;
}

public Action EnableLerp(int iClient, int iArgs)
{
	char Arg1[2];
	GetCmdArg(1, Arg1, sizeof(Arg1));
	if(SetClientCookiePreference(g_cookieLerp, iClient, Arg1))
		ReplyToCommand(iClient, "%s Screen Tilt", GetCookieInt(g_cookieLerp, iClient) ? "Enabled" : "Disabled");

	return Plugin_Handled;
}

public Action UserMsg_VoiceSubtitle(UserMsg eID, BfRead hMsg, const int[] iPlayers, int iPlayerNum, bool bReliable, bool bInit)
{
	return Plugin_Handled;
}

public Action Command_VoiceMenu(int iClient, const char[] sCommand, int iArgs)
{
	if (!IsValidLivingClient(iClient))
		return Plugin_Continue;
		
	if (iArgs < 2) return Plugin_Handled;
	
	char sArg1[32];
	char sArg2[32];
	GetCmdArg(1, sArg1, sizeof(sArg1));
	GetCmdArg(2, sArg2, sizeof(sArg2));
	
	//Capture call for medic commands (represented by "voicemenu 0 0").
	if (sArg1[0] == '0' && sArg2[0] == '0')
	{
		//If an item was succesfully grabbed
		if (AttemptGrabItem(iClient))
			return Plugin_Handled;
	}
	
	return Plugin_Continue;
}

public Action DebugBBox(int iClient, int iArgs)
{
	float vecMaxs[3], vecMins[3];
	GetEntPropVector(iClient, Prop_Data, "m_vecMaxs", vecMaxs);
	GetEntPropVector(iClient, Prop_Data, "m_vecMins", vecMins);

	ReplyToCommand(iClient, "%.1f %.1f %.1f %.1f %.1f %.1f", vecMins[0], vecMins[1], vecMins[2], vecMaxs[0], vecMins[1], vecMins[2]);
	return Plugin_Continue;
}

public Action DebugCoords(int iClient, int iArgs)
{
	if (!IsValidClient(iClient))
		return Plugin_Handled;
	
	char strArg[16];
	float vecOrigin[3];
	
	for (int i = 0; i < 3; i++)
	{
		GetCmdArg(i+1, strArg, sizeof(strArg));
		vecOrigin[i] = StringToFloat(strArg);
	}

	TeleportEntity(iClient, vecOrigin, NULL_VECTOR, NULL_VECTOR);
	return Plugin_Handled;
}

public Action RestartTutorial(int iClient, int iArgs)
{
	if (!IsValidClient(iClient))
		return Plugin_Handled;
	
	if (!IsPlayerAlive(iClient) || TF2_GetClientTeam(iClient) == TFTeam_Spectator || TF2_GetClientTeam(iClient) == TFTeam_Unassigned)
	{
		CReplyToCommand(iClient, "{red}You must be alive and on a team to start the tutorial!");
		return Plugin_Handled;
	}
	
	if(hTutorialTimer[iClient] != INVALID_HANDLE)
		delete hTutorialTimer[iClient];
	hTutorialTimer[iClient] = CreateTimer(1.0, DisplayTutorialScreen, iClient, TIMER_REPEAT);
	CPFTutorialController.Restart(iClient);
	
	return Plugin_Handled;
}

public Action SkipTutorial(int iClient, int iArgs)
{
	if (!IsValidClient(iClient))
		return Plugin_Handled;
	
	if (!IsPlayerAlive(iClient) || TF2_GetClientTeam(iClient) == TFTeam_Spectator || TF2_GetClientTeam(iClient) == TFTeam_Unassigned)
	{
		CReplyToCommand(iClient, "{red}You must be alive and on a team to skip the tutorial!");
		return Plugin_Handled;
	}
	
	CPFTutorialController.Complete(iClient);
	CPFTutorialController.ClearOverlay(iClient);
	return Plugin_Continue;
}

public void PFTeleportPlayer(int iClient, const float origin[3], const float angles[3], const float velocity[3])
{
	if (!IsNullVector(origin))
	{
		SetEntPropVector(iClient, Prop_Data, "m_vecOrigin", origin);
	}
	
	if (!IsNullVector(angles))
	{
		ServerCommand("script PlayerInstanceFromIndex(%i).SnapEyeAngles(QAngle(%f, %f, %f));", iClient, angles[0], angles[1], angles[2]);
	}
	
	if (!IsNullVector(velocity))
	{
		SetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", velocity);
	}
}

public void OnSuccessfulTeleport(int iClient)
{
	if (!IsValidClient(iClient) || !IsPlayerAlive(iClient) || TF2_GetClientTeam(iClient) == TFTeam_Spectator || TF2_GetClientTeam(iClient) == TFTeam_Unassigned)
		return;
	
	PFState eState = CPFStateController.Get(iClient);
	
	DebugOutput("OnSuccessfulTeleport --- %N", iClient);
	
	switch(eState)
	{
		case State_Roll:
		{
			DebugOutput("OnSuccessfulTeleport --- Disengaging roll");
			CPFRollHandler.Disengage(iClient);
		}
		
		case State_Hang:
		{
			DebugOutput("OnSuccessfulTeleport --- Disengaging hang");
			CPFHangHandler.Disengage(iClient, LEDGEGRAB_DISENGAGE_CROUCH);
		}
		
		case State_Climb:
		{
			DebugOutput("OnSuccessfulTeleport --- Disengaging climb");
			CPFClimbHandler.Disengage(iClient, CLIMB_DISENGAGE_LEAVETRIGGER);
		}
		
		case State_Rail:
		{
			DebugOutput("OnSuccessfulTeleport --- Disengaging rail");
			CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_TELEPORT);
		}
		
		default:
		{
			DebugOutput("OnSuccessfulTeleport --- Client state at restart: %d", view_as<int>(eState));
		}
	}
	
	SetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", view_as<float>({0.0, 0.0, 0.0}));
	
	CPFViewController.Kill(iClient);
	CPFViewController.Spawn(iClient);
	
	TF2_RemoveCondition(iClient, TFCond_Bonked);
	
	return;
}

public void OnMapStart()
{
	CPFSoundController.Init();
	PrecacheModels();
	ClearTutorials();
	Weapons_Refresh();
	CreateTimer(1.0, OnMapStartFrame);
}

void ClearTutorials()
{
	for (int i = 0; i < MaxClients + 1; i++)
		g_bTutorialFetched[i] = false;
}

void PrecacheModels()
{
	/** --- Internal --- **/
	/* Zipline beams? */
	g_iTEBeams = PrecacheModel("materials/sprites/laser.vmt", true);
	PrecacheModel("materials/sprites/laserbeam.vmt", true);
	PrecacheModel("materials/sprites/old_xfire.vmt", true);
	PrecacheModel("materials/sprites/glow.vmt", true);
	PrecacheModel("materials/sprites/light_glow03.vmt", true);
	PrecacheModel("materials/cable/cable.vmt", true);
	PrecacheModel("materials/cable/rope.vmt", true);
	
	/** --- External --- **/
	/* Legacy Props */
	// AC Unit
	SuperPrecacheModel("models/parkoursource/air_condition_unit.mdl");
	SuperPrecacheMaterial("models/parkoursource/acunit_box", true);
	PrecacheGeneric("materials/models/parkoursource/acunit_box_spec.vtf", true);
	AddFileToDownloadsTable("materials/models/parkoursource/acunit_box_spec.vtf");
	
	// AC Unit (Wall)
	SuperPrecacheModel("models/parkoursource/air_condition_unit_wall.mdl");
	SuperPrecacheMaterial("models/parkoursource/acunit_wall", true);
	PrecacheGeneric("materials/models/parkoursource/acunit_wall_bump.vtf", true);
	AddFileToDownloadsTable("materials/models/parkoursource/acunit_wall_bump.vtf");
	
	// Awning
	SuperPrecacheModel("models/parkoursource/awning.mdl");
	SuperPrecacheMaterial("models/parkoursource/awning", true);
	SuperPrecacheMaterial("models/parkoursource/awning_blue", true);
	SuperPrecacheMaterial("models/parkoursource/awning_green", true);
	SuperPrecacheMaterial("models/parkoursource/awning_purple", true);
	SuperPrecacheMaterial("models/parkoursource/awning_red", true);
	SuperPrecacheMaterial("models/parkoursource/awning_yellow", true);
	
	// Billboard
	SuperPrecacheModel("models/parkoursource/billboard.mdl");
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_02", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_03", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_04", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_05", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_06", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_07", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_08", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_09", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_10", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_11", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_12", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_13", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_17", true);
	SuperPrecacheMaterial("models/parkoursource/billboard/billboard_empty", true);
	
	// Door
	SuperPrecacheModel("models/parkoursource/door_skybox.mdl");
	SuperPrecacheModel("models/parkoursource/door_standard.mdl");
	SuperPrecacheMaterial("models/parkoursource/door_e_96", true);
	SuperPrecacheMaterial("models/parkoursource/door_e_96_blue", true);
	SuperPrecacheMaterial("models/parkoursource/door_e_96_green", true);
	SuperPrecacheMaterial("models/parkoursource/door_e_96_white", true);
	PrecacheGeneric("materials/models/parkoursource/door_e_96_g.vtf", true);
	AddFileToDownloadsTable("materials/models/parkoursource/door_e_96_g.vtf");
	
	// Fan Vent
	SuperPrecacheModel("models/parkoursource/fan_vent.mdl");
	SuperPrecacheMaterial("models/parkoursource/fanvent_6", true);
	PrecacheGeneric("materials/models/parkoursource/fanvent_6_spec.vtf", true);
	AddFileToDownloadsTable("materials/models/parkoursource/fanvent_6_spec.vtf");
	
	// Pipe
	SuperPrecacheModel("models/parkoursource/pipe_runner.mdl");
	SuperPrecacheModel("models/parkoursource/pipe_standard.mdl");
	SuperPrecacheMaterial("models/parkoursource/pipe192", true);
	SuperPrecacheMaterial("models/parkoursource/pipe192_blue", true);
	SuperPrecacheMaterial("models/parkoursource/pipe192_gray", true);
	PrecacheGeneric("materials/models/parkoursource/pipe_bump.vtf", true);
	AddFileToDownloadsTable("materials/models/parkoursource/pipe_bump.vtf");
	
	// Zipwire Tower
	SuperPrecacheModel("models/parkoursource/zipwire_tower.mdl");
	SuperPrecacheMaterial("models/parkoursource/zipwire_tower", true);
	SuperPrecacheMaterial("models/parkoursource/zipwire_tower_blue", true);
	
	/* New Props */
	
	// Billboard
	SuperPrecacheModel("models/reduxsource/billboard.mdl");
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_02", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_03", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_04", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_05", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_06", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_07", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_08", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_09", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_10", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_11", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_12", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_13", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_17", true);
	SuperPrecacheMaterial("models/reduxsource/billboard/billboard_empty", true);
	
	// Door + Door Static
	SuperPrecacheModel("models/reduxsource/door_standard.mdl");
	SuperPrecacheModel("models/reduxsource/door_standard_static.mdl");
	SuperPrecacheMaterial("models/reduxsource/door_e_96", true);
	SuperPrecacheMaterial("models/reduxsource/door_e_96_2", true);
	SuperPrecacheMaterial("models/reduxsource/door_e_96_blue", true);
	SuperPrecacheMaterial("models/reduxsource/door_e_96_green", true);
	SuperPrecacheMaterial("models/reduxsource/door_e_96_white", true);
	PrecacheGeneric("materials/models/reduxsource/door_e_96_g.vtf", true);
	AddFileToDownloadsTable("materials/models/reduxsource/door_e_96_g.vtf");
	
	// Pipe
	SuperPrecacheModel("models/reduxsource/pipe_standard.mdl");
	SuperPrecacheMaterial("models/reduxsource/pipe192_blue", true);
	SuperPrecacheMaterial("models/reduxsource/pipe192_gray", true);
	SuperPrecacheMaterial("models/reduxsource/pipe192_kir_green", true);
	SuperPrecacheMaterial("models/reduxsource/pipe192_kir_stripes", true);
	PrecacheGeneric("materials/models/reduxsource/pipe_bump.vtf", true);
	AddFileToDownloadsTable("materials/models/reduxsource/pipe_bump.vtf");
	PrecacheGeneric("materials/models/reduxsource/pipe192_unlit.vmt", true);
	AddFileToDownloadsTable("materials/models/reduxsource/pipe192_unlit.vmt");
	PrecacheGeneric("materials/models/reduxsource/pipe192.vtf", true);
	AddFileToDownloadsTable("materials/models/reduxsource/pipe192.vtf");
	
	// Zipwire Tower
	SuperPrecacheModel("models/reduxsource/zipwire_tower.mdl");
	SuperPrecacheMaterial("models/reduxsource/zipwire_tower", true);
	SuperPrecacheMaterial("models/reduxsource/zipwire_tower_blue", true);
	SuperPrecacheMaterial("models/reduxsource/zipwire_tower_kir_green", true);
}

Action OnMapStartFrame(Handle hTimer)
{
	if (!g_bLate)
	{
		PrintToServer("Initializing PF Objects");
		InitObjects(true);
		InitSDK();
		InitWeapons();
		InitOther();
	}
	
	//Comment - This doesn't work
	FindConVar("tf_weapon_criticals").SetBool(!g_cvarPvP.BoolValue);
	FindConVar("tf_weapon_criticals_melee").SetBool(!g_cvarPvP.BoolValue);
	FindConVar("tf_use_fixed_weaponspreads").SetBool(g_cvarPvP.BoolValue);

	g_bMapLoaded = true;

	return Plugin_Continue;
}

public void Mapvote_OnPvPMap()
{
	Weapons_Refresh();
}

public void OnMapEnd()
{
	g_bMapLoaded = false;
	g_bLate = false;
	
	RemoveMaxSpeedPatch();
}

public Action WeaponReload(int client, int args)
{
	Weapons_Refresh();
	return Plugin_Handled;
}

public Action DebugReach(int client, int args)
{
	ReplyToCommand(client, "Got it! %d %d %d", CPFRopeController.Total(), CPFDoorController.Total(), CPFPipeController.Total());
	return Plugin_Handled;
}

void InitSDK()
{
	GameData hGameData = new GameData("parkourdata"); 
	if (hGameData == null)
		SetFailState("Failed to load gamedata parkourdata.txt");

	StartPrepSDKCall(SDKCall_Raw);
	PrepSDKCall_SetFromConf(hGameData, SDKConf_Virtual, "CBaseEntity::GetBaseEntity");
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	g_hSDKGetBaseEntity = EndPrepSDKCall();

	//This hook calls when someone won a round
	int iOffset = GameConfGetOffset(hGameData, "CTeamplayRoundBasedRules::SetWinningTeam"); 
	g_hHookSetWinningTeam = DHookCreate(iOffset, HookType_GameRules, ReturnType_Void, ThisPointer_Ignore);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Int);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Int);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Bool);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Bool);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Bool);
	DHookAddParam(g_hHookSetWinningTeam, HookParamType_Bool);

	iOffset = GameConfGetOffset(hGameData, "CTFPlayer::GiveNamedItem"); 
	g_hHookGiveNamedItem = DHookCreate(iOffset, HookType_Entity, ReturnType_CBaseEntity, ThisPointer_CBaseEntity);
	DHookAddParam(g_hHookGiveNamedItem, HookParamType_CharPtr);
	DHookAddParam(g_hHookGiveNamedItem, HookParamType_Int);
	DHookAddParam(g_hHookGiveNamedItem, HookParamType_ObjectPtr);
	DHookAddParam(g_hHookGiveNamedItem, HookParamType_Bool);
	
	char strBuf[4];
	hGameData.GetKeyValue("CGameMovement::player", strBuf, sizeof(strBuf));
	offsets.player = StringToInt(strBuf);
	
	g_hSDKAirAccelerate = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_Address);
	DHookSetFromConf(g_hSDKAirAccelerate, hGameData, SDKConf_Signature, "CGameMovement::AirAccelerate");
	DHookAddParam(g_hSDKAirAccelerate, HookParamType_VectorPtr);
	DHookAddParam(g_hSDKAirAccelerate, HookParamType_Float);
	DHookAddParam(g_hSDKAirAccelerate, HookParamType_Float);
	DHookEnableDetour(g_hSDKAirAccelerate, false, AirAccelerate);
	
	g_hSDKAccelerate = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_Address);
	DHookSetFromConf(g_hSDKAccelerate, hGameData, SDKConf_Signature, "CGameMovement::Accelerate");
	DHookAddParam(g_hSDKAccelerate, HookParamType_VectorPtr);
	DHookAddParam(g_hSDKAccelerate, HookParamType_Float);
	DHookAddParam(g_hSDKAccelerate, HookParamType_Float);
	DHookEnableDetour(g_hSDKAccelerate, false, Accelerate);

	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetFromConf(hGameData, SDKConf_Signature, "CTFPlayer::GetMaxAmmo");
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_Plain);
	g_hSDKGetMaxAmmo = EndPrepSDKCall();
	if (g_hSDKGetMaxAmmo == null)
		LogMessage("Failed to create call: CTFPlayer::GetMaxAmmo!");

	//This function is used to get wearable equipped in loadout slots
	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetFromConf(hGameData, SDKConf_Signature, "CTFPlayer::GetEquippedWearableForLoadoutSlot");
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_Plain);
	PrepSDKCall_SetReturnInfo(SDKType_CBaseEntity, SDKPass_Pointer);
	g_hSDKGetEquippedWearable = EndPrepSDKCall();
	if (g_hSDKGetEquippedWearable == null)
		LogMessage("Failed to create call: CTFPlayer::GetEquippedWearableForLoadoutSlot!");

	delete hGameData;
	hGameData = new GameData("sm-tf2.games");
	if (hGameData == null)
		SetFailState("Could not find sm-tf2.games gamedata!");
	
	int iRemoveWearableOffset = GameConfGetOffset(hGameData, "RemoveWearable"); 
	//This function is used to remove a player wearable properly
	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetVirtual(iRemoveWearableOffset);
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	g_hSDKRemoveWearable = EndPrepSDKCall();
	if(g_hSDKRemoveWearable == null)
		LogMessage("Failed to create call: CBasePlayer::RemoveWearable!");
	
	//This function is used to equip wearables
	StartPrepSDKCall(SDKCall_Player);
	PrepSDKCall_SetVirtual(iRemoveWearableOffset-1);// Assume EquipWearable is always behind RemoveWearable
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer);
	g_hSDKEquipWearable = EndPrepSDKCall();
	if(g_hSDKEquipWearable == null)
		LogMessage("Failed to create call: CBasePlayer::EquipWearable!");
		
	delete hGameData;
}

public MRESReturn Client_OnGiveNamedItem(int iClient, Handle hReturn, Handle hParams)
{
    // Block if one of the pointers is null
    if (DHookIsNullParam(hParams, 1) || DHookIsNullParam(hParams, 3))
    {
        DHookSetReturn(hReturn, 0);
        return MRES_Supercede;
    }    
    
    char sClassname[64];
    Address ClassnameAddress = DHookGetParamAddress(hParams, 1);
    
    PtrToString(view_as<int>(ClassnameAddress), sClassname, sizeof(sClassname));
    
    int iIndex = DHookGetParamObjectPtrVar(hParams, 3, 4, ObjectValueType_Int) & 0xFFFF;
    
    Action iAction = OnGiveNamedItem(sClassname, iIndex);
    
    if (iAction == Plugin_Handled)
    {
        DHookSetReturn(hReturn, 0);
        return MRES_Supercede;
    }
    
    return MRES_Ignored;
}

public void DHook_OnGiveNamedItemRemoved(int iHookId)
{
	for (int iClient = 1; iClient <= MaxClients; iClient++)
	{
		if (g_iHookIdGiveNamedItem[iClient] == iHookId)
		{
			g_iHookIdGiveNamedItem[iClient] = 0;
			return;
		}
	}
}

public MRESReturn AirAccelerate(Address pThis, Handle hParams)
{
	DHookSetParam(hParams, 3, g_flAirAccel[view_as<CGameMovement>(pThis).player]);
	return MRES_ChangedOverride;
}

public MRESReturn Accelerate(Address pThis, Handle hParams)
{
	DHookSetParam(hParams, 3, g_flAccel[view_as<CGameMovement>(pThis).player]);
	return MRES_ChangedOverride;
}

void SetPlayerAirAccel(int iClient, float flValue)
{
	if (flValue > 0.0)
		g_flAirAccel[iClient] = flValue;
}

float GetPlayerAirAccel(int iClient)
{
	return g_flAirAccel[iClient];
}

void SetPlayerAccel(int iClient, float flValue)
{
	if (flValue > 0.0)
		g_flAccel[iClient] = flValue;
}

// Thanks FlaminSarge
void ApplyMaxSpeedPatch()
{
	g_pPatchLocation = Address_Null;
	g_iRestoreData = 0;

	Handle hGameData = LoadGameConfigFile("tf.maxspeed");
	if (hGameData == INVALID_HANDLE)
	{
		LogError("Failed to load maxspeed patch: Missing gamedata/tf.maxspeed.txt");
		return;
	}

	g_pPatchLocation = GameConfGetAddress(hGameData, "CTFGameMovement::ProcessMovement_limit");
	if (g_pPatchLocation == Address_Null)
	{
		LogError("Failed to load maxspeed patch: Failed to locate \"CTFGameMovement::ProcessMovement_limit\"");
		delete hGameData;
		return;
	}
	
	delete hGameData;

	g_iRestoreData = LoadFromAddress(g_pPatchLocation, NumberType_Int32);
	if (view_as<float>(g_iRestoreData) != DEFAULT_MAXSPEED)
	{
		LogError("Value at (0x%.8X) was not expected: (%.4f) != %.1f. Cowardly refusing to do things.", g_pPatchLocation, g_iRestoreData, DEFAULT_MAXSPEED);
		g_iRestoreData = 0;
		g_pPatchLocation = Address_Null;
		return;
	}
	
	LogMessage("Patching ProcessMovement data at (0x%.8X) from (%.4f) to (%.4f).", g_pPatchLocation, view_as<float>(g_iRestoreData), g_flMaxSpeedVal);
	StoreToAddress(g_pPatchLocation, view_as<int>(g_flMaxSpeedVal), NumberType_Int32);
}

void RemoveMaxSpeedPatch() {
	if (g_pPatchLocation == Address_Null || g_iRestoreData <= 0)
		return;

	LogMessage("Restoring ProcessMovement data at (0x%.8X) to (%.4f).", g_pPatchLocation, view_as<float>(g_iRestoreData));
	StoreToAddress(g_pPatchLocation, g_iRestoreData, NumberType_Int32);

	g_pPatchLocation = Address_Null;
	g_iRestoreData = 0;
}

Action OnGiveNamedItem(char[] sClassname, int iItem)
{
	if (StrContains(sClassname, "tf_wearable") == 0) // starts with tf_wearable
	{
		if (StrEqual(sClassname, "tf_wearable_demoshield") || StrEqual(sClassname, "tf_wearable_razorback") || StrEqual(sClassname, "tf_wearable_robot_arm"))
			return Plugin_Handled;
		return Plugin_Continue;
	}
	
	if (StrEqual(sClassname, "tf_weapon_fists") && iItem == WEAPON_FISTS) // regular fists and not something like GRU
		return Plugin_Continue;
	
	return Plugin_Handled; // get outta here
}

public MRESReturn CBaseEntity_KeyValue(int iEnt, Handle hParams)
{
	if (!g_bMapLoaded || !IsValidEntity(iEnt))
		return MRES_Ignored;

	char strClassname[128];
	if (!GetEntityClassname(iEnt, strClassname, sizeof(strClassname)))
		return MRES_Ignored;
	
	if (!(StrEqual(strClassname, "trigger_multiple") || !strncmp("prop_", strClassname, 5) || StrEqual(strClassname, "move_rope")))
		return MRES_Ignored;

	char strKey[128], strValue[128];
	DHookGetParamString(hParams, 1, strKey, sizeof(strKey));
	DHookGetParamString(hParams, 2, strValue, sizeof(strValue));
	
	if (StrEqual(strKey, "targetname") && !strncmp("trigger_", strClassname, 8))
	{
		char strTargetname[256];
		GetEntPropString(iEnt, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrEqual("climbable", strTargetname))
		{
			CPFPipe hPipe = new CPFPipe(iEnt);
			if (hPipe != null)
				CPFPipeController.AddPipe(hPipe);
		}
	}
	else if (StrEqual(strKey, "modelname") && !strncmp("prop_", strClassname, 5) && CPFDoorController.IsDoor(iEnt))
	{
		CPFDoor hDoor = new CPFDoor(iEnt);
		if (hDoor != null)
			CPFDoorController.AddDoor(hDoor);
	}
	else if (StrEqual(strKey, "targetname") && StrEqual("move_rope", strClassname) && CPFRopeController.IsRopeStart(iEnt))
	{
		CPFRope hRope = new CPFRope(iEnt);
		if (hRope != null)
		{
			CPFRopeController.AddRope(hRope);
			RequestFrame(ProcessRadialsPostKeyValue);
		}
	}
	
	return MRES_Ignored;
}

stock int SDK_GetMaxAmmo(int iClient, int iSlot)
{
	if (g_hSDKGetMaxAmmo != null)
		return SDKCall(g_hSDKGetMaxAmmo, iClient, iSlot, -1);
	
	return -1;
}

stock void SDK_EquipWearable(int iClient, int iWearable)
{
	if (g_hSDKEquipWearable != null)
		SDKCall(g_hSDKEquipWearable, iClient, iWearable);
}

stock int SDK_GetEquippedWearable(int iClient, int iSlot)
{
	if (g_hSDKGetEquippedWearable != null)
		return SDKCall(g_hSDKGetEquippedWearable, iClient, iSlot);
	
	return -1;
}

void InitOther()
{
	for(int iClient = 1; iClient < MaxClients + 1; iClient++) {
		if(IsValidClient(iClient))
			OnClientPutInServer(iClient);
	}
	
	CPFSoundController.Init();
	ResetAirAccel();
	
	SDKHookClassname("trigger_stun", SDKHook_StartTouch, OnStartTouchTrigger);
	SDKHookClassname("trigger_once", SDKHook_StartTouch, OnStartTouchTrigger);
	SDKHookClassname("trigger_multiple", SDKHook_StartTouch, OnStartTouchTrigger);
	SDKHookClassname("trigger_catapult", SDKHook_StartTouch, OnStartTouchTrigger);
	SDKHookClassname("trigger_push", SDKHook_StartTouch, OnStartTouchTrigger);
	SDKHookClassname("trigger_hurt", SDKHook_StartTouch, OnStartTouchTrigger);

	SDKHookClassname("trigger_stun", SDKHook_EndTouch, OnEndTouchTrigger);
	SDKHookClassname("trigger_once", SDKHook_EndTouch, OnEndTouchTrigger);
	SDKHookClassname("trigger_multiple", SDKHook_EndTouch, OnEndTouchTrigger);
	SDKHookClassname("trigger_catapult", SDKHook_EndTouch, OnEndTouchTrigger);
	SDKHookClassname("trigger_push", SDKHook_EndTouch, OnEndTouchTrigger);
	SDKHookClassname("trigger_hurt", SDKHook_EndTouch, OnEndTouchTrigger);
	
	HookEntityOutput("prop_dynamic", "OnAnimationBegin", OnAnimationBegin);
	HookEntityOutput("prop_dynamic", "OnAnimationDone", OnAnimationDone);
}

void InitObjects(bool bForce = false)
{
	for(int iEntity = 0; iEntity <= GetMaxEntities(); iEntity++)
	{
		if (IsValidEntity(iEntity))
		{
			char strEntName[64];
			GetEntPropString(iEntity, Prop_Data, "m_iName", strEntName, sizeof(strEntName));

			if (StrEqual("pf_railsprite", strEntName))
				RemoveEntity(iEntity);
		}
	}

	CPFRopeController.Init(bForce);
	CPFPipeController.Init(bForce);
	CPFDoorController.Init(bForce);
}

void InitMovements()
{
	if (g_bLate)
	{
		CPFTraceur.Init();
		CPFSpeedController.Init();
		
		for (int i = 1; i < MaxClients; i++)
		{
			if (!IsValidClient(i)) continue;
			SDKUnhook(i, SDKHook_PreThink, OnPreThink);
			SDKUnhook(i, SDKHook_PostThink, OnPostThink);
			SDKUnhook(i, SDKHook_WeaponSwitch, OnWeaponSwitch);
			SDKHook(i, SDKHook_PreThink, OnPreThink);
			SDKHook(i, SDKHook_PostThink, OnPostThink);
			SDKHook(i, SDKHook_WeaponSwitch, OnWeaponSwitch);
		}
	}
}

//public void OnPlayerRunCmdPost(int client, int buttons, int impulse, const float vel[3], const float angles[3], int weapon, int subtype, int cmdnum, int tickcount, int seed, const int mouse[2])
public void OnGameFrame()
{
	CPFZiplineHandler.OnGameFrame();
	
	if (CPFRopeController.HasRails())
		CPFRailHandler.OnGameFrame();
}

public void OnClientPutInServer(int iClient) {
	CPFTutorialController.InitPlayer(iClient);
	CPFSoundController.InitPlayer(iClient);
}

public void OnClientPostAdminCheck(int iClient)
{
	if (!IsValidClient(iClient)) return;
	
	SetPlayerAirAccel(iClient, g_cvarAirAcceleration.FloatValue);
	SetPlayerAccel(iClient, g_cvarAcceleration.FloatValue);
	
	if (!IsFakeClient(iClient))
		g_iHookIdGiveNamedItem[iClient] = DHookEntity(g_hHookGiveNamedItem, true, iClient, DHook_OnGiveNamedItemRemoved, Client_OnGiveNamedItem);
}

public void OnConfigsExecuted()
{
	ApplyMaxSpeedPatch();
}

public void OnChangePvP(ConVar cvarTime, const char[] strOldValue, const char[] strNewValue)
{
	PrintToServer("%i", StringToInt(strNewValue));
	FindConVar("tf_weapon_criticals").BoolValue = !!!StringToInt(strNewValue);
	FindConVar("tf_weapon_criticals_melee").BoolValue = !!!StringToInt(strNewValue);
	FindConVar("tf_use_fixed_weaponspreads").BoolValue = !!StringToInt(strNewValue);
}

public Action OnPlayerSpawn(Event hEvent, const char[] strName, bool bDontBroadcast)
{
	CPFTraceur iClient = CPFTraceur(GetClientOfUserId(hEvent.GetInt("userid")));
	if (iClient == CPFTRACEUR_INVALID)
		return Plugin_Continue;
	
	// Only include functions here that can't be in CPFTraceur::Spawn
	
	iClient.Spawn();
	CPFStateController.AddFlags(view_as<int>(iClient), SF_SPAWNING);
	CPFSpeedController.SetSpeed(view_as<int>(iClient), 250.0);
	CPFSpeedController.SetStoredSpeed(view_as<int>(iClient), 250.0);
	CPFSoundController.SetIntensity(view_as<int>(iClient), 0.0);

	RequestFrame(OnPlayerSpawn_Post, EntIndexToEntRef(view_as<int>(iClient)));

#if defined _PFVIEWMODEL_INCLUDED
	CPFViewController.Spawn(view_as<int>(iClient));
#endif

	return Plugin_Continue;
}

public void OnPlayerSpawn_Post(int iData)
{
	int iClient = EntRefToEntIndex(iData);
	
#if defined _PF_INCLUDED
	if (g_bTutorialFetched[iClient] && CPFTutorialController.GetStage(iClient) != TUTORIAL_COMPLETE)
		CPFTutorialController.Restart(iClient);
#endif
	
	CPFStateController.RemoveFlags(iClient, SF_SPAWNING);
}


#if defined _PF_INCLUDED
public void Tutorial_OnGetPlayerStage(int iClient, TutorialStage eStage)
{
	g_bTutorialFetched[iClient] = true;
	
	if (eStage != TUTORIAL_COMPLETE)
		CPFTutorialController.Restart(iClient);
}
#endif

public void GiveFists(int iClient)
{
	int iWeapon = TF2_CreateAndEquipWeapon(iClient, WEAPON_FISTS, "1 ; 0.80");
	if(!g_cvarPvP.BoolValue)
		SetEntProp(iWeapon, Prop_Send, "m_iAccountID", GetSteamAccountID(iClient));
	SetCollisionGroup(iWeapon, COLLISION_GROUP_NONE);
}

public Action OnInventoryPost(Event hEvent, const char[] strName, bool bDontBroadcast)
{
	int iClient = GetClientOfUserId(hEvent.GetInt("userid"));
	
	CheckClientWeapons(iClient);
	
	return Plugin_Continue;
}

public Action OnRoundStart(Event hEvent, const char[] strName, bool bDontBroadcast)
{
	InitObjects(true);
	Mapvote_OnMapsLoaded();

	return Plugin_Continue;
}

public Action OnRoundEnd(Event hEvent, const char[] strName, bool bDontBroadcast)
{
	CPFViewController.KillAll();
	return Plugin_Continue;
}

public void TF2_OnWaitingForPlayersEnd()
{
	for (int i = 1; i < MaxClients; i++)
		CPFRollHandler.Disengage(i);
}

public Action OnWeaponSwitch(int iClient, int iNewWeapon)
{
	int iCurWeapon = GetEntPropEnt(iClient, Prop_Data, "m_hActiveWeapon");
	DebugOutput("prev weapon: %i", iCurWeapon);
	if (IsValidEntity(iCurWeapon))
	{
		if (GetEntProp(iCurWeapon, Prop_Send, "m_iItemDefinitionIndex") == 1152 && CPFStateController.HasFlags(iClient, SF_STRIPHOOKSHOT)) //grappling hook
		{
			TF2_RemoveWeaponSlot(iClient, 5);
			CPFStateController.RemoveFlags(iClient, SF_STRIPHOOKSHOT);
		}
		
		if (!HasAnyAmmo(iClient, iCurWeapon)) //(!SDK_GetAnyAmmo(iCurWeapon))
		{
			DebugOutput("killing weapon %i", iCurWeapon);
			RemovePlayerItem(iClient, iCurWeapon);
			AcceptEntityInput(iCurWeapon, "Kill");
		}
	}
	
	if (!IsValidEntity(iNewWeapon)) return Plugin_Continue;
	
	char sClassname[128];
	if (!GetEntityClassname(iNewWeapon, sClassname, sizeof(sClassname))) return Plugin_Continue;
	
	bool isFists = (StrEqual(sClassname, "tf_weapon_fists", false));
	
	SetEntProp(iClient, Prop_Send, "m_bDrawViewmodel", !(isFists));
	switch (isFists)
	{
		case true:
		{
			CPFViewController.SetHidden(iClient, false);
			CPFViewController.Unhide(iClient, false);
		}
		case false:
		{
			CPFViewController.Hide(iClient);
			CPFViewController.SetHidden(iClient, true);
			
			if (StrContains(sClassname, "tf_weapon_sniperrifle") == 0)
			{
				CPFSpeedController.SetBoost(iClient, false);
				CPFSpeedController.SetStoredSpeed(iClient, fMin(CPFSpeedController.GetSpeed(iClient), 400.0));
				CPFSpeedController.SetSpeed(iClient, fMin(CPFSpeedController.GetSpeed(iClient), 400.0));
			}
		}
	}

	return Plugin_Continue;
}

public Action TF2_CalcIsAttackCritical(int iClient, int weapon, char[] weaponname, bool& result)
{
	if (StrEqual(weaponname, "tf_weapon_fists", false) && 
	!(CPFViewController.GetSequence(iClient) == AnimState_Climb ||
	CPFViewController.GetSequence(iClient) == AnimState_ClimbIdle ||
	CPFViewController.GetSequence(iClient) == AnimState_Ledgegrab ||
	CPFViewController.GetSequence(iClient) == AnimState_LedgegrabIdle ||
	CPFViewController.GetSequence(iClient) == AnimState_Leap) &&
	!CPFViewController.GetDontInterrupt(iClient))
	{
		if (result)
			CPFViewController.Queue(iClient, AnimState_PunchCrit, 1.0, true);
		else
			CPFViewController.Queue(iClient, AnimState_Punch, 1.0, true);
		CPFViewController.SetDontInterrupt(iClient, true);
		
	}

	return Plugin_Continue;
}

public Action OnAnimationBegin(char[] strOutput, int iEnt, int iBitch, float flDelay)
{
	int iClient = GetEntPropEnt(iEnt, Prop_Send, "m_hOwnerEntity");
	if (iClient < 1 || iClient > MAXPLAYERS + 1 || iEnt != CPFViewController.GetPFViewmodel(iClient))
	{
		return Plugin_Continue;
	}
	
	CPFViewController.SetDontInterrupt(iClient, false);

	return Plugin_Continue;
}

public Action OnAnimationDone(char[] strOutput, int iEnt, int iBitch, float flDelay)
{
	int iClient = GetEntPropEnt(iEnt, Prop_Send, "m_hOwnerEntity");
	if (iClient < 1 || iClient > MAXPLAYERS + 1 || iEnt != CPFViewController.GetPFViewmodel(iClient))
	{
		return Plugin_Continue;
	}
	
	if (CPFViewController.GetQueuedSequence(iClient) == AnimState_None)
	{
		CPFViewController.SetSequence(iClient, CPFViewController.GetDefaultSequence(iClient), false);
	}
	
	CPFViewController.SetDontInterrupt(iClient, false);

	return Plugin_Continue;
}

public void OnClientDisconnect(int iClient)
{
	TRACEUR(iClient).Disconnect();
	
	CPFStateController.Set(iClient, State_None);
	CPFSpeedController.SetStoredSpeed(iClient, 250.0);
	
	CPFSoundController.SetIntensity(iClient, 0.0);
	CPFSoundController.SetCurrentLevel(iClient, 0);
	CPFSoundController.SetCurrentMusic(iClient, 0);
	CPFSoundController.KillDelayTimer(iClient);
		
	CPFStateController.SetFlags(iClient, SF_NONE);
	CPFRollHandler.Disconnect(iClient);
	
	CPFViewController.Disconnect(iClient);
	
	Weapons_ClientDisconnect(iClient);
	
	if (g_iHookIdGiveNamedItem[iClient])
	{
		DHookRemoveHookID(g_iHookIdGiveNamedItem[iClient]);
		g_iHookIdGiveNamedItem[iClient] = 0;
	}
		
	g_bTutorialFetched[iClient] = false;
}

public void OnEntityCreated(int iEntity, const char[] strClassname)
{
	if (StrEqual("tf_viewmodel", strClassname, false))
	{
		DebugOutput("Caught tf_viewmodel as %i", iEntity);
		CPFViewController.SetTFViewmodel(iEntity);
	}
	else if (StrEqual("tf_projectile_grapplinghook", strClassname))
		SDKHook(iEntity, SDKHook_StartTouch, OnHookshotTouch);
}

public void OnEntityCreatedPost(int iEntity)
{
	if (!IsValidEntity(iEntity)) return;
	
	char strClassname[128];
	if (!GetEntityClassname(iEntity, strClassname, sizeof(strClassname))) return;
	
	if (!strncmp("trigger_", strClassname, 8))
	{
		char strTargetname[256];
		GetEntPropString(iEntity, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
		if (StrEqual("climbable", strTargetname))
		{
			CPFPipe hPipe = new CPFPipe(iEntity);
			if (hPipe != null)
				CPFPipeController.AddPipe(hPipe);
		}
	}
	else if (strncmp("prop_", strClassname, 5))
	{
		char strModel[255];
		GetEntPropString(iEntity, Prop_Data, "m_ModelName", strModel, sizeof(strModel));
		if (CPFDoorController.IsDoor(iEntity))
		{
			CPFDoor hDoor = new CPFDoor(iEntity);
			if (hDoor != null)
				CPFDoorController.AddDoor(hDoor);
		}
	}
	else if (StrEqual("move_rope", strClassname) && CPFRopeController.IsRopeStart(iEntity))
	{
		CPFRope hRope = new CPFRope(iEntity);
		if (hRope != null)
		{
			CPFRopeController.AddRope(hRope);
			RequestFrame(ProcessRadialsPostKeyValue);
		}
	}
}

public void CustomMusicNotifier(int iClient)
{
	if (!IsValidClient(iClient))
		return;

	switch (CurrentMapHasCustomMusic())
	{
		case 1: //Map has music, can be changed with CHAR_DRYMIX
		{
			CPrintToChat(iClient, "{fullred}This map has custom music playing!\nType {green}snd_musicvolume <0.1 - 1.0> {fullred}in console to enable it.\nTo prevent parkour music overlapping, type {green}/pf_music{fullred} to toggle it.");
		}
		case 2: //Map has music, can't be changed with CHAR_DRYMIX
		{
			CPrintToChat(iClient, "{fullred}This map has custom music playing!\nTo prevent parkour music overlapping, type {green}/pf_music{fullred} to toggle it.");
		}
	}
}

int CurrentMapHasCustomMusic()
{
	//Check for an existing map info_target entity named pf_musicplayer
	int ent = -1;
	while( (ent = FindEntityByClassname(ent, "info_target")) != -1)
	{
		char targetname[32];
		GetEntPropString(ent, Prop_Data, "m_iName", targetname, sizeof(targetname));
		if(StrEqual(targetname, "pf_musicplayer"))
			return 1;
	}

	//Otherwise check for a map key
	char cfg[32] = "configs/pf/custom_map_music.cfg";
	char path[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, path, sizeof(path), cfg);    // Create path
	KeyValues kv = new KeyValues("maps");
	
	if (!FileExists(path))	//Check folder structure
	{
		LogError("Unable to find %s", path);
		delete kv;
		return 0;
	}

	if (!kv.ImportFromFile(path)) //Check if there's a keyvalue structure
	{
		LogError("Unable to import keyvalues from file %s", cfg);
		delete kv;
		return 0;
	}

	char section[PLATFORM_MAX_PATH];
	if (!kv.GetSectionName(section, sizeof(section)))	//Check for a valid starting section
	{
		LogError("Unable to find the first key in file %s", cfg);	
		delete kv;
		return 0;
	}
	
	if (!kv.GotoFirstSubKey(false))
	{
		LogError("Unable to find subkey within section %s", section);    //No map keys exist
		delete kv;
		return 0;
	}

	char map[32];
	GetCurrentMap(map, sizeof(map));

	do
	{
		if (!kv.GetSectionName(section, sizeof(section)))
			continue;

		if (StrEqual(section, map))
		{
			char value[2];
			kv.GetString(NULL_STRING, value, sizeof(value));
			return StringToInt(value);
		}
	}
	while (kv.GotoNextKey(false));

	return 0;
}

public void ProcessRadialsPostKeyValue(any aData)
{
	ProcessRadials();
}

Action AddStripHookshot(Handle hTimer, int iClient)
{
	if (IsValidClient(iClient))
		CPFStateController.AddFlags(iClient, SF_STRIPHOOKSHOT);

	return Plugin_Continue;
}

public Action OnHookshotTouch(int iHookshot, int iCollider) 
{
	//TODO: Check iCollider's solid flags
	if (!(GetEntProp(iCollider, Prop_Send, "m_nSolidType") == 0 || GetEntProp(iCollider, Prop_Send, "m_usSolidFlags") & 4))
	{
		int iOwner = GetEntPropEnt(iHookshot, Prop_Send, "m_hOwnerEntity");
		if (!IsValidClient(iOwner)) return Plugin_Handled;
		
		CreateTimer(0.1, AddStripHookshot, iOwner, TIMER_FLAG_NO_MAPCHANGE);
		//CPFStateController.AddFlags(iClient, SF_STRIPHOOKSHOT);
		
		switch (CPFStateController.Get(iOwner))
		{
			case State_Roll:
			{
				//TODO: Kill the projectile instead
				CPFRollHandler.Disengage(iOwner);
			}
			
			case State_Climb:
			{
				CPFClimbHandler.Disengage(iOwner, CLIMB_DISENGAGE_LEAVETRIGGER);
			}
			
			case State_Hang:
			{
				CPFHangHandler.Disengage(iOwner, LEDGEGRAB_DISENGAGE_JUMP);
			}
			
			case State_Slide:
			{
				CPFSlideHandler.End(iOwner);
			}
			
			case State_Vault:
			{
				//TODO: Does this need to be cancelled?
				CPFStateController.Set(iOwner, State_None);
			}
			
			case State_Zipline:
			{
				if (!CPFRopeController.GetRope(iCollider))
					CPFZiplineHandler.Break(iOwner, ZIPLINE_DISENGAGE_END);
				else
					return Plugin_Handled;
			}
			
			case State_Wallclimb:
			{
				CPFWallclimbHandler.Break(iOwner, WALLCLIMB_DISENGAGE_END);
			}
			
			case State_Wallrun:
			{
				CPFWallrunHandler.Break(iOwner, WALLRUN_DISENGAGE_END);
			}
			
			case State_DoorBoost:
			{
				CPFDoorHandler.Dismount(iOwner);
			}
			
			default:
			{
				if (CPFStateController.HasFlags(iOwner, SF_LONGJUMP))
					CPFLongjumpHandler.End(iOwner);
			}
		}
	}
	
	return Plugin_Continue;
}

public void OnEntityDestroyed(int iEntity)
{
	if (!IsValidEntity(iEntity)) return;
	
	char sClassname[128];
	if (!GetEntityClassname(iEntity, sClassname, sizeof(sClassname))) return;
	
	if (StrEqual("tf_projectile_grapplinghook", sClassname))
	{
		int iOwner = GetEntPropEnt(iEntity, Prop_Send, "m_hOwnerEntity");
		if (!IsValidClient(iOwner) || !CPFStateController.HasFlags(iOwner, SF_STRIPHOOKSHOT)) return;
		
		DebugOutput("OnEntityDestroyed --- Removing hookshot for %N", iOwner);
		CPFStateController.RemoveFlags(iOwner, SF_STRIPHOOKSHOT);
		TF2_RemoveWeaponSlot(iOwner, 5);
		TF2_SwitchToSlot(iOwner, WeaponSlot_Melee);
	}
}

public Action OnStartTouchTrigger(int iEnt, int iClient)
{
	if (!IsValidClient(iClient))
		return Plugin_Continue;
	
	char strTargetname[128];
	GetEntPropString(iEnt, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
	
	PFState eState = CPFStateController.Get(iClient);
	
	if (StrContains(strTargetname, "airaccelerate") > -1)
	{
		float flValue = float(StringToInt(strTargetname));
		if (flValue > 0.0)
			SetPlayerAirAccel(iClient, flValue);
		
		return Plugin_Continue;
	}
	else if (StrContains(strTargetname, "accelerate") > -1)
	{
		ReplaceString( strTargetname, sizeof(strTargetname), "accelerate", "" );
		float flValue = float(StringToInt(strTargetname));
		if (flValue > 0.0)
			SetPlayerAccel(iClient, flValue);
		
		return Plugin_Continue;
	}
	
	if (StrEqual("lockcontrols", strTargetname))
	{
		if(GetEntityMoveType(iClient) == MOVETYPE_NOCLIP)
			return Plugin_Continue;
		
		CPFStateController.ResetClient(iClient);
		
		CPFStateController.Set(iClient, State_Locked);
		SetEntityFlags(iClient, GetEntityFlags(iClient)|FL_ATCONTROLS);
		return Plugin_Continue;
	}
	else if (StrEqual("unlockcontrols", strTargetname))
	{
		if(eState != State_Locked)
			return Plugin_Continue;
		
		CPFStateController.ResetClient(iClient);
		return Plugin_Continue;
	}
	
	if (StrEqual("nofalldeath", strTargetname) || StrEqual("nofalldamage", strTargetname))
	{
		// FIXME: This doesn't account for entering DBF while in the trigger
		CPFSpeedController.SetFallDeathImmunity(iClient, true);
		if (CPFStateController.Get(iClient) == State_Falling)
			CPFStateController.Set(iClient, State_None);
	}
	
	if (StrContains(strTargetname, "giveboost") > -1)
	{
		CPFStateController.SetFlags(iClient, SF_INFINITEBOOST);
			
		return Plugin_Continue;
	}
	
	if (StrContains(strTargetname, "infinitejump") > -1 && CPFStateController.Get(iClient) != State_Falling)
	{
		CPFStateController.SetFlags(iClient, SF_INFINITEJUMP);
		
		return Plugin_Continue;
	}
	
	if (!StrEqual("climbable", strTargetname) || eState == State_Roll || eState == State_Climb) // || CPFStateController.IsOnCooldown(iClient, State_Climb)
		return Plugin_Continue;
	
	if (eState == State_Wallclimb || eState == State_Wallrun)
	{
		DebugOutput("OnStartTouchTrigger --- Climbable touched by %N during wallclimb/wallrun state", iClient);
	}
	
	CPFPipe hPipe = CPFPipeController.GetPipeByEntIndex(iEnt);
	if (hPipe != null)
	{
		if (!(GetClientButtons(iClient) & IN_DUCK) && (CPFPipeController.GetClientLastPipe(iClient) != view_as<CPFPipe>(iEnt)) && CPFStateController.Get(iClient) != State_Falling)
		{
			DebugOutput("OnStartTouchTrigger --- Attempting to mount %N", iClient);		
			if (CPFStateController.Get(iClient) == State_Wallrun)
			{
				DebugOutput("OnStartTouchTrigger --- Killing Wallrun for %N", iClient);
				CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_PIPE);
			}
			CPFClimbHandler.Mount(iClient, hPipe);
		}
		else
		{
			DebugOutput("OnStartTouchTrigger --- Mount checks for %N failed", iClient);
			if (GetClientButtons(iClient) & IN_DUCK)
			{
				DebugOutput("OnStartTouchTrigger --- Player ducking, trying again soon", iClient);
				RequestFrame(OnStartTouchPost, iClient);
				CreateTimer(0.06, CheckPlayerTrigger, iClient);
						
			}	
		}
	}
	else
	{
		DebugOutput("OnStartTouchTrigger --- Attempted to set null pipe for %N", iClient);
		return Plugin_Continue;
	}
		
	DebugOutput("OnStartTouchTrigger --- Climbable touch registered for %N", iClient);
	return Plugin_Continue;
}

public void OnStartTouchPost(int iClient)
{
	SetEntityFlags(iClient, GetEntityFlags(iClient) | FL_DONTTOUCH);
}

public Action OnEndTouchTrigger(int iEnt, int iClient)
{
	if (!IsValidClient(iClient))
		return Plugin_Continue;
	
	char strTargetname[128];
	GetEntPropString(iEnt, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
	
	if (StrEqual("nofalldeath", strTargetname) || StrEqual("nofalldamage", strTargetname))
	{
		CPFSpeedController.SetFallDeathImmunity(iClient, false);
	}
	
	if (StrContains(strTargetname, "giveboost") > -1)
	{
		CPFStateController.RemoveFlags(iClient, SF_INFINITEBOOST);
			
		return Plugin_Continue;
	}

	if (StrContains(strTargetname, "infinitejump") > -1)
	{
		CPFStateController.RemoveFlags(iClient, SF_INFINITEJUMP);
		
		return Plugin_Continue;
	}
	
	if (!StrEqual("climbable", strTargetname) || CPFStateController.Get(iClient) == State_Roll)
		return Plugin_Continue;
	
	if (CPFStateController.Get(iClient) == State_Climb)
	{
		CPFClimbHandler.Disengage(iClient, CLIMB_DISENGAGE_LEAVETRIGGER);
	}
	
	return Plugin_Continue;
}

Action CheckPlayerTrigger(Handle hTimer, int iClient)
{
	
	if (IsValidClient(iClient))
	{
		DebugOutput("CheckPlayerTrigger --- Removing FL_DONTTOUCH", iClient);
		SetEntityFlags(iClient, GetEntityFlags(iClient) & ~FL_DONTTOUCH);
	}
	else
		DebugOutput("CheckPlayerTrigger --- Not a valid client (%i)", iClient);

	return Plugin_Continue;
}

/* This function stays here just because it uses values not accessable by the main include with our stocks. */
stock float NormalToYaw(float vecWallNormal[3], const eWallrunSide eSide)
{
	float vecWallAngle[3];
	float vecNormalAdjusted[3];

	vecNormalAdjusted[2] = 0.0;
	if (eSide != WALLRUN_NONE)
	{
		vecNormalAdjusted[1] = (!eSide) ? vecWallNormal[0] : -vecWallNormal[0];
		vecNormalAdjusted[0] = (!eSide) ? -vecWallNormal[1] : vecWallNormal[1];
	}
	else
		vecNormalAdjusted = vecWallNormal;

	GetVectorAngles(vecNormalAdjusted, vecWallAngle);

	return vecWallAngle[1];
}

// This one too
public void UnscopeRifle(int iClient)
{
	TF2_RemoveCondition(iClient, TFCond_Zoomed);
	TF2_RemoveCondition(iClient, TFCond_Slowed);
	FakeClientCommand(iClient, "-attack");
	CPFSpeedController.SetStoredSpeed(iClient, 250.0);
}

public Action OnTakeDamage(int iClient, int &iAttacker, int &iInflictor, float &flDamage, int &iDamageType)
{

	const float MINIMUM_SPEED_PENALTY = 300.0;
	const float SPEED_PENALTY_MULT = 8.0;
	const float WEAPON_RIFLE_FALLOFF_MIN = 1024.0;
	const float WEAPON_RIFLE_FALLOFF_MAX = 3072.0;
	const float WEAPON_RIFLE_FALLOFF_REDUCTION = 0.666;
	
	if (!IsValidClient(iClient))
	{
		return Plugin_Handled;
	}
	
	Action eAction = Plugin_Continue;
	float flStartHealTime = g_cvarPvP.BoolValue? 5.0 : 2.0;
	float flNewSpeed;

	if (iDamageType & (1 << 5)) // Fall damage
	{
		if (CPFStateController.Get(iClient) == State_Rail)
			return Plugin_Handled;
		
		if (CPFSpeedController.GetFallDeathImmunity(iClient))
		{
			flDamage = 0.0;
			eAction = Plugin_Changed;
		}
		
		if (CPFRollHandler.Try(iClient))
		{
			flDamage *= 2.0;
			flNewSpeed = CPFSpeedController.GetSpeed(iClient, true) - (flDamage * SPEED_PENALTY_MULT);
			eAction = Plugin_Changed;
			DebugOutput("Plugin_Changed");
		}
		else
			eAction = Plugin_Handled;
	}
	else if (g_cvarPvP.BoolValue)
	{
		if (IsValidClient(iAttacker) && IsClassnameContains(TF2_GetActiveWeapon(iAttacker), "tf_weapon_sniperrifle")) // damage falloff
		{			
			float vecClient[3], vecAttacker[3];
			float fDist;
			GetClientAbsOrigin(iClient, vecClient);
			GetClientAbsOrigin(iAttacker, vecAttacker);
			fDist = GetVectorDistance(vecClient, vecAttacker);
			if (fDist > WEAPON_RIFLE_FALLOFF_MIN)
			{
				flDamage *=  1.0 - (fDist * (iDamageType & DMG_CRIT) ?  WEAPON_RIFLE_FALLOFF_REDUCTION : 1.0)/WEAPON_RIFLE_FALLOFF_MAX;
				eAction = Plugin_Changed;
			}
		}
		flNewSpeed = CPFSpeedController.GetSpeed(iClient, true) - (flDamage * 0.5);

	}

	if (!g_cvarPvP.BoolValue && IsValidClient(iAttacker))
	{
		eAction = Plugin_Handled;
	}
	
	if (eAction != Plugin_Handled)
	{
		CPFSpeedController.ValidateSpeed(iClient, flNewSpeed, .flMaxClampAt = MINIMUM_SPEED_PENALTY, .flMaxClampTo = MINIMUM_SPEED_PENALTY);
		CPFSpeedController.SetStoredSpeed(iClient, flNewSpeed);
			
		if (CPFStateController.Get(iClient) == State_None)
			CPFSpeedController.RestoreSpeed(iClient);

		CreateTimer(flStartHealTime, StartHealPlayer, GetClientUserId(iClient));	
			
		DebugOutput("OnTakeDamage --- Speed Penalty: %N - %.1f", iClient, flNewSpeed);
	}

	return eAction;
}

public void OnPlayerDeath(Event event, const char[] name, bool dontBroadcast)
{
	int client = GetClientOfUserId(GetEventInt(event, "userid"));
	
	CPFSoundController.StopAllSounds(client);
	
	CPFStateController.Set(client, State_None);
	CPFStateController.RemoveFlags(client, SF_BEINGHEALED);
	CPFStateController.RemoveFlags(client, SF_INFINITEJUMP);
	CPFStateController.RemoveFlags(client, SF_INFINITEBOOST);

	CPFViewController.Kill(client);
}

Action StartHealPlayer(Handle hTimer, int iUserID)
{
	int iClient = GetClientOfUserId(iUserID);
	if (!IsValidClient(iClient) || !IsPlayerAlive(iClient) ||  CPFStateController.HasFlags(iClient, SF_BEINGHEALED))
	{
		return Plugin_Stop;
	}

	float flHealth = (float(GetEntProp(iClient, Prop_Send, "m_iHealth")) * 1.02) + 2.0;
	if (flHealth >= 125.0)
	{
		SetEntityHealth(iClient, 125);
	}
	else
	{
		SetEntityHealth(iClient, RoundToFloor(flHealth));
		CreateTimer(1.0, StartHealPlayerRepeat, GetClientUserId(iClient), TIMER_REPEAT);
		CPFStateController.AddFlags(iClient, SF_BEINGHEALED);
	}
	
	return Plugin_Stop;
}

Action StartHealPlayerRepeat(Handle hTimer, int iUserID)
{
	int iClient = GetClientOfUserId(iUserID);
	if (!IsValidClient(iClient) || !IsPlayerAlive(iClient) || !CPFStateController.HasFlags(iClient, SF_BEINGHEALED))
	{
		if (hTimer != null)
			KillTimer(hTimer);
		
		return Plugin_Stop;
	}
	
	int iWaterLevel = CPFStateController.GetWaterLevel(iClient);
	if (iWaterLevel == 3)
		return Plugin_Continue;
	
	float flHealth = (float(GetEntProp(iClient, Prop_Send, "m_iHealth")) * 1.02) + 2.0;
	if (flHealth >= 125.0)
	{
		SetEntityHealth(iClient, 125);
		KillTimer(hTimer);
		CPFStateController.RemoveFlags(iClient, SF_BEINGHEALED);
		return Plugin_Stop;
	}
	else
	{
		SetEntityHealth(iClient, RoundToFloor(flHealth));
		return Plugin_Continue;
	}
}

public void OnChangeAirAccel(ConVar cvarAirAccel, const char[] strOldValue, const char[] strNewValue)
{
	g_flStockAirAccel = StringToFloat(strNewValue);
	float flOldValue = StringToFloat(strOldValue);
	
	for (int i = 1; i < MaxClients; i++)
	{
		if (g_flAirAccel[i] == flOldValue)
			g_flAirAccel[i] = g_flStockAirAccel;
	}
}

public void OnChangeAccel(ConVar cvarAccel, const char[] strOldValue, const char[] strNewValue)
{
	g_flStockAccel = StringToFloat(strNewValue);
	float flOldValue = StringToFloat(strOldValue);
	
	for (int i = 1; i < MaxClients; i++)
	{
		if (g_flAccel[i] == flOldValue)
			g_flAccel[i] = g_flStockAccel;
	}
}

public void OnPostThink(int iClient)
{
	if (!IsValidClient(iClient)) return;
	
	/* Only run sound controller once per this amount of ticks*/
	const int TICKS_PER_SOUND_THINK = 11;
	
	if (GetGameTickCount() % TICKS_PER_SOUND_THINK == 0)
		CPFSoundController.Think(iClient);
	
	if (!IsPlayerAlive(iClient)) return;
	
	int iButtonsNotHeld = CPFStateController.GetButtons(iClient);
	int iButtons = GetClientButtons(iClient);
	PFState eState = CPFStateController.Get(iClient);
	
	if (CPFStateController.GetLast(iClient) == State_Falling)
		CPFSoundController.StopAllSounds(iClient);
	
	switch (eState)
	{
		case State_None:
		{
			ProcessKeybinds(iClient, iButtons, iButtonsNotHeld); // , true
		}
	
		case State_Slide:
		{
			CPFSlideHandler.Slide(iClient, iButtons);
		}
		
		case State_Wallrun:
		{
			CPFWallrunHandler.Wallrun(iClient);
		}
		
		case State_Rail:
		{
			if (iButtons & IN_MOVELEFT)
				CPFRailHandler.SetSide(iClient, RAILSIDE_LEFT);
			else if (iButtons & IN_MOVERIGHT)
				CPFRailHandler.SetSide(iClient, RAILSIDE_RIGHT);
			else
				CPFRailHandler.SetSide(iClient, RAILSIDE_NONE);
		}
	}
}

public Action OnPlayerRunCmd(int iClient, int &iButtons) 
{ 
	if (!IsValidClient(iClient))
		return Plugin_Handled;
	
	if (CPFStateController.Get(iClient) == State_Locked && (iButtons & IN_JUMP))
	{
		iButtons &= ~IN_JUMP;
		return Plugin_Changed;
	}
	
	if (CPFStateController.HasFlags(iClient, SF_INFINITEJUMP))
	{
		if (iButtons & IN_JUMP)
		{
			SetEntProp(iClient, Prop_Send, "m_iAirDash", 0);
		}
		
		TF2Attrib_SetByName(iClient, "increased jump height", 1.5);
	}
	else
	{
		TF2Attrib_SetByName(iClient, "increased jump height", 1.0);
	}
	
	if (CPFStateController.HasFlags(iClient, SF_STRIPHOOKSHOT) && (iButtons & IN_ATTACK))
	{
		int iWeapon = TF2_GetActiveWeapon(iClient);
		if (IsValidEntity(iWeapon) &&  GetEntProp(iWeapon, Prop_Send, "m_iItemDefinitionIndex") == 1152) //grappling hook
		{
			int iProjectile = GetEntPropEnt(iWeapon, Prop_Send, "m_hProjectile");
			if (!HandleHookshot(iClient, iProjectile))
			{
				iButtons &= ~IN_ATTACK;
				return Plugin_Changed;
			}
		}
	}
	if ((iButtons & IN_ATTACK2) && (CPFStateController.Get(iClient) != State_None 
	&& CPFStateController.Get(iClient) != State_Slide && CPFStateController.Get(iClient) != State_Noclip))
	{
		if (PlayerHasRifleActive(iClient))
		{
			iButtons &= ~IN_ATTACK2;
			return Plugin_Changed;
		}
	}

	return Plugin_Continue;
}

public void OnPreThink(int iClient)
{
	if (!IsValidClient(iClient) || !IsPlayerAlive(iClient)) return;
	
	/* Speed at which you slow from running animation */
	const float ANIM_SPEED_TRANSITION = 16.0;
	/* Damage to take from death by fall */
	const float DEATH_BY_FALL_DAMAGE = 5000.0;
	/* Air velocity needed to run a hang think every tick */
	const float AIRVEL_TO_HANG_THINK = 300.0;
	/* Only run hang think once per this amount of ticks, if 
   air velocity isn't higher than AIRVEL_TO_HANG THINK */
	const int TICKS_PER_HANG_THINK = 2;
	/* If in wallclimb, maximum amount of ticks at which you'll 
		vault rather than hang given a valid hang think. */
	const int TICK_MAX_TO_VAULT_FROM_HANG = 6;
	/* If in wallclimb, maximum amount of ticks at which you'll
		attempt to vault. */
	const int TICK_MAX_TO_VAULT = 4;
	
	int iButtonsNotHeld = CPFStateController.GetButtons(iClient);
	int iButtons = GetClientButtons(iClient);
	PFState eState = CPFStateController.Get(iClient);
	
	if (CPFStateController.GetLast(iClient) == State_Falling)
		CPFSoundController.StopAllSounds(iClient);
	
#if defined _PFVIEWMODEL_INCLUDED
	CPFViewController.Update(iClient);
	if (!(CPFViewController.GetDontInterrupt(iClient)))
	{
		if (IsOnGround(iClient) && CPFStateController.Get(iClient) == State_None)
		{
			float vecVelocity[3];
			GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
			float flSpeed = GetVectorLength(vecVelocity, false);
			
			if (CPFViewController.GetSequence(iClient) == AnimState_Longfall)
				CPFViewController.Queue(iClient, AnimState_Running, 1.0, true);
			
			if (flSpeed <= ANIM_SPEED_TRANSITION)
			{
				CPFViewController.SetPlaybackRate(iClient, 1.0);
				if ((CPFViewController.GetSequence(iClient) != AnimState_Idle || CPFViewController.GetSequence(iClient) == AnimState_Running))
				{
					CPFViewController.Queue(iClient, AnimState_Idle, 1.0, true);
					CPFViewController.SetDefaultSequence(iClient, AnimState_Idle);
				}
			}
			else
			{
				if ((CPFViewController.GetSequence(iClient) != AnimState_Running || CPFViewController.GetSequence(iClient) == AnimState_Idle) && 
				CPFViewController.GetSequence(iClient) != AnimState_Doorsmash)
				{
					CPFViewController.Queue(iClient, AnimState_Running, 1.0, true);
					CPFViewController.SetDefaultSequence(iClient, AnimState_Running);
				}
				CPFViewController.SetPlaybackRate(iClient, flSpeed / SPEED_MAX);	
			}
				
			
			if(CPFViewController.GetSequence(iClient) != AnimState_Running &&
			CPFViewController.GetSequence(iClient) != AnimState_Waterslide &&
			CPFViewController.GetSequence(iClient) != AnimState_Zipline &&
			CPFViewController.GetSequence(iClient) != AnimState_ZiplineIdle)
				CPFViewController.SetDefaultSequence(iClient, AnimState_Running);
		}
		else
		{
			if(CPFViewController.GetSequence(iClient) != AnimState_Longfall &&
			CPFStateController.Get(iClient) == State_None)
				CPFViewController.Queue(iClient, AnimState_Longfall, 1.0, true);
		}
	}
#endif
	
	float vecClientAbsVel[3];
	GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", vecClientAbsVel);
	
	switch (eState)
	{	
		case State_Falling:
		{
			if (IsOnGround(iClient) || CPFStateController.GetWaterLevel(iClient) > 0)
			{
				if (!CPFSpeedController.GetFallDeathImmunity(iClient))
					SDKHooks_TakeDamage(iClient, iClient, iClient, DEATH_BY_FALL_DAMAGE, DMG_FALL);
				
				CPFStateController.Set(iClient, State_None);
			}
			
			if (GetEntityMoveType(iClient) == MOVETYPE_NOCLIP)
			{
				CPFViewController.SetDefaultSequence(iClient, AnimState_Longfall);
				CPFStateController.Set(iClient, State_Noclip);
			}
		}
		
		case State_None:
		{
			if (CPFStateController.GetLast(iClient) == State_Roll)
			{
				SetClientViewEntity(iClient, iClient);
				CPFRollHandler.ForceRemoveCamera(iClient);
			}

			if (!TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
			{
				if (FloatAbs(vecClientAbsVel[2]) > AIRVEL_TO_HANG_THINK)
				{
					if (CPFHangHandler.Think(iClient, iButtons))
						CPFHangHandler.Mount(iClient);
				}
				else if (GetGameTickCount() % TICKS_PER_HANG_THINK)
				{
					if (CPFHangHandler.Think(iClient, iButtons))
						CPFHangHandler.Mount(iClient);
				}
			}
		
			CPFDoorHandler.Think(iClient);
			
			ProcessKeybinds(iClient, iButtons, iButtonsNotHeld);
			
			CPFSpeedController.Think(iClient, iButtons);
			
			if (GetEntityMoveType(iClient) == MOVETYPE_NOCLIP)
			{
				CPFViewController.SetDefaultSequence(iClient, AnimState_Longfall);
				CPFStateController.Set(iClient, State_Noclip);
			}
			
			if (IsOnGround(iClient))
			{
				if (CPFStateController.HasFlags(iClient, SF_LONGJUMP))
					CPFLongjumpHandler.End(iClient);
				if (CPFStateController.HasFlags(iClient, SF_CAMEFROMSLIDE))
					CPFStateController.RemoveFlags(iClient, SF_CAMEFROMSLIDE);
				CPFRollHandler.Queue(iClient, false);
			}
			else
			{
				if ((CPFViewController.GetSequence(iClient) == AnimState_Idle ||
					CPFViewController.GetSequence(iClient) == AnimState_Running) &&
					(CPFViewController.GetSequence(iClient) != AnimState_HandslideLeft &&
					CPFViewController.GetSequence(iClient) != AnimState_HandslideRight) &&
					CPFViewController.GetSequence(iClient) != AnimState_Leap)
				{
					CPFViewController.SetDefaultSequence(iClient, AnimState_Longfall);
				}
				else
				{
					CPFViewController.SetDefaultSequence(iClient, AnimState_Idle);
				}
				
				CPFSpeedController.StoreAirVel(iClient);
				CPFStateController.AddFlags(iClient, SF_CAMEFROMSLIDE);
			}
		}
		
		case State_Noclip:
		{
			if (GetEntityMoveType(iClient) != MOVETYPE_NOCLIP)
			{
				CPFStateController.Set(iClient, State_None);
			}
		}
		
		case State_Roll:
		{
			CPFRollHandler.Roll(iClient);
		}
		
		case State_Climb:
		{
			if (iButtons & IN_JUMP && (iButtonsNotHeld & IN_JUMP))
				CPFClimbHandler.Disengage(iClient, CLIMB_DISENGAGE_JUMP);
			else if (iButtons & IN_DUCK)
				CPFClimbHandler.Disengage(iClient, CLIMB_DISENGAGE_CROUCH);
			else
				CPFClimbHandler.Climb(iClient, CPFPipeController.GetClientPipe(iClient));
				
		}
		
		case State_Wallclimb:
		{
			bool hangResult = false;
			if (FloatAbs(vecClientAbsVel[2]) > AIRVEL_TO_HANG_THINK)
			{
				hangResult = CPFHangHandler.Think(iClient, iButtons);
			}
			else if (GetGameTickCount() % TICKS_PER_HANG_THINK)
			{
				hangResult = CPFHangHandler.Think(iClient, iButtons);
			}
			
			if (hangResult)
			{
				if (GetGameTickCount() - CPFWallclimbHandler.StartTick(iClient) <= TICK_MAX_TO_VAULT_FROM_HANG)
				{
					if (CPFVaultHandler.Try(iClient))
						CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_VAULT);
				} 
				else 
				{
					CPFHangHandler.Mount(iClient);
					CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_HANG); // Allow for hangs during wallclimbs
				}
			}
			else if (iButtons & IN_JUMP && iButtonsNotHeld & IN_JUMP)
				CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_JUMP);
			else if (iButtons & IN_DUCK && iButtonsNotHeld & IN_DUCK)
				CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_DUCK);
			else if ((GetGameTickCount() - CPFWallclimbHandler.StartTick(iClient)) < TickModify(TICK_MAX_TO_VAULT) && CPFVaultHandler.Try(iClient) && !CPFStateController.IsOnCooldown(iClient, State_Wallclimb))
				CPFWallclimbHandler.Wallclimb(iClient);
			else if ((GetGameTickCount() - CPFWallclimbHandler.StartTick(iClient)) < TickModify(WALLCLIMB_MAX_TICKS) && !CPFStateController.IsOnCooldown(iClient, State_Wallclimb))
				CPFWallclimbHandler.Wallclimb(iClient);
			else
				CPFWallclimbHandler.Break(iClient, WALLCLIMB_DISENGAGE_END);
		}
		
		case State_Wallrun:
		{
			if (iButtons & IN_JUMP && iButtonsNotHeld & IN_JUMP)
			{
				if (CPFWallrunHandler.CanBreak(iClient))
					CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_JUMP);
				else
					CPFWallrunHandler.SetJump(iClient, true);
			}
			else if (CPFWallrunHandler.CanBreak(iClient) && CPFWallrunHandler.GetJump(iClient))
			{
				CPFWallrunHandler.SetJump(iClient, false);
				CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_JUMP);
			}
			else if (iButtons & IN_DUCK)
				CPFWallrunHandler.Break(iClient, WALLRUN_DISENGAGE_CROUCH);
		}
		
		case State_Hang:
		{
			if (iButtons & IN_JUMP && iButtonsNotHeld & IN_JUMP)
				CPFHangHandler.Disengage(iClient, LEDGEGRAB_DISENGAGE_JUMP);
			else if (iButtons & IN_DUCK)
				CPFHangHandler.Disengage(iClient, LEDGEGRAB_DISENGAGE_CROUCH);
			else
				CPFHangHandler.Hang(iClient);
		}
		
		case State_Zipline:
		{
			if (iButtons & IN_DUCK)
				CPFZiplineHandler.Break(iClient, ZIPLINE_DISENGAGE_CROUCH);
			else
				CPFZiplineHandler.Think(iClient);
		}
		
		case State_Rail:
		{
			if (iButtons & IN_JUMP)
			{
				if (iButtons & IN_MOVELEFT)
					CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_JUMP_TRYLEFT);
				else if (iButtons & IN_MOVERIGHT)
					CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_JUMP_TRYRIGHT);
				else
					CPFRailHandler.Disengage(iClient, RAIL_DISENGAGE_JUMP);
			}
			else
				CPFRailHandler.Think(iClient);
		}
		
		case State_DoorBoost:
		{
			CPFDoorHandler.DoorBoost(iClient);
		}
		
		case State_Locked:
		{
			CPFDoorHandler.Think(iClient);
			CPFSpeedController.Think(iClient, iButtons);
		}
		
		default:
		{
			
		}
	}
	
	if (CPFStateController.HasFlags(iClient, SF_INFINITEBOOST))
	{	
		CPFSpeedController.SetSpeed(iClient, view_as<float>(SPEED_MAX_BOOST));
		CPFSpeedController.SetBoost(iClient, true);
	}

	CPFStateController.UpdateButtons(iClient, iButtons);
}

void ProcessKeybinds(int iClient, int iButtons, int iButtonsNotHeld) //bool bPostHook = false
{
	/* Minimum horizontal speed required to register the 
		start of a slide */
	const float HORIZONTAL_SPEED_TO_SLIDE = 200.0;
	
	if (iButtons & IN_ATTACK2 && CPFSpeedController.GetBoost(iClient) && CPFSpeedController.GetSpeed(iClient) >= SPEED_MAX && IsOnGround(iClient))
		CPFLongjumpHandler.Longjump(iClient);
	else if (iButtons & IN_DUCK && iButtonsNotHeld & IN_DUCK && !IsOnGround(iClient) && CPFStateController.Get(iClient) == State_None)
		CPFRollHandler.Queue(iClient);
	else if ((iButtons & IN_DUCK && ClientHorizontalSpeed(iClient) > HORIZONTAL_SPEED_TO_SLIDE && !(iButtons & IN_JUMP && iButtonsNotHeld & IN_JUMP)) 
			&& ((CPFStateController.HasFlags(iClient, SF_CAMEFROMSLIDE) || CPFStateController.GetLast(iClient) != State_Slide && !IsFullyDucked(iClient)) || iButtonsNotHeld & IN_DUCK) 
			&& IsOnGround(iClient))
	{
		if (!CheckIfForceDucked(iClient))
		{
			if (CPFStateController.HasFlags(iClient, SF_CAMEFROMSLIDE))
			{
				CPFSlideHandler.Try(iClient, iButtons, true);
				CPFStateController.RemoveFlags(iClient, SF_CAMEFROMSLIDE);
			}
			else
				CPFSlideHandler.Try(iClient, iButtons, false);
		}
	
	}
	else if (iButtons & IN_WALLRUN_LEFT == IN_WALLRUN_LEFT && !(iButtons & IN_MOVERIGHT) && iButtonsNotHeld & IN_JUMP && !(iButtons & IN_DUCK) && IsOnGround(iClient))
		CPFWallrunHandler.Try(iClient, WALLRUN_LEFT);
	else if (iButtons & IN_WALLRUN_RIGHT == IN_WALLRUN_RIGHT && !(iButtons & IN_MOVELEFT) && iButtonsNotHeld & IN_JUMP && !(iButtons & IN_DUCK) && IsOnGround(iClient))
		CPFWallrunHandler.Try(iClient, WALLRUN_RIGHT);
	else if (iButtons & IN_WALLCLIMB == IN_WALLCLIMB && !(iButtons & IN_MOVERIGHT) && !(iButtons & IN_MOVELEFT) && !(iButtons & IN_DUCK) && iButtonsNotHeld & IN_JUMP && IsOnGround(iClient))
	{
		CPFWallclimbHandler.Try(iClient);
		DebugOutput("ProcessKeyBinds --- Tried Wallclimb for %N", iClient);
	}
	else if (iButtons && !(iButtons & IN_WALLCLIMB == IN_WALLCLIMB && !(iButtons & IN_MOVERIGHT) && !(iButtons & IN_MOVELEFT) && !(iButtons & IN_DUCK) && iButtonsNotHeld & IN_JUMP && IsOnGround(iClient)))
	{
		//DebugOutput("ProcessKeyBinds --- No Wallclimb for %N, %d %d %d %d %d %d", iClient, (iButtons & IN_WALLCLIMB == IN_WALLCLIMB), !(iButtons & IN_MOVERIGHT), !(iButtons & IN_MOVELEFT), !(iButtons & IN_DUCK), iButtonsNotHeld & IN_JUMP, IsOnGround(iClient));
	}
}
