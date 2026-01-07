/**
 * vim: set ts=4 :
 * =============================================================================
 * Rock The Upgrades!
 *
 * Starts a vote.
 *
 * If the vote passes, enables the upgrade system and adds an upgrade station to
 * every resupply locker for the current map.
 *
 * Upgrades require currency which is gained primarily from kills, assists, and
 * objectives. See the included `currency_controller` module for more details.
 *
 * Currency is managed by the included `Bank` module which is modeled after the
 * brilliant work done in the community map `Freaky Fair`. Together, with the
 * Accout module, Bank provides a number of QoL improvements for players, most
 * notably when reconnecting or when changing classes.
 *
 * Several convars are provided for fine tuning of difficulty.
 *
 * =============================================================================
 * ACKNOWLEDGEMENTS
 *
 * Developed by MurderousIntent while standing on the shoulders of giants.
 *
 * Giants include
 *  - Valve (https://www.valvesoftware.com)
 *  - Sourcemod (https://www.sourcemod.net/)
 *  - Freaky Fair (https://steamcommunity.com/sharedfiles/filedetails/?id=3326591381)
 * … and the unforunate players who endured our testing in production
 *
 * Voting portion inspired by AlliedModder's RTV SourceMod Plugin
 * https://github.com/alliedmodders/sourcemod/blob/master/plugins/rockthevote.sp
 *
 * Depends on https://github.com/FlaminSarge/tf2attributes to revert upgrades
 *
 * Originally developed for the Bangerz.tf community to be used in their version
 * of the `Engineer Fortress` game mode (https://bangerz.tf), which I highly
 * recommend.
 *
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */



/**
 * TOC
 * ============================================================================
 * t.1 Includes
 * t.2 Plugin Info
 * t.3 Global Variables
 * t.4 Forwards
 * t.5 Initializers
 * t.6 Events
 * t.7 Commands
 */



/**
 * t.1 Includes
 * ============================================================================
 */

// SM/TF2 Boilerplate
#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <tf2>

// Shared functions
#include <rock_the_upgrades/shared>

// Enable/Disable upgrade system, and easily reset entity upgrades
#include <rock_the_upgrades/upgrades_controller>

// Manages configurable currency gains and losses across various events
#include <rock_the_upgrades/currency_controller>

// Persistent, multi-target timer
#include <rock_the_upgrades/combat_timer>

// Allows upgrade menu access via bindable chat command
#include <rock_the_upgrades/pocket_upgrades>

// Enables voting and auto-enable of upgrade system
#include <rock_the_upgrades/voting>

#pragma semicolon 1
#pragma newdecls required

/**
 * t.2 Plugin Info
 * ===========================================================================
 */

public Plugin myinfo = {
	name = "Rock The Upgrades (aka Freaky Fair Anywhere)",
	author = "MurderousIntent",
	description = "Provides a chat command to trigger a vote (similar to Rock the Vote) which, when passed, enables MvM upgrades for the current map.",
	version = SOURCEMOD_VERSION,
	url = "https://github.com/mattmilan/rock_the_upgrades"
};

/**
 * t.3 Global Variables
 * ==========================================================================
 */

ConVar g_Cvar_VoteThreshold;
ConVar g_Cvar_MultiStageReset;
ConVar g_Cvar_AutoEnableThreshold;
ConVar g_Cvar_CombatTimeout; // TODO: move to combat_timer.inc?

bool WaitingForPlayers; 	 // Disallows voting while "Waiting for Players"
int PlayerCount;			 // Number of connected clients (excluding bots)
bool RTULateLoad;			 // Might be needed to get SteamIDs in lateload
VoteMap votes;			 	 // Manages votes
UpgradesController upgrades; // Manages enabling/disabling/resetting upgrades
PocketUpgrades pocketMenu;	 	 // Access upgrades menu via chat command
CombatTimer combatTimer;	 // Manages persistent combat timers for all human clients

/**
 * t.4 Forwards
 * =========================================================================
 */

// TODO: might not be needed, requires exploration
public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	RTULateLoad = late;
    return APLRes_Success;
}

public void OnPluginStart() {
	// plugin setup
	HookEvents();
	RegisterCommands();

	// included setup
	InitCurrencyController();
	votes = new VoteMap();
	upgrades = new UpgradesController();
	upgrades.OnPluginStarted();
	// combatTimer will set locks for the duration of CombatTimeout
	combatTimer.Init(g_Cvar_CombatTimeout.IntValue);
	// pocket will set locks according to the values in combatTimer
	pocket.Init(combatTimer);

	// boilerplate configs
	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");
	AutoExecConfig(true, "rtu");

	if (RTULateLoad) {
		// Ensures the voting threshold is reasonable
		for (int i=1; i<=MaxClients; i++) {
			if (IsClientConnected(i)) {
				OnClientConnected(i);
			}
		}
	}
}

public void OnPluginEnd() {
	CloseCurrencyController();
	votes.Close();
	upgrades.OnPluginEnded();
	combatTimer.Stop();
}

public void OnMapStart() {
	RevengeTracker.Clear();
	votes.Reset();
	upgrades.OnMapStarted();
	combatTimer.Start();

	if (bank != INVALID_HANDLE) bank.Close();
	bank = new Bank();
}

public void OnMapEnd() {
	pocketMenu.Reset();
	combatTimer.Stop();
}

// Simply increment voting threshold. Continued in `OnClientAuthorized`
public void OnClientConnected(int client) {
	if (IsFakeClient(client)) return;

	votes.PlayerCount++;
}

// Bank requires a unique trusted identifier which is now available
public void OnClientAuthorized(int client) {
	if (IsFakeClient(client)) return;

	bank.Connect(client);
}

// NOTE: We count votes because the vote might pass if a player disconnects without voting
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) return;

	bank.Disconnect(client);
	votes.Drop(client);
	if (!votes.Count()) return;

	bank.Sync();
	upgrades.Enable(.silent=true);
}

// Disallow voting during the waiting phase
public void TF2_OnWaitingForPlayersStart() {
    votes.WaitingForPlayers = true;
}

// Re-allow voting once waiting is complete, and trigger optional Auto-Enable
public void TF2_OnWaitingForPlayersEnd() {
    votes.WaitingForPlayers = false;

	if (!votes.Count()) return;

	PrintToChatAll("[RTU] %t", "RTU AutoEnable");
	bank.Sync();
	upgrades.Enable(.silent=true);
}

/**
 * t.5 Initializers
 * =========================================================================
 */

void InitConvars() {
	g_Cvar_VoteThreshold = CreateConVar("rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);
	g_Cvar_MultiStageReset = CreateConVar("rtu_multistage_reset", "1", "Enable or disable resetting currency and upgrades on multi-stage map restarts/extensions [1, 0,1]", 0, true, 0.0, true, 1.0);
	g_Cvar_AutoEnableThreshold = CreateConVar("rtu_auto_enable_threshold", "0.8", "Number of players required at end of waiting stage to auto-enable upgrades. A value of 0 disables auto-enable. [16, 0..]", 16, true, 0.0, false);
	g_Cvar_CombatTimeout = CreateConVar("rtu_combat_timeout", "3.0", "Duration in seconds after taking or dealing damage that a player is considered 'in combat' and cannot open the upgrade menu. [3.0, 0..]", 0, true, 0.0, false);
}

void HookEvents() {
	HookEvent("teamplay_round_start", Event_TeamplayRoundStart, EventHookMode_Post);
	HookEvent("player_hurt", Event_PlayerHurt, EventHookMode_Post);
	HookEvent("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
	HookEvent("teamplay_win_panel", Event_TeamplayWinPanel, EventHookMode_Post);
	HookEvent("post_inventory_application", Event_PostInventoryApplication, EventHookMode_Post);
}

void RegisterCommands() {
	RegConsoleCmd("rtu", Command_RTU, "Starts a vote to enable the upgrade system for the current map.");
	RegConsoleCmd("rtu_account", Command_RTUAccount, "Debug: Show full account data for the caller");
	RegAdminCmd("rtu_banks", Command_RTUBanks, ADMFLAG_GENERIC, "Debug: Show full bank data");
	RegAdminCmd("rtu_pay", Command_RTUPay, ADMFLAG_GENERIC, "Debug: Pay 100 currency to the caller");
	RegAdminCmd("rtu_enable", Command_RTUEnable, ADMFLAG_GENERIC, "Immediately enable the upgrade system without waiting for a vote.");
	RegAdminCmd("rtu_disable", Command_RTUDisable, ADMFLAG_GENERIC, "Immediately disable the upgrade system and revert all currency and upgrades");
	RegAdminCmd("rtu_reset", Command_RTUReset, ADMFLAG_GENERIC, "Remove all upgrades and currency but leave the upgrade system enabled.");
}

/**
 * t.6 Events
 * =========================================================================
 */

// Immediately closes and locks the pocket upgrade menu when dealing/receiving damage
Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast) {
	if (!upgrades.Enabled) return Plugin_Continue;

	int clients[2];
	clients[0] = GetClientOfUserId(event.GetInt("attacker"));
	clients[1] = GetClientOfUserId(event.GetInt("userid"));

	char accountKey[MAX_AUTHID_LENGTH];

	for (int i = 0; i < 2; i++) {
		int client = clients[i];
		if (!ValidClient(client)) continue;

		bank.GetAccountKey(client, accountKey);
		combatTimer.Add(accountKey);
		SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);
	}

	return Plugin_Continue;
}

// Critical function. Prevents the dodge exploit and keeps currency synced between class changes
Action Event_PlayerSpawn(Event event, const char[] name, bool dontBroadcast) {
	// Extract event variables
	int client = GetClientOfUserId(event.GetInt("userid"));
	TFClassType classType = view_as<TFClassType>(event.GetInt("class"));
	TFTeam team = view_as<TFTeam>(event.GetInt("team"));

	// Validate
	if (!ValidPlayer(client, classType, team)) return Plugin_Continue;

	// Update or create account and sync balance
	bool revert = bank.OnPlayerSpawn(client, classType, team);

	if (revert) CreateTimer(0.1, Timer_RevertClient, client);
	else bank.Sync(client);

	return Plugin_Continue;
}

// Must be called after a 100ms delay to give game state time to settle
Action Timer_RevertClient(Handle timer, any client) {
	upgrades.ResetPlayer(client);
	bank.Revert(client);

	return Plugin_Stop;
}

// Reset on round start unless configured otherwise. This may be firing too often.
Action Event_TeamplayRoundStart(Event event, const char[] name, bool dontBroadcast) {
	if (upgrades.ResetOnRoundStart) {
		bank.ResetAccounts();
		upgrades.Reset();
	}

	pocketMenu.Unlock();

	return Plugin_Continue;
}

Action Event_TeamplayWinPanel(Event event, const char[] name, bool dontBroadcast) {
	pocketMenu.Lock(.message="until next round");

	return Plugin_Continue;
}

// TODO: Guard against firing too often
Action Event_PostInventoryApplication(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	if (upgrades.Enabled && ValidClient(client)) bank.ResolveDelta(client);

	return Plugin_Continue;
}

/**
 * t.7 Commands
 * =========================================================================
 */

// Player combo command - either start a vote or open the upgrades menu
Action Command_RTU(int client, int args) {
	if (client <= 0) PrintToServer("[RTU] Command `rtu` is client-only.");
	else if (!upgrades.Enabled) {
		votes.Add(client);
		if (votes.Count()) {
			upgrades.Enable();
        	bank.Sync();
		}
	}
	else {
		char accountKey[MAX_AUTHID_LENGTH];
		bank.GetAccountKey(client, accountKey);
		pocketMenu.Show(client, accountKey);
	}

	return Plugin_Handled;
}

// Player Command - show account data for all of client's classes
Action Command_RTUAccount(int client, int args) {
	if (client > 0) bank.PrintAccount(client);
	else PrintToServer("[RTU] %t", "Command `rtu_account` can only be used by clients.");

	return Plugin_Handled;
}

// Admin Command - show account data for all clients' current class
Action Command_RTUBanks(int client, int args) {
	bank.PrintToServer();
	return Plugin_Handled;
}

// Admin command - skip voting and enable immediately
Action Command_RTUEnable(int client, int args) {
	if (upgrades.Enabled) {
		ReplyToCommand(client, "[RTU] %t", "RTU Already Enabled");
	} else {
		votes.Passed = true; // prevent votes from re-triggering an enable event
		bank.Sync();
		upgrades.Enable(); // reports enable to chat
	}

	return Plugin_Handled;
}

// Admin command - disable immediately
// TODO: Determine and support cases where we would not want to reset (doubtful)
Action Command_RTUDisable(int client, int args) {
	if (!upgrades.Enabled) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		votes.Revert();
		bank.ResetAccounts();
		upgrades.Reset(.silent = true);
		upgrades.Disable(); // reports disable to chat
	}

	return Plugin_Handled;
}

// Admin command - revert all gains without disabling the upgrade system
Action Command_RTUReset(int client, int args) {
	if (!upgrades.Enabled) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		bank.ResetAccounts();
		upgrades.Reset(); // reports reset to chat
	}

	return Plugin_Handled;
}

Action Command_RTUPay(int client, int args) {
	// Show usage if no args provided
	if (args == 0) {
		ReplyToCommand(client, "[RTU] Usage: rtu_pay <amount> (optional)<red|blu|all|name>");
		return Plugin_Handled;
	}

	// Determine amount
	float amount = GetCmdArgFloat(1);

	// Validate amount
	if (amount < 1) {
		ReplyToCommand(client, "[RTU] Invalid Amount %f", amount);
		return Plugin_Handled;
	}

	// Determine target
	char target[MAX_NAME_LENGTH]; GetCmdArg(2, target, MAX_NAME_LENGTH);

	// Resolve to target
	if (target[0]) bank.DepositTarget(target, amount, .replyTo=client);
	// Resolve to client
	else if (client > 0) bank.Deposit(amount, client);
	// Tell server that a target is required
	else ReplyToCommand(client, "[RTU] Command `rtu_pay` cannot be called from server without specifying a target");

	return Plugin_Handled;
}
