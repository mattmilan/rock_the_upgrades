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
 * - add a timer for re-voting after a failed vote
 * - reset currency if map is extended
 * - add more configuration options
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

#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
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
// TODO: add a timer for re-voting after a failed vote
ConVar g_Cvar_Interval;
ConVar g_Cvar_CurrencyOnKill;

char g_RTUStationModel[] = "models/props_gameplay/resupply_locker.mdl";
bool g_RTUAllowed = false;	    // True if RTU is available to players. Used to delay rtu votes.
int g_Voters = 0;				// Total voters connected. Doesn't include fake clients.
int g_Votes = 0;				// Total number of votes
int g_VotesNeeded = 0;			// Necessary votes before upgrades are activated. (voters * percent_needed)
bool g_Voted[MAXPLAYERS+1] = {false, ...};

public void OnPluginStart() {
	// Ensure this model is precached (see notes at top of file)
	if (!IsModelPrecached(g_RTUStationModel)) { PrecacheModel(g_RTUStationModel, true); }

	// TODO: tried event `rd_robot_killed` but it never seemed to fire
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);

	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");

	g_Cvar_Needed = CreateConVar("sm_rtu_needed", "0.60", "Percentage of players needed to rockthevote (Def 60%)", 0, true, 0.05, true, 1.0);
	g_Cvar_MinPlayers = CreateConVar("sm_rtu_minplayers", "0", "Number of players required before RTU will be enabled.", 0, true, 0.0, true, float(MAXPLAYERS));
	g_Cvar_InitialDelay = CreateConVar("sm_rtu_initialdelay", "15.0", "Time (in seconds) before first RTU can be held", 0, true, 0.00);
	g_Cvar_Interval = CreateConVar("sm_rtu_interval", "120.0", "Time (in seconds) after a failed RTU before another can be held", 0, true, 0.00);
	g_Cvar_CurrencyOnKill = CreateConVar("sm_rtu_currency_on_kill", "25", "Amount of currency to give to players on robot kill", 0, true, 0.0, true, 1000.0);

	RegConsoleCmd("sm_rtu", Command_RTU);

	AutoExecConfig(true, "rtu");

	OnMapEnd();

	/* Handle late load */
	for (int i=1; i<=MaxClients; i++) {
		if (IsClientConnected(i)) {
			OnClientConnected(i);
		}
	}
}

public Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	int client = GetClientOfUserId(event.GetInt("attacker"));
	AddClientCurrency(client, g_Cvar_CurrencyOnKill.IntValue);
	return Plugin_Continue;
}

public void OnMapEnd() {
	g_RTUAllowed = false;
	g_Voters = 0;
	g_Votes = 0;
	g_VotesNeeded = 0;
}

public int GetClientCurrency(int client) {
	return GetEntProp(client, Prop_Send, "m_nCurrency");
}

public void SetClientCurrency(int client, int amount) {
	SetEntProp(client, Prop_Send, "m_nCurrency", amount);
}

public void AddClientCurrency(int client, int amount) {
	SetClientCurrency(client, amount+GetClientCurrency(client));
}

public void OnConfigsExecuted() {
	// TODO: do we still need this flag? is there a better one? can we use null?
	CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayRTU, _, TIMER_FLAG_NO_MAPCHANGE);
}

// Raise the threshold of required votes
public void OnClientConnected(int client) {
	if (!IsFakeClient(client)) {
		g_Voters++;
		g_VotesNeeded = RoundToCeil(float(g_Voters) * g_Cvar_Needed.FloatValue);
	}
}

// Lower the threshold of required votes. Activate RTU if exceeded
// Clear the clients vote if present
// Check votes against new threshold and activate RTU if exceeded
public void OnClientDisconnect(int client) {
	if (g_Voted[client]) {
		g_Votes--;
		g_Voted[client] = false;
	}

	if (!IsFakeClient(client)) {
		g_Voters--;
		g_VotesNeeded = RoundToCeil(float(g_Voters) * g_Cvar_Needed.FloatValue);
	}
	// TODO: simplify
	if (g_Votes &&
		g_Voters &&
		g_Votes >= g_VotesNeeded &&
		g_RTUAllowed ) {
		ActivateRTU();
	}
}

// Triggered when a client chats "/rtu" or "!rtu"
public Action Command_RTU(int client, int args) {
	if (client) { AttemptRTU(client); }
	return Plugin_Handled;
}

// Activate RTU, or reply to player with a failure message
void AttemptRTU(int client) {
	if (!g_RTUAllowed) {
		ReplyToCommand(client, "[SM] %t", "RTU Not Allowed");
		return;
	}

	if (GetClientCount(true) < g_Cvar_MinPlayers.IntValue) {
		ReplyToCommand(client, "[SM] %t", "Minimal Players Not Met");
		return;
	}

	if (g_Voted[client]) {
		ReplyToCommand(client, "[SM] %t", "Already Voted", g_Votes, g_VotesNeeded);
		return;
	}

	char requestedBy[MAX_NAME_LENGTH];
	GetClientName(client, requestedBy, sizeof(requestedBy));

	g_Votes++;
	g_Voted[client] = true;

	PrintToChatAll("[SM] %t", "RTU Requested", requestedBy, g_Votes, g_VotesNeeded);

	if (g_Votes >= g_VotesNeeded) { ActivateRTU(); }
}

public Action Timer_DelayRTU(Handle timer) {
	g_RTUAllowed = true;

	return Plugin_Continue;
}
