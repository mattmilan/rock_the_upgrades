ArrayList SFX_Enable;
ArrayList SFX_Disable;

void PrecacheRequiredAssets() {
	// This model will be assigned to each func_upgradestation
	// It is not visibile, but it's required for collisions to work
    char g_ArbitraryModel[] = "models/props_gameplay/resupply_locker.mdl";
    if (!IsModelPrecached(g_ArbitraryModel)) { PrecacheModel(g_ArbitraryModel, true); }

    SFX_Enable = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
	SFX_Disable = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));

    SFX_Enable.PushString("mvm/mvm_bought_in.wav");
    SFX_Enable.PushString("vo/engineer_mvm_collect_credits01.mp3");
    SFX_Enable.PushString("vo/heavy_mvm_collect_credits01.mp3");
    SFX_Enable.PushString("vo/medic_mvm_collect_credits03.mp3");
    SFX_Enable.PushString("vo/medic_mvm_collect_credits04.mp3");
    SFX_Enable.PushString("vo/soldier_mvm_collect_credits01.mp3");
    SFX_Enable.PushString("vo/soldier_mvm_collect_credits02.mp3");

    SFX_Disable.PushString("vo/announcer_sd_monkeynaut_end_crash01.mp3");
	SFX_Disable.PushString("vo/demoman_sf12_badmagic07.mp3")
	SFX_Disable.PushString("mvm/mvm_player_died.wav");

    // Precache all sounds
    for (int i = 0; i < SFX_Enable.Length; i++) {
        char sound[PLATFORM_MAX_PATH];
        SFX_Enable.GetString(i, sound, sizeof(sound));
        PrecacheSound(sound, true);
    }
    for (int i = 0; i < SFX_Disable.Length; i++) {
        char sound[PLATFORM_MAX_PATH];
        SFX_Disable.GetString(i, sound, sizeof(sound));
        PrecacheSound(sound, true);
    }
}

public void EnableUpgrades() {
	// This is also being checked and reported upstream
	if (UpgradesEnabled()) { return; }

	// Ensure Prop is set before granting currency or spawning upgrade stations
	PrintToChatAll("[SM] %t", "RTU Enabled");
	PlayRandomSoundFrom(SFX_Enable);
	GameRules_SetProp("m_nForceUpgrades", 2, 4);
	SetAllCurrency(StartingCurrency());
	SpawnUpgradeStations();
}

public void DisableUpgrades() {
	// This is also being checked and reported upstream
	if (!UpgradesEnabled()) { return; }

	PrintToChatAll("[SM] %t", "RTU Disabled");
	PlayRandomSoundFrom(SFX_Disable);
	ResetUpgrades(.silent = true)
	GameRules_SetProp("m_nForceUpgrades", 0, 4);
	RemoveUpgradeStations();
}

void ResetUpgrades(bool silent = false) {
	// This is also being checked and reported upstream
	if (!UpgradesEnabled()) { return; }

	if (!silent) { PrintToChatAll("[SM] %t", "RTU Reset"); }

	SetAllCurrency(StartingCurrency());
	ClearPlayerAndItemUpgrades();
}

void ClearPlayerAndItemUpgrades() {
	for (int i = 1; i <= MaxClients; i++) {
		if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }

		// Clear player upgrades
		TF2Attrib_RemoveAll(i);

		// Clear weapon upgrades
		int weaponSlots = 5;
		for (int j = 0; j < weaponSlots; j++) {
			int weapon = GetPlayerWeaponSlot(i, j);
			if (weapon == -1) { continue; }

			TF2Attrib_RemoveAll(weapon);
		}

		// Remove lingering effects of upgrades
		TF2_RegeneratePlayer(i);
	}
}

public bool UpgradesEnabled() {
	return GameRules_GetProp("m_nForceUpgrades") == 2;
}

void SpawnUpgradeStations() {
	int resupplyEntity = -1;
	while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
		SpawnUpgradeStationAt(resupplyEntity);
	}
}

void SpawnUpgradeStationAt(int entity) {
	float center[3]; GetEntPropVector(entity, Prop_Data, "m_vecOrigin", center);

	// TODO: I dont like this. What if a func_regenerate has origin at (0,0,0)?
	if (GetVectorLength(center) < 0.001) GetCenterFromMinsMaxes(entity, center);

	BuildUpgradeStation(center);
}

void RemoveUpgradeStations() {
	int entity = -1;
	while ((entity = FindEntityByClassname(entity, "func_upgradestation")) != -1) {
		RemoveEntity(entity);
	}
}

void BuildUpgradeStation(float center[3]) {
	// Create, spawn, and position the upgrade station
	int funcUpgrade = CreateEntityByName("func_upgradestation");
	DispatchKeyValueVector(funcUpgrade, "origin", center);
	if (!DispatchSpawn(funcUpgrade)) { return; }

	// Some properties need to be set after spawning/activation
	ActivateEntity(funcUpgrade);
	TeleportEntity(funcUpgrade, center, NULL_VECTOR, NULL_VECTOR);
	AssignPropertiesTo(funcUpgrade);
}

// Make the upgrade station collidable, which opens/closes the upgrades menu
void AssignPropertiesTo(int funcUpgrade) {
	// HACK: collision detection doesn't work unless we assign some arbitrary pre-cached model
	SetEntityModel(funcUpgrade, "models/props_gameplay/resupply_locker.mdl");

	// make collidable with players
	SetEntProp(funcUpgrade, Prop_Data, "m_nSolidType", 2);
	SetEntProp(funcUpgrade, Prop_Data, "m_usSolidFlags", 12);

	// TODO: Sizing could be fine tuned pending player feedback; decent starting point.
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMins", {-64.0, -64.0, -32.0 });
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMaxs", { 64.0, 64.0, 32.0 });
}


void GetCenterFromMinsMaxes(int entity, float center[3]) {
	float   mins[3]; GetEntPropVector(entity, Prop_Data, "m_vecMins", mins);
	float   maxs[3]; GetEntPropVector(entity, Prop_Data, "m_vecMaxs", maxs);

	SubtractVectors(maxs, mins, center);
	ScaleVector(center, 0.5);
	AddVectors(mins, center, center);
}

void PlayRandomSoundFrom(ArrayList soundArray) {
	int rand = GetRandomInt(0, soundArray.Length - 1)
	char sound[PLATFORM_MAX_PATH];
	soundArray.GetString(rand, sound, sizeof(sound));
	EmitSoundToAll(sound);
}
