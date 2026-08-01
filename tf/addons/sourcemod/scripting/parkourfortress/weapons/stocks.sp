//Required for TF2_FlagWeaponNoDrop

////////////////////////////////////////////////////////////
//
// Math Utils
//
////////////////////////////////////////////////////////////

stock int max(int a, int b)
{
	return (a > b) ? a : b;
}

stock int min(int a, int b)
{
	return (a < b) ? a : b;
}

stock float fMax(float a, float b)
{
	return (a > b) ? a : b;
}

stock float fMin(float a, float b)
{
	return (a < b) ? a : b;
}

////////////////////////////////////////////////////////////
//
// Client Validity Utils
//
////////////////////////////////////////////////////////////

stock bool IsValidLivingClient(int iClient)
{
	return IsValidClient(iClient) && IsPlayerAlive(iClient);
}

////////////////////////////////////////////////////////////
//
// Round Utils
//
////////////////////////////////////////////////////////////

stock void TF2_EndRound(TFTeam nTeam)
{
	int iIndex = FindEntityByClassname(-1, "team_control_point_master");
	if (iIndex == -1)
	{
		iIndex = CreateEntityByName("team_control_point_master");
		DispatchSpawn(iIndex);
	}
	
	if (iIndex == -1)
	{
		LogError("[PF] Can't create 'team_control_point_master,' can't end round!");
	}
	else
	{
		AcceptEntityInput(iIndex, "Enable");
		SetVariantInt(view_as<int>(nTeam));
		AcceptEntityInput(iIndex, "SetWinner");
	}
}

////////////////////////////////////////////////////////////
//
// Weapon State Utils
//
////////////////////////////////////////////////////////////

stock int TF2_GetActiveWeapon(int iClient)
{
	return GetEntPropEnt(iClient, Prop_Send, "m_hActiveWeapon");
}

stock int TF2_GetActiveWeaponIndex(int iClient)
{
	int iWeapon = TF2_GetActiveWeapon(iClient);
	if (iWeapon > MaxClients)
		return GetEntProp(iWeapon, Prop_Send, "m_iItemDefinitionIndex");
	
	return -1;
}

stock bool TF2_GetActiveWeaponClassname(int iClient, char[] buffer, int maxlength)
{
	int iWeapon = TF2_GetActiveWeapon(iClient);
	if (iWeapon > MaxClients)
	{
		GetEdictClassname(iWeapon, buffer, maxlength);
		return true;
	}
	return false;
}

stock int TF2_GetSlotIndex(int iClient, int iSlot)
{
	int iWeapon = GetPlayerWeaponSlot(iClient, iSlot);
	if (iWeapon > MaxClients)
		return GetEntProp(iWeapon, Prop_Send, "m_iItemDefinitionIndex");
	
	return -1;
}

stock int TF2_GetActiveSlot(int iClient)
{
	int iWeapon = TF2_GetActiveWeapon(iClient);
	
	for (int iSlot = 0; iSlot <= WeaponSlot_BuilderEngie; iSlot++)
		if (GetPlayerWeaponSlot(iClient, iSlot) == iWeapon)
			return iSlot;
	
	return -1;
}

stock bool TF2_IsEquipped(int iClient, int iIndex)
{
	for (int iSlot = 0; iSlot <= WeaponSlot_BuilderEngie; iSlot++)
		if (TF2_GetSlotIndex(iClient, iSlot) == iIndex)
			return true;
	
	return false;
}

stock bool TF2_IsWielding(int iClient, int iIndex)
{
	return TF2_GetActiveWeaponIndex(iClient) == iIndex;
}

stock bool TF2_IsSlotClassname(int iClient, int iSlot, char[] sClassname)
{
	int iWeapon = GetPlayerWeaponSlot(iClient, iSlot);
	if (iWeapon > MaxClients && IsValidEdict(iWeapon))
	{
		char sClassname2[32];
		GetEdictClassname(iWeapon, sClassname2, sizeof(sClassname2));
		if (StrEqual(sClassname, sClassname2))
			return true;
	}
	
	return false;
}

stock int TF2_GetSlotInItem(int iIndex, TFClassType nClass)
{
	return 0;
}

////////////////////////////////////////////////////////////
//
// Entity Name Utils
//
////////////////////////////////////////////////////////////

stock bool IsClassnameContains(int iEntity, const char[] sClassname)
{
	if (IsValidEdict(iEntity) && IsValidEntity(iEntity))
	{
		char sClassname2[32];
		GetEdictClassname(iEntity, sClassname2, sizeof(sClassname2));
		return (StrContains(sClassname2, sClassname, false) != -1);
	}
	
	return false;
}

////////////////////////////////////////////////////////////
//
// Glow Utils
//
////////////////////////////////////////////////////////////

stock void TF2_SetGlow(int iClient, bool bEnable)
{
	SetEntProp(iClient, Prop_Send, "m_bGlowEnabled", bEnable);
}

////////////////////////////////////////////////////////////
//
// Ammo Add/Sub Utils
//
////////////////////////////////////////////////////////////

stock int TF2_GetClip(int iClient, int iSlot, bool bSlot=true)
{
	int iWeapon = GetPlayerWeaponSlot(iClient, iSlot);
	iWeapon = bSlot ? GetPlayerWeaponSlot(iClient, iSlot) : iSlot;
	if (iWeapon > MaxClients)
	{
		char sClassname[128];
		if (GetEntityClassname(iWeapon, sClassname, sizeof(sClassname)) && !(StrContains(sClassname, "tf_weapon_sniperrifle") == 0))
			return GetEntProp(iWeapon, Prop_Send, "m_iClip1");
	}
		
	return 0;
}

stock void TF2_SetClip(int iClient, int iSlot, int iClip, bool bSlot=true)
{
	int iWeapon;
	iWeapon = bSlot ? GetPlayerWeaponSlot(iClient, iSlot) : iSlot;
	if (iWeapon > MaxClients)
		SetEntProp(iWeapon, Prop_Send, "m_iClip1", iClip);
}

stock void TF2_AddClip(int iClient, int iSlot, int iClip)
{
	iClip += TF2_GetClip(iClient, iSlot);
	TF2_SetClip(iClient, iSlot, iClip);
}

stock void TF2_RemoveClip(int iClient, int iSlot, int iClip)
{
	iClip -= TF2_GetClip(iClient, iSlot);
	TF2_SetClip(iClient, iSlot, max(iClip, 0));
}

stock int TF2_GetAmmo(int iClient, int iSlot, bool bSlot=true)
{
	int iWeapon;
	iWeapon = bSlot ? GetPlayerWeaponSlot(iClient, iSlot) : iSlot;
	if (iWeapon > MaxClients)
	{
		int iAmmoType = GetEntProp(iWeapon, Prop_Send, "m_iPrimaryAmmoType");
		if (iAmmoType > -1)
			return GetEntProp(iClient, Prop_Send, "m_iAmmo", _, iAmmoType);
	}
	
	return 0;
}

stock void TF2_SetAmmo(int iClient, int iSlot, int iAmmo, bool bSlot=true)
{
	int iWeapon;
	iWeapon = bSlot ? GetPlayerWeaponSlot(iClient, iSlot) : iSlot;
	if (iWeapon > 0)
	{
		int iAmmoType = GetEntProp(iWeapon, Prop_Send, "m_iPrimaryAmmoType");
		if (iAmmoType > -1)
			SetEntProp(iClient, Prop_Send, "m_iAmmo", iAmmo, _, iAmmoType);
	}
}

stock void TF2_AddAmmo(int iClient, int iSlot, int iAmmo)
{
	iAmmo += TF2_GetAmmo(iClient, iSlot);
	TF2_SetAmmo(iClient, iSlot, iAmmo);
}

stock void TF2_RemoveAmmo(int iClient, int iSlot, int iAmmo)
{
	iAmmo -= TF2_GetAmmo(iClient, iSlot);
	TF2_SetAmmo(iClient, iSlot, max(iAmmo, 0));
}

////////////////////////////////////////////////////////////
//
// Weapon Utils
//
////////////////////////////////////////////////////////////

stock int TF2_CreateAndEquipWeapon(int iClient, int iIndex, char[] sAttribs = "")
{
	/*char sCommandResultBuf[256];
	ServerCommandEx(sCommandResultBuf, sizeof(sCommandResultBuf), "compat_giveweapon %d %d \"%s\"", iClient, iIndex, 
		sAttribs);
	
	return StringToInt(sCommandResultBuf);*/
	return -1; //TODO: Re-implement
}

stock void TF2_FlagWeaponDontDrop(int iWeapon, bool bVisibleHack = true)
{
}

stock int TF2_GetItemInSlot(int iClient, int iSlot)
{
	if (!IsValidClient(iClient) || iClient < 1)
		return -1;

	int iEntity = GetPlayerWeaponSlot(iClient, iSlot);
	if (iEntity > MaxClients)
		return iEntity;
	
	iEntity = SDK_GetEquippedWearable(iClient, iSlot);
	if (iEntity > MaxClients)
		return iEntity;
	
	return -1;
}

stock void TF2_RemoveItemInSlot(int iClient, int iSlot)
{
	int iEntity = GetPlayerWeaponSlot(iClient, iSlot);
	if (iEntity > MaxClients)
		TF2_RemoveWeaponSlot(iClient, iSlot);
	
	int iWearable = SDK_GetEquippedWearable(iClient, iSlot);
	if (iWearable > MaxClients)
		TF2_RemoveWearable(iClient, iWearable);
}

stock void TF2_SwitchToSlot(int iClient, int iSlot)
{
	if (!IsValidClient(iClient) || iClient < 1)
		return;
	int iWeapon = TF2_GetItemInSlot(iClient, WeaponSlot_Melee);
	if (iWeapon > MaxClients)
	{
		char sClassname[128];
		if (!GetEntityClassname(iWeapon, sClassname, sizeof(sClassname))) return;
		FakeClientCommand(iClient, "use %s", sClassname);
		SetEntPropEnt(iClient, Prop_Send, "m_hActiveWeapon", iWeapon);
	}
}

stock void CheckClientWeapons(int iClient)
{
	if (!IsValidClient(iClient) || iClient < 1)
		return;
	
	int iWeapon = 0;
	for (int iSlot = WeaponSlot_Primary; iSlot <= WeaponSlot_BuilderEngie; iSlot++)
	{
		iWeapon = TF2_GetItemInSlot(iClient, iSlot);
		if (iWeapon > MaxClients)
		{
			char sClassname[256];
			GetEntityClassname(iWeapon, sClassname, sizeof(sClassname));
			
			if (OnGiveNamedItem(sClassname, GetEntProp(iWeapon, Prop_Send, "m_iItemDefinitionIndex")) >= Plugin_Handled)
				TF2_RemoveItemInSlot(iClient, iSlot);
		}
	}
	GiveFists(iClient);
}

stock int FindEntityByTargetname(const char[] sTargetName, const char[] sClassname)
{
	char sBuffer[32];
	int iEntity = -1;
	
	while(strcmp(sClassname, sTargetName) != 0 && (iEntity = FindEntityByClassname(iEntity, sClassname)) != -1)
		GetEntPropString(iEntity, Prop_Data, "m_iName", sBuffer, sizeof(sBuffer));
	
	return iEntity;
}

stock void Shake(int iClient, float flAmplitude, float flDuration)
{
	BfWrite bf = UserMessageToBfWrite(StartMessageOne("Shake", iClient));
	bf.WriteByte(0); //0x0000 = start shake
	bf.WriteFloat(flAmplitude);
	bf.WriteFloat(1.0);
	bf.WriteFloat(flDuration);
	EndMessage();
}

stock void SpawnPickup(int iClient, const char[] sClassname)
{
	float vecOrigin[3];
	GetClientAbsOrigin(iClient, vecOrigin);
	vecOrigin[2] += 16.0;
	
	int iEntity = CreateEntityByName(sClassname);
	DispatchKeyValue(iEntity, "OnPlayerTouch", "!self,Kill,,0,-1");
	if (DispatchSpawn(iEntity))
	{
		SetEntProp(iEntity, Prop_Send, "m_iTeamNum", 0, 4);
		TeleportEntity(iEntity, vecOrigin, NULL_VECTOR, NULL_VECTOR);
		CreateTimer(0.15, Timer_KillEntity, EntIndexToEntRef(iEntity));
	}
}

Action Timer_KillEntity(Handle hTimer, int iRef)
{
	int iEntity = EntRefToEntIndex(iRef);
	if (IsValidEntity(iEntity))
		AcceptEntityInput(iEntity, "Kill");

	return Plugin_Continue;
}

//https://github.com/Mikusch/tfgo/blob/c6109ad9a2f04ac0267e0916145a8274c9f6662e/addons/sourcemod/scripting/tfgo/stocks.sp#L205-L237 :)
stock int TF2_GetItemSlot(int iIndex, TFClassType iClass)
{
	return 0;
}
