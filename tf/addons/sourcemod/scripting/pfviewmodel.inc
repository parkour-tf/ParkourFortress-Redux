#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFVIEWMODEL_INCLUDED
	#endinput
#endif
#define _PFVIEWMODEL_INCLUDED

stock const char SCOUT_MODEL_V[] = "models/kirillian/weapons/pf/pf_arms_pub_v3_6.mdl";

stock const char g_strAnimState[][] =
{
	"none",
	"climb",
	"climb_idle",
	"doorsmash",
	"handslide_left",
	"handslide_right",
	"idle",
	"ledgegrab",
	"ledgegrab_idle",
	"leap",
	"longfall",
	"longfall_idle",
	"onlydeathbelow",
	"punch_a",
	"punch_crit",
	"roll",
	"running",
	"thumbs_up",
	"verticalclimb",
	"waterslide",
	"zipline",
	"zipline_idle"
};

enum eAnimState
{
    AnimState_None = 0,
    AnimState_Climb,
    AnimState_ClimbIdle,
    AnimState_Doorsmash,
    AnimState_HandslideLeft,
    AnimState_HandslideRight,
    AnimState_Idle,
    AnimState_Ledgegrab,
    AnimState_LedgegrabIdle,
    AnimState_Leap,
    AnimState_Longfall,
    AnimState_LongfallIdle,
    AnimState_OnlyDeathBelow,
    AnimState_Punch,
    AnimState_PunchCrit,
    AnimState_Roll,
    AnimState_Running,
    AnimState_ThumbsUp,
    AnimState_VerticalClimb,
    AnimState_Waterslide,
    AnimState_Zipline,
    AnimState_ZiplineIdle
};

enum struct ViewmodelData
{
	int Viewmodel;
	int TFViewmodel;
	int Proxy;
	int PhysProp;
	float LastVelocity;
	bool Locked;
	bool UsingAltLock;
	bool DontInterrupt;
	bool Hidden;
	float AngleLock;
	float HeightOffset;
	eAnimState Sequence;
	eAnimState QueuedSequence;
	eAnimState DefaultSequence;
}

static ViewmodelData g_ViewmodelData[MAXPLAYERS + 1];

methodmap CPFViewController
{
	
	public static bool GetDontInterrupt(int iClient)
	{
		return g_ViewmodelData[iClient].DontInterrupt;
	}
	
	public static void SetDontInterrupt(int iClient, bool input)
	{
		g_ViewmodelData[iClient].DontInterrupt = input;
	}
	
	public static eAnimState GetQueuedSequence(int iClient)
	{
		return g_ViewmodelData[iClient].QueuedSequence;
	}
	
	public static int GetPFViewmodel(int iClient)
	{
		return EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
	}
	
	public static eAnimState GetSequence(int iClient)
	{
		return g_ViewmodelData[iClient].Sequence;
	}
	
	public static void SetSequence(int iClient, eAnimState eSequence, bool StartAnimation = true)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		if (iViewModel > 0 && IsValidEntity(iViewModel))
		{
			g_ViewmodelData[iClient].Sequence = eSequence;
			CPFViewController.SetDontInterrupt(iClient, false);
			
			if (StartAnimation)
			{
				SetVariantString(g_strAnimState[view_as<int>(eSequence)]);
				AcceptEntityInput(iViewModel, "SetAnimation");
			}
		}
	}
	
	public static eAnimState GetDefaultSequence(int iClient)
	{
		return g_ViewmodelData[iClient].DefaultSequence;
	}
	
	public static void SetDefaultSequence(int iClient, eAnimState eSequence)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		if (iViewModel > 0 && IsValidEntity(iViewModel))
		{
			g_ViewmodelData[iClient].DefaultSequence = eSequence;
			
			SetVariantString(g_strAnimState[view_as<int>(eSequence)]);
			AcceptEntityInput(iViewModel, "SetDefaultAnimation");
		}
	}
	
	
	public static void Init()
	{
		g_cookieViewmodel = new Cookie("parkourviewmodel", "Disable rendering viewmodel", CookieAccess_Protected);
		
		SuperPrecacheModel(SCOUT_MODEL_V);
		SuperPrecacheMaterial("models/player/kirillian/pk_scout/pk_scout");
		SuperPrecacheMaterial("models/player/kirillian/pk_scout/pk_scout_blue");
		SuperPrecacheMaterial("models/player/kirillian/pk_scout/pk_scout_blue_invun");
		
		AddFileToDownloadsTable("materials/models/player/kirillian/pk_scout/pk_scout_normal.vtf");
		
		AddFileToDownloadsTable("materials/models/player/kirillian/pk_scout/pk_scout_red_invun.vtf");
		AddFileToDownloadsTable("materials/models/player/kirillian/pk_scout/pk_scout_invun.vmt");
		PrecacheModel("models/player/kirillian/pk_scout/pk_scout_invun", true);
		PrecacheModel("models/props_gameplay/can_crushed001.mdl", true);
	}
	
	public static void SetTFViewmodel(int iEntity)
	{
		FindTFViewModelOwner(iEntity);
	}
	
	public static void Kill(int iClient)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		int iProxyEnt = EntRefToEntIndex(g_ViewmodelData[iClient].Proxy);
		if (iViewModel < 1 || !IsValidEntity(iViewModel) || iProxyEnt < 1 || !IsValidEntity(iProxyEnt)) return;
		
		SDKUnhook(iViewModel, SDKHook_SetTransmit, OnSetTransmitViewmodel);
		SDKUnhook(iProxyEnt, SDKHook_SetTransmit, OnSetTransmitProxy);
		
		RemoveEntity(iViewModel);
		RemoveEntity(iProxyEnt);
		
		g_ViewmodelData[iClient].Viewmodel = -1;
		g_ViewmodelData[iClient].Proxy = -1;
		g_ViewmodelData[iClient].PhysProp = -1;
		g_ViewmodelData[iClient].DontInterrupt = false;
		g_ViewmodelData[iClient].Locked = false;
		g_ViewmodelData[iClient].UsingAltLock = false;
		g_ViewmodelData[iClient].AngleLock = -1.0;
		g_ViewmodelData[iClient].HeightOffset = -1.0;
	}


	public static void KillAll()
	{
		for (int iClient = 1; iClient < MaxClients; iClient++)
		{
			CPFViewController.Kill(iClient);
		}
	}
	
	public static void Spawn(int iClient)
	{
		if (g_ViewmodelData[iClient].Viewmodel > 0 && IsValidEntity(EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel)))
		{
			CPFViewController.Kill(iClient);
		}
		
		g_ViewmodelData[iClient].Viewmodel = CreateViewmodel(iClient);
	}
	
	public static void Disconnect(int iClient)
	{
		CPFViewController.Kill(iClient);
		g_ViewmodelData[iClient].TFViewmodel = -1;
	}
	
	
	public static void SetPlaybackRate(int iClient, float flPlaybackRate)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		if (iViewModel > 0 && IsValidEntity(iViewModel))
		{
			SetVariantFloat(flPlaybackRate);
			AcceptEntityInput(iViewModel, "SetPlaybackRate");
		}
	}
	
	public static void Queue(int iClient, eAnimState eSequence, float flPlaybackRate = -1.0, bool bForceInstant = false)
	{
		g_ViewmodelData[iClient].QueuedSequence = eSequence;
		
		if (flPlaybackRate > 0.0)
			CPFViewController.SetPlaybackRate(iClient, flPlaybackRate);
		
		if (bForceInstant)
		{
			CPFViewController.SetSequence(iClient, eSequence);
		}
			
	}
	
	public static void LockRotation(int iClient, float flYaw, float flHeightOffset = 0.0)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		int iProxyEnt = EntRefToEntIndex(g_ViewmodelData[iClient].Proxy);
		if ((iViewModel < 1 || !IsValidEntity(iViewModel)) || (iProxyEnt < 1 || !IsValidEntity(iProxyEnt)))
			return;
		
		RemoveEFlags(iProxyEnt, EF_PARENT_ANIMATES|EF_BONEMERGE|EF_BONEMERGE_FASTCULL);
		AddEFlags(iViewModel, EF_PARENT_ANIMATES|EF_BONEMERGE|EF_BONEMERGE_FASTCULL);
		SetVariantString("");
		AcceptEntityInput(iProxyEnt, "SetParent");
		SetEntProp(iViewModel, Prop_Data, "m_nBody", 2);
		
		g_ViewmodelData[iClient].Locked = true;
		g_ViewmodelData[iClient].AngleLock = flYaw;
		g_ViewmodelData[iClient].HeightOffset = flHeightOffset;
		
		float vecOrigin[3], vecAngles[3];
		//GetEntPropVector(g_ViewmodelData[iClient].Proxy, Prop_Send, "m_angRotation", vecAngles);
		vecAngles[1] = flYaw;
		GetClientEyePosition(iClient, vecOrigin);
		TeleportEntity(iProxyEnt, vecOrigin, vecAngles, NULL_VECTOR);
	}
	
	public static void UnlockRotation(int iClient)
	{
		CPFViewController.Kill(iClient);
		g_ViewmodelData[iClient].Viewmodel = CreateViewmodel(iClient);
	}
	
	public static void SetHidden(int iClient, bool hidden)
	{
		g_ViewmodelData[iClient].Hidden = hidden;
	}
	
	public static void Hide(int iClient)
	{
		int iViewmodel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		SetEFlags(iViewmodel, EF_NODRAW);
	}
	
	public static void Unhide(int iClient, bool force = true)
	{
		int iViewmodel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		if (!force)
			if (GetCookieInt(g_cookieViewmodel, iClient))
				return;
		if (!g_ViewmodelData[iClient].Hidden && IsValidEntity(iViewmodel))
		{
			RemoveEFlags(iViewmodel, EF_NODRAW);
			AcceptEntityInput(iViewmodel, "DisableShadow");
		}
	}

	
	public static void Update(int iClient)
	{
		int iViewModel = EntRefToEntIndex(g_ViewmodelData[iClient].Viewmodel);
		if (iViewModel < 1 || !IsValidEntity(iViewModel))
			return;
		
		if (g_ViewmodelData[iClient].Sequence != g_ViewmodelData[iClient].QueuedSequence)
		{
			CPFViewController.SetSequence(iClient, g_ViewmodelData[iClient].QueuedSequence);
			g_ViewmodelData[iClient].QueuedSequence = AnimState_None;
		}
			
			
		if ((g_ViewmodelData[iClient].Sequence == AnimState_Ledgegrab || g_ViewmodelData[iClient].Sequence == AnimState_LedgegrabIdle)
			&& CPFStateController.Get(iClient) == State_Hang)
		{
			float vecAngles[3];
			GetClientEyeAngles(iClient, vecAngles);
			if (vecAngles[0] > 35.0)
			{
				vecAngles[0] = 35.0;
				TeleportEntity(iClient, NULL_VECTOR, vecAngles, NULL_VECTOR);
			}
		}
		
		if (g_ViewmodelData[iClient].Locked)
		{
			if (g_ViewmodelData[iClient].UsingAltLock)
				UpdatePosition(iClient);
			else
			{
				float vecOrigin[3], vecAngles[3], vecVelocity[3];
				GetClientAbsOrigin(iClient, vecOrigin);
				GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
				//AddVectors(vecOrigin, vecVelocity, vecOrigin);
				vecOrigin[2] += (38.0 + g_ViewmodelData[iClient].HeightOffset);
			
				GetEntPropVector(g_ViewmodelData[iClient].Proxy, Prop_Send, "m_angRotation", vecAngles);
				vecAngles[1] = g_ViewmodelData[iClient].AngleLock;
			
				TeleportEntity(g_ViewmodelData[iClient].Proxy, vecOrigin, vecAngles, NULL_VECTOR);
				TeleportEntity(g_ViewmodelData[iClient].Viewmodel, NULL_VECTOR, NULL_VECTOR, vecVelocity);
			}
		}
	}
};

int CreateViewmodel(int iClient)
{
	if (!IsValidClient(iClient))
		return -1;

	int iEntity = CreateEntityByName("prop_dynamic");
	if (!IsValidEntity(iEntity))
		return -1;
	
	if (TF2_GetClientTeam(iClient) == TFTeam_Blue)
		DispatchKeyValue(iEntity, "skin", "1");
	
	DispatchKeyValue(iEntity, "model", SCOUT_MODEL_V);
	DispatchKeyValue(iEntity, "DefaultAnim", "idle");
	//DispatchKeyValue(iEntity, "sequence", "6"); //run == 17
	DispatchKeyValue(iEntity, "disablereceiveshadows", "0");
	DispatchKeyValue(iEntity, "disableshadows", "1");
	DispatchKeyValue(iEntity, "solid", "0");
	if (!g_cvarViewmodels.IntValue || g_ViewmodelData[iClient].Hidden)
		SetEFlags(iEntity, EF_NODRAW);
	
	float vecOrigin[3], vecAngles[3];
	GetClientAbsOrigin(iClient, vecOrigin);
	GetClientAbsAngles(iClient, vecAngles);
	
	vecAngles[1] += 180.0;
	vecOrigin[2] += 4.0; //remove this once hitboxes fixed
	TeleportEntity(iEntity, vecOrigin, vecAngles, NULL_VECTOR);
	DispatchSpawn(iEntity);
	
	int iProxy = CreateProxyEntity(iClient, iEntity);
	if (iProxy == -1)
	{
		if (iEntity > 1 && IsValidEntity(iEntity))
			AcceptEntityInput(iEntity, "Kill");
		
		return -1;
	}
	
	SetEntityMoveType(iEntity, MOVETYPE_NOCLIP);
	
	SetEntPropEnt(iEntity, Prop_Send, "m_hOwnerEntity", iClient);
	
	SetCollisionGroup(iEntity, COLLISION_GROUP_NONE);
	
	SDKHook(iEntity, SDKHook_SetTransmit, OnSetTransmitViewmodel);
	

	
	return EntIndexToEntRef(iEntity);
}

int CreateProxyEntity(int iClient, int iProp)
{
	int iEntity = CreateEntityByName("tf_wearable");
	if (!IsValidEntity(iEntity) || !IsValidEntity(iProp))
		return -1;
	
	float vecOrigin[3], vecAngles[3];
	GetClientAbsOrigin(iClient, vecOrigin);
	GetClientAbsAngles(iClient, vecAngles);
	vecAngles[1] += 180.0;
	
	TeleportEntity(iEntity, vecOrigin, vecAngles, NULL_VECTOR);
	DispatchSpawn(iEntity);
	
	SetVariantString("!activator");
	AcceptEntityInput(iProp, "SetParent", iEntity);
	SetVariantString("!activator");
	AcceptEntityInput(iEntity, "SetParent", g_ViewmodelData[iClient].TFViewmodel);
	
	SetEFlags(iEntity, EF_BONEMERGE|EF_BONEMERGE_FASTCULL|EF_PARENT_ANIMATES|EF_NODRAW);
	
	SDKHook(iEntity, SDKHook_SetTransmit, OnSetTransmitProxy);
	
	g_ViewmodelData[iClient].Proxy = EntIndexToEntRef(iEntity);
	return g_ViewmodelData[iClient].Proxy;
}

public void UpdatePosition(int iClient)
{
	int iPhys = EntRefToEntIndex(g_ViewmodelData[iClient].PhysProp);
	if (IsValidEntity(iPhys))
	return;
	
	float vecAngles[3], vecAbsVelocity[3], vecOrigin[3], vecPropOrigin[3];
	GetClientEyePosition(iClient, vecOrigin);
	GetEntPropVector(iClient, Prop_Data, "m_vecOrigin", vecPropOrigin);
	
	vecOrigin[2] -= 16.0;
	
	GetEntPropVector(iClient, Prop_Data, "m_vecAbsVelocity", vecAbsVelocity);
	
	if (VectorIsZero(vecAbsVelocity))
	{
		TeleportEntity(iPhys, vecOrigin, NULL_VECTOR, NULL_VECTOR);
	}
	else if ((vecAbsVelocity[2] > 0.0 && FloatAbs(g_ViewmodelData[iClient].LastVelocity) < 0.01) || (vecAbsVelocity[2] > 0.0 && g_ViewmodelData[iClient].LastVelocity < 0.0))
	{
		vecOrigin[2] += 20.0;
		TeleportEntity(iPhys, vecOrigin, NULL_VECTOR, NULL_VECTOR);
	}
	else if ((vecAbsVelocity[2] < 0.0 && FloatAbs(g_ViewmodelData[iClient].LastVelocity) < 0.01) || (vecAbsVelocity[2] < 0.0 && g_ViewmodelData[iClient].LastVelocity > 0.0))
	{
		vecOrigin[2] -= 22.0;
		TeleportEntity(iPhys, vecOrigin, NULL_VECTOR, NULL_VECTOR);
	}
	
	if (vecAbsVelocity[2] > 0.0)
	{
		ScaleVector(vecAbsVelocity, 1.06);
	}
	else
	{
		ScaleVector(vecAbsVelocity, 0.9825);
	}
	
	vecAngles[1] = g_ViewmodelData[iClient].AngleLock;
	
	g_ViewmodelData[iClient].LastVelocity = vecAbsVelocity[2];
	TeleportEntity(iPhys, NULL_VECTOR, vecAngles, vecAbsVelocity);
}

public Action OnSetTransmitProxy(int iEntity, int iClient)
{
	if (!IsValidClient(iClient))
		return Plugin_Continue;
	
	if (!IsValidEntity(iEntity))
		return Plugin_Continue;
		
	int iEntOwner = GetEntPropEnt(iEntity, Prop_Send, "m_hOwnerEntity");
	if (!IsValidClient(iEntOwner))
		return Plugin_Continue;
		
	if (iEntity != EntRefToEntIndex(g_ViewmodelData[iClient].Proxy))
	{
		DebugOutput("Proxy %i vs %i", iEntity, EntRefToEntIndex(g_ViewmodelData[iClient].Proxy));
		AcceptEntityInput(iEntity, "Kill");
		RemoveEdict(iEntity);
		SDKUnhook(iEntity, SDKHook_SetTransmit, OnSetTransmitProxy);
		return Plugin_Continue;
	}
	
	return Plugin_Continue;
}

public Action OnSetTransmitViewmodel(int iEntity, int iClient)
{
	if (!IsValidClient(iClient))
		return Plugin_Continue;
	
	if (!IsValidEntity(iEntity))
		return Plugin_Continue;
	
	int iEntOwner = GetEntPropEnt(iEntity, Prop_Send, "m_hOwnerEntity");
	if (!IsValidClient(iEntOwner))
		return Plugin_Continue;
		
	if (iEntity != EntRefToEntIndex(g_ViewmodelData[iEntOwner].Viewmodel))
	{
		DebugOutput("Viewmodel %i vs %i", iEntity, EntRefToEntIndex(g_ViewmodelData[iEntOwner].Viewmodel));
		SDKUnhook(iEntity, SDKHook_SetTransmit, OnSetTransmitViewmodel);
		RemoveEntity(iEntity);
		
		return Plugin_Handled;
	}
	
	if (g_cookieViewmodel != null)
	{
		if (GetCookieInt(g_cookieViewmodel, iClient))
		return Plugin_Handled;
	}
	
	if (iEntOwner != iClient)
	{
		int iObsTarget = GetEntPropEnt(iClient, Prop_Send, "m_hObserverTarget");
		if (iObsTarget == iEntOwner && view_as<SpecMode_t>(GetEntProp(iClient, Prop_Send, "m_iObserverMode")) == SPECMODE_FIRSTPERSON)
		    return Plugin_Continue;
	
		if (iEntity == g_ViewmodelData[iClient].Viewmodel)
			return Plugin_Continue;
	
		return Plugin_Handled;
	}
	
	return	(TF2_IsPlayerInCondition(iClient, TFCond_Taunting) || (!IsPlayerAlive(iClient)) || TF2_IsPlayerInCondition(iClient, TFCond_Dazed) || GetEntProp(iClient, Prop_Send, "m_nForceTauntCam")) ? Plugin_Handled : Plugin_Continue;
}

public void FindTFViewModelOwner(int iEntity)
{
	int iClient = GetEntPropEnt(iEntity, Prop_Send, "m_hOwner");
	if (IsValidClient(iClient))
	{
		g_ViewmodelData[iClient].TFViewmodel = EntIndexToEntRef(iEntity);
		DebugOutput("FindTFViewModelOwner --- Registered %i as tf_viewmodel for %N", iEntity, iClient);
	}
	else
	{
		RequestFrame(FindTFViewModelOwnerPost, iEntity);
	}
}

public void FindTFViewModelOwnerPost(int iEntity)
{
	int iClient = GetEntPropEnt(iEntity, Prop_Send, "m_hOwner");
	if (IsValidClient(iClient))
	{
		g_ViewmodelData[iClient].TFViewmodel = EntIndexToEntRef(iEntity);
		DebugOutput("FindTFViewModelOwnerPost --- Registered %i as tf_viewmodel for %N", iEntity, iClient);
	}
}