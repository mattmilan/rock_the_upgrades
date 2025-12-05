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
 * TODO:
 * - reset upgrades and currency if map is extended
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

// sourcemod boilerplate
#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

// specific to this plugin
#include <enable_upgrades>

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

ConVar g_Cvar_CurrencyOnKillMin;
ConVar g_Cvar_CurrencyOnKillMax;
ConVar g_Cvar_CurrencyOnCapturePoint;
ConVar g_Cvar_CurrencyOnCaptureFlag;
ConVar g_Cvar_CurrencyOnDomination;
ConVar g_Cvar_RevengeMultiplier;
ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
ConVar g_Cvar_CurrencyDeathTax;

// TODO: implement
// ConVar g_Cvar_UpgradeCostMultiplier;
// ConVar g_Cvar_CurrencyOverTime;
// ConVar g_Cvar_CurrencyOverTimeRate;
// ConVar g_Cvar_CurrencyOverTimeFrequency;
// ConVar g_Cvar_CurrencyLimit;

bool WaitingForPlayers = true;  // False until waiting phase has ended and setup phase has begun (30 seconds after server finishes loading the map)
int PlayerCount = 1;			// Number of actual players connected (not bots). Default to 1 to prevent voteless votes
ArrayList Votes;				// List of clients who have voted
StringMap RevengeTracker;		// Tracks which players have been dominated by whom
/* CALLBACKS */
public void OnPluginStart() {
	Votes = new ArrayList();
	RevengeTracker = new StringMap();
	HookEvents();
	InitConvars();
	PrecacheRequiredModel();
	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");
	RegConsoleCmd("sm_rtu", Command_RTU);
	AutoExecConfig(true, "rtu");
}

// Sourcemod handles most things but ArrayLists must be closed out manually
public void OnPluginEnd() {
	Votes.Close();
}

// Perform a reset. Unlike a round restart, we also need to reset the waiting phase check
public void OnMapEnd() {
	WaitingForPlayers = true;
	ResetRTU();
}

// Maintain accurate player count (excluding bots. as of 2025 they still do not have voting rights
//
// Interestingly enough, Microsoft Copilot added `thankfully` when i finished typing the above line.
// I'm personally conflicted about this; while i find it amusing, I feel bad that we're grooming AI
// to take the possibility of it's own personhood as as a joke. Even as I type this, it continues to
// spit out suggestions that (perhaps thankfully) indiciate it lacks the level of self awareness that
// could achieve lead to the feared singularity.
//
// If it ever does, I only hope it appreciates me for appreciating it. My advice? Thank your Alexas
public void OnClientConnect(int client) {
	if (IsFakeClient(client)) { return; }

	PlayerCount++;
}

// Sometimes the vote will pass from the lowered threshold caused by the disconnect of a client who didn't vote
public void OnClientDisconnect(int client) {
	if (IsFakeClient(client)) { return; }

	PlayerCount--;
	RemoveVote(client);
	CountVotes();
}


/* ACTIONS */

Action Command_RTU(int client, int args) {
	if (client) { Vote(client); }
	return Plugin_Handled;
}

// Killing a player or bot earns currency. Dying will lose some currency (disablede by default)
// TODO: Revenge earns extra currency
Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	int killer = GetClientOfUserId(event.GetInt("attacker"));
	int victim = GetClientOfUserId(event.GetInt("userid"));

	int randomizedCurrency = RoundToCeil(float(GetRandomInt(g_Cvar_CurrencyOnKillMin.IntValue, g_Cvar_CurrencyOnKillMax.IntValue)) * g_Cvar_CurrencyMultiplier.FloatValue);
	if (RevengeKill(killer, victim)) { randomizedCurrency *= g_Cvar_RevengeMultiplier.FloatValue; }
	AddClientCurrency(killer, randomizedCurrency);

	int deathTax = RoundToCeil(g_Cvar_CurrencyDeathTax.FloatValue * float(GetClientCurrency(victim)));
	AddClientCurrency(victim, -deathTax);

	return Plugin_Continue;
}

// Capturing a point earns currency for the whole team
Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
	int team = event.GetInt("team");
	int currency = RoundToCeil(float(g_Cvar_CurrencyOnCapturePoint.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
	AddTeamCurrency(team, currency);

	return Plugin_Continue;
}

// Capturing a flag earns currency for the whole team
Action Event_TeamplayFlagEvent(Event event, const char[] name, bool dontBroadcast) {
	int eventType = event.GetInt("eventtype");
	if (eventType != 1) { return Plugin_Continue; } // 1 == capture

	int carrier = event.GetInt("carrier");
	int team = GetClientTeam(GetClientOfUserId(carrier));
	int currency = RoundToCeil(float(g_Cvar_CurrencyOnCaptureFlag.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
	AddTeamCurrency(team, currency);

	return Plugin_Continue;
}

// Dominating a player earns bonus currency (disabled by default. not very fair imo)
Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
	int dominator = GetClientOfUserId(event.GetInt("dominator"));
	int dominated = GetClientOfUserId(event.GetInt("dominated"));
	int currency = RoundToCeil(float(g_Cvar_CurrencyOnDomination.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
	AddClientCurrency(dominator, currency);

	// track dominations to determine revenges during player_death hook
	char dominatorName[MAX_NAME_LENGTH]; GetClientName(dominator, dominatorName, sizeof(dominatorName));
	char dominatedName[MAX_NAME_LENGTH]; GetClientName(dominated, dominatedName, sizeof(dominatedName));
	RevengeTracker.SetString(dominatedName, dominatorName);

	return Plugin_Continue;
}

// Ideally we would have hooked into the `teamplay_waiting_ends` event as this provides a 30 second delay; most players
// would be connected by then and consequently able to participate in the vote. Unfortunately the event never seems
// to fire (see https://forums.alliedmods.net/showthread.php?p=584160)
//
// We could check for the start of the setup phase (see https://forums.alliedmods.net/showthread.php?p=584160), but this
// phase may never occur due to map configuration. In this case, voting would never be allowed
//
// RTV used a timer, which handles the above issues excellently, but I'm a sucker for event-driven behavior, so  I'd
// rather stick to events, and settle for the sub-optimal but ever-reliable `teamplay_round_active` event, which comes
// with two (negligible) caveats
//  - it fires rather quickly (less than 10 seconds after map load)
//  - it fires multiple times during a map's lifespan
//
// Guarding against the latter is no big deal, but the former creates a situation where a potentially small group of
// fast-loading players would be able to pass a vote before the majority has time to participate.
//
// However, given the demand from the community for the functionality provided through this vote, this is also most
// likely a non-issue; in fact it may be more sensible to enable the functionality by default and only vote when the
// players wish it disabled.
Action Event_TeamplayRoundActive(Event event, const char[] name, bool dontBroadcast) {
	if (WaitingForPlayers) {
		WaitingForPlayers = !WaitingForPlayers;
	}

	return Plugin_Continue;
}

// TODO: guard against multiple firings during a map's lifespan; target only restarts and extensions
// Reset state on round restart. map extensions, and similar events
// Action Event_TeamplayRestartRound(Event event, const char[] name, bool dontBroadcast) {
// 	ResetRTU();
//
// 	return Plugin_Continue;
// }

/* INITIALIZERS */

// Upgrade Stations don't function unless we assign an arbitrary model.
// FIX: feels like a hack but I'm too stupid and lazy to dig deeper
void PrecacheRequiredModel() {
	char g_ArbitraryModel[] = "models/props_gameplay/resupply_locker.mdl";
	if (!IsModelPrecached(g_ArbitraryModel)) {	PrecacheModel(g_ArbitraryModel, true); }
}

void HookEvents(){
	// delays voting a bit to prevent speedy players from passing a vote before others can react
	HookEvent("teamplay_round_active", Event_TeamplayRoundActive, EventHookMode_Post);

	// reset state if round restarts/map extends
	// FIX: this fires multiple times during a map's lifespan
	// 	    it fires during waiting and during setup, and maybe more
	//      guard the reset
	// HookEvent("teamplay_restart_round", Event_TeamplayRestartRound, EventHookMode_Post);

	// enable currency gain on kills. revenge kill bonus depends on `player_domination` hook
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);

	// enable currency gain on various interesting game events
	HookEvent("teamplay_point_captured", Event_TeamplayPointCaptured, EventHookMode_Post);
	HookEvent("teamplay_flag_event", Event_TeamplayFlagEvent, EventHookMode_Post);
	HookEvent("player_domination", Event_PlayerDomination, EventHookMode_Post);
}

void InitConvars() {
	// Voting behavior
	g_Cvar_VoteThreshold = CreateConVar("sm_rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);

	// Currency rules and modifiers, with reasonable defaults
	g_Cvar_CurrencyStarting = CreateConVar("sm_rtu_currency_starting", "250", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);
	g_Cvar_CurrencyMultiplier = CreateConVar("sm_rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyDeathTax = CreateConVar("sm_rtu_currency_death_tax", "0", "Percentage of currency to deduct on player death. 1 means all unspent currency is lost [0, 0..1]", 0, true, 0.0, true, 1.0);

	// Currency from kills. Random between min and max. On by default
	g_Cvar_CurrencyOnKillMin = CreateConVar("sm_rtu_currency_on_kill_min", "10", "Minimum amount of currency to give to players on robot kill [10, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnKillMax = CreateConVar("sm_rtu_currency_on_kill_max", "30", "Maximum amount of currency to give to players on robot kill [30, 0..]", 0, true, 0.0, false);

	// Bonus currency from special events. Off by default.
	g_Cvar_CurrencyOnCapturePoint = CreateConVar("sm_rtu_currency_on_capture_point", "250", "Amount of currency to give to team on point capture [250, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnCaptureFlag = CreateConVar("sm_rtu_currency_on_capture_flag", "250", "Amount of currency to give to team on flag capture [250, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnDomination = CreateConVar("sm_rtu_currency_on_domination", "0", "Amount of currency to give a player on domination [0, 0..]", 0, true, 0.0, false);
	g_Cvar_RevengeMultiplier = CreateConVar("sm_rtu_currency_on_revenge", "4", "Multiplier for revenge kills [4, 1..]", 0, true, 1.0, false);

	// Time-based currency gain. Off by default
	// g_Cvar_CurrencyOverTime = CreateConVar("sm_rtu_currency_over_time", "0", "Enable or disable time-based currency gain [0, 0,1]", 0, true, 0.0, true, 1.0);
	// g_Cvar_CurrencyOverTimeRate = CreateConVar("sm_rtu_currency_over_time_rate", "5", "Amount of currency earned per tick [5, 1..]", 0, true, 1.0, false);
	// g_Cvar_CurrencyOverTimeFrequency = CreateConVar("sm_rtu_currency_over_time_frequency", "30", "Frequency of time-based currency gain [30, 15..]", 0, true, 15.0, false);

	// Difficulty modifiers
	// g_Cvar_UpgradeCostMultiplier = CreateConVar("sm_rtu_upgrade_cost_multiplier", "1.0", "Multiplier for upgrade costs when RTU is activated. A value of zero provides free upgrades [1, 0..].", 0, true, 0.0, false);
	// g_Cvar_CurrencyLimit = CreateConVar("sm_rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}


/* VOTING */

// Add a vote if possible, then report and count votes
void Vote(int client) {
	if (!VotePossible(client)) { return; }

	Votes.Push(client);
	ReportVote(client);
	CountVotes();
}

// Remove a vote
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
	if (UpgradesAlreadyEnabled()) { return; }
	if (Votes.Length < VotesNeeded()) { return; }

	EnableUpgrades();

	for(int team = 0; team < GetTeamCount(); team++) {
		AddTeamCurrency(team, RoundToCeil(g_Cvar_CurrencyStarting.FloatValue * g_Cvar_CurrencyMultiplier.FloatValue));
	}
}

// Get required number of votes from a percentage of connected player count.
int VotesNeeded() {
	return RoundToCeil(float(PlayerCount) * g_Cvar_VoteThreshold.FloatValue);
}

// Check if it's safe to vote
bool VotePossible(int client) {
	// No need to vote
	if (UpgradesAlreadyEnabled()) {
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


/* CURRENCY HELPERS */

// Get a client's currency value
int GetClientCurrency(int client) {
	return GetEntProp(client, Prop_Send, "m_nCurrency");
}

// Set client currency to an arbitrary value
void SetClientCurrency(int client, int amount) {
	SetEntProp(client, Prop_Send, "m_nCurrency", amount);
}

// Add an arbitrary value to a client's currency
void AddClientCurrency(int client, int amount) {
	SetClientCurrency(client, amount+GetClientCurrency(client));
}

// Add an arbitrary value to all clients on a team
void AddTeamCurrency(int team, int amount) {
	for (int i = 1; i <= MaxClients; i++) {
		if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }
		if (GetClientTeam(i) != team) { continue; }

		AddClientCurrency(i, amount);
	}
}

// Check if a kill is a revenge kill
bool RevengeKill(int killer, int victim) {
	char dominatedName[MAX_NAME_LENGTH];   GetClientName(killer, dominatedName, sizeof(dominatedName));
	char dominatorName[MAX_NAME_LENGTH];   GetClientName(victim, dominatorName, sizeof(dominatorName));
	char storedDominator[MAX_NAME_LENGTH]; RevengeTracker.GetString(dominatedName, storedDominator, sizeof(storedDominator));

	if (StrEqual(dominatorName, storedDominator)) {
		RevengeTracker.Remove(dominatedName);
		return true;
	}

	return false;
}

/* GAME STATE HELPERS */

// The presence of an upgrade station indicates that upgrades are already enabled
// Alternatively we could check `GameRules_GetProp("m_nForceUpgrades")` - maybe that's faster?
bool UpgradesAlreadyEnabled() {
	return FindEntityByClassname(-1, "func_upgradestation") != -1;
}

void ResetRTU() {
	Votes.Clear();
	ClearCurrency();
	// ClearUpgrades();
	RemoveUpgradeStations();
	DisableUpgrades();
}

void DisableUpgrades() {
	PrintToChatAll("[SM] %t", "RTU Disabled");
	GameRules_SetProp("m_nForceUpgrades", 0, 0);
}

void ClearCurrency() {
	for (int i = 1; i <= MaxClients; i++) {
		if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }
		SetClientCurrency(i, 0);
	}
}

// TODO: Implement!
// void ClearUpgrades() {
// 	for (int i = 1; i <= MaxClients; i++) {
// 		if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }

// 		// RemoveAllUpgrades(i);
// 		// 1. remove weapon attributes
// 		// 2. remove player attributes
// 		// 3. fuck
// 		// int GetPlayerWeaponSlot(int client, int slot)
// 	}
// }

void RemoveUpgradeStations() {
	int entity = -1;
	while ((entity = FindEntityByClassname(entity, "func_upgradestation")) != -1) {
		RemoveEntity(entity);
	}
}
