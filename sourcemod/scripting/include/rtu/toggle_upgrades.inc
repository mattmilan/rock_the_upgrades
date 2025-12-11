ArrayList SFX;
// Wierd hack that's required for upgrade stations to function
void PrecacheRequiredAssets() {
    char g_ArbitraryModel[] = "models/props_gameplay/resupply_locker.mdl";
    if (!IsModelPrecached(g_ArbitraryModel)) {	PrecacheModel(g_ArbitraryModel, true); }

    SFX = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));

    SFX.PushString("mvm/mvm_bought_in.wav");
    SFX.PushString("vo/engineer_mvm_collect_credits01.mp3");
    SFX.PushString("vo/heavy_mvm_collect_credits01.mp3");
    SFX.PushString("vo/medic_mvm_collect_credits03.mp3");
    SFX.PushString("vo/medic_mvm_collect_credits04.mp3");
    SFX.PushString("vo/soldier_mvm_collect_credits01.mp3");
    SFX.PushString("vo/soldier_mvm_collect_credits02.mp3");

    // Precache all sounds
    for (int i = 0; i < SFX.Length; i++) {
        char sound[PLATFORM_MAX_PATH];
        SFX.GetString(i, sound, sizeof(sound));
        PrecacheSound(sound, true);
    }
}

public void EnableUpgrades() {
	if (UpgradesEnabled()) { return; }

	PlayActivationSound();
	PrintToChatAll("[SM] %t", "RTU Enabled");
	GameRules_SetProp("m_nForceUpgrades", 2, 4);
	SpawnUpgradeStations();
}

public void DisableUpgrades() {
	if (!UpgradesEnabled()) { return; }
	// TODO: play deactivation sound
	PrintToChatAll("[SM] %t", "RTU Disabled");
	GameRules_SetProp("m_nForceUpgrades", 0, 0);
	RemoveUpgradeStations();
}

public bool UpgradesEnabled() {
	int val = GameRules_GetProp("m_nForceUpgrades", 2);
	return (val == 4);
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
	// NOTE: collision detection requires assignment of an arbitrary, pre-cached model
	// TODO: Not all maps precache this model - need a solution on that
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


void PlayActivationSound() {
	int rand = GetRandomInt(0, SFX.Length - 1)
	char sound[PLATFORM_MAX_PATH];
	SFX.GetString(rand, sound, sizeof(sound));
	EmitSoundToAll(sound);
}