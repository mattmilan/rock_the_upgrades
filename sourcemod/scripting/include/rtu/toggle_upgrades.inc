ArrayList SFX_Enable;
ArrayList SFX_Disable;
// Wierd hack that's required for upgrade stations to function
void PrecacheRequiredAssets() {
    char g_ArbitraryModel[] = "models/props_gameplay/resupply_locker.mdl";
    if (!IsModelPrecached(g_ArbitraryModel)) {	PrecacheModel(g_ArbitraryModel, true); }

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
	SFX_Disable.PushString("items/cart_warning_single.wav");
	SFX_Disable.PushString("vo/demoman_sf12_badmagic07.mp3")
	SFX_Disable.PushString("music/mvm_lost_wave.wav");
	SFX_Disable.PushString("vo/taunts/heavy/heavy_taunt_flip_fail_19.mp3");
	SFX_Disable.PushString("vo/heavy_mvm_rage03.mp3");
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
	PrintToChatAll("[SM] Attempting to Enable Upgrades");
	if (UpgradesEnabled()) { return; }

	SetCurrencyForAll(StartingCurrency());
	PlayActivationSound();
	PrintToChatAll("[SM] %t", "RTU Enabled");
	GameRules_SetProp("m_nForceUpgrades", 2, 4);
	SpawnUpgradeStations();
}

public void DisableUpgrades() {
	PrintToChatAll("[SM] Attempting to Disable Upgrades");
	if (!UpgradesEnabled()) { return; }

	PlayDeactivationSound();
	PrintToChatAll("[SM] %t", "RTU Disabled");
	GameRules_SetProp("m_nForceUpgrades", 0, 4);
	RemoveUpgradeStations();
}

void ResetUpgrades() {
	PrintToChatAll("[SM] Attempting to Reset Upgrades");
	if (!UpgradesEnabled()) { return; }
	// TODO: SFX Explicitly for reset
	PlayDeactivationSound();
	PrintToChatAll("[SM] %t", "RTU Reset");
	SetCurrencyForAll(StartingCurrency());
	ClearPlayerAndItemUpgrades();
}

void ClearPlayerAndItemUpgrades() {
	for (int i = 1; i <= MaxClients; i++) {
		if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }

		// Clear player upgrades
		TF2Attrib_RemoveAll(i);

		// NOTE: I saw this being done in other plugins that messed wiuth weapon state
		//		Not positive we need this, could use some testing
		// TF2_RemoveAllWeapons(i);

		// Clear player's weapon upgrades
		// MAGIC NUMBER 5: number of weapon slots - we only care about the first six
		// i think the last two are for spellbook or canteen. maybe.
		// NOTE: See TF2 Weapon Loadout Slots in the tf2_stocks.inc
		for (int j = 0; j < 5; j++) {
			int weapon = GetPlayerWeaponSlot(i, j);
			if (weapon != -1) {
				TF2Attrib_RemoveAll(weapon);
			}
		}

		// NOTE: I saw this being done in other plugins that messed with weapon state
		//       I think this is meant to set the active weapon to what it was prior to our mucking about
		// SetEntPropEnt(client, Prop_Send, "m_hActiveWeapon", GetPlayerWeaponSlot(client, slot));
	}
}

public bool UpgradesEnabled() {
	int status = GameRules_GetProp("m_nForceUpgrades");
	PrintToChatAll("Current upgrade status: %d", status);
	return status == 2;
	// return GameRules_GetProp("m_nForceUpgrades") == 2;
}

void RemoveUpgradeStations() {
	int entity = -1;
	while ((entity = FindEntityByClassname(entity, "func_upgradestation")) != -1) {
		RemoveEntity(entity);
	}
}

void SpawnUpgradeStations() {
	int resupplyEntity = -1;
	while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
		SpawnUpgradeStationAt(resupplyEntity);
	}
}

void SpawnUpgradeStationAt(int entity) {
	// TODO: simplify; get the entity center directly
	float   mins[3]; GetEntPropVector(entity, Prop_Data, "m_vecMins", mins);
	float   maxs[3]; GetEntPropVector(entity, Prop_Data, "m_vecMaxs", maxs);
	float center[3]; GetCenter(mins, maxs, center);

	BuildUpgradeStation(center);
}

void BuildUpgradeStation(float center[3]) {
	int funcUpgrade = CreateEntityByName("func_upgradestation");

	DispatchKeyValueVector(funcUpgrade, "origin", center);

	if (!DispatchSpawn(funcUpgrade)) { return; }

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

	// TODO: fine tune; just needs to be a little bigger than the resupply locker model
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMins", {-64.0, -64.0, -32.0 });
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMaxs", { 64.0, 64.0, 32.0 });
}

// TODO: does such a method already exist? then use it
void GetCenter(float mins[3], float maxs[3], float center[3]) {
	SubtractVectors(maxs, mins, center);
	ScaleVector(center, 0.5);
	AddVectors(mins, center, center);
}

// DRY: SFX
void PlayActivationSound() {
	int rand = GetRandomInt(0, SFX_Enable.Length - 1)
	char sound[PLATFORM_MAX_PATH];
	SFX_Enable.GetString(rand, sound, sizeof(sound));
	EmitSoundToAll(sound);
}

// DRY: SFX
void PlayDeactivationSound() {
	int rand = GetRandomInt(0, SFX_Disable.Length - 1)
	char sound[PLATFORM_MAX_PATH];
	SFX_Disable.GetString(rand, sound, sizeof(sound));
	EmitSoundToAll(sound);
}