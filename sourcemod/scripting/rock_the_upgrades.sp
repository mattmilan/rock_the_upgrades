/**
 * vim: set ts=4 :
 * =============================================================================
 * Rock The Upgrades!
 *
 * Starts a vote.
 *
 * If the vote passes, enables the upgrade system and adds an upgrade station to
 * every resupply locker for the current map
 *
 * Borrows heavily from the original `Rock The Vote` plugin by AlliedModders LLC
 *
 * NOTE: func_upgradestation must have an arbitrary model set or it will not
 *       open/close the upgrades menu when players enter/exit its bounding box.
 *		 The resupply locker model is a good candidate for this, but it's not
 *       garunteed - some maps use an alternate model, so we will check this and
 *       precache when necessary
 *
 * TODO: Revert currency/upgrades on map extension
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

// Simplify upgrade cleanup during multi-stage transitions
#include <tf2attributes>

// Manages the state of the upgrade system and access to upgrade stations
#include <rtu/toggle_upgrades>

// Manages configurable currency gains and losses across various events
#include <rtu/currency_controller>

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

bool WaitingForPlayers; 	// Disallows voting while "Waiting for Players"
int PlayerCount;			// Number of connected clients (excluding bots)
ArrayList Votes;			// List of clients who have voted


public void OnPluginStart() {
	Votes = new ArrayList();
	PrecacheRequiredAssets();
	InitCurrencyController();
	InitConvars();
	HookEvents();
	RegisterCommands();

	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");
	AutoExecConfig(true, "rtu");
}

public void OnPluginEnd() {
	Votes.Close();
	CloseCurrencyController();
}

public void OnClientConnected(int client) {
	if (IsFakeClient(client)) { return; }

	PlayerCount++;
}

// NOTE: The vote might pass if a player disconnects without voting
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) { return; }

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

// Admin command - skip voting and enable immediately
Action Command_RTUEnable(int client, int args) {
	if (UpgradesEnabled()) { EnableUpgrades(); }
	else { ReplyToCommand(client, "[SM] %t", "RTU Already Enabled"); }

	return Plugin_Handled;
}

// Admin command - disable immediately
Action Command_RTUDisable(int client, int args) {
	if (UpgradesEnabled()) { DisableUpgrades(); }
	else { ReplyToCommand(client, "[SM] %t", "RTU Not Enabled"); }

	return Plugin_Handled;
}

// Admin command - revert all gains without disabling the upgrade system
Action Command_RTUReset(int client, int args) {
	if (UpgradesEnabled()) { ResetUpgrades(); }
	else { ReplyToCommand(client, "[SM] %t", "RTU Not Enabled"); }

	return Plugin_Handled;
}

void HookEvents() {
	// MULTI-STAGE RESETS
	HookEvent("teamplay_win_panel", Event_TeamplayWinPanel, EventHookMode_Post);

	// REFILL AMMO/METAL ON UPGRADE
	HookEvent("player_upgraded", Event_PlayerUpgraded_Pre, EventHookMode_Pre);
    HookEvent("player_upgraded", Event_PlayerUpgraded_Post, EventHookMode_Post);
}

void InitConvars() {
	// Voting behavior
	g_Cvar_VoteThreshold = CreateConVar("sm_rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);

	// Upgrades behavior
	g_Cvar_MultiStageReset = CreateConVar("sm_rtu_multistage_reset", "1", "Enable or disable resetting currency and upgrades on multi-stage map restarts/extensions [1, 0,1]", 0, true, 0.0, true, 1.0);
}

void RegisterCommands() {
	RegConsoleCmd("sm_rtu", Command_RTU, "Starts a vote to enable the upgrade system for the current map.");
	RegAdminCmd("sm_rtu_enable", Command_RTUEnable, ADMFLAG_GENERIC, "Immediately enable the upgrade system without waiting for a vote.");
	RegAdminCmd("sm_rtu_disable", Command_RTUDisable, ADMFLAG_GENERIC, "Immediately disable the upgrade system and revert all currency and upgrades");
	RegAdminCmd("sm_rtu_reset", Command_RTUReset, ADMFLAG_GENERIC, "Remove all upgrades and currency but leave the upgrade system enabled.");
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
	PrintToChatAll("[SM] %t", "RTU Requested", requestedBy, Votes.Length, VotesNeeded());
}

// Enable upgrades and award starting currency if vote passes
void CountVotes() {
	// No need to vote
	if (UpgradesEnabled()) { return; }

	// Invalid vote
	if (PlayerCount < 1 || VotesNeeded() < 1) { return; }

	// Insufficient votes
	if (Votes.Length < VotesNeeded()) { return; }

	EnableUpgrades();
}

// Get required number of votes from a percentage of connected player count. Ensure a minimum of 1 to prevent unintended activation
int VotesNeeded() {
	return RoundToCeil(float(PlayerCount) * g_Cvar_VoteThreshold.FloatValue);
}

// Check if it's safe to vote
bool VotePossible(int client) {
	// No need to vote
	if (UpgradesEnabled()) {
		ReplyToCommand(client, "[SM] %t", "RTU Already Enabled");
		return false;
	}

	// Too soon to vote
	if (WaitingForPlayers) {
		ReplyToCommand(client, "[SM] %t", "RTU Not Allowed");
		return false;
	}

	// Already voted
	if (Votes.FindValue(client) >= 0) {
		ReplyToCommand(client, "[SM] %t", "RTU Already Voted", Votes.Length, VotesNeeded());
		return false;
	}

	return true;
}

Action Event_TeamplayWinPanel(Event event, const char[] name, bool dontBroadcast) {
	// Bail if we're configured to not reset
	if (g_Cvar_MultiStageReset.IntValue == 0)  { return Plugin_Continue; }

	// Bail if the round is complete
	if (GetEventInt(event, "round_complete") == 1) { return Plugin_Continue; }

	// Bail if BLU lost
	if (GetEventInt(event, "winning_team") != 3) { return Plugin_Continue; }

	ResetUpgrades();
	return Plugin_Continue;
}

// `player_upgraded` is the event we need to hook as we want to fire some logic immediately after an upgrade is purchased
Action Event_PlayerUpgraded_Pre(Event event, const char[] name, bool dontBroadcast) {
    int client = GetClientOfUserId(event.GetInt("player"));
    if (client > 0 && IsClientInGame(client)) {
        int userid = GetClientUserId(client);

		// Connect the unique event id with the userid. I will be impressed if this works
        char key[16];
        IntToString(event.GetInt("eventid"), key, sizeof(key));
        UsersWhoUpgraded.SetValue(key, userid);
    }

    return Plugin_Continue;
}

// if the above pre_hook works then we will catch the player id which we need to refill their increased ammo capacity
Action Event_PlayerUpgraded_Post(Event event, const char[] name, bool dontBroadcast) {
    char key[16];
    IntToString(event.GetInt("eventid"), key, sizeof(key));

    int userid;
    if (UsersWhoUpgraded.GetValue(key, userid)) {
        TF2_RegeneratePlayer(userid);
        UsersWhoUpgraded.Remove(key);
    }

    return Plugin_Continue;
}
