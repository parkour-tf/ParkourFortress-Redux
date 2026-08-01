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

static bool g_bCanPickup[MAXPLAYERS + 1] = {false, ...};
static bool g_bTriggerEntity[2048] = {true, ...};

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

	return Plugin_Continue;
}

public Action Event_ResetPickup(Event event, const char[] name, bool dontBroadcast)
{
	int iClient = GetClientOfUserId(event.GetInt("userid"));
	
	if (IsValidClient(iClient))
	{
		g_bCanPickup[iClient] = true;
	}

	return Plugin_Continue;
}

bool AttemptGrabItem(int iClient)
{
	return false;
}

void PickupWeapon(int iClient, Weapon wep, int iTarget)
{
}

Action Timer_RegenWeapon(Handle timer, int iRef)
{
	int iEntity = EntRefToEntIndex(iRef);
	DebugOutput("Timer_RegenWeapon -- unhiding & rerolling weapon index %i", iEntity);
	RemoveEFlags(iEntity, EF_NODRAW);
	Weapons_HandleDefault(iEntity, GetWeaponType(iEntity));
	//AcceptEntityInput(iEntity, "EnableCollision");

	return Plugin_Continue;
}

Action Timer_ResetPickup(Handle timer, any iClient)
{
	if (IsValidClient(iClient))
		g_bCanPickup[iClient] = true;

	return Plugin_Continue;
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
