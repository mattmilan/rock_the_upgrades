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
	name = "Rock The Upgrades",
	author = "MurderousIntent",
	description = "Provides a chat command to trigger a vote (similar to Rock the Vote) which, when passed, enables MvM upgrades for the current map.",
	version = SOURCEMOD_VERSION,
	url = "https://github.com/mattmilan/rock_the_upgrades"
};

ConVar g_Cvar_Needed;
ConVar g_Cvar_MinPlayers;
ConVar g_Cvar_InitialDelay;

ConVar g_Cvar_CurrencyOnKillMin;
ConVar g_Cvar_CurrencyOnKillMax;
ConVar g_Cvar_CurrencyOnCapturePoint;
ConVar g_Cvar_CurrencyOnCaptureFlag;
ConVar g_Cvar_CurrencyOnDomination;
ConVar g_Cvar_CurrencyOnRevenge;
ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
ConVar g_Cvar_CurrencyDeathTax;

// TODO: implement
// ConVar g_Cvar_UpgradeCostMultiplier;
// ConVar g_Cvar_CurrencyOverTime;
// ConVar g_Cvar_CurrencyOverTimeRate;
// ConVar g_Cvar_CurrencyOverTimeFrequency;
// ConVar g_Cvar_CurrencyLimit;

bool g_RTUAllowed = false;	    // False until voting is allowed. Used along with a timer, but could be tied to events
bool g_RTUAvailable = false;    // False if the map already implements some kind of upgrades (like in cp_freaky_fair)
bool g_RTUActivated = false;	// False until the vote passes. Prevents accidental revoting
int g_VotesNeeded = 1;			// Necessary votes before upgrades are activated. (voters * percent_needed)
ArrayList g_Votes;				// List of clients who have voted

/* SOURCEMOD CALLBACKS */
	public void OnPluginStart() {
		g_Votes = new ArrayList();

		EnsureRTUAvailable();
		EnsureRequiredModelIsPrecached();
		HookEvents();
		InitConvars();
		LoadTranslations("common.phrases");
		LoadTranslations("rock_the_upgrades.phrases");

		RegConsoleCmd("sm_rtu", Command_RTU);
		AutoExecConfig(true, "rtu");
	}

	public void OnPluginEnd() {
		if (g_Votes != null) { return; }

		g_Votes.Close();
	}

	public void OnMapEnd() {
		g_RTUAllowed = false;
		g_RTUActivated = false;
		g_RTUAvailable = false;
		g_Votes.Clear();
		g_VotesNeeded = 1;
	}

	public void OnConfigsExecuted() {
		// TODO: do we still need this flag? is there a better one? can we use null?
		CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayRTU, _, TIMER_FLAG_NO_MAPCHANGE);
	}

	// Update vote threshold (Ignore bots)
	public void OnClientConnected(int client) {
		if (IsFakeClient(client)) { return; }

		CountVotes();
	}

	// Clear vote
	public void OnClientDisconnect(int client) {
		RemoveVote(client);
	}
/* END CALLBACKS */

/* ACTIONS */
	public Action Command_RTU(int client, int args) {
		if (!client) { return Plugin_Continue; }

		Vote(client);
		return Plugin_Handled;
	}

	public Action Timer_DelayRTU(Handle timer) {
		g_RTUAllowed = true;
		return Plugin_Continue;
	}

	public Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
		int client = GetClientOfUserId(event.GetInt("attacker"));
		int randomizedCurrency = RoundToCeil(float(GetRandomInt(g_Cvar_CurrencyOnKillMin.IntValue, g_Cvar_CurrencyOnKillMax.IntValue)) * g_Cvar_CurrencyMultiplier.FloatValue);
		AddClientCurrency(client, randomizedCurrency);

		int victim = GetClientOfUserId(event.GetInt("userid"));
		int deathTax = RoundToCeil(g_Cvar_CurrencyDeathTax.FloatValue * float(GetClientCurrency(victim)));
		AddClientCurrency(victim, -deathTax);
		return Plugin_Continue;
	}

	Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
		int team = event.GetInt("team");
		int currency = RoundToCeil(float(g_Cvar_CurrencyOnCapturePoint.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
		GiveTeamCurrency(team, currency);
		return Plugin_Continue;
	}

	Action Event_TeamplayFlagCaptured(Event event, const char[] name, bool dontBroadcast) {
		int team = event.GetInt("team");
		int currency = RoundToCeil(float(g_Cvar_CurrencyOnCaptureFlag.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
		GiveTeamCurrency(team, currency);
		return Plugin_Continue;
	}

	Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
		int client = GetClientOfUserId(event.GetInt("dominator"));
		int currency = RoundToCeil(float(g_Cvar_CurrencyOnDomination.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
		AddClientCurrency(client, currency);
		return Plugin_Continue;
	}

	Action Event_PlayerRevenge(Event event, const char[] name, bool dontBroadcast) {
		int client = GetClientOfUserId(event.GetInt("revenge"));
		int currency = RoundToCeil(float(g_Cvar_CurrencyOnRevenge.IntValue)* g_Cvar_CurrencyMultiplier.FloatValue);
		AddClientCurrency(client, currency);
		return Plugin_Continue;
	}
/* END ACTIONS */

/* INITIALIZERS */
	void EnsureRTUAvailable() {
		g_RTUAvailable = FindEntityByClassname(-1, "func_upgradestation") == -1;
	}

	void EnsureRequiredModelIsPrecached() {
		char g_ArbitraryModel[] = "models/props_gameplay/resupply_locker.mdl";
		if (!IsModelPrecached(g_ArbitraryModel)) {	PrecacheModel(g_ArbitraryModel, true); }
	}

	void HookEvents(){
		// TODO: tried event `rd_robot_killed` but it never seemed to fire
		HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
		HookEvent("teamplay_point_captured", Event_TeamplayPointCaptured, EventHookMode_Post);
		HookEvent("teamplay_flag_captured", Event_TeamplayFlagCaptured, EventHookMode_Post);
		HookEvent("player_domination", Event_PlayerDomination, EventHookMode_Post);
		HookEvent("player_revenge", Event_PlayerRevenge, EventHookMode_Post);

	}

	void GiveTeamCurrency(int team, int amount) {
		for (int i = 1; i <= MaxClients; i++) {
			if (!IsClientInGame(i) || IsFakeClient(i)) { continue; }
			if (GetClientTeam(i) != team) { continue; }

			AddClientCurrency(i, amount);
		}
	}

	// NOTE: Many of these defaults and boundaries were decided arbitrarily and without much testing -
	void InitConvars() {
		// Voting behavior
		g_Cvar_Needed = CreateConVar("sm_rtu_voting_threshold", "0.55", "Percentage of players needed to rockthevote. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);
		g_Cvar_MinPlayers = CreateConVar("sm_rtu_min_players", "1", "Number of players required before RTU will be enabled. [1, 0..MAXPLAYERS]", 0, true, 0.0, true, float(MAXPLAYERS));
		g_Cvar_InitialDelay = CreateConVar("sm_rtu_initial_delay", "0.0", "Time (in seconds) before first RTU can be held. [30, 0..]", 0, true, 0.0, false);

		// Currency rules and modifiers, with reasonable defaults
		// g_Cvar_UpgradeCostMultiplier = CreateConVar("sm_rtu_upgrade_cost_multiplier", "1.0", "Multiplier for upgrade costs when RTU is activated. A value of zero provides free upgrades [1, 0..].", 0, true, 0.0, false);
		g_Cvar_CurrencyStarting = CreateConVar("sm_rtu_currency_starting", "250", "Starting amount of currency for players. Negative values incur a debt. Don't blame me, blame Merasmus.  [250, -inf..inf]", 0, false, 0.0, false);
		// g_Cvar_CurrencyLimit = CreateConVar("sm_rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
		g_Cvar_CurrencyMultiplier = CreateConVar("sm_rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyDeathTax = CreateConVar("sm_rtu_currency_death_tax", "0", "Percentage of currency to deduct on player death. 1 means all unspent currency is lost [0, 0..1]", 0, true, 0.0, true, 1.0);

		// Currency from kills. Random between min and max. On by default
		g_Cvar_CurrencyOnKillMin = CreateConVar("sm_rtu_currency_on_kill_min", "10", "Minimum amount of currency to give to players on robot kill [10, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyOnKillMax = CreateConVar("sm_rtu_currency_on_kill_max", "30", "Maximum amount of currency to give to players on robot kill [30, 0..]", 0, true, 0.0, false);

		// Bonus currency from special events. Off by default.
		g_Cvar_CurrencyOnCapturePoint = CreateConVar("sm_rtu_currency_on_capture_point", "250", "Amount of currency to give to team on point capture [250, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyOnCaptureFlag = CreateConVar("sm_rtu_currency_on_capture_flag", "250", "Amount of currency to give to team on flag capture [250, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyOnDomination = CreateConVar("sm_rtu_currency_on_domination", "0", "Amount of currency to give a player on domination [0, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyOnRevenge = CreateConVar("sm_rtu_currency_on_revenge", "100", "Amount of currency to give a player on revenge [100, 0..]", 0, true, 0.0, false);

		// Time-based currency gain. Off by default
		// g_Cvar_CurrencyOverTime = CreateConVar("sm_rtu_currency_over_time", "0", "Enable or disable time-based currency gain [0, 0,1]", 0, true, 0.0, true, 1.0);
		// g_Cvar_CurrencyOverTimeRate = CreateConVar("sm_rtu_currency_over_time_rate", "5", "Amount of currency earned per tick [5, 1..]", 0, true, 1.0, false);
		// g_Cvar_CurrencyOverTimeFrequency = CreateConVar("sm_rtu_currency_over_time_frequency", "30", "Frequency of time-based currency gain [30, 15..]", 0, true, 15.0, false);
	}
/* END INITIALIZERS */

/* VOTING */
	// Add a vote and then check if vote passes
	void Vote(int client) {
		if (!VotePossible(client)) { return; }

		g_Votes.Push(client);
		ReportVote(client);
		CountVotes();
	}

	// Remove a vote and then check if vote passes
	void RemoveVote(int client) {
		int vote = g_Votes.FindValue(client);
		if (vote == -1) { return; }

		g_Votes.Erase(vote);
		CountVotes();
	}

	// Alert all players of the vote and current count
	void ReportVote(int client) {
		char requestedBy[MAX_NAME_LENGTH];
		GetClientName(client, requestedBy, sizeof(requestedBy));
		PrintToChatAll("[SM] %t", "RTU Requested", requestedBy, g_Votes.Length, g_VotesNeeded);
	}

	// Update threshold JIT and enable upgrades if vote passes
	void CountVotes() {
		UpdateVoteThreshold();
		if (g_Votes.Length < g_VotesNeeded) { return; }

		g_RTUActivated = true;
		EnableUpgrades();

		for(int team = 0; team < GetTeamCount(); team++) {
			GiveTeamCurrency(team, RoundToCeil(g_Cvar_CurrencyStarting.FloatValue * g_Cvar_CurrencyMultiplier.FloatValue));
		}
	}

	// Get required number of votes from a percentage of connected player count
	void UpdateVoteThreshold() {
		int needed = RoundToCeil(float(GetClientCount(true)) * g_Cvar_Needed.FloatValue);
		g_VotesNeeded = needed < 1 ? 1 : needed;
		//g_VotesNeeded = RoundToCeil(float(GetClientCount(true)) * g_Cvar_Needed.FloatValue);
	}

	// Check if it's safe to vote
	bool VotePossible(int client) {
		if (!g_RTUAvailable) {
			ReplyToCommand(client, "[SM] %t", "RTU Not Available");
			return false;
		}

		if (g_RTUActivated) {
			ReplyToCommand(client, "[SM] %t", "RTU Already Activated");
			return false;
		}

		if (!g_RTUAllowed) {
			ReplyToCommand(client, "[SM] %t", "RTU Not Allowed");
			return false;
		}

		if (g_Votes.FindValue(client) >= 0) {
			ReplyToCommand(client, "[SM] %t", "Already Voted", g_Votes.Length, g_VotesNeeded);
			return false;
		}

		if (GetClientCount(true) < g_Cvar_MinPlayers.IntValue) {
			ReplyToCommand(client, "[SM] %t", "Minimal Players Not Met");
			return false;
		}



		return true;
	}
/* END VOTING */

/* CURRENCY HELPERS */
	// Get a client's currency value
	public int GetClientCurrency(int client) {
		return GetEntProp(client, Prop_Send, "m_nCurrency");
	}

	// Set client currency to an arbitrary value
	public void SetClientCurrency(int client, int amount) {
		SetEntProp(client, Prop_Send, "m_nCurrency", amount);
	}

	// Add an arbitrary value to a client's currency
	public void AddClientCurrency(int client, int amount) {
		SetClientCurrency(client, amount+GetClientCurrency(client));
	}
/* END CURRENCY HELPERS */