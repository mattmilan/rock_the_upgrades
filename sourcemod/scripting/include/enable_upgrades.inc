public void ActivateRTU() {
    PrintToConsoleAll("\n\nRock the BUpgrade!\n\n");
	GameRules_SetProp("m_nForceUpgrades", 2, 4);

    int resupplyEntity = -1; // Start with -1 to iterate through all entities
    while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
		SpawnUpgradeStationAt(resupplyEntity);
    }
}

public void SpawnUpgradeStationAt(int entity) {
	float mins[3]; GetEntPropVector(entity, Prop_Data, "m_vecMins", mins);
	float maxs[3]; GetEntPropVector(entity, Prop_Data, "m_vecMaxs", maxs);
	float center[3]; GetCenter(mins, maxs, center);

	BuildUpgradeStation(center);
}


public void BuildUpgradeStation(float center[3]) {
	int funcUpgrade = CreateEntityByName("func_upgradestation");

	DispatchKeyValueVector(funcUpgrade, "origin", center);

	if (!DispatchSpawn(funcUpgrade)) { PrintToServer("DispatchSpawn Failed!"); }	// Spawn the entity into the world
	else { PrintToServer("Spawned Upgrade Station at (%.2f, %.2f, %.2f)", center[0], center[1], center[2]); }

	ActivateEntity(funcUpgrade);
	TeleportEntity(funcUpgrade, center, NULL_VECTOR, NULL_VECTOR);
	SetEntityModel(funcUpgrade, "models/props_gameplay/resupply_locker.mdl");

	// might not be needed
	new enteffects = GetEntProp(funcUpgrade, Prop_Send, "m_fEffects");
	enteffects |= 32;
	SetEntProp(funcUpgrade, Prop_Send, "m_fEffects", enteffects);

	// Set the entity's bounding box
	float mins[] = {-64.0, -64.0, -32.0 };
	float maxs[] = { 64.0, 64.0, 32.0 };
	// TODO: we don't really need both Send and Data - resolve
	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMins", mins);
	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMaxs", maxs);
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMins", mins);
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMaxs", maxs);
	SetEntProp(funcUpgrade, Prop_Send, "m_nSolidType", 2);
	SetEntProp(funcUpgrade, Prop_Send, "m_usSolidFlags", 12);
	SetEntProp(funcUpgrade, Prop_Data, "m_nSolidType", 2);
	SetEntProp(funcUpgrade, Prop_Data, "m_usSolidFlags", 12);
}

// public Action Hook_OnTouch(int call, int client) {
// 	PrintToServer("touched");
// }

public void GetCenter(float mins[3], float maxs[3], float center[3]) {
	SubtractVectors(maxs, mins, center);
	ScaleVector(center, 0.5);
	AddVectors(mins, center, center);
}

public void PrintVector(char[] prefix, float[] vector) {
	PrintToServer("%s(%.2f,%.2f,%.2f)", prefix, vector[0], vector[1], vector[2]);
}
