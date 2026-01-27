/**
 * Rock The Upgrades: BankTxn Controller
 * TODO: description
 */

#if defined _rewards_included
 #endinput
#endif
#define _rewards_included

// #include "shared"
// #include <tf2>

#include "bank"
#include "revenge_tracker"

ConVar g_Cvar_CurrencyOnKillMin;
ConVar g_Cvar_CurrencyOnKillMax;
ConVar g_Cvar_CurrencyOnObjectDestroyed;
ConVar g_Cvar_CurrencyDeathTax;
ConVar g_Cvar_RevengeMultiplier;
ConVar g_Cvar_CurrencyOnCapturePoint;
ConVar g_Cvar_CurrencyOnCaptureFlag;
ConVar g_Cvar_CurrencyOnDomination;

// TODO: implement
// ConVar g_Cvar_CurrencyOverTime;
// ConVar g_Cvar_CurrencyOverTimeRate;
// ConVar g_Cvar_CurrencyOverTimeFrequency;

// public void OnPluginEnd() {
// 	BankTxn.Close();
// }


methodmap Reward  {
	public static BankTxn PlayerKilled(Event event) {
		BankTxn payment;
		PrintToServer("[RTU] Rewarding Player Kill Event Start");
		int killer   = GetClientOfUserId(event.GetInt("attacker"));
		int assister = GetClientOfUserId(event.GetInt("assister"));
		int victim   = GetClientOfUserId(event.GetInt("userid"));

		// No one gets paid for self destructs - return an empty list
		if (Reward.SelfDestruct(killer, victim)) return payment;

		bool wasRevenge = Reward.RevengeKill(killer, assister, victim);

		float reward = GetRandomFloat(
			g_Cvar_CurrencyOnKillMin.FloatValue,
			g_Cvar_CurrencyOnKillMax.FloatValue
		) * Reward.RevengeMultiplier(wasRevenge);

		float penalty = Reward.DeathPenalty(victim);

		if (ValidClient(killer))   payment.AddDeposit(reward, killer);
		if (ValidClient(assister)) payment.AddDeposit(reward, assister);
		if (ValidClient(victim))   payment.AddBurn(penalty, victim);
		PrintToServer("[RTU] Rewarding Player Kill Event Finish");
		return payment;
	}

	public static BankTxn ObjectDestroyed(Event event) {
		BankTxn payment;

		// Ignore sappers completely
		if (event.GetInt("objecttype") == view_as<int>(TFObject_Sapper)) return payment;

		// Play SFX when destroying an unfinished building
		// if (event.GetBool("was_building")) { ObjectDenied(event); }

		int objectIndex = event.GetInt("index");
		int upgradeLevel = GetEntProp(objectIndex, Prop_Send, "m_iUpgradeLevel");
		float reward =  upgradeLevel * g_Cvar_CurrencyOnObjectDestroyed.FloatValue;

		int attacker = GetClientOfUserId(event.GetInt("attacker"));
		int assister = GetClientOfUserId(event.GetInt("assister"));

		if (ValidClient(attacker)) payment.AddDeposit(reward, attacker);
		if (ValidClient(assister)) payment.AddDeposit(reward, assister);

		return payment;
	}

	public static BankTxn PointCaptured(Event event) {
		BankTxn payment;

		float reward = g_Cvar_CurrencyOnCapturePoint.FloatValue;

		TFTeam team = view_as<TFTeam>(event.GetInt("team"));

		payment.AddDeposit(reward, .team=team);

		return payment;
	}

	public static BankTxn FlagCaptured(Event event) {
		BankTxn payment;

		// MAGIC NUMBER: 2 is "captured"
		if (event.GetInt("eventtype") != 2) return payment;

		float reward = g_Cvar_CurrencyOnCaptureFlag.FloatValue;

		int player = event.GetInt("player");

		// event.GetInt("team") won't work as some maps have inverted flag logic
		TFTeam team = view_as<TFTeam>(GetClientTeam(player));

		payment.AddDeposit(reward, .team=team);

		return payment;
	}

	public static BankTxn PlayerDominated(Event event) {
		BankTxn payment;

		float reward = g_Cvar_CurrencyOnDomination.FloatValue;

		int dominator = GetClientOfUserId(event.GetInt("dominator"));
		int dominated = GetClientOfUserId(event.GetInt("dominated"));

		RevengeTracker.Instance().Track(dominator, dominated);

		if (ValidClient(dominator)) payment.AddDeposit(reward, dominator);

		return payment;
	}

	// Deny payment
	public static bool SelfDestruct(int killer, int victim) {
		return killer < 1 || killer == victim;
	}

	// Returns a ConVar-defined float on revenge, or 1 (which will have no effect)
	public static float RevengeMultiplier(bool revenge=false) {
		return revenge ? g_Cvar_RevengeMultiplier.FloatValue : 1.0;
	}

	public static bool Dominating(int dominator, int dominated) {
		return RevengeTracker.Instance().Check(dominator, dominated);
	}

	// Revenge kills trigger a bonus multiplier
	// TODO: optimize by avoiding conversion from id to name, if possible
	public static bool RevengeKill(int killer, int assister, int victim) {
		return Reward.Dominating(victim, killer) || Reward.Dominating(victim, assister);
	}

	// Dying may incur a penalty (disabled by default)
	// Returns a positive value, should be inverted when called
	public static float DeathPenalty(int victim) {
		PlayerProfile p;
		PlayerProfiles.Find(victim, p);

		return p.Balance()
			* g_Cvar_CurrencyDeathTax.FloatValue;
	}
}

// Play SFX when buildings are destroyed while being built
// TODO: Debug the heck out of this
// void ObjectDenied(Event event) {
// 	PrintToServer("[RTU] ObjectDenied event triggered");
// 	int builder = GetClientOfUserId(event.GetInt("userid"));
// 	int attacker = GetClientOfUserId(event.GetInt("attacker"));
// 	int assister = GetClientOfUserId(event.GetInt("assister"));

// 	if (ValidClient(builder))  EmitGameSoundToClient(builder,  "vo/engineer_no01.wav");
// 	if (ValidClient(attacker)) EmitGameSoundToClient(attacker, "vo/engineer_gunslingerpunch02.wav");
// 	if (ValidClient(assister)) EmitGameSoundToClient(assister, "vo/engineer_gunslingerpunch02.wav");
// }



void InitBankTxnConVars() {
	// Currency gain
	g_Cvar_CurrencyOnKillMin = CreateConVar("rtu_currency_on_kill_min", "10.0", "Minimum amount of currency to give to players on robot kill [10, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnKillMax = CreateConVar("rtu_currency_on_kill_max", "30.0", "Maximum amount of currency to give to players on robot kill [30, 0..]", 0, true, 0.0, false);
	g_Cvar_RevengeMultiplier = CreateConVar("rtu_currency_on_revenge", "4.0", "Multiplier for revenge kills [4, 1..]", 0, true, 1.0, false);
	g_Cvar_CurrencyOnDomination = CreateConVar("rtu_currency_on_domination", "0.0", "Amount of currency to give a player on domination [0, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnObjectDestroyed = CreateConVar("rtu_currency_on_object_destroyed", "5.0", "Base amount of currency to give to players on building destruction, multiplied by building level [5, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnCapturePoint = CreateConVar("rtu_currency_on_capture_point", "150.0", "Amount of currency to give to team on point capture in addition to the built-in default 100 currency [150, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnCaptureFlag = CreateConVar("rtu_currency_on_capture_flag", "250.0", "Amount of currency to give to team on flag capture [250, 0..]", 0, true, 0.0, false);

    // Currency loss
    g_Cvar_CurrencyDeathTax = CreateConVar("rtu_currency_death_tax", "0.0", "Percentage of currency to deduct on player death. 1 means all unspent currency is lost [0, 0..1]", 0, true, 0.0, true, 1.0);
}
