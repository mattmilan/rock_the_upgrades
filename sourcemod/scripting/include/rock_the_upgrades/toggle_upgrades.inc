ArrayList SFX_Enable;
ArrayList SFX_Disable;

void PrecacheRequiredAssets() {
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

void EnableUpgrades(bool silent = false) {
	// This is also being checked and reported upstream
	if (UpgradesEnabled()) { return; }

	// Ensure Prop is set before granting currency or spawning upgrade stations
	if (!silent) PrintToChatAll("[RTU] %t", "RTU Enabled");

	PlayRandomSoundFrom(SFX_Enable);
	GameRules_SetProp("m_nForceUpgrades", 2, 4);

	// Without at least one upgrade station, purchases are not saved
	BuildUpgradeStation({0.0, 0.0, 0.0});

	// Touching resupply lockers will trigger the upgrades menu
	HookUpgradesMenuToResupply();
}

void DisableUpgrades() {
	// This is also being checked and reported upstream
	if (!UpgradesEnabled()) { return; }

	PrintToChatAll("[RTU] %t", "RTU Disabled");
	PlayRandomSoundFrom(SFX_Disable);
	GameRules_SetProp("m_nForceUpgrades", 0, 4);
}

void ResetUpgrades(bool silent = false) {
	// This is also being checked and reported upstream
	if (!UpgradesEnabled()) { return; }

	if (!silent) { PrintToChatAll("[RTU] %t", "RTU Reset"); }

	ResetPlayers();
}

// TODO: next test
// Can we save and reapply upgrades?
void ResetPlayers() {
	for (int i = 1; i <= MaxClients; i++) {
		ResetPlayer(i);
	}
}

void ResetPlayer(int client) {
	if (!IsClientInGame(client) || IsFakeClient(client)) { return; }
	// Clear player upgrades
	TF2Attrib_RemoveAll(client);

	// Clear weapon upgrades
	int weaponSlots = 5;
	for (int j = 0; j < weaponSlots; j++) {
		int weapon = GetPlayerWeaponSlot(client, j);
		if (weapon == -1) { continue; }

		TF2Attrib_RemoveAll(weapon);
	}

	// Remove lingering effects of upgrades
	TF2_RegeneratePlayer(client);
}

bool UpgradesEnabled() {
	return GameRules_GetProp("m_nForceUpgrades") == 2;
}

// Approaching/leaving resupply lockers shows/hides the upgrades menu
void HookUpgradesMenuToResupply() {
	int resupplyEntity = -1;
	while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
		SDKHook(resupplyEntity, SDKHook_StartTouchPost, OnUpgradeTriggerStartTouchPost);
		SDKHook(resupplyEntity, SDKHook_EndTouchPost, OnUpgradeTriggerEndTouchPost);
	}
}

// Introducing some trickiness due to the menu-opening chat command
// The chat command opens the menu but leaves the player in a bad state
// The bad state prevents the resupply locker triggers from working correctly
// This bad state can be resolved by cycling a netprop with a timer
// For more info search the TF2 source code for `m_bWasInZone`
void OnUpgradeTriggerStartTouchPost(int trigger, int client) {
	if (!UpgradesEnabled()) return;
	if (!IsClientInGame(client) || IsFakeClient(client)) { return; }

	HideUpgradeMenuFromClient(client);
	CreateTimer(0.1, ShowUpgradeMenuToClient, client);
}

void OnUpgradeTriggerEndTouchPost(int trigger, int client) {
	if (!UpgradesEnabled()) return;
	if (!IsClientInGame(client) || IsFakeClient(client)) { return; }

	HideUpgradeMenuFromClient(client);
}

void ShowUpgradeMenuToClient(Handle timer, int client) {
	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 1);
}

void HideUpgradeMenuFromClient(int client) {
	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
}

// This entity simply needs to exist and be active for upgrades to 'work'
// Thought it also controls upgrade menu visibility via player collision, that
// has been delegated directly to the resupply lockers instead
void BuildUpgradeStation(float center[3]) {
	// Create, spawn, and position the upgrade station
	int funcUpgrade = CreateEntityByName("func_upgradestation");
	DispatchKeyValueVector(funcUpgrade, "origin", center);
	if (!DispatchSpawn(funcUpgrade)) { return; }

	// Some properties need to be set after spawning/activation
	ActivateEntity(funcUpgrade);
	TeleportEntity(funcUpgrade, center, NULL_VECTOR, NULL_VECTOR);
}

void PlayRandomSoundFrom(ArrayList soundArray) {
	int rand = GetRandomInt(0, soundArray.Length - 1)
	char sound[PLATFORM_MAX_PATH];
	soundArray.GetString(rand, sound, sizeof(sound));
	EmitSoundToAll(sound);
}
