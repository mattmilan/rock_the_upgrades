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
 * Depends on https://github.com/Bradasparky/sm_vscript_comms to revert upgrades
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
 * t.6 Commands
 * t.7 Voting
 * t.8 Helpers
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
#include <tf2_stocks>

#include <morecolors>

// Enable/Disable upgrade system, and easily reset entity upgrades
#include <rock_the_upgrades/upgrades_controller>

// Parse and apply custom upgrades files
#include <rock_the_upgrades/custom_upgrades>

// Manages configurable currency gains and losses across various events
#include <rock_the_upgrades/currency_controller>

// Persistent, multi-target timer
#include <rock_the_upgrades/combat_timer>

// Allows upgrade menu access via chat command
#include <rock_the_upgrades/pocket_upgrades>

#pragma semicolon 1
#pragma newdecls required

#define RTU_BRAND "{yellow}[{gold}RTU{yellow}]{default}"
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
ArrayList Votes;			 // List of clients who have voted
UpgradesController upgrades; // Manages enabling/disabling/resetting upgrades
PocketUpgrades pocket;	 	 // Access upgrades menu via chat command
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
	InitConvars();
	HookEvents();
	RegisterCommands();
	Votes = new ArrayList();
	// CombatTimes = new StringMap();

	// included setup
	InitCurrencyController();
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

	if (RTULateLoad) HandleLateLoad();
}

public void OnPluginEnd() {
	upgrades.OnPluginEnded();
	Votes.Close();
	combatTimer.Stop();
	CloseCurrencyController();
}

public void OnMapStart() {
	FindAndAddUpgradesFilesToDownloadsTable();
	ApplyCustomUpgradesFile();
 	PlayerCount = 0;
	WaitingForPlayers = false;
	RevengeTracker.Clear(); // from currency_controller
 	Votes.Clear();
	combatTimer.Start();

	if (bank != INVALID_HANDLE) bank.Close();
	bank = new Bank();

	upgrades.OnMapStarted();
}

public void OnMapEnd() {
	pocket.Reset();
	combatTimer.Stop();
}

// prevent ResolveDelta from miscalculating currency for reconnecting players
public void OnClientPostAdminCheck(int client) {
	if (IsFakeClient(client)) return;

	PlayerCount++;
	bank.Connect(client);
	bank.Sync(client);
	AttemptAutoEnable();
}

// NOTE: We count votes because the vote might pass if a player disconnects without voting
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) return;

	// Clean up damage timer
	char accountKey[MAX_AUTHID_LENGTH];
	bank.GetAccountKey(client, accountKey);
	combatTimer.Set(accountKey, -1);

	bank.Disconnect(client);

	PlayerCount--;
	RemoveVote(client);
	CountVotes();
}

// Disallow voting during the waiting phase
public void TF2_OnWaitingForPlayersStart() {
    WaitingForPlayers = true;
}

// Re-allow voting once waiting is complete
public void TF2_OnWaitingForPlayersEnd() {
    WaitingForPlayers = false;
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
	RegConsoleCmd("rtu_pay", Command_RTUPay, "Send currency to other players.");
	RegAdminCmd("rtu_banks", Command_RTUBanks, ADMFLAG_GENERIC, "Debug: Show full bank data");
	RegAdminCmd("rtu_enable", Command_RTUEnable, ADMFLAG_GENERIC, "Immediately enable the upgrade system without waiting for a vote.");
	RegAdminCmd("rtu_disable", Command_RTUDisable, ADMFLAG_GENERIC, "Immediately disable the upgrade system and revert all currency and upgrades");
	RegAdminCmd("rtu_reset", Command_RTUReset, ADMFLAG_GENERIC, "Remove all upgrades and currency but leave the upgrade system enabled.");
}

/**
 * t.6 Events
 * =========================================================================
 */

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

Action Timer_RevertClient(Handle timer, any client) {
	// Very important for these two events to happen together
	upgrades.ResetPlayer(client);
	bank.Revert(client);

	return Plugin_Stop;
}

// Reset on round start unless configured otherwise. This may be firing too often.
Action Event_TeamplayRoundStart(Event event, const char[] name, bool dontBroadcast) {
	if (g_Cvar_MultiStageReset.IntValue == 1) {
		bank.ResetAccounts();
		upgrades.Reset(); // also attempts to force-close upgrade menus
		pocket.Unlock();
		CPrintToChatAll("%s %t", RTU_BRAND, "RTU Reset");
	}

	return Plugin_Continue;
}

Action Event_TeamplayWinPanel(Event event, const char[] name, bool dontBroadcast) {
	pocket.Lock(.message="until next round");

	return Plugin_Continue;
}

// TODO: Guard against firing too often
Action Event_PostInventoryApplication(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));

	if (upgrades.Enabled && ValidClient(client)) bank.ResolveDelta(client);

	return Plugin_Continue;
}

/**
 * t.6 Commands
 * =========================================================================
 */

// Player combo command - either start a vote or open the upgrades menu
Action Command_RTU(int client, int args) {
	if (client <= 0) PrintToServer("[RTU] Command `rtu` is client-only.");
	else if (!upgrades.Enabled) Vote(client);
	else {
		char accountKey[MAX_AUTHID_LENGTH];
		bank.GetAccountKey(client, accountKey);
		pocket.Show(client, accountKey);
	}

	return Plugin_Handled;
}

// Player Command - show account data for all of client's classes
Action Command_RTUAccount(int client, int args) {
	if (client > 0) {
		bank.PrintAccount(client);
		CPrintToChat(client, "%s Account Details printed to console.", RTU_BRAND);
	}
	else PrintToServer("[RTU] Command `rtu_account` can only be used by clients.");

	return Plugin_Handled;
}

// Admin Command - show account data for all clients' current class
Action Command_RTUBanks(int client, int args) {
	bank.PrintToConsole(client);

	CPrintToChat(client, "%s %s", RTU_BRAND, "Bank Data printed to console.");

	return Plugin_Handled;
}

// Admin command - skip voting and enable immediately
Action Command_RTUEnable(int client, int args) {
	if (upgrades.Enabled) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Already Enabled");
	} else {
		bank.Sync();
		upgrades.Enable();
		CPrintToChatAll("%s %t", RTU_BRAND, "RTU Enabled");
	}

	return Plugin_Handled;
}

// Admin command - disable immediately
// TODO: Determine and support cases where we would not want to reset (doubhtful)
Action Command_RTUDisable(int client, int args) {
	if (!upgrades.Enabled) { CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Not Enabled"); }
	else {
		Votes.Clear();
		bank.ResetAccounts();
		upgrades.Reset();
		upgrades.Disable();
		CPrintToChatAll("%s %t", RTU_BRAND, "RTU Disabled");
	}

	return Plugin_Handled;
}

// Admin command - revert all gains without disabling the upgrade system
Action Command_RTUReset(int client, int args) {
	if (!upgrades.Enabled) { CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Not Enabled"); }
	else {
		bank.ResetAccounts();
		upgrades.Reset();
		CPrintToChatAll("%s %t", RTU_BRAND, "RTU Reset");
	}

	return Plugin_Handled;
}

// MAGIC NUMBER: 100 as a minimum to discourage spam. 100 was the cost of the cheapest upgrade during development.
// MAGIC NUMBER: 30,000 is far beyond anything players earn during 60-minute rounds, and close to the netprop max value
// TODO: Convars for the above limits
Action Command_RTUPay(int client, int args) {
	if (!upgrades.Enabled) { CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Not Enabled"); }

	// Fetch client name for reporting, with a fallback of "The Server"
	char clientName[MAX_NAME_LENGTH]; strcopy(clientName, sizeof(clientName), "The Server");
	if (client > 0) GetClientName(client, clientName, sizeof(clientName));

	// Buffer for reporting target name, once determined
	char targetName[MAX_NAME_LENGTH];

	// Admins bypass the funds check because they grant currency rather than pay it
	// bool checkFunds = view_as<int>(GetUserAdmin(client)) > 0;
	bool checkFunds = !CheckCommandAccess(client, "", ADMFLAG_GENERIC, true);

	// Attempt to parse args a bit early to keep the conditional clean
	float amount = GetCmdArgFloat(1);
	char target[MAX_NAME_LENGTH]; GetCmdArg(2, target, MAX_NAME_LENGTH);

	// Perform several validations. If they pass, attempt and report the result
	if (args == 0) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Pay Usage");
	} else if (amount < 100.0 || amount > 30000.0) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Pay Amount Invalid", RoundToCeil(amount));
	} else if (checkFunds && amount > bank.Earned(client)) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Pay Insufficient Funds");
	} else if (!target[0]) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Pay Missing Target");
	} else if (!bank.DepositTarget(target, amount, client, targetName)) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Pay Invalid Target", target);
	} else {
		// NOTE: The DepositTarget method will print to center on success
		//CPrintToChatAll("%s %t", RTU_BRAND, "RTU Pay Success", clientName, RoundToCeil(amount), targetName);
		if (checkFunds) bank.Deposit(-amount, client);
	}

	return Plugin_Handled;
}

/**
 * t.7 Voting
 * =========================================================================
 */

// Add a vote and trigger a count
void Vote(int client) {
	if (!VotePossible(client)) { return; }

	Votes.Push(client);
	ReportVote(client);
	CountVotes();
}

// Remove a vote when a client disconnects. Does not trigger a count
void RemoveVote(int client) {
	int vote = Votes.FindValue(client);
	if (vote > -1) { Votes.Erase(vote); }
}

// Alert all players of the client's vote, vote count, and votes needed
void ReportVote(int client) {
	char requestedBy[MAX_NAME_LENGTH];
	GetClientName(client, requestedBy, sizeof(requestedBy));
	CPrintToChatAll("%s %t", RTU_BRAND, "RTU Requested", requestedBy, Votes.Length, VotesNeeded());
}

// Enable upgrades and award starting currency if vote passes
void CountVotes() {
	// Too soon to vote
	if (WaitingForPlayers) { return; }

	// No need to vote
	if (upgrades.Enabled) { return; }

	// Invalid vote
	if (PlayerCount < 1 || VotesNeeded() < 1) { return; }

	// Insufficient votes
	if (Votes.Length < VotesNeeded()) { return; }

	upgrades.Enable();
	bank.Sync();
	CPrintToChatAll("%s %t", RTU_BRAND, "RTU Enabled");
}

// Get required number of votes from a percentage of connected player count. Ensure a minimum of 1 to prevent unintended activation
int VotesNeeded() {
	float needed = float(PlayerCount) * g_Cvar_VoteThreshold.FloatValue;
	return needed < 1 ? 1 : RoundToCeil(needed);
}

// Check if it's safe to vote
bool VotePossible(int client) {
	// Too soon to vote
	if (WaitingForPlayers) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Not Allowed");
		return false;
	}

	// No need to vote
	if (upgrades.Enabled) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Already Enabled");
		return false;
	}

	// Already voted
	if (Votes.FindValue(client) >= 0) {
		CReplyToCommand(client, "%s %t", RTU_BRAND, "RTU Already Voted", Votes.Length, VotesNeeded());
		return false;
	}

	return true;
}

/**
 * t.8 Helpers
 * =========================================================================
 */


bool ValidPlayer(int client, TFClassType classType, TFTeam team) {
	return ValidClient(client) &&
		ValidClass(classType) &&
		ValidTeam(team);
}

// Duplicated in Bank module - consider centralizing
bool ValidClient(int client, bool checkConnected=true, bool checkInGame=true, bool checkFake=true) {
	// REQUIRED: within integer bounds
	if (client < 1 || client > MaxClients) return false;

	// OPTIONAL: connected
	if (checkConnected && !IsClientConnected(client)) return false;

	// OPTIONAL: in-game
	if (checkInGame && !IsClientInGame(client)) return false;

	// OPTIONAL: is human
	if (checkFake && IsFakeClient(client)) return false;

	return true;
}

// Not Unknown and within TF2 Class Range
bool ValidClass(TFClassType classType) {
	return classType > TFClass_Unknown && classType <= TFClass_Engineer;
}

// Not Unassigned or Spectator
bool ValidTeam(TFTeam team) {
	return team == TFTeam_Red || team == TFTeam_Blue;
}

// Automatically enable upgrades if enough players are present
void AttemptAutoEnable(){
	if (g_Cvar_AutoEnableThreshold.IntValue <= 0) return;
	if (PlayerCount < g_Cvar_AutoEnableThreshold.IntValue) return;
	if (upgrades.Enabled) return;

	CPrintToChatAll("%s %t", RTU_BRAND, "RTU AutoEnable");
	bank.Sync();
	upgrades.Enable();
}

void HandleLateLoad() {
	// Ensures the voting threshold is reasonable
	for (int i=1; i<=MaxClients; i++) {
		if (IsClientConnected(i)) {
			OnClientPostAdminCheck(i);
		}
	}
}
