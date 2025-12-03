// Primary method, called by plugin
public void EnableUpgrades() {
	PrintToChatAll("[SM] %t", "RTU Enabled");
	GameRules_SetProp("m_nForceUpgrades", 2, 4);
	SpawnUpgradeStations();
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
