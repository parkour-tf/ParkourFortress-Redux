#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PFCLIENT_INCLUDED
	#endinput
#endif
#define _PFCLIENT_INCLUDED

#define THISCLIENT	view_as<int>(this)

#define TRACEUR(%0)	view_as<CPFTraceur>(%0)
#define CPFTRACEUR_INVALID TRACEUR(0)

static bool g_bSDKHooked[MAXPLAYERS + 1];

methodmap CPFTraceur
{
	public CPFTraceur(int iClient)
	{
		return (IsValidClient(iClient)) ? view_as<CPFTraceur>(iClient) : view_as<CPFTraceur>(0);
	}
	
	public void Disconnect()
	{
		SDKUnhook(THISCLIENT, SDKHook_PreThink, OnPreThink);
		SDKUnhook(THISCLIENT, SDKHook_PostThink, OnPostThink);
		SDKUnhook(THISCLIENT, SDKHook_OnTakeDamage, OnTakeDamage);
		SDKUnhook(THISCLIENT, SDKHook_WeaponSwitch, OnWeaponSwitch);
		
		g_bSDKHooked[THISCLIENT] = false;
	}

	public void Spawn(bool bLate = false)
	{
		if (IsValidClient(THISCLIENT) && THISCLIENT > 0)
		{
			SetEntProp(THISCLIENT, Prop_Data, "m_takedamage", view_as<int>(DAMAGE_YES));
			SetEntProp(THISCLIENT, Prop_Send, "m_bDrawViewmodel", 0);
			
			SetPlayerAirAccel(THISCLIENT, g_cvarAirAcceleration.FloatValue);
			SetPlayerAccel(THISCLIENT, g_cvarAcceleration.FloatValue);
			
			if (GetEntityFlags(THISCLIENT) & FL_ATCONTROLS)
				SetEntityFlags(THISCLIENT, GetEntityFlags(THISCLIENT)&~FL_ATCONTROLS);
			
			if (TF2_GetPlayerClass(THISCLIENT) != TFClass_Scout && TF2_GetPlayerClass(THISCLIENT) != TFClass_Unknown)
			{
				TF2_SetPlayerClass(THISCLIENT, TFClass_Scout, _, true);
				RequestFrame(InstantRespawn, GetClientUserId(THISCLIENT));
			}
			else
			{
				if (!g_bSDKHooked[THISCLIENT])
				{
					SDKHook(THISCLIENT, SDKHook_PreThink, OnPreThink);
					SDKHook(THISCLIENT, SDKHook_PostThink, OnPostThink);
					SDKHook(THISCLIENT, SDKHook_OnTakeDamage, OnTakeDamage);
					SDKHook(THISCLIENT, SDKHook_WeaponSwitch, OnWeaponSwitch);
					
					g_bSDKHooked[THISCLIENT] = true;
				}
				
				SetCollisionGroup(THISCLIENT, g_ePFCollisionGroup);
				
				RemoveWallclimbCooldown(THISCLIENT); // TODO: FIND ANOTHER WAY AROUND THIS
			}
			
			CheckClientRopeCvars(THISCLIENT);
			CheckClientDownloadCvar(THISCLIENT);
			CheckClientWeapons(THISCLIENT);
		}
	}
	
	public static void Init(bool bLate = true)
	{
		if (bLate)
		{
			for (int i = 1; i < MaxClients; i++)
				(CPFTraceur(i)).Spawn(true);
		}
	}
};

public void InstantRespawn(int iUserID)
{
	TF2_RespawnPlayer(GetClientOfUserId(iUserID));
}
