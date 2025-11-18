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
 *       open/close the upgrades menu when players enter/exit it's bounding box
 *
 * NOTE: for dev we're using "models/props_gameplay/resupply_locker.mdl" but not
 *       all maps use this model.
 *
 * TODO: Solve for the above...
 *       - Can plugins precache models? If so - no problem
 *       - Can plugins query the list of precached models? if so - just grab one
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
#include <mapchooser>
#include <nextmap>
#include <sdkhooks>
#include <enable_upgrades>

#pragma semicolon 1
#pragma newdecls required

public Plugin myinfo =
{
	name = "Rock The Upgrades",
	author = "Matt Milan (MurderousIntent)",
	description = "Provides a chat command to trigger a vote (similar to Rock the Vote) which, when passed, enables MvM upgrades for the current map.",
	version = SOURCEMOD_VERSION,
	url = "https://github.com/mattmilan/rock_the_upgrades"
};

ConVar g_Cvar_Needed;
ConVar g_Cvar_MinPlayers;
ConVar g_Cvar_InitialDelay;
ConVar g_Cvar_Interval;
ConVar g_Cvar_ChangeTime;
ConVar g_Cvar_RTUPostVoteAction;

bool g_RTUAllowed = false;	    // True if RTU is available to players. Used to delay rtu votes.
int g_Voters = 0;				// Total voters connected. Doesn't include fake clients.
int g_Votes = 0;				// Total number of votes
int g_VotesNeeded = 0;			// Necessary votes before upgrades are activated. (voters * percent_needed)
bool g_Voted[MAXPLAYERS+1] = {false, ...};

bool g_InChange = false;

public void OnPluginStart()
{
	LoadTranslations("common.phrases");
	LoadTranslations("rock_the_upgrades.phrases");

	g_Cvar_Needed = CreateConVar("sm_rtu_needed", "0.60", "Percentage of players needed to rockthevote (Def 60%)", 0, true, 0.05, true, 1.0);
	g_Cvar_MinPlayers = CreateConVar("sm_rtu_minplayers", "0", "Number of players required before RTU will be enabled.", 0, true, 0.0, true, float(MAXPLAYERS));
	g_Cvar_InitialDelay = CreateConVar("sm_rtu_initialdelay", "30.0", "Time (in seconds) before first RTU can be held", 0, true, 0.00);
	g_Cvar_Interval = CreateConVar("sm_rtu_interval", "240.0", "Time (in seconds) after a failed RTU before another can be held", 0, true, 0.00);
	g_Cvar_ChangeTime = CreateConVar("sm_rtu_changetime", "0", "When to change the map after a succesful RTU: 0 - Instant, 1 - RoundEnd, 2 - MapEnd", _, true, 0.0, true, 2.0);
	g_Cvar_RTUPostVoteAction = CreateConVar("sm_rtu_postvoteaction", "0", "What to do with RTU's after a mapvote has completed. 0 - Allow, success = instant change, 1 - Deny", _, true, 0.0, true, 1.0);

	RegConsoleCmd("sm_rtu", Command_RTU);

	AutoExecConfig(true, "rtu");

	OnMapEnd();

	/* Handle late load */
	for (int i=1; i<=MaxClients; i++)
	{
		if (IsClientConnected(i))
		{
			OnClientConnected(i);
		}
	}
}

public void OnMapEnd()
{
	g_RTUAllowed = false;
	g_Voters = 0;
	g_Votes = 0;
	g_VotesNeeded = 0;
	g_InChange = false;
}

public void OnConfigsExecuted()
{
	CreateTimer(g_Cvar_InitialDelay.FloatValue, Timer_DelayRTU, _, TIMER_FLAG_NO_MAPCHANGE);
}

public void OnClientConnected(int client)
{
	if (!IsFakeClient(client))
	{
		g_Voters++;
		g_VotesNeeded = RoundToCeil(float(g_Voters) * g_Cvar_Needed.FloatValue);
	}
}

public void OnClientDisconnect(int client)
{
	if (g_Voted[client])
	{
		g_Votes--;
		g_Voted[client] = false;
	}

	if (!IsFakeClient(client))
	{
		g_Voters--;
		g_VotesNeeded = RoundToCeil(float(g_Voters) * g_Cvar_Needed.FloatValue);
	}

	if (g_Votes &&
		g_Voters &&
		g_Votes >= g_VotesNeeded &&
		g_RTUAllowed )
	{
		if (g_Cvar_RTUPostVoteAction.IntValue == 1 && HasEndOfMapVoteFinished())
		{
			return;
		}

		StartRTU();
	}
}

public void OnClientSayCommand_Post(int client, const char[] command, const char[] sArgs)
{
	if (!client || IsChatTrigger())
	{
		return;
	}

	if (strcmp(sArgs, "rtu", false) == 0 || strcmp(sArgs, "rocktheupgrade", false) == 0)
	{
		//ActivateRTU();
		ActivatePush();

		//ReplySource old = SetCmdReplySource(SM_REPLY_TO_CHAT);

		//AttemptRTU(client);

		//SetCmdReplySource(old);
	}
}

public Action Command_RTU(int client, int args)
{
	if (!client)
	{
		return Plugin_Handled;
	}

	AttemptRTU(client);

	return Plugin_Handled;
}

void AttemptRTU(int client)
{
	// if (!g_RTUAllowed || (g_Cvar_RTUPostVoteAction.IntValue == 1 && HasEndOfMapVoteFinished()))
	// {
	// 	ReplyToCommand(client, "[SM] %t", "RTU Not Allowed");
	// 	return;
	// }

	if (!CanMapChooserStartVote())
	{
		ReplyToCommand(client, "[SM] %t", "RTU Started");
		return;
	}

	if (GetClientCount(true) < g_Cvar_MinPlayers.IntValue)
	{
		ReplyToCommand(client, "[SM] %t", "Minimal Players Not Met");
		return;
	}

	if (g_Voted[client])
	{
		ReplyToCommand(client, "[SM] %t", "Already Voted", g_Votes, g_VotesNeeded);
		return;
	}

	char name[MAX_NAME_LENGTH];
	GetClientName(client, name, sizeof(name));

	g_Votes++;
	g_Voted[client] = true;

	PrintToChatAll("[SM] %t", "RTU Requested", name, g_Votes, g_VotesNeeded);

	if (g_Votes >= g_VotesNeeded)
	{
		StartRTU();
	}
}

public Action Timer_DelayRTU(Handle timer)
{
	g_RTUAllowed = true;

	return Plugin_Continue;
}

void StartRTU() {
	if (g_InChange)
	{
		return;
	}

    ActivateRTU();
}

void ResetRTU()
{
	g_Votes = 0;

	for (int i=1; i<=MAXPLAYERS; i++)
	{
		g_Voted[i] = false;
	}
}

public Action Timer_ChangeMap(Handle hTimer)
{
	g_InChange = false;

	LogMessage("RTU changing map manually");

	char map[PLATFORM_MAX_PATH];
	if (GetNextMap(map, sizeof(map)))
	{
		ForceChangeLevel(map, "RTU after mapvote");
	}

	return Plugin_Stop;
}
