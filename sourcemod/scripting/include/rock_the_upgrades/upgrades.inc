/**
 * Rock The Upgrades: Upgrades Controller
 * TODO: description
 */

#if defined _upgrades_included
 #endinput
#endif
#define _upgrades_included

#include <sourcemod>
#include <sdkhooks>
#include "../tf2attributes.inc"

// Enum representing the various states the upgrades system can be in
// Useful for gating initialization and functionality
enum UPGRADES_STATE {
	UPGRADES_STATE_ERROR,
	UPGRADES_STATE_UNSET,
	UPGRADES_STATE_INITIAL,
	UPGRADES_STATE_ENABLED_BY_MAP,
	UPGRADES_STATE_ENABLED_BY_RTU,
	UPGRADES_STATE_DISABLED_BY_RTU,
	UPGRADES_STATE_COUNT
}

const int MAX_UPGRADES_STATE_NAME = 32;

ConVar g_Cvar_MultiStageReset;


static Upgrades g_Upgrades;

/*
 * MethodMap UpgradesController
 *
 * State manager for the upgrades system
 * Also facilitates player upgrade resets
 */
methodmap Upgrades < StringMap {
	public static Upgrades Instance() {
		if (g_Upgrades == null) Upgrades.Init();

		return g_Upgrades;
	}

	public static void Init() {
		if (g_Upgrades != null) return;

		g_Cvar_MultiStageReset = CreateConVar("rtu_multistage_reset", "1", "Enable or disable resetting currency and upgrades on multi-stage map restarts/extensions [1, 0,1]", 0, true, 0.0, true, 1.0);

		g_Upgrades = view_as<Upgrades>(new StringMap());
	}

	public static bool End() {
		g_Upgrades.Close();
		g_Upgrades = null;
		return true;
	}

	// OnMapStart
	public static bool Start() {
		if (g_Upgrades == null) Upgrades.Init();
		// g_Upgrades

		// Clear guards
		g_Upgrades.Remove("ready");
		g_Upgrades.Remove("hooked");
		g_Upgrades.Remove("built");

		// Fire guarded initializers
		g_Upgrades.SetInitialUpgradesState();

		// Set guard
		g_Upgrades.SetValue("ready", 1);
		return true;
	}

	public static bool Enabled() {
		return g_Upgrades.State == UPGRADES_STATE_ENABLED_BY_MAP
			|| g_Upgrades.State == UPGRADES_STATE_ENABLED_BY_RTU;
	}

	// Sets the initial upgrades state based on map settings
	// Can only be called once per map load and should result in one of two states:
	// 		INITIAL: Upgrades have not been enabled by any means
	// 		ENABLED_BY_MAP: Upgrades are already enabled by the map
	public bool SetInitialUpgradesState() {
		if (this.ready) return false;

		this.State = this.CheckInitialUpgradesState();
		return true;
	}

	// Check the game rules prop to determine if upgrades are already enabled by map
	// Should not be called mid-map as this is an initializer method and cannot playerProfile for mid-map game states
	// TODO: probably still need a path to determine if we need to add a func_upgradestation
	public UPGRADES_STATE CheckInitialUpgradesState() {
		return GameRules_GetProp("m_nForceUpgrades") == 2
			? UPGRADES_STATE_ENABLED_BY_MAP
			: UPGRADES_STATE_INITIAL;
	}

	public static bool Enable() {
		if (g_Upgrades.enabled) return false;

		g_Upgrades.State = UPGRADES_STATE_ENABLED_BY_RTU;

		// MAGIC NUMBER: 2 enables upgrades (There may be an ENUM for this)
		GameRules_SetProp("m_nForceUpgrades", 2, 4);

		// One upgrade station is required for purchased upgrades to persist
		g_Upgrades.BuildUpgradeStation({0.0, 0.0, 0.0});

		// Delegate upgrades menu visbility to resupply locker touch events
		g_Upgrades.HookUpgradesMenuToResupply();

		return true;
	}

	// This is not a complete reversal of the `Enable` method, which hooks
	// lockers and builds an upgradestation. Those will persist, and in the event
	// of a re-enable, they are guarded against subsequent calls for the current map
	// TODO: Verify if changing the GameRules prop to 0 disables the menu
	public static bool Disable(bool silent = false) {
		if (!g_Upgrades.enabled) return false;

		g_Upgrades.State = UPGRADES_STATE_DISABLED_BY_RTU;

		// MAGIC NUMBER: 0 disables upgrades (There may be an ENUM for this)
		GameRules_SetProp("m_nForceUpgrades", 0, 4);

		return true;
	}

	// Call between rounds to clear player upgrades
	public static bool Reset(bool silent = false) {
		if (!g_Upgrades.enabled) return false;

		if (!silent) PrintToChatAll("[RTU] %t", "RTU Reset");
		for (int i = 1; i <= MaxClients; i++) Upgrades.ResetPlayer(i);

		return true;
	}

	// Resets upgrades only. Currency reset via bank
	public static void ResetPlayer(int client) {
		if (!IsClientInGame(client) || IsFakeClient(client)) return;

		// Clear player upgrades
		TF2Attrib_RemoveAll(client);

		// Number of weapon slots to check
		int weaponSlots = 5;

		// Clear weapon upgrades
		for (int j = 0; j < weaponSlots; j++) {
			int weapon = GetPlayerWeaponSlot(client, j);
			if (weapon == -1) { continue; }

			TF2Attrib_RemoveAll(weapon);
		}

		// Remove lingering effects of upgrades
		TF2_RegeneratePlayer(client);
	}

	// Approaching/leaving resupply lockers will show/hide the upgrades menu
	// TODO: Solve for maps that have no `func_regenerate` entities
	public void HookUpgradesMenuToResupply() {
		if (this.hooked) {
			PrintToServer("*****Lockers already hooked");
			return;
		}
		PrintToServer("*****Hooking Lockers");

		this.SetValue("hooked", 1);

		int resupplyEntity = -1;

		while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
			SDKHook(resupplyEntity, SDKHook_StartTouch, OnRTUUpgradeTriggerStartTouchPost);
			SDKHook(resupplyEntity, SDKHook_EndTouch, OnRTUUpgradeTriggerEndTouchPost);
		}
	}

	// Builds a func_upgradestation if one does not already exist
	// Required for purchases and refunds to take effect
	public void BuildUpgradeStation(float center[3]) {
		if (this.ContainsKey("built")) return;

		this.SetValue("built", 1);

		if (this.UpgradeStationExists()) {
			PrintToServer("[RTU][UpgradesController] Upgrade station already exists, skipping build.");
			return;
		}

		// Create, spawn, and position the upgrade station (position is arbitrary)
		int funcUpgrade = CreateEntityByName("func_upgradestation");
		DispatchKeyValueVector(funcUpgrade, "origin", center);
		if (!DispatchSpawn(funcUpgrade)) { return; }

		// Some properties need to be set after spawning/activation
		ActivateEntity(funcUpgrade);
		TeleportEntity(funcUpgrade, center, NULL_VECTOR, NULL_VECTOR);
	}

	public bool UpgradeStationExists() {
		return FindEntityByClassname(-1, "func_upgradestation") != -1;
	}


	// Returns true if map-enabled or plugin-enabled, but does not distinguish
	property bool enabled {
		public get() {
			return this.State == UPGRADES_STATE_ENABLED_BY_MAP
				|| this.State == UPGRADES_STATE_ENABLED_BY_RTU;
		}
		// private set via `Enable` and `Disable` methods
	}

	// Used to prevent re-initialization
	property bool ready {
		public get() { return this.ContainsKey("ready"); }
		// private set during `OnMapStarted`
	}

	// Whether resupply lockers have been hooked to show the upgrades menu
	property bool hooked {
		public get() {
			int hooked;
			this.GetValue("hooked", hooked);
			return hooked == 1;
		}
		// private set during `HookUpgradesMenuToResupply`
	}

	// Whether a `func_upgradestation` has been added to the current map
	property bool built {
		public get() {
			int built;
			this.GetValue("built", built);
			return built == 1;
		}
		// private set during `BuildUpgradeStation`
	}

	// whether or not to signal that upgrades (and probably currency) should be reset on
	// round start. Think of maps like cp_dustbowl that are 3 rounds in one map cycle
	property bool ResetOnRoundStart {
		public get() {
			return this.enabled && g_Cvar_MultiStageReset.IntValue == 1;
		}
	}

	// Current upgrades state. Assigns and returns UNSET by default
	property UPGRADES_STATE State {
		public get() {
			UPGRADES_STATE state;

			// Default
			if (!this.ContainsKey("state")) {
				this.SetValue("state", UPGRADES_STATE_UNSET);
				return UPGRADES_STATE_UNSET;
			}

			this.GetValue("state", state)
			return state;
		}

		// TODO: fallback for when setting invalid state value?
		public set(UPGRADES_STATE newState) {
			// Debug buffers
			char currentName[MAX_UPGRADES_STATE_NAME]; this.StringFromUpgradeState(this.State, currentName, sizeof(currentName));
			char newName[MAX_UPGRADES_STATE_NAME]; this.StringFromUpgradeState(newState, newName, sizeof(newName));
			bool validState = this.ValidUpgradesState(newState);

			if (validState) this.SetValue("state", newState);
		}
	}

	// Ensure value is within enum bounds
	public bool ValidUpgradesState(UPGRADES_STATE state) {
		if (state < UPGRADES_STATE_UNSET) return false;
		if (state > UPGRADES_STATE_DISABLED_BY_RTU) return false;
		return true;
	}

	public void StringFromUpgradeState(UPGRADES_STATE state, char[] buffer, int bufferSize) {
		switch (state) {
			case UPGRADES_STATE_ERROR:
				Format(buffer, bufferSize, "UPGRADE STATE ERROR");
			case UPGRADES_STATE_UNSET:
				Format(buffer, bufferSize, "UPGRADE STATE UNSET");
			case UPGRADES_STATE_INITIAL:
				Format(buffer, bufferSize, "UPGRADE STATE INITIAL");
			case UPGRADES_STATE_ENABLED_BY_MAP:
				Format(buffer, bufferSize, "UPGRADE STATE ENABLED_BY_MAP");
			case UPGRADES_STATE_ENABLED_BY_RTU:
				Format(buffer, bufferSize, "UPGRADE STATE ENABLED_BY_RTU");
			case UPGRADES_STATE_DISABLED_BY_RTU:
				Format(buffer, bufferSize, "UPGRADE STATE DISABLED_BY_RTU");
			default:
				Format(buffer, bufferSize, "UPGRADE STATE UNKNOWN");
		}
	}
}

// The following methods, used by SDKHooks and Timers, could not be contained
// within the methodmap, as they are required to be static global functions, and
// as a result have been given verbose names to reduce collision

// Introducing some trickiness due to the menu-opening chat command
// The chat command opens the menu but leaves the player in a bad state
// The bad state prevents the resupply locker triggers from working correctly
// This bad state can be resolved by cycling a netprop with a timer
// Without a timer, an internal variable can't observe the change
// For more info search the TF2 source code for `m_bWasInZone`
void OnRTUUpgradeTriggerStartTouchPost(int trigger, int client) {

	if (!ValidClient(client)) return;
	PrintToServer("[RTU] StartTouch Close %d", client);

	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
	CreateTimer(0.1, Timer_ShowRTUUpgradesMenu, client);
}

void OnRTUUpgradeTriggerEndTouchPost(int trigger, int client) {
	PrintToServer("[RTU] EndTouch Close %d", client);

	if (!ValidClient(client)) return;

	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
}

Action Timer_ShowRTUUpgradesMenu(Handle timer, int client) {
	PrintToServer("[RTU] StartTouch Open %d", client);
	// PrintToServer("[RTU] Timer_ShowRTUUpgradesMenu fired for client %d", client);
	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 1);

	return Plugin_Handled;
}
