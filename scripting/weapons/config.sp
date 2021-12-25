#define CONFIG_WEAPONS "configs/pf/weapons.cfg"

ArrayList Config_LoadWeaponData()
{
	KeyValues kv = LoadFile(CONFIG_WEAPONS, "Weapons");
	if (kv == null) return null;
	
	static StringMap mRarity;
	if (mRarity == null)
	{
		mRarity = new StringMap();
		mRarity.SetValue("common", eWeaponsRarity_Common);
		mRarity.SetValue("uncommon", eWeaponsRarity_Uncommon);
		mRarity.SetValue("rare", eWeaponsRarity_Rare);
		mRarity.SetValue("pickup", eWeaponsRarity_Pickup);
	}
	
	static StringMap mSlot;
	if (mSlot == null)
	{
		mSlot = new StringMap();
		mSlot.SetValue("invalid", -1);
		mSlot.SetValue("primary", TFWeaponSlot_Primary);
		mSlot.SetValue("secondary", TFWeaponSlot_Secondary);
		mSlot.SetValue("melee", TFWeaponSlot_Melee);
		mSlot.SetValue("action", TFWeaponSlot_Item1);
	}
	
	ArrayList aWeapons = new ArrayList(sizeof(Weapon));
	int iLength = 0;
	
	if (kv.JumpToKey("general", false))
	{
		if (kv.GotoFirstSubKey(false))
		{
			do
			{
				Weapon wep;
				
				char sBuffer[256];
				kv.GetSectionName(sBuffer, sizeof(sBuffer));
				
				wep.iIndex = StringToInt(sBuffer);
				
				kv.GetString("rarity", sBuffer, sizeof(sBuffer), "common");
				CStrToLower(sBuffer);
				
				mRarity.GetValue(sBuffer, wep.nRarity);
				
				kv.GetString("slot", sBuffer, sizeof(sBuffer), "invalid");
				CStrToLower(sBuffer);
				
				mSlot.GetValue(sBuffer, wep.iSlot);
				
				kv.GetString("model", wep.sModel, sizeof(wep.sModel));
				if (wep.sModel[0] == '\0') 
				{
					LogError("Weapon must have a model.");
					continue;
				}
				
				//Check if the model is already taken by another weapon
				Weapon duplicate;
				for (int i = 0; i < iLength; i++) 
				{
					aWeapons.GetArray(i, duplicate);
					
					if (StrEqual(wep.sModel, duplicate.sModel))
					{
						LogError("%i: Model \"%s\" is already taken by weapon %i.", wep.iIndex, wep.sModel, duplicate.iIndex);
						continue;
					}
				}
				
				kv.GetString("text", wep.sText, sizeof(wep.sText));
				kv.GetString("attrib", wep.sAttribs, sizeof(wep.sAttribs));
				kv.GetString("sound", wep.sSound, sizeof(wep.sSound));
				
				kv.GetString("callback", sBuffer, sizeof(sBuffer));
				wep.callback = view_as<Weapon_OnPickup>(GetFunctionByName(null, sBuffer));
				
				int iColor[4];
				kv.GetColor4("color", iColor);
				
				wep.iColor[0] = iColor[0];
				wep.iColor[1] = iColor[1];
				wep.iColor[2] = iColor[2];
				
				kv.GetVector("offset_origin", wep.vecOrigin);
				kv.GetVector("offset_angles", wep.vecAngles);
				
				aWeapons.PushArray(wep);
				iLength++;
			} 
			while (kv.GotoNextKey(false));
		}
	}
	
	delete kv;
	return aWeapons;
}

KeyValues LoadFile(const char[] sConfigFile, const char [] sConfigSection)
{
	char sConfigPath[PLATFORM_MAX_PATH];
	BuildPath(Path_SM, sConfigPath, sizeof(sConfigPath), sConfigFile);
	if(!FileExists(sConfigPath))
	{
		LogMessage("Failed to load PF config file (file missing): %s!", sConfigPath);
		return null;
	}
	
	KeyValues kv = new KeyValues(sConfigSection);
	kv.SetEscapeSequences(true);
	
	if(!kv.ImportFromFile(sConfigPath))
	{
		LogMessage("Failed to parse PF config file: %s!", sConfigPath);
		delete kv;
		return null;
	}
	
	return kv;
}
