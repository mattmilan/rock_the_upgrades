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

	// Debug
	// PrintVector("Mins", mins);
	// PrintVector("Maxs", maxs);
	// PrintVector("Center", center);

	BuildUpgradeStation(center);
}


public void BuildUpgradeStation(float center[3]) {
	int funcUpgrade = CreateEntityByName("func_upgradestation");

	// DispatchKeyValue(funcUpgrade, "StartDisabled", "false");
	// DispatchKeyValue(funcUpgrade, "targetname", "rtu_station");
	// DispatchKeyValueInt(funcUpgrade, "spawnflags", 1);
	DispatchKeyValueVector(funcUpgrade, "origin", center);



	// SDKHook(funcUpgrade, SDKHook_StartTouch, Hook_OnTouch);
	// AcceptEntityInput(funcUpgrade, "Disable");
	// AcceptEntityInput(funcUpgrade, "Enable");
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
	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMins", mins);
	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMaxs", maxs);
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMins", mins);
	SetEntPropVector(funcUpgrade, Prop_Data, "m_vecMaxs", maxs);
	SetEntProp(funcUpgrade, Prop_Send, "m_nSolidType", 2);
	SetEntProp(funcUpgrade, Prop_Send, "m_usSolidFlags", 12);
	SetEntProp(funcUpgrade, Prop_Data, "m_nSolidType", 2);
	SetEntProp(funcUpgrade, Prop_Data, "m_usSolidFlags", 12);

	// SetEntityCollisionGroup(func_upgrade, )
	// SetEntProp(funcUpgrade, Prop_Send, "m_iSolidity", 2, 1);
	// SetEntProp(funcUpgrade, Prop_Data, "solid", 2);

	// return Plugin_Handled;
}






public Action Hook_OnTouch(int call, int client) {
	PrintToServer("touched");
}

public void ActivatePush() {
    // int resupplyEntity = -1; // Start with -1 to iterate through all entities
    // while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {

	// 	float resupplyCenter[3], resupplyMins[3], resupplyMaxs[3];

	// 	GetEntPropVector(resupplyEntity, Prop_Data, "m_vecMins", resupplyMins);
	// 	GetEntPropVector(resupplyEntity, Prop_Data, "m_vecMaxs", resupplyMaxs);

	// 	SubtractVectors(resupplyMaxs, resupplyMins, resupplyCenter);
	// 	ScaleVector(resupplyCenter, 0.5);
	// 	AddVectors(resupplyMins, resupplyCenter, resupplyCenter);

	// 	PrintVector("Mins", resupplyMins);
	// 	PrintVector("Maxs", resupplyMaxs);
	// 	PrintVector("Center", resupplyCenter);

	// 	int funcUpgrade = CreateEntityByName("func_precipitation");

	// 	if (funcUpgrade == -1)
	// 	{
	// 		PrintToServer("Failed to create trigger entity!");
	// 		return;
	// 	}

	// 	DispatchKeyValue(funcUpgrade, "StartDisabled", "false");
	// 	DispatchKeyValue(funcUpgrade, "targetname", "rtu_station");
	// 	// DispatchKeyValueInt(funcUpgrade, "spawnflags", 1);
	// 	DispatchKeyValueVector(funcUpgrade, "origin", resupplyCenter);

	// 	// Spawn the entity into the world
	// 	DispatchSpawn(funcUpgrade);

	// 	// Activate the entity
	// 	ActivateEntity(funcUpgrade);

	// 	// Set the entity's bounding box
	// 	float mins[] = {-64.0, -64.0, -64.0 };
	// 	float maxs[] = { 64.0, 64.0, 64.0 };
	// 	// AddVectors(mins, resupplyMins, mins);
	// 	// AddVectors(maxs, resupplyMaxs, maxs);
	// 	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMins", mins);
	// 	SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMaxs", maxs);



	// 	// TODO: hope this works...
	// 	// SetEntProp(funcUpgrade, Prop_Send, "m_nSolidType", 2);
	// 	// SetEntProp(funcUpgrade, Prop_Send, "m_usSolidFlags", 12);
	// 	// SDKHook(funcUpgrade, SDKHook_StartTouch, Hook_OnTouch);
 	// 	// AcceptEntityInput(funcUpgrade, "Disable");
	// 	AcceptEntityInput(funcUpgrade, "Enable");
    // }
}

public void GetCenter(float mins[3], float maxs[3], float center[3]) {
	SubtractVectors(maxs, mins, center);
	ScaleVector(center, 0.5);
	AddVectors(mins, center, center);
}

public void PrintVector(char[] prefix, float[] vector) {
	PrintToServer("%s(%.2f,%.2f,%.2f)", prefix, vector[0], vector[1], vector[2]);
}




// an upgrade_station when spawned by map script
// (table : 0x000001ABDEA26CC0)
// {
//    m_nRenderFX	= 0
//    m_nModelIndex	= 0
//    m_Collision =
//    {
//       m_vecMinsPreScaled	= (vector : (-64.000000, -64.000000, -64.000000)
//       m_vecMaxs	= (vector : (64.000000, 64.000000, 64.000000)
//       m_vecSpecifiedSurroundingMaxs	= (vector : (0.000000, 0.000000, 0.000000)
//       m_vecSpecifiedSurroundingMins	= (vector : (0.000000, 0.000000, 0.000000)
//       m_vecSpecifiedSurroundingMinsPreScaled	= (vector : (0.000000, 0.000000, 0.000000)
//       m_vecSpecifiedSurroundingMaxsPreScaled	= (vector : (0.000000, 0.000000, 0.000000)
//       m_triggerBloat	= 0
//       m_bUniformTriggerBloat	= false
//       m_usSolidFlags	= 12
//       m_vecMaxsPreScaled	= (vector : (64.000000, 64.000000, 64.000000)
//       m_vecMins	= (vector : (-64.000000, -64.000000, -64.000000)
//       m_nSurroundType	= 0
//       m_nSolidType	= 2
//    }
//    m_iTeamNum	= 0
//    m_hOwnerEntity	= null
//    m_ubInterpolationFrame	= 0
//    m_flShadowCastDistance	= 0
//    m_iParentAttachment	= 0
//    movecollide	= 0
//    m_hEffectEntity	= null
//    m_nModelIndexOverrides =
//    {
//       002	= 0
//       003	= 0
//       000	= 0
//       001	= 0
//    }
//    moveparent	= null
//    m_bAnimatedEveryTick	= false
//    m_bAlternateSorting	= false
//    predictable_id =
//    {
//       m_PredictableID	= 0
//       m_bIsPlayerSimulated	= false
//    }
//    m_bSimulatedEveryTick	= false
//    m_clrRender	= -1
//    m_iTextureFrameIndex	= 0
//    m_nRenderMode	= 0
//    m_CollisionGroup	= 0
//    m_vecOrigin	= (vector : (-2152.000000, -2282.000000, -857.000000)
//    movetype	= 0
//    m_flElasticity	= 1
//    m_flSimulationTime	= 123
//    m_fEffects	= 32
//    AnimTimeMustBeFirst =
//    {
//       m_flAnimTime	= 0
//    }
//    m_angRotation	= (vector : (0.000000, 0.000000, 0.000000)
// }
// Drawing Entity
//   Origin: (vector : (-2152.000000, -2282.000000, -857.000000)
//   Mins: (vector : (-64.000000, -64.000000, -64.000000)
//   Maxs: (vector : (64.000000, 64.000000, 64.000000)
//   target: (vector : (0.000000, 0.000000, 0.000000)
//   Center: (vector : (-2152.000000, -2282.000000, -857.000000)


// OLD code

// TODO: the stations get created but their props are incomplete
// void ActivateRTU() {
//     PrintToConsoleAll("\n\nRock the Upgrade!\n\n");
// 	GameRules_SetProp("m_nForceUpgrades", 2, 4);

//     int resupplyEntity = -1; // Start with -1 to iterate through all entities
//     while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
// 		float resupplyCenter[3], resupplyAdded[3], resupplyMins[3], resupplyMaxs[3];

// 		GetEntPropVector(resupplyEntity, Prop_Data, "m_vecMins", resupplyMins);
// 		GetEntPropVector(resupplyEntity, Prop_Data, "m_vecMaxs", resupplyMaxs);
// 		AddVectors(resupplyMins, resupplyMaxs, resupplyCenter);
// 		// ScaleVector(resupplyCenter, 0.5)

// 		PrintToServer("Mins %.2f %.2f %.2f", resupplyMins[0], resupplyMins[1], resupplyMins[2]);
// 		PrintToServer("Maxes %.2f %.2f %.2f", resupplyMaxs[0], resupplyMaxs[1], resupplyMaxs[2]);
// 		PrintToServer("Added %.2f %.2f %.2f", resupplyCenter[0], resupplyCenter[1], resupplyCenter[2]);
// 		// PrintToServer("Scaled %.2f %.2f %.2f", resupplyCenter[0], resupplyCenter[1], resupplyCenter[2]);

// 		int funcUpgrade = CreateEntityByName("func_upgradestation");

// 		if (funcUpgrade == -1)
// 		{
// 			PrintToServer("Failed to create trigger entity!");
// 			return;
// 		}

// 		// TODO: visualize
// 		// DebugDrawTrigger(trigger, 0, 255, 0, 0, 500);

// 		// TODO: is this working?
// 		// Set the entity's position
// 		DispatchKeyValueVector(funcUpgrade, "origin", resupplyCenter);

// 		// Set the entity's bounding box
// 		float mins[] = {-64.0, -64.0, -64.0 };
// 		float maxs[] = { 64.0, 64.0, 64.0 };
// 		// SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMins", mins);
// 		// SetEntPropVector(funcUpgrade, Prop_Send, "m_vecMaxs", maxs);
// 		// SetEntPropVector(funcUpgrade, Prop_Send, "m_Collision.m_vecMins", mins);
// 		// SetEntPropVector(funcUpgrade, Prop_Send, "m_Collision.m_vecMaxs", maxs);
// 		DispatchKeyValueVector(funcUpgrade,  "m_Collision.m_vecMins", mins);
// 		DispatchKeyValueVector(funcUpgrade,  "m_Collision.m_vecMaxs", maxs);

// 		// TODO: hope this works...
// 		// SetEntProp(funcUpgrade, Prop_Send, "m_nSolidType", 2);
// 		// SetEntProp(funcUpgrade, Prop_Send, "m_Collision.m_nSolidType", 2);
// 		DispatchKeyValueInt(funcUpgrade,  "SolidType", 2);

// 		// TODO: did this really work? or is it needed?
// 		//       We're trying to set spawnflags = 1 // allow players only
// 		//		 this might actually be optional, or it blocks bot access
// 		// SetEntProp(funcUpgrade, Prop_Send, "m_usSolidFlags", 12);
// 		//SetEntProp(funcUpgrade, Prop_Send, "m_Collusion.m_usSolidFlags", 12);
// 		DispatchKeyValueInt(funcUpgrade,  "SolidFlags", 12);


// 		// Enable the trigger
// 		DispatchKeyValue(funcUpgrade, "StartDisabled", "0");

// 		// Set a target name (optional, for identification)
// 		DispatchKeyValue(funcUpgrade, "targetname", "rock_the_upgrade_station");

// 		// Spawn the entity into the world
// 		DispatchSpawn(funcUpgrade);

// 		// Activate the entity
// 		ActivateEntity(funcUpgrade);
//     }
// }