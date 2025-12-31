/*
 * Rock The Upgrades: Upgrades Controller
 * TODO
 */

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <tf2>
#include <tf2attributes>

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

const int MAX_UPGRADES_STATE_NAME = 16;

// A random sound to play when upgrades are enabled
static ArrayList SFX_UPGRADES_ENABLE;
static char pathsForEnableSFX[][] = {
	"mvm/mvm_bought_in.wav",
	"vo/engineer_mvm_collect_credits01.mp3",
	"vo/heavy_mvm_collect_credits01.mp3",
	"vo/medic_mvm_collect_credits03.mp3",
	"vo/medic_mvm_collect_credits04.mp3",
	"vo/soldier_mvm_collect_credits01.mp3",
	"vo/soldier_mvm_collect_credits02.mp3"
};

// A random sound to play when upgrades are disabled
static ArrayList SFX_UPGRADES_DISABLE;
static char pathsForDisableSFX[][] = {
	"vo/announcer_sd_monkeynaut_end_crash01.mp3",
	"vo/demoman_sf12_badmagic07.mp3",
	"mvm/mvm_player_died.wav"
};

/*
 * MethodMap UpgradesController
 *
 * State manager for the upgrades system
 * Also facilitates player upgrade resets
 */
methodmap UpgradesController < StringMap {

	// Minimal constructor - yet another glorified StringMap
	public UpgradesController(bool debug = false) {
		// PrintToServer("[RTU][UpgradesController] Constructor called.");
		StringMap upgradesController = new StringMap();
		upgradesController.SetValue("debug", debug ? 1 : 0);

		if (debug) PrintToServer("[RTU][UpgradesController] Debug mode enabled.");

		return view_as<UpgradesController>(upgradesController);
	}

	// Establish values which persist between maps
	public bool OnPluginStarted() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Plugin started, initializing SFX arrays.");

		this.InitSFXArrays();
		return true;
	}

	public bool OnPluginEnded() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Plugin ending, clearing SFX arrays.");

		SFX_UPGRADES_ENABLE.Clear();
		SFX_UPGRADES_DISABLE.Clear();
		return true;
	}

	// Enforce safe defaults
	public bool OnMapStarted() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Map started, setting initial upgrades state and precaching SFX.");

		this.SetInitialUpgradesState();
		this.PrecacheSFX();
		this.Hooked = false;
		return true;
	}

	// Copy static string arrays into ArrayLists to facilitate random access
	public void InitSFXArrays() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Initializing SFX arrays.");

		SFX_UPGRADES_ENABLE = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
		for (int i = 0; i < sizeof(pathsForEnableSFX); i++)
			SFX_UPGRADES_ENABLE.PushString(pathsForEnableSFX[i]);

		SFX_UPGRADES_DISABLE = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
		for (int i = 0; i < sizeof(pathsForDisableSFX); i++)
			SFX_UPGRADES_DISABLE.PushString(pathsForDisableSFX[i]);
	}

	// Needs to be called every OnMapStart when asset precaching occurs
	// TODO: Is this even required?
	public void PrecacheSFX() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Precaching SFX.");

		// Precache all sounds
		for (int i = 0; i < SFX_UPGRADES_ENABLE.Length; i++) {
			char sfx[PLATFORM_MAX_PATH];
			SFX_UPGRADES_ENABLE.GetString(i, sfx, sizeof(sfx));
			PrecacheSound(sfx, true);
		}

		for (int i = 0; i < SFX_UPGRADES_DISABLE.Length; i++) {
			char sfx[PLATFORM_MAX_PATH];
			SFX_UPGRADES_DISABLE.GetString(i, sfx, sizeof(sfx));
			PrecacheSound(sfx, true);
		}
	}

	// Sets the initial upgrades state based on map settings
	// Can only be called once per map load and should result in one of two states:
	// 		INITIAL: Upgrades have not been enabled by any means
	// 		ENABLED_BY_MAP: Upgrades are already enabled by the map
	public bool SetInitialUpgradesState() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Setting initial upgrades state.");

		if (this.State > UPGRADES_STATE_UNSET) {
			PrintToServer("[RTU][UpgradesController] Initial state already set (currently %d).", this.State);
			return false;
		}

		this.State = this.CheckInitialUpgradesState();
		return true;
	}

	// Check the game rules prop to determine if upgrades are already enabled by map
	// Should not be called mid-map as this is an initializer method and cannot account for mid-map game states
	// TODO: probably still need a path to determine if we need to add a func_upgradestation
	public UPGRADES_STATE CheckInitialUpgradesState() {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Checking initial upgrades state from GameRules prop.");

		return GameRules_GetProp("m_nForceUpgrades") == 2
			? UPGRADES_STATE_ENABLED_BY_MAP
			: UPGRADES_STATE_INITIAL;
	}

	public bool Enable(bool silent = false) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Enabling upgrades (silent=%s).", silent ? "true" : "false");

		if (this.Enabled) return false;

		this.State = UPGRADES_STATE_ENABLED_BY_RTU;

		// Ensure Prop is set before granting currency or spawning upgrade stations
		if (!silent) PrintToChatAll("[RTU] %t", "RTU Enabled");

		this.PlayRandom(SFX_UPGRADES_ENABLE);
		GameRules_SetProp("m_nForceUpgrades", 2, 4);

		// Without at least one upgrade station, purchases are not saved
		this.BuildUpgradeStation({0.0, 0.0, 0.0});
		// Touching resupply lockers will trigger the upgrades menu
		this.HookUpgradesMenuToResupply();

		return true;
	}

	// NOTE: We cannot Un-SDKHook the resupply lockers (or can we?)
	// TODO: Verify if changing the GameRules prop to 0 disables the menu
	public bool Disable(bool silent = false) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Disabling upgrades (silent=%s).", silent ? "true" : "false");

		if (!this.Enabled) return false;

		this.State = UPGRADES_STATE_DISABLED_BY_RTU;

		if (!silent) PrintToChatAll("[RTU] %t", "RTU Disabled");

		this.PlayRandom(SFX_UPGRADES_DISABLE);

		// MAGIC NUMBER: 0 disables upgrades (There may be an ENUM for this)
		GameRules_SetProp("m_nForceUpgrades", 0, 4);

		return true;
	}

	// Call between rounds
	public bool Reset(bool silent = false) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Resetting all player upgrades (silent=%s).", silent ? "true" : "false");

		if (!this.Enabled) return false;

		if (!silent) PrintToChatAll("[RTU] %t", "RTU Reset");
		for (int i = 1; i <= MaxClients; i++) this.ResetPlayer(i);

		return true;
	}

	// Resets upgrades only. Currency reset via bank
	public void ResetPlayer(int client) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Resetting upgrades for client %d.", client);

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
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Hooking upgrades menu to resupply lockers (hooked: %s).", this.Hooked ? "true" : "false");

		if (this.Hooked) return;

		this.Hooked = true;

		int resupplyEntity = -1;

		while ((resupplyEntity = FindEntityByClassname(resupplyEntity, "func_regenerate")) != -1) {
			SDKHook(resupplyEntity, SDKHook_StartTouchPost, OnRTUUpgradeTriggerStartTouchPost);
			SDKHook(resupplyEntity, SDKHook_EndTouchPost, OnRTUUpgradeTriggerEndTouchPost);
		}
	}

	// This entity simply needs to exist and be active for upgrades to 'work'
	// Thought it also controls upgrade menu visibility via player collision, that
	// has been delegated directly to the resupply lockers instead
	public void BuildUpgradeStation(float center[3]) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Building upgrade station at (%.2f, %.2f, %.2f).", center[0], center[1], center[2]);

		if (this.UpgradeStationExists()) {
			PrintToServer("[RTU][UpgradesController] Upgrade station already exists, skipping build.");
			return;
		}
		// Create, spawn, and position the upgrade station
		int funcUpgrade = CreateEntityByName("func_upgradestation");
		DispatchKeyValueVector(funcUpgrade, "origin", center);
		if (!DispatchSpawn(funcUpgrade)) { return; }

		// Some properties need to be set after spawning/activation
		ActivateEntity(funcUpgrade);
		TeleportEntity(funcUpgrade, center, NULL_VECTOR, NULL_VECTOR);
	}

	public bool UpgradeStationExists() {
		bool exists = FindEntityByClassname(-1, "func_upgradestation") != -1;
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Checking for existing upgrade station: %s.", exists ? "found" : "not found");
		return exists;
	}

	// Accompany state changes with appropriate SFX
	public void PlayRandom(ArrayList soundArray) {
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Playing random sound from provided array.");

		int rand = GetRandomInt(0, soundArray.Length - 1)
		char sound[PLATFORM_MAX_PATH];
		soundArray.GetString(rand, sound, sizeof(sound));
		EmitSoundToAll(sound);
	}

	property bool Enabled {
		public get() {
			bool enabled = this.State == UPGRADES_STATE_ENABLED_BY_MAP
				|| this.State == UPGRADES_STATE_ENABLED_BY_RTU;

			if (this.Debug) PrintToServer("[RTU][UpgradesController] Checking if upgrades are enabled (current state: %d).", enabled ? "yes" : "no");

			return enabled;
		}
	}

	property bool Debug {
		public get() {
			int debug;
			this.GetValue("debug", debug);

			if (this.Debug) PrintToServer("[RTU][UpgradesController] Retrieving debug property: %s.", debug ? "true" : "false");

			return debug == 1;
		}

	}
	// Whether resupply lockers have been hooked to show the upgrades menu
	property bool Hooked {
		public get() {
			int hooked;
			this.GetValue("hooked", hooked);
			if (this.Debug) PrintToServer("[RTU][UpgradesController] Retrieving hooked property: %s.", hooked ? "true" : "false");
			return hooked == 1;
		}

		public set(bool isHooked) {
			this.SetValue("hooked", isHooked ? 1 : 0);
		}
	}

	// Current upgrades state. Assigns and returns UNSET by default
	property UPGRADES_STATE State {
		public get() {
			UPGRADES_STATE state;
			PrintToServer("[RTU][UpgradesController] Retrieving current upgrades state.");
			if (!this.ContainsKey("state")) {
				PrintToServer("[RTU][UpgradesController] No state found, assigning UNSET.");
				this.SetValue("state", UPGRADES_STATE_UNSET);
			}

			this.GetValue("state", state)
			PrintToServer("[RTU][UpgradesController] Current upgrades state is %d.", state);
			return state;
		}

		// TODO: fallback for when setting invalid state value?
		public set(UPGRADES_STATE newState) {
			// Debug buffers
			char currentName[MAX_UPGRADES_STATE_NAME]; this.StringFromUpgradeState(this.State, currentName, sizeof(currentName));
			char newName[MAX_UPGRADES_STATE_NAME]; this.StringFromUpgradeState(newState, newName, sizeof(newName));
			bool validState = this.ValidUpgradesState(newState);

			if (validState) this.SetValue("state", newState);

			if (this.Debug) {
				if (validState) PrintToServer("[RTU][UpgradesController] Setting upgrades state to %s (was %s).", newName, currentName);
				else PrintToServer("[RTU][UpgradesController] Attempted to set invalid upgrades state %s (was %s).", newName, currentName);
			}
		}
	}

	// Ensure value is within enum bounds
	public bool ValidUpgradesState(UPGRADES_STATE state) {
		bool valid = state >= UPGRADES_STATE_UNSET && state <= UPGRADES_STATE_DISABLED_BY_RTU;
		if (this.Debug) PrintToServer("[RTU][UpgradesController] Validating upgrades state %d: %s.", state, valid ? "valid" : "invalid");
		return valid;
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
// within the methodmap, as they are required to be static global functions

// Introducing some trickiness due to the menu-opening chat command
// The chat command opens the menu but leaves the player in a bad state
// The bad state prevents the resupply locker triggers from working correctly
// This bad state can be resolved by cycling a netprop with a timer
// For more info search the TF2 source code for `m_bWasInZone`
void OnRTUUpgradeTriggerStartTouchPost(int trigger, int client) {
	PrintToServer("[RTU] OnRTUUpgradeTriggerStartTouchPost fired for client %d", client);
	if (!client || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client)) { return; }

	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
	CreateTimer(0.1, Timer_ShowUpgradesMenu, client);
}

void OnRTUUpgradeTriggerEndTouchPost(int trigger, int client) {
	PrintToServer("[RTU] OnRTUUpgradeTriggerEndTouchPost fired for client %d", client);
	if (!client || client > MaxClients || !IsClientInGame(client) || IsFakeClient(client)) { return; }

	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
}

Action Timer_ShowUpgradesMenu(Handle timer, int client) {
	PrintToServer("[RTU] Timer_ShowUpgradesMenu fired for client %d", client);
	SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 1);

	return Plugin_Handled;
}
	// public void LegacyPrecacheSFX() {
		// SFX_UPGRADES_ENABLE.PushString("mvm/mvm_bought_in.wav");
		// SFX_UPGRADES_ENABLE.PushString("vo/engineer_mvm_collect_credits01.mp3");
		// SFX_UPGRADES_ENABLE.PushString("vo/heavy_mvm_collect_credits01.mp3");
		// SFX_UPGRADES_ENABLE.PushString("vo/medic_mvm_collect_credits03.mp3");
		// SFX_UPGRADES_ENABLE.PushString("vo/medic_mvm_collect_credits04.mp3");
		// SFX_UPGRADES_ENABLE.PushString("vo/soldier_mvm_collect_credits01.mp3");
		// SFX_UPGRADES_ENABLE.PushString("vo/soldier_mvm_collect_credits02.mp3");

		// SFX_UPGRADES_DISABLE.PushString("vo/announcer_sd_monkeynaut_end_crash01.mp3");
		// SFX_UPGRADES_DISABLE.PushString("vo/demoman_sf12_badmagic07.mp3")
		// SFX_UPGRADES_DISABLE.PushString("mvm/mvm_player_died.wav");

		// Precache all sounds
		// for (int i = 0; i < SFX_UPGRADES_ENABLE.Length; i++) {
		// 	char sound[PLATFORM_MAX_PATH];
		// 	SFX_UPGRADES_ENABLE.GetString(i, sound, sizeof(sound));
		// 	PrecacheSound(sound, true);
		// }

		// for (int i = 0; i < SFX_UPGRADES_DISABLE.Length; i++) {
		// 	char sound[PLATFORM_MAX_PATH];
		// 	SFX_UPGRADES_DISABLE.GetString(i, sound, sizeof(sound));
		// 	PrecacheSound(sound, true);
		// }
	// }