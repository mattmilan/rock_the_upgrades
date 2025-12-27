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

// SourceMod Boilerplate
#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <tf2>

// Facilitates the removal of upgrades during resets
#include <tf2attributes>

// Enable/Disable upgrade system, and easily reset entity upgrades
#include <rock_the_upgrades/toggle_upgrades>

// Manages configurable currency gains and losses across various events
#include <rock_the_upgrades/currency_controller>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo = {
	name = "Rock The Upgrades (or Freaky Fair Anywhere)",
	author = "MurderousIntent",
	description = "Provides a chat command to trigger a vote (similar to Rock the Vote) which, when passed, enables MvM upgrades for the current map.",
	version = SOURCEMOD_VERSION,
	url = "https://github.com/mattmilan/rock_the_upgrades"
};

ConVar g_Cvar_VoteThreshold;
ConVar g_Cvar_MultiStageReset;

bool WaitingForPlayers; 		 // Disallows voting while "Waiting for Players"
int PlayerCount;				 // Number of connected clients (excluding bots)
bool RTULateLoad;				 // Might be needed to get SteamIDs in lateload
ArrayList Votes;				 // List of clients who have voted

// TODO: might not be needed, requires exploration
public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
    RTULateLoad = late;
    return APLRes_Success;
}

public void OnPluginStart() {
	// plugin setup
	Votes = new ArrayList();
	InitCurrencyController();
	InitConvars();
	HookEvents();
	RegisterCommands();

	// boilerplate configs
	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");
	AutoExecConfig(true, "rtu");

	if (RTULateLoad) HandleLateLoad();
}

// Clean up if the plugin is unloaded/reloaded
public void OnPluginEnd() {
	Votes.Close();
	CloseCurrencyController();
}

public void OnMapStart() {
	PrintToServer("[RTU] Map Start");
	bank.PrintAccounts();
 	PlayerCount = 0;
	WaitingForPlayers = true;
	RevengeTracker.Clear(); // from currency_controller
 	Votes.Clear();
	bank.Clear();
	PrecacheRequiredAssets(); // from toggle_upgrades
}

// Simply increment voting threshold. Continued in `OnClientAuthorized`
public void OnClientConnected(int client) {
	PrintToServer("[RTU] Client Connected");
	if (IsFakeClient(client)) { return; }

	PlayerCount++;
}

// Banks maintain accounts for each connected client for the duration of the map
// When reconnecting, their currency is retained; upgrades must be repurchased
// (the disconnect event resets their `spent` value)
// This is only possible by using a trusted unique identifier - SteamID - as the
// bank key. This is the earliest forward in which that identifier is available.
public void OnClientAuthorized(int client) {
	PrintToServer("[RTU] Client Authorized");
	if (IsFakeClient(client)) { return; }

	bank.Connect(client);
}

// NOTE: The vote might pass if a player disconnects without voting
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) { return; }

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

// Player command - attempts to vote
Action Command_RTU(int client, int args) {
	if (client) { Vote(client); }
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
	if (UpgradesEnabled()) {
		ReplyToCommand(client, "[RTU] %t", "RTU Already Enabled");
	} else {
		bank.Sync();
		EnableUpgrades();
	}

	return Plugin_Handled;
}

// Admin command - disable immediately
// TODO: Determine and support cases where we would not want to reset (doubhtful)
Action Command_RTUDisable(int client, int args) {
	if (!UpgradesEnabled()) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		Votes.Clear();
		bank.ResetAccounts();
		ResetUpgrades(.silent = true);
		DisableUpgrades();
	}

	return Plugin_Handled;
}

// Admin command - revert all gains without disabling the upgrade system
Action Command_RTUReset(int client, int args) {
	if (!UpgradesEnabled()) { ReplyToCommand(client, "[RTU] %t", "RTU Not Enabled"); }
	else {
		bank.ResetAccounts();
		ResetUpgrades();
	}

	return Plugin_Handled;
}

Action Command_RTUPay(int client, int args) {
	// Show usage if no args provided
	if (args == 0) {
		ReplyToCommand(client, "[RTU] Usage: rtu_pay <amount> <optional|all|player>");
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

	// Pay self if no target specified
	if (!target[0] && client > 0) {
		bank.Deposit(amount, client);
	} else if (!bank.DepositTarget(target, amount, .replyTo=client)) {
		ReplyToCommand(client, "[RTU] Could not find player %s", target);
	} else {
		ReplyToCommand(client, "[RTU] Command `rtu_pay` cannot be called from server without specifying a target");
	}

	return Plugin_Handled;
}


void HookEvents() {
	HookEvent("teamplay_round_start", Event_TeamplayRoundStart, EventHookMode_Post);
	HookEvent("player_initial_spawn", Event_PlayerInitialSpawn, EventHookMode_Post);
	HookEvent("player_changeclass", Event_PlayerChangeClass, EventHookMode_Post);
}

// Bank tracks currency per class - inform it of all class changes
Action Event_PlayerChangeClass(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("userid"));
	if (IsFakeClient(client)) { return Plugin_Continue; }

	TFClassType classType = view_as<TFClassType>(event.GetInt("class"));
	bank.SetClass(client, classType);

	return Plugin_Continue;
}

// Synchronize currency as soon as it's safe; ie once the clients `m_nCurrency` prop exists
Action Event_PlayerInitialSpawn(Event event, const char[] name, bool dontBroadcast) {
	int client = event.GetInt("index");
	if (IsFakeClient(client)) { return Plugin_Continue; }
	// UpdateAccountTFClass(event);
	// if (UpgradesEnabled())
	bank.Sync(client);

	return Plugin_Continue;
}

// Reset on round start unless configured otherwise. This may be firing too often.
Action Event_TeamplayRoundStart(Event event, const char[] name, bool dontBroadcast) {
	if (g_Cvar_MultiStageReset.IntValue == 1) {
		bank.ResetAccounts();
		ResetUpgrades();
	}

	return Plugin_Continue;
}

void InitConvars() {
	g_Cvar_VoteThreshold = CreateConVar("rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);
	g_Cvar_MultiStageReset = CreateConVar("rtu_multistage_reset", "1", "Enable or disable resetting currency and upgrades on multi-stage map restarts/extensions [1, 0,1]", 0, true, 0.0, true, 1.0);
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
	PrintToChatAll("[RTU] %t", "RTU Requested", requestedBy, Votes.Length, VotesNeeded());
}

// Enable upgrades and award starting currency if vote passes
void CountVotes() {
	// Too soon to vote
	if (WaitingForPlayers) { return; }

	// No need to vote
	if (UpgradesEnabled()) { return; }

	// Invalid vote
	if (PlayerCount < 1 || VotesNeeded() < 1) { return; }

	// Insufficient votes
	if (Votes.Length < VotesNeeded()) { return; }

	EnableUpgrades();
	bank.Sync();
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
		ReplyToCommand(client, "[RTU] %t", "RTU Not Allowed");
		return false;
	}

	// No need to vote
	if (UpgradesEnabled()) {
		ReplyToCommand(client, "[RTU] %t", "RTU Already Enabled");
		return false;
	}

	// Already voted
	if (Votes.FindValue(client) >= 0) {
		ReplyToCommand(client, "[RTU] %t", "RTU Already Voted", Votes.Length, VotesNeeded());
		return false;
	}

	return true;
}

void HandleLateLoad() {
	PrecacheRequiredAssets();

	// Ensures the voting threshold is reasonable
	for (int i=1; i<=MaxClients; i++) {
		if (IsClientConnected(i)) {
			OnClientConnected(i);
		}
	}
}
