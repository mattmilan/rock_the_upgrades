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



/*=== TOC ====================================================================*/
/* t.1 Includes     */
/* t.2 Plugin Info  */
/* t.3 Variables    */
/* t.4 Forwards     */
/* t.5 Initializers */
/* t.6 Events       */
/* t.7 Commands     */

/*===( t.1 Includes )=========================================================*/

// SM/TF2 Boilerplate
#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <tf2>

#include <rock_the_upgrades/rock_the_includes>
// #include <Bank.Instance()>
// #include <Bank.Instance()/printer>

#pragma semicolon 1
#pragma newdecls required

/*===( t.2 Plugin Info )======================================================*/

public Plugin myinfo = {
	       name = "Rock The Upgrades (aka Freaky Fair Anywhere)",
	     author = "MurderousIntent",
	description = "Provides a chat command to trigger a vote (similar to Rock the Vote) which, when passed, enables MvM upgrades for the current map.",
	    version = SOURCEMOD_VERSION,
	        url = "https://github.com/mattmilan/rock_the_upgrades"
};

/*===( t.3 Variables )========================================================*/

ConVar g_Cvar_CombatTimeout;    // TODO: move to combat_timer.inc?
UpgradesController upgrades;    // Manages upgrades state, setup, and cleanup
    PocketUpgrades pocketMenu;	// Allows chat command to open upgrades menu
       CombatTimer combatTimer;	// A persistent timer to fade client combat status
              bool RTULateLoad; // Might be needed to get SteamIDs in lateload

 /*===( t.4 Forwards )========================================================*/

// TODO: might not be needed, requires exploration
public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	RTULateLoad = late;
    return APLRes_Success;
}

public void OnPluginStart() {
	InitPlugin();
	InitDependencies();
	if (!RTULateLoad) return;

	for (int i=1; i<=MaxClients; i++) {
		if (!IsClientConnected(i)) continue;

		OnClientConnected(i);
		OnClientAuthorized(i, "");
	}
}

void InitPlugin() {
	HookEvents();
	InitConvars();
	RegisterCommands();
	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");
	AutoExecConfig(true, "rtu");
}

void InitDependencies() {
	SendUpgradesFileToClients();
	InitBankTxnConVars();

	upgrades = new UpgradesController();
	upgrades.OnPluginStarted();
	// revengeTracker = RevengeTracker.Instance();
	// combatTimer will set locks for the duration of CombatTimeout
	combatTimer.Init(g_Cvar_CombatTimeout.IntValue);
	// pocket will set locks according to the values in combatTimer
	pocketMenu.Init(combatTimer);
	// payment.Init(Bank.Instance());
}

public void OnPluginEnd() {
	// delete payment;
	Votes.Instance().Close();
	upgrades.OnPluginEnded();
	combatTimer.Stop();
	Bank.Instance().Close();
}

public void OnMapStart() {
	ApplyCustomUpgradesFile();

	// payment.Reset();
	Votes.Instance().Reset();
	upgrades.OnMapStarted();
	combatTimer.Start();
}

public void OnMapEnd() {
	pocketMenu.Reset();
	combatTimer.Stop();
	Bank.Instance().Wipe();
}

// Simply increment voting threshold. Continued in `OnClientAuthorized`
public void OnClientConnected(int client) {
	if (IsFakeClient(client)) return;

	Votes.Instance().PlayerCount++;
}

// TheBank requires a unique trusted identifier which is now available
public void OnClientAuthorized(int client, const char[] authString) {
	if (IsFakeClient(client)) return;

	Bank.Instance().Connect(client);
}

// NOTE: We count Votes.Instance() because the vote might pass if a player disconnects without voting
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) return;

	Bank.Instance().Disconnect(client);
	Votes.Instance().Drop(client);
	if (!Votes.Instance().Count()) return;

	Bank.Instance().Sync();
	upgrades.Enable(.silent=true);
}

// Disallow voting during the waiting phase
public void TF2_OnWaitingForPlayersStart() {
    Votes.Instance().WaitingForPlayers = true;
}

// Re-allow voting once waiting is complete, and trigger optional Auto-Enable
public void TF2_OnWaitingForPlayersEnd() {
    Votes.Instance().WaitingForPlayers = false;

	if (!Votes.Instance().Count()) return;

	PrintToChatAll("[RTU] %t", "RTU AutoEnable");
	Bank.Instance().Sync();
	upgrades.Enable(.silent=true);
}

/**
 * t.5 Initializers
 * =========================================================================
 */

void InitConvars() {
	g_Cvar_CombatTimeout = CreateConVar("rtu_combat_timeout", "3.0", "Duration in seconds after taking or dealing damage that a player is considered 'in combat' and cannot open the upgrade menu. [3.0, 0..]", 0, true, 0.0, false);
}

void HookEvents() {
	HookEvent("teamplay_round_start", Event_TeamplayRoundStart, EventHookMode_Post);
	HookEvent("player_hurt", Event_PlayerHurt, EventHookMode_Post);
	HookEvent("player_spawn", Event_PlayerSpawn, EventHookMode_Post);
	HookEvent("teamplay_win_panel", Event_TeamplayWinPanel, EventHookMode_Post);
	HookEvent("post_inventory_application", Event_PostInventoryApplication, EventHookMode_Post);
	HookEvent("upgrades_file_changed", Event_UpgradesFileChanged, EventHookMode_Post);
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

		AuthKeys.Get(client, accountKey);
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
	bool revert = Bank.Instance().OnPlayerSpawn(client, classType, team);

	// Revert needs game state to settle before running
	if (revert) CreateTimer(0.1, Timer_RevertClient, client);

	return Plugin_Continue;
}

// Must be called after a 100ms delay to give game state time to settle
Action Timer_RevertClient(Handle timer, any client) {
	upgrades.ResetPlayer(client);
	Bank.Instance().Revert(client);

	return Plugin_Stop;
}

// Reset on round start unless configured otherwise. This may be firing too often.
Action Event_TeamplayRoundStart(Event event, const char[] name, bool dontBroadcast) {
	if (upgrades.ResetOnRoundStart) {
		Bank.Instance().ResetAccounts();
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

	if (upgrades.Enabled && ValidClient(client)) Bank.Instance().ResolveDelta(client);

	return Plugin_Continue;
}

// Debug
Action Event_UpgradesFileChanged(Event event, const char[] name, bool dontBroadcast) {
	char path[PLATFORM_MAX_PATH]; event.GetString("path", path, sizeof(path));
	PrintToServer("[RTU] Upgrades file changed, reapplying custom upgrades file: %s.", path);

	return Plugin_Continue;
}

/* PAYMENT EVENTS */

// Killing enemies rewards the killer and the assister. Bonus currency for revenge kills
Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	CloseMenuOnDeath(event);
	BankTxn txn; txn = Reward.PlayerKilled(event);
	Bank.Transact(txn);
	return Plugin_Continue;
}

// Dominations earn bonus currency (disabled by default) and provide data for revenge kills
Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
	BankTxn txn; txn = Reward.PlayerDominated(event);
	Bank.Transact(txn);
	return Plugin_Continue;
}

// Destroying an engineer building rewards increasing currency per building upgrade
Action Event_ObjectDestroyed(Event event, const char[] name, bool dontBroadcast) {
	BankTxn txn; txn = Reward.ObjectDestroyed(event);
	Bank.Transact(txn);
	return Plugin_Continue;
}

// Capturing a point earns team currency
Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
	BankTxn txn; txn = Reward.PointCaptured(event);
	Bank.Transact(txn);
	return Plugin_Continue;
}

// Capturing a flag earns team currency
Action Event_TeamplayFlagEvent(Event event, const char[] name, bool dontBroadcast) {
	BankTxn txn; txn = Reward.FlagCaptured(event);
	Bank.Transact(txn);
	return Plugin_Continue;
}

/* END PAYMENT EVENTS */

/**
 * t.7 Commands
 * =========================================================================
 */

// Player combo command - either start a vote or open the upgrades menu
Action Command_RTU(int client, int args) {
	if (client <= 0) PrintToServer("[RTU] Command `rtu` is client-only.");
	else if (!upgrades.Enabled) {
		Votes.Instance().Add(client);
		if (Votes.Instance().Count()) {
			upgrades.Enable();
        	Bank.Instance().Sync();
		}
	}
	else {
		char accountKey[MAX_AUTHID_LENGTH];
		AuthKeys.Get(client, accountKey);
		pocketMenu.Show(client, accountKey);
	}

	return Plugin_Handled;
}

// Player Command - show account data for all of client's classes
Action Command_RTUAccount(int client, int args) {
	if (client == 0) PrintToServer("[RTU] %t", "Command `rtu_account` can only be used by clients.");
	else Bank.Instance().PrintAccount(client);

	return Plugin_Handled;
}

// Admin Command - show account data for all clients' current class
Action Command_RTUBanks(int client, int args) {
	Bank.Instance().PrintToServer();

	return Plugin_Handled;
}

// Admin command - skip voting and enable immediately
Action Command_RTUEnable(int client, int args) {
	if (upgrades.Enabled) {
		ReplyToCommand(client, "[RTU] %t", "RTU Already Enabled");
	} else {
		Votes.Instance().Passed = true; // prevent Votes.Instance() from re-triggering an enable event
		Bank.Instance().Sync();
		upgrades.Enable(); // reports enable to chat
	}

	return Plugin_Handled;
}

// Admin command - disable immediately
// TODO: Determine and support cases where we would not want to reset (doubtful)
Action Command_RTUDisable(int client, int args) {
	if (!upgrades.Enabled) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		Votes.Instance().Revert();
		Bank.Instance().ResetAccounts();
		upgrades.Reset(.silent = true);
		upgrades.Disable(); // reports disable to chat
	}

	return Plugin_Handled;
}

// Admin command - revert all gains without disabling the upgrade system
Action Command_RTUReset(int client, int args) {
	if (!upgrades.Enabled) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		Bank.Instance().ResetAccounts();
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
	if (target[0]) Bank.Instance().DepositTarget(amount, target, .replyTo=client);

	// Resolve to client
	else if (client > 0) Bank.Instance().Deposit(amount, client);

	// Tell server that a target is required
	else ReplyToCommand(client, "[RTU] Command `rtu_pay` cannot be called from server without specifying a target");

	return Plugin_Handled;
}

// Opening the upgrades menu via chat leaves this prop with a value of 1
// We need to reset it or the menu will immediately open on player spawn
void CloseMenuOnDeath(Event event) {
	int victim = GetClientOfUserId(event.GetInt("userid"));
	if (!ValidClient(victim)) return;

	SetEntProp(victim, Prop_Send, "m_bInUpgradeZone", 0);
}
