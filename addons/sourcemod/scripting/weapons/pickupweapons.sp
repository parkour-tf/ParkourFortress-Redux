#define PICKUP_COOLDOWN 	2.0

#define ENT_ONPICKUP	"FireUser1"
#define ENT_ONKILL		"FireUser2"

enum WeaponType
{
	WeaponType_Invalid,
	WeaponType_Static,
	WeaponType_Default,
	WeaponType_Spawn,
	WeaponType_Rare,
	WeaponType_RareSpawn,
	WeaponType_StaticSpawn,
	WeaponType_DefaultNoPickup,
};

char g_sVoWeaponScout[][PLATFORM_MAX_PATH] =
{
	"scout_mvm_loot_common03.mp3",
	"scout_mvm_loot_common04.mp3",
	"scout_mvm_loot_rare01.mp3",
	"scout_mvm_loot_rare02.mp3"
};

static float vecClampMins[3] = {-32.0, -32.0, -8.0};
static float vecClampMaxs[3] = {32.0, 32.0, 24.0};

static bool g_bCanPickup[TF_MAXPLAYERS] = false;
static bool g_bTriggerEntity[2048] = true;

static int m_iRare = 0;

void InitWeapons()
{
	HookEvent("teamplay_round_start", Event_WeaponsRoundStart);
	HookEvent("player_spawn", Event_ResetPickup);
	HookEvent("player_death", Event_ResetPickup);
}

void Weapons_ClientDisconnect(int iClient)
{
	g_bCanPickup[iClient] = true;
}

public void Weapons_HandleDefault(int iEntity, WeaponType nWeaponType)
{
	//If rare weapon cap is unreached and a dice roll is met, make it a "rare" weapon
	if (m_iRare < g_cvarWeaponMaxRare.IntValue && !GetRandomInt(0, g_cvarWeaponRareChance.IntValue))
	{
		SetRandomWeapon(iEntity, eWeaponsRarity_Rare);
		m_iRare++;
	}

	//Pick-ups
	else if (!GetRandomInt(0, 9) && nWeaponType != WeaponType_DefaultNoPickup)
	{
		SetRandomPickup(iEntity);
	}

	//Else make it either common or uncommon weapon
	else
	{
	int iCommon = GetRarityWeaponCount(eWeaponsRarity_Common);
	int iUncommon = GetRarityWeaponCount(eWeaponsRarity_Uncommon);
				
	if (GetRandomInt(0, iCommon + iUncommon) < iCommon)
		SetRandomWeapon(iEntity, eWeaponsRarity_Common);
	else
		SetRandomWeapon(iEntity, eWeaponsRarity_Uncommon);
	}
	
	//adjust bounding box to make pickup easier
	float vecMins[3], vecMaxs[3];
	GetEntPropVector(iEntity, Prop_Data, "m_vecMins", vecMins);
	GetEntPropVector(iEntity, Prop_Data, "m_vecMaxs", vecMaxs);
	
	vecMins[0] = fMin(vecMins[0],  vecClampMins[0]);
	vecMins[1] = fMin(vecMins[1],  vecClampMins[1]);
	vecMins[2] = fMin(vecMins[2],  vecClampMins[2]);
	
	vecMaxs[0] = fMax(vecMaxs[0],  vecClampMaxs[0]);
	vecMaxs[1] = fMax(vecMaxs[1],  vecClampMaxs[1]);
	vecMaxs[2] = fMax(vecMaxs[2],  vecClampMaxs[2]);
	
	SetEntPropVector(iEntity, Prop_Data, "m_vecMins", vecMins);
	SetEntPropVector(iEntity, Prop_Data, "m_vecMaxs", vecMaxs);
}

public Action Event_WeaponsRoundStart(Event event, const char[] name, bool dontBroadcast)
{
	int iEntity = -1;
	m_iRare = 0;
	
	ArrayList aWeaponsCommon = GetAllWeaponsWithRarity(eWeaponsRarity_Common);
	
	while ((iEntity = FindEntityByClassname2(iEntity, "prop_dynamic")) != -1)
	{
		WeaponType nWeaponType = GetWeaponType(iEntity);
		
		switch (nWeaponType)
		{
			case WeaponType_Spawn:
			{
				if (aWeaponsCommon.Length > 0)
				{
					//Make sure every spawn weapons is different
					int iRandom = GetRandomInt(0, aWeaponsCommon.Length - 1);
					
					Weapon wep;
					aWeaponsCommon.GetArray(iRandom, wep);
					
					SetWeaponModel(iEntity, wep);
					aWeaponsCommon.Erase(iRandom);
				}
				else
				{
					//If we already went through every spawn weapons, no point having rest of it
					AcceptEntityInput(iEntity, "Kill");
					continue;
				}
			}
			case WeaponType_Rare:
			{
				//If rare weapon cap is unreached, make it a "rare" weapon
				if (m_iRare < g_cvarWeaponMaxRare.IntValue)
				{
					SetRandomWeapon(iEntity, eWeaponsRarity_Rare);
					m_iRare++;
				}
				//Else make it a uncommon weapon
				else
				{
					SetRandomWeapon(iEntity, eWeaponsRarity_Uncommon);
				}
			}
			case WeaponType_RareSpawn:
			{
				SetRandomWeapon(iEntity, eWeaponsRarity_Rare);
			}
			case WeaponType_Default, WeaponType_DefaultNoPickup:
			{
				DebugOutput("Weapons_HandleSpawn -- working on index %i", iEntity);
				Weapons_HandleDefault(iEntity, nWeaponType);
			}
			default:
			{
				continue;
			}
		}
			
		AcceptEntityInput(iEntity, "DisableShadow");
		//AcceptEntityInput(iEntity, "EnableCollision");
			
		//Relocate weapon to higher height, looks much better
		float flPosition[3];
		GetEntPropVector(iEntity, Prop_Send, "m_vecOrigin", flPosition);
		flPosition[2] += 0.8;
		TeleportEntity(iEntity, flPosition, NULL_VECTOR, NULL_VECTOR);
			
		g_bTriggerEntity[iEntity] = true; //Indicate reset of the OnUser triggers
	}
	
	delete aWeaponsCommon;
}

public Action Event_ResetPickup(Event event, const char[] name, bool dontBroadcast)
{
	int iClient = GetClientOfUserId(event.GetInt("userid"));
	
	if (IsValidClient(iClient))
	{
		g_bCanPickup[iClient] = true;
	}
}

bool AttemptGrabItem(int iClient)
{	
	if (!g_bCanPickup[iClient]) return false;
	
	int iTarget = GetClientEntityVisible(iClient, "prop_dynamic", g_cvarWeaponGrabDistance.FloatValue);
	if (iTarget <= 0 || GetWeaponType(iTarget) == WeaponType_Invalid)
		return false;
	
	char sModel[256];
	GetEntityModel(iTarget, sModel, sizeof(sModel));
	Weapon wep;
	if (!GetWeaponFromModel(wep, sModel))
		return false;
	
	bool bAllowPickup = true;
	if (wep.callback != INVALID_FUNCTION)
	{
		Call_StartFunction(null, wep.callback);
		Call_PushCell(iClient);
		Call_Finish(bAllowPickup);
	}	
	
	if (wep.nRarity == eWeaponsRarity_Pickup)
	{
		if (!bAllowPickup)
			return false;
		
		if (wep.sSound[0] != '\0')
			EmitSoundToClient(iClient, wep.sSound);
		
		AcceptEntityInput(iTarget, ENT_ONKILL, iClient, iClient);
		AcceptEntityInput(iTarget, "Kill");
		
		return true;
	}
	
	int iIndex = wep.iIndex;
	
	if (iIndex > -1)
	{
		int iSlot = wep.iSlot;
		if (iSlot == -1)
			iSlot = TF2_GetItemSlot(iIndex, TF2_GetPlayerClass(iClient)); //fallback if no slot provided
			/*FIXME: weapon ammo isn't currently kept for exchanged weapons. 
			this leads to zero effort infinite ammo, which is a no-no when combined with sniper rifles
			fixing this will require a refactor of the weapon system which i do not currently have the
			brain blast for. for now, just deny pickup if the player already has something in that slot*/
		if (iSlot >= 0 && bAllowPickup && TF2_GetItemInSlot(iClient, iSlot) == -1)
		{
			PickupWeapon(iClient, wep, iTarget);
			return true;
		}
	}
	return false;
}

void PickupWeapon(int iClient, Weapon wep, int iTarget)
{
	if (wep.sSound[0] == '\0')
		EmitSoundToClient(iClient, "ui/item_heavy_gun_pickup.wav");
	else
		EmitSoundToClient(iClient, wep.sSound);
	
	g_bCanPickup[iClient] = false;
	CreateTimer(PICKUP_COOLDOWN, Timer_ResetPickup, iClient);
	
	char sSound[PLATFORM_MAX_PATH];
	Format(sSound, sizeof(sSound), g_sVoWeaponScout[GetRandomInt(0, sizeof(g_sVoWeaponScout)-1)]);
	
	EmitSoundToAll(sSound, iClient, SNDCHAN_VOICE, SNDLEVEL_SCREAMING);
	
	TFClassType nClass = TF2_GetPlayerClass(iClient);
	int iSlot = wep.iSlot;
	if (iSlot == -1)
		iSlot = TF2_GetItemSlot(wep.iIndex, nClass); //fallback if no slot provided
	
	if (GetWeaponType(iTarget) != WeaponType_Spawn
	&& GetWeaponType(iTarget) != WeaponType_RareSpawn
	&& GetWeaponType(iTarget) != WeaponType_StaticSpawn)
	{
		Weapon oldwep;
		
		int iEntity = GetPlayerWeaponSlot(iClient, iSlot);
		if (!IsValidEdict(iEntity))
		{
			//If weapon not found in slot, check if it a wearable
			int iWearable = SDK_GetEquippedWearable(iClient, iSlot);
			if (iWearable > MaxClients)
				iEntity = iWearable;
		}
		
		if (iEntity > MaxClients && IsValidEdict(iEntity))
		{
			int iOldIndex = GetEntProp(iEntity, Prop_Send, "m_iItemDefinitionIndex");
			if (9 <= iOldIndex && iOldIndex <= 12)	//Shotgun
				iOldIndex = 9;
			
			GetWeaponFromIndex(oldwep, iOldIndex);
		}
		
		if (oldwep.iIndex > 0)
		{
			EmitSoundToClient(iClient, "ui/item_heavy_gun_drop.wav");
			SetWeaponModel(iTarget, oldwep);
		}
		else
		{
			DebugOutput("PickupWeapon -- hiding weapon index %i", iTarget);
			//we're respawning, why bother killing the ent?
			AcceptEntityInput(iTarget, "DisableCollision");
			AddEFlags(iTarget, EF_NODRAW);
			AcceptEntityInput(iTarget, ENT_ONKILL, iClient, iClient);

			
			float fTime = g_cvarWeaponRespawnRandom.BoolValue ?
			GetRandomFloat(g_cvarWeaponRespawnMin.FloatValue, g_cvarWeaponRespawn.FloatValue) : g_cvarWeaponRespawn.FloatValue;
			
			CreateTimer(fTime, Timer_RegenWeapon, EntIndexToEntRef(iTarget));
			//AcceptEntityInput(iTarget, "Kill");
		}
	}

	//Remove sniper scope and slowdown cond if have one, otherwise can cause client crashes
	if (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed))
	{
		TF2_RemoveCondition(iClient, TFCond_Zoomed);
		TF2_RemoveCondition(iClient, TFCond_Slowed);
	}

	//Force crit reset
	int iRevengeCrits = GetEntProp(iClient, Prop_Send, "m_iRevengeCrits");
	if (iRevengeCrits > 0)
	{
		SetEntProp(iClient, Prop_Send, "m_iRevengeCrits", 0);
		TF2_RemoveCondition(iClient, TFCond_Kritzkrieged);
	}
	
	//If player already have item in his inv, remove it before we generate new weapon for him
	TF2_RemoveItemInSlot(iClient, iSlot);
	
	//Generate and equip weapon
	int iWeapon = TF2_CreateAndEquipWeapon(iClient, wep.iIndex, wep.sAttribs, wep.sText);
	SetEntProp(iClient, Prop_Send, "m_bDrawViewmodel", 1);
	CPFViewController.Hide(iClient);
	
	char sClassname[256];
	TF2Econ_GetItemClassName(wep.iIndex, sClassname, sizeof(sClassname));
	if (StrContains(sClassname, "tf_wearable") == 0) 
	{ 
		if (GetEntPropEnt(iClient, Prop_Send, "m_hActiveWeapon") <= MaxClients)
		{
			//Looks like player's active weapon got replaced into wearable, fix that by using melee
			int iMelee = GetPlayerWeaponSlot(iClient, TFWeaponSlot_Melee);
			SetEntPropEnt(iClient, Prop_Send, "m_hActiveWeapon", iMelee);
		}
	}
	else 
	{
		SetEntPropEnt(iClient, Prop_Send, "m_hActiveWeapon", iWeapon);
		TF2_FlagWeaponDontDrop(iWeapon);
	}
	
	//Set ammo as weapon's max ammo
	if (HasEntProp(iWeapon, Prop_Send, "m_iPrimaryAmmoType"))	//Wearables dont have ammo netprop
	{
		int iAmmoType = GetEntProp(iWeapon, Prop_Send, "m_iPrimaryAmmoType");
		if (iAmmoType > -1)
		{
			//We want to set gas passer ammo empty, because thats how normal gas passer works
			int iMaxAmmo;
			if (wep.iIndex == 1180)
			{
				iMaxAmmo = 0;
				SetEntPropFloat(iClient, Prop_Send, "m_flItemChargeMeter", 0.0, 1);
			}
			else
			{
				iMaxAmmo = SDK_GetMaxAmmo(iClient, iAmmoType);
			}
			
			SetEntProp(iClient, Prop_Send, "m_iAmmo", iMaxAmmo, _, iAmmoType);
		}
	}
	
	//Trigger ENT_ONPICKUP
	if (g_bTriggerEntity[iTarget])
	{
		AcceptEntityInput(iTarget, ENT_ONPICKUP, iClient, iClient);
		g_bTriggerEntity[iTarget] = false;
	}
	
	Call_StartForward(g_hForwardWeaponPickup);
	Call_PushCell(iClient);
	Call_PushCell(iWeapon);
	Call_PushCell(wep.nRarity);
	Call_Finish();
}

public Action Timer_RegenWeapon(Handle timer, int iRef)
{
	int iEntity = EntRefToEntIndex(iRef);
	DebugOutput("Timer_RegenWeapon -- unhiding & rerolling weapon index %i", iEntity);
	RemoveEFlags(iEntity, EF_NODRAW);
	Weapons_HandleDefault(iEntity, GetWeaponType(iEntity));
	//AcceptEntityInput(iEntity, "EnableCollision");

	return Plugin_Continue;
}

public Action Timer_ResetPickup(Handle timer, any iClient)
{
	if (IsValidClient(iClient))
		g_bCanPickup[iClient] = true;
}

stock WeaponType GetWeaponType(int iEntity)
{
	char sName[255];
	GetEntPropString(iEntity, Prop_Data, "m_iName", sName, sizeof(sName));
	
	//Strcontains versus strequals on 2048 entities obviously shows strcontains as the winner
	if (StrContains(sName, "pf_weapon", false) != -1)
	{
		DebugOutput("GetWeaponType -- targetname %s found", sName);
		if (StrContains(sName, "pf_weapon_spawn", false) == 0) return WeaponType_Spawn; //Spawn: dont expire on pickup
		else if (StrContains(sName, "pf_weapon_rare_spawn", false) == 0) return WeaponType_RareSpawn; //Guaranteed rare and non-expiring
		else if (StrContains(sName, "pf_weapon_rare", false) == 0) return WeaponType_Rare; //Guaranteed rare
		else if (StrContains(sName, "pf_weapon_static_spawn", false) == 0) return WeaponType_StaticSpawn; //Static: don't change model and non-expiring
		else if (StrContains(sName, "pf_weapon_static", false) == 0) return WeaponType_Static; //Static: don't change model
		else if (StrContains(sName, "pf_weapon_nopickup", false) == 0) return WeaponType_DefaultNoPickup; //No pickup: this weapon can never become a pickup
		else return WeaponType_Default; //Normal
	}

	
	
	return WeaponType_Invalid;
}

stock void SetRandomPickup(int iEntity)
{
	//Reset angle
	float vecAngles[3];
	
	TeleportEntity(iEntity, NULL_VECTOR, vecAngles, NULL_VECTOR);
	SetRandomWeapon(iEntity, eWeaponsRarity_Pickup);
}

stock void SetRandomWeapon(int iEntity, eWeaponsRarity nRarity)
{
	DebugOutput("SetRandomWeapon --  index %i", iEntity);
	ArrayList aList = GetAllWeaponsWithRarity(nRarity);
	int iRandom = GetRandomInt(0, aList.Length - 1);
	
	Weapon wep;
	aList.GetArray(iRandom, wep);
	
	SetWeaponModel(iEntity, wep);
	
	if (wep.iColor[0] + wep.iColor[1] + wep.iColor[2] > 0)
	{
		SetEntityRenderMode(iEntity, RENDER_TRANSCOLOR);
		SetEntityRenderColor(iEntity, wep.iColor[0], wep.iColor[1], wep.iColor[2], 255);
	}
	
	delete aList;
}

stock void SetWeaponModel(int iEntity, Weapon wep)
{
	char sOldModel[256];
	GetEntityModel(iEntity, sOldModel, sizeof(sOldModel));
	
	float vecOrigin[3];
	GetEntPropVector(iEntity, Prop_Send, "m_vecOrigin", vecOrigin);
	
	float vecAngles[3];
	GetEntPropVector(iEntity, Prop_Send, "m_angRotation", vecAngles);
	
	//Offsets (will only work for pickups for now)
	if (wep.nRarity == eWeaponsRarity_Pickup)
	{
		AddVectors(vecOrigin, wep.vecOrigin, vecOrigin);
		AddVectors(vecAngles, wep.vecAngles, vecAngles);
		
		TeleportEntity(iEntity, vecOrigin, vecAngles, NULL_VECTOR);
	}

	SetEntityModel(iEntity, wep.sModel);

}

//Grabs the entity model by looking in the precache database of the server
stock void GetEntityModel(int iEntity, char[] sModel, int iMaxSize, char[] sPropName = "m_nModelIndex")
{
	int iIndex = GetEntProp(iEntity, Prop_Send, sPropName);
	GetModelPath(iIndex, sModel, iMaxSize);
}

stock void GetModelPath(int iIndex, char[] sModel, int iMaxSize)
{
	int iTable = FindStringTable("modelprecache");
	ReadStringTable(iTable, iIndex, sModel, iMaxSize);
}
