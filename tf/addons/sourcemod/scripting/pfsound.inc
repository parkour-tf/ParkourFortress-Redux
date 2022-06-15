#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFSOUND_INCLUDED
	#endinput
#endif
#define _PFSOUND_INCLUDED

enum struct MusicData
{
	//char[] LastMusic;
	float Intensity;
	int CurrentLevel;
	int CurrentMusic;
	int TickStart;
	Handle DelayTimer;
	bool BeepCooldown;
}

static MusicData g_MusicData[MAXPLAYERS + 1];

#define SFX_DOOR_OPEN	"doors/vent_open3.wav"
#define SFX_DOOR_CLOSE	"doors/metal_stop1.wav"
#define SFX_LEDGEGRAB	"physics/concrete/rock_impact_hard2.wav"
#define SFX_SLIDE 		"physics/body/body_medium_scrape_rough_loop1.wav"

#define SFX_FOOTSTEP1	"player/footsteps/concrete1.wav"
#define SFX_FOOTSTEP2	"player/footsteps/concrete2.wav"

#define SFX_LADDER1		"player/footsteps/ladder1.wav"
#define SFX_LADDER2		"player/footsteps/ladder2.wav"

#define SFX_ZIPLINE		"parkoursource/wiredown1.mp3"
#define SFX_RAILGRIND	"parkoursource/railgrind1.mp3"
#define SFX_RAILMOUNT	"parkoursource/rail1end.mp3"
#define SFX_RAILEND		"parkoursource/rail1end2.mp3"
#define SFX_FALLDEATH	"parkoursource/onlydeathbelow.mp3"
#define SFX_PIPESLIDE	"parkoursource/climbdown_metal_start.mp3"

#define SFX_JINGLE_MILD1 "parkoursource/event_mild_jingle1.mp3"
#define SFX_JINGLE_MILD2 "parkoursource/event_mild_jingle2.mp3"
#define SFX_JINGLE_MILD3 "parkoursource/event_mild_jingle4.mp3" // there's no 3. oops!

#define SFX_JINGLE_BIG1 "parkoursource/event_mild_big_jingle1.mp3"
#define SFX_JINGLE_BIG2 "parkoursource/event_mild_big_jingle2.mp3"
#define SFX_JINGLE_BIG3 "parkoursource/event_mild_big_jingle3.mp3"
#define SFX_JINGLE_BIG4 "parkoursource/event_mild_big_jingle4.mp3"

#define SFX_TOTAL 21

static const char strSfx[SFX_TOTAL][64] =
{
	SFX_DOOR_OPEN, SFX_DOOR_CLOSE, SFX_LEDGEGRAB, SFX_SLIDE, SFX_FOOTSTEP1, SFX_FOOTSTEP2, SFX_LADDER1, SFX_LADDER2,
	SFX_ZIPLINE, SFX_FALLDEATH, SFX_PIPESLIDE,
	SFX_JINGLE_MILD1, SFX_JINGLE_MILD2, SFX_JINGLE_MILD3, SFX_JINGLE_BIG1, SFX_JINGLE_BIG2, SFX_JINGLE_BIG3, SFX_JINGLE_BIG4,
	SFX_RAILGRIND, SFX_RAILEND, SFX_RAILMOUNT
};

#define MUS_NONE "vo/null.wav"

#define MUS_CALM1 "parkoursource/music/menutheme.mp3"
#define MUS_CALM2 "parkoursource/music/patterns.mp3"
#define MUS_CALM3 "parkoursource/music/pieces_form_the_whole.mp3"

#define MUS_STEADY1 "parkoursource/music/sharp2.mp3"
#define MUS_STEADY2 "parkoursource/music/sol.mp3"
#define MUS_STEADY3 "parkoursource/music/beat_theme.mp3"

#define MUS_INTENSE1 "parkoursource/music/poetry_in_motion.mp3"
#define MUS_INTENSE2 "parkoursource/music/sharp_intense.mp3"
#define MUS_INTENSE3 "parkoursource/music/the_walk.mp3"
#define MUS_INTENSE4 "parkoursource/music/sugar.mp3"

#define MUSIC_TOTAL 11

static const char strMusic[MUSIC_TOTAL][64] =
{
	MUS_NONE, MUS_CALM1, MUS_CALM2, MUS_CALM3,
	MUS_STEADY1, MUS_STEADY2, MUS_STEADY3,
	MUS_INTENSE1, MUS_INTENSE2, MUS_INTENSE3, MUS_INTENSE4
};

#define MUS_FOLDER "parkoursource/music/"
#define SOUND_FOLDER "sound/"

methodmap CPFSoundController
{
	public static void Init()
	{
		char strBuffer[64];
		
		for (int i = 1; i < MUSIC_TOTAL; i++)
		{
			PrecacheSound(strMusic[i], true);
			
			strBuffer = SOUND_FOLDER;
			StrCat(strBuffer, 64, strMusic[i]);
			AddFileToDownloadsTable(strBuffer);
		}
		
		for (int i = 0; i < SFX_TOTAL; i++)
		{
			PrecacheSound(strSfx[i], true);
			
			if (i > 7) //external sfx
			{
				strBuffer = SOUND_FOLDER;
				StrCat(strBuffer, 64, strSfx[i]);
				AddFileToDownloadsTable(strBuffer);
			}

		}
		
		g_cookieMusic = new Cookie("parkourmusic", "Disable background music", CookieAccess_Protected);
		g_cookieSound = new Cookie("parkoursfx", "Disable ambient sounds", CookieAccess_Protected);
	}
	
	public static bool ShouldPlayMusic(int iClient)
	{
		if (!g_cvarMusicEnabled.IntValue)
			return false;

		if (GetCookieInt(g_cookieMusic, iClient))
			return false;
		
		return true;
	}
	
	public static void AddIntensity(int iClient, float input)
	{
		if (g_MusicData[iClient].DelayTimer != INVALID_HANDLE)
			return;
		g_MusicData[iClient].Intensity += input;
	}
	
	public static void SubtractIntensity(int iClient, float input)
	{
		if (g_MusicData[iClient].DelayTimer != INVALID_HANDLE)
			return;
		g_MusicData[iClient].Intensity -= input;
	}
	
	public static void SetIntensity(int iClient, float input)
	{
		g_MusicData[iClient].Intensity = input;
	}
	
	public static void SetCurrentLevel(int iClient, int input)
	{
		g_MusicData[iClient].CurrentLevel = input;
	}
	
	public static int GetCurrentMusic(int iClient)
	{
		return g_MusicData[iClient].CurrentMusic;
	}
	
	public static void SetCurrentMusic(int iClient, int input)
	{
		g_MusicData[iClient].CurrentMusic = input;
	}
	
	public static void KillDelayTimer(int iClient)
	{
		if (g_MusicData[iClient].DelayTimer != INVALID_HANDLE)
		{
			delete g_MusicData[iClient].DelayTimer;
			g_MusicData[iClient].DelayTimer = INVALID_HANDLE;
		}
	}
	
	public static void InitDelayTimer(int iClient)
	{
		g_MusicData[iClient].DelayTimer = INVALID_HANDLE;
	}
	
	public static bool GetOnCooldown(int iClient)
	{
		return g_MusicData[iClient].BeepCooldown;
	}
	
	public static void SetOnCooldown(int iClient, float flLength)
	{
		g_MusicData[iClient].BeepCooldown = true;
		CreateTimer(flLength, ResetBeepCooldown, iClient);
	}
	
	
	public static float GetClientMusicVol(int iClient)
	{
		char MusicVolume[8];
		g_cookieMusicVolume.Get(iClient, MusicVolume, sizeof(MusicVolume));
		return StringToFloat(MusicVolume);
	}
	
	public static void PlayDoor(int iDoor, const bool bClosing)
	{
		if (!IsSoundPrecached((bClosing) ? SFX_DOOR_CLOSE : SFX_DOOR_OPEN))
			PrecacheSound((bClosing) ? SFX_DOOR_CLOSE : SFX_DOOR_OPEN);
		
		if (!IsValidEntity(iDoor) || !iDoor)
			return;
		
		float vecOrigin[3];
		GetEntPropVector(iDoor, Prop_Send, "m_vecOrigin", vecOrigin);
		
		DebugOutput("CPFSoundController::PlayDoor --- Playing sound for door %d", iDoor);
		EmitAmbientSound(	.name = ((bClosing) ? SFX_DOOR_CLOSE : SFX_DOOR_OPEN), 
							.pos = vecOrigin,
							.entity = SOUND_FROM_WORLD,
							.level = SNDLEVEL_TRAIN		);
	}
	
	public static void PlayLedgegrab(int iClient)
	{
		if (!IsSoundPrecached(SFX_LEDGEGRAB))
			PrecacheSound(SFX_LEDGEGRAB);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_LEDGEGRAB,
							.level = SNDLEVEL_TRAIN		);
	}
	
	public static void PlaySlide(int iClient)
	{
		if (!IsSoundPrecached(SFX_SLIDE))
			PrecacheSound(SFX_SLIDE);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_SLIDE,
							.level = 35);
	}
	
	public static void StopSlide(int iClient)
	{
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO, 
					.name = SFX_SLIDE		);
	}
	
	public static void StopZipline(int iClient)
	{
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO, 
					.name = SFX_ZIPLINE		);
	}
	
	public static void PlayZipline(int iClient)
	{
		if (!IsSoundPrecached(SFX_ZIPLINE))
			PrecacheSound(SFX_ZIPLINE);
		
		if (!IsValidClient(iClient))
			return;
		
		CPFSoundController.StopZipline(iClient);
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_ZIPLINE,
							.level = SNDCHAN_AUTO	);
	}
	
	public static void StopRailGrind(int iClient)
	{
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO, 
					.name = SFX_RAILGRIND		);
	}
	
	public static void PlayRailGrind(int iClient, int iPitch = SNDPITCH_NORMAL)
	{
		if (!IsSoundPrecached(SFX_RAILGRIND))
			PrecacheSound(SFX_RAILGRIND);
		
		if (!IsValidClient(iClient))
			return;
		
		CPFSoundController.StopRailGrind(iClient);
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_RAILGRIND,
							.level = SNDCHAN_AUTO,
							.pitch = iPitch);
	}
	
	public static void PlayRailMount(int iClient)
	{
		if (!IsSoundPrecached(SFX_RAILMOUNT))
			PrecacheSound(SFX_RAILMOUNT);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_RAILMOUNT,
							.level = SNDCHAN_AUTO	);
	}
	
	public static void PlayRailEnd(int iClient)
	{
		if (!IsSoundPrecached(SFX_RAILEND))
			PrecacheSound(SFX_RAILEND);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_RAILEND,
							.level = SNDCHAN_AUTO	);
	}
	
	public static void PlayPipeSlide(int iClient)
	{
		if (!IsSoundPrecached(SFX_PIPESLIDE))
			PrecacheSound(SFX_PIPESLIDE);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_PIPESLIDE,
							.level = SNDLEVEL_TRAIN		);
	}
	
	public static void StopPipeSlide(int iClient)
	{
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO, 
					.name = SFX_PIPESLIDE		);
	}
	
	public static void PlayFallDeath(int iClient)
	{
		if (!IsSoundPrecached(SFX_FALLDEATH))
			PrecacheSound(SFX_FALLDEATH);
		
		if (!IsValidClient(iClient))
			return;
		
		EmitSoundToClient(	.client = iClient,
							.sample = SFX_FALLDEATH,
							.level = SNDLEVEL_TRAIN		);
	}

	public static void StopFallDeath(int iClient)
	{
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO, 
					.name = SFX_FALLDEATH	);
	}
	
	public static void PlayFallVO(int iClient)
	{
		SetVariantString("HalloweenLongFall");
		AcceptEntityInput(iClient, "SpeakResponseConcept");
	}
	
	public static void StopFallVO(int iClient)
	{
		StopSound(iClient, SNDCHAN_VOICE, "vo/scout_sf12_falling01.mp3");
		StopSound(iClient, SNDCHAN_VOICE, "vo/scout_sf12_falling02.mp3");
		StopSound(iClient, SNDCHAN_VOICE, "vo/scout_sf12_falling03.mp3");
	}
	
	public static void PlayWallrun(int iClient, int iTickCount)
	{
		if (!IsSoundPrecached(SFX_FOOTSTEP1))
			PrecacheSound(SFX_FOOTSTEP1);
		
		if (!IsSoundPrecached(SFX_FOOTSTEP2))
			PrecacheSound(SFX_FOOTSTEP2);
		
		if (!(iTickCount % 30))
			EmitSoundToClient(	.client = iClient,
								.sample = SFX_FOOTSTEP1,
								.level = 35		);
		else if (!(iTickCount % 15))
			EmitSoundToClient(	.client = iClient,
								.sample = SFX_FOOTSTEP2,
								.level = 35	);
		else
			return;
	}
	
	public static void PlayPainVO (int iClient)
	{
		SetVariantString("IsDominating:0");
		AcceptEntityInput(iClient, "AddContext");
		
		SetVariantString("TLK_PLAYER_ATTACKER_PAIN");
		AcceptEntityInput(iClient, "SpeakResponseConcept");
		
		AcceptEntityInput(iClient, "ClearContext");
	}
	
	/*public static void PlayLongjumpVO (int iClient) //TODO: Make this work, would give us lipsync & be more proper overall
	{
		SetVariantString("randomnum:100");
		AcceptEntityInput(iClient, "AddContext");
		
		SetVariantString("IsDoubleJumping:1");
		AcceptEntityInput(iClient, "AddContext");
		
		SetVariantString("WeaponIsScattergunDouble:The Force-a-Nature");
		AcceptEntityInput(iClient, "AddContext");
		
		SetVariantString("TLK_FIREWEAPON");
		AcceptEntityInput(iClient, "SpeakResponseConcept");
		
		AcceptEntityInput(iClient, "ClearContext");
	}*/
	
	public static void PlayLongjumpVO (int iClient)
	{
		int random = GetRandomInt(1, 20);
		DebugOutput ("PlayLongjumpVO - Rolled %i", random);
		if (random <= 5)
		{
			float vecOrigin[3];
			char sRandVO[26];
			Format(sRandVO, sizeof(sRandVO), "vo/scout_ApexofJump0%i.mp3", random);
			GetClientEyePosition(iClient, vecOrigin);
			
			if (!IsSoundPrecached(sRandVO))
				PrecacheSound(sRandVO);
			
			EmitAmbientSound(	.name = sRandVO, 
								.pos = vecOrigin,
								.entity = iClient,
								.level = SNDLEVEL_MINIBIKE);
		}
	}
	
	public static void PlayPipeclimb(int iClient)
	{
		if (!IsSoundPrecached(SFX_LADDER1))
			PrecacheSound(SFX_LADDER1);
		
		if (!IsSoundPrecached(SFX_LADDER2))
			PrecacheSound(SFX_LADDER2);
		
		int iTickCount = GetGameTickCount();
		
		if (!(iTickCount % 30))
			EmitSoundToClient(	.client = iClient,
					.sample = SFX_LADDER1,
					.level = 35		);
		else if (!(iTickCount % 15))
			EmitSoundToClient(	.client = iClient,
					.sample = SFX_LADDER2,
					.level = 35		);
		else
			return;
	}
	
	public static void PlaySmallDing(int iClient)
	{
		if (CPFSoundController.GetOnCooldown(iClient))
			return;
		
		int random = GetRandomInt(11, 13);
		
		PlayToAllButClient(iClient, strSfx[random]);
		
		CPFSoundController.SetOnCooldown(iClient, 1.0);
	}
	
	public static void PlayBigDing(int iClient)
	{
		if (CPFSoundController.GetOnCooldown(iClient))
			return;
		
		int random = GetRandomInt(14, 17);
		
		switch (GetCookieInt(g_cookieSelfAmbientSound, iClient))
		{
			case true:
			{
				EmitSoundToAll(	.sample = strSfx[random],
				.entity = iClient,
				.level = SNDLEVEL_TRAIN		);
			}
			default:
			{
				PlayToAllButClient(iClient, strSfx[random]);
			}
		}
		
		
		CPFSoundController.SetOnCooldown(iClient, 1.0);
	}
	
	public static void StopAllMusic(int iClient)
	{
		if (!IsValidClient(iClient))
			return;
		
		for (int i = 0; i < MUSIC_TOTAL; i++)
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO,
					.name = strMusic[i]);
					
		CPFSoundController.SetCurrentMusic(iClient, 0);
	}
	
	public static void StopCurrentMusic(int iClient)
	{
		if (!IsValidClient(iClient))
			return;
		
		int iCurrent;
		iCurrent = CPFSoundController.GetCurrentMusic(iClient);
		StopSound(	.entity = iClient, 
					.channel = SNDCHAN_AUTO,
					.name = strMusic[iCurrent]);
					
		CPFSoundController.SetCurrentMusic(iClient, 0);
	}
	
	public static void SwitchMusic(int iClient, bool reset = false)
	{
		if (!CPFSoundController.ShouldPlayMusic(iClient)) return;

		int random;
		
		if (!reset)
			CPFSoundController.StopCurrentMusic(iClient);
		
		g_MusicData[iClient].TickStart = GetGameTickCount();
		
		switch (g_MusicData[iClient].CurrentLevel)
		{
			case 0:
			{
				random = GetRandomInt(1, 3);
			}
			case 1:
			{
				random = GetRandomInt(4, 6);
			}
			case 2:
			{
				random = GetRandomInt(7, 10);
			}
		}
		DebugOutput("CPFSoundController::SwitchMusic --- Playing %s for %N", strMusic[random], iClient);
		EmitSoundToClient(	.client = iClient,
							.sample = strMusic[random],
							.channel = SNDCHAN_AUTO,
							.volume = CPFSoundController.GetClientMusicVol(iClient));
		g_MusicData[iClient].CurrentMusic = random;
		
	}
	
	public static void UpdateCurrentMusic(int iClient)
	{
		int iCurrent;
		float flVolume;
		iCurrent = CPFSoundController.GetCurrentMusic(iClient);
		if (CPFSoundController.GetCurrentMusic(iClient) == 0) return;
		
		flVolume = CPFSoundController.GetClientMusicVol(iClient);
		
		EmitSoundToClient(	.client = iClient,
							.sample = strMusic[iCurrent],
							.volume = flVolume,
							.flags = SND_CHANGEVOL );
	}
	
	public static void StopAllSounds(int iClient)
	{
		CPFSoundController.StopZipline(iClient);
		CPFSoundController.StopRailGrind(iClient);
		CPFSoundController.StopFallDeath(iClient);
		CPFSoundController.StopPipeSlide(iClient);
		CPFSoundController.StopSlide(iClient);
		CPFSoundController.StopFallVO(iClient);
	}
	
	public static void Think(int iClient)
	{
		if (!IsValidClient(iClient))
			return;
		
		if (GetGameTickCount() % 33 == 0) CPFSoundController.UpdateCurrentMusic(iClient);
		
		if (g_MusicData[iClient].TickStart - GetGameTickCount() > 7920) // 2 minutes
		{
			CPFSoundController.SwitchMusic(iClient);
			return;
		}
		
		if (g_MusicData[iClient].DelayTimer != INVALID_HANDLE)
			return;
		
		float flNewIntensity = g_MusicData[iClient].Intensity;
		int newLevel;
		
		flNewIntensity -= 0.003 * (g_MusicData[iClient].CurrentLevel + 1.0);
		if (flNewIntensity > 20.0)
			flNewIntensity = 20.0;
		else if (flNewIntensity < 0.0)
			flNewIntensity = 0.0;
			
		//DebugOutput ("%.1f", flNewIntensity);
			
		if (flNewIntensity >= 17.5)
		{
			newLevel = 2;
		}
		else if (flNewIntensity < 17.5 && flNewIntensity >= 6.0)
		{
			newLevel = 1;
		}
		else
		{
			newLevel = 0;
		}
		
		if (newLevel != g_MusicData[iClient].CurrentLevel)
		{
			
			if (flNewIntensity >= 17.5)
			{
				flNewIntensity += 2.5;
			}
			else if (flNewIntensity < 17.5 && flNewIntensity >= 6.0)
			{
				flNewIntensity += 5.0;
			}
			else
			{
				flNewIntensity -= 5.0;
			}
			
			g_MusicData[iClient].CurrentLevel = newLevel;
			g_MusicData[iClient].DelayTimer = CreateTimer(60.0, ResetMusicCooldown, iClient);
			CPFSoundController.SwitchMusic(iClient);
		}
		g_MusicData[iClient].Intensity = flNewIntensity;
	}

};

public void PlayToAllButClient(int iClient, char[] sJingle)
{
	int iClients[31];
	int total;
	int i;
	
	for (i = 1; i <= MaxClients; i++)
	{
		if (IsValidClient(i) && i != iClient && !GetCookieInt(g_cookieSound, i))
		{
			iClients[total++] = i;
		}
	}
	if (!total) return;
	
	EmitSound(	.clients = iClients,
				.numClients = total,
				.sample = sJingle,
				.entity = iClient,
				.level = SNDLEVEL_TRAIN		);
}

public Action ResetMusicCooldown(Handle hTimer, int iClient)
{
	g_MusicData[iClient].DelayTimer = INVALID_HANDLE;
}

public Action ResetBeepCooldown(Handle hTimer, int iClient)
{
	g_MusicData[iClient].BeepCooldown = false;
}