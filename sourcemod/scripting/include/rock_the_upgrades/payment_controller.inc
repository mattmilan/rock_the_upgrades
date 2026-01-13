/**
 * Rock The Upgrades: Payment Controller
 * TODO: description
 */

#if defined _payment_controller_included
 #endinput
#endif
#define _payment_controller_included

// #include "shared"
// #include <tf2>

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
// 	Payment.Close();
// }


enum struct PaymentController {
	StringMap RevengeTracker;
	Bank bank;

	void Init(Bank bank) {
		// RevengeTracker = new StringMap();
		this.bank = bank;
		this.RevengeTracker = new StringMap();
	}

	void Reset() {
		this.RevengeTracker.Clear();
	}

	void Track(char dominated[MAX_NAME_LENGTH], char dominator[MAX_NAME_LENGTH]) {
		this.RevengeTracker.SetString(dominated, dominator);
	}

	float ForKill(bool revenge=false) {
		return GetRandomFloat(
			g_Cvar_CurrencyOnKillMin.FloatValue,
			g_Cvar_CurrencyOnKillMax.FloatValue
		) * this.RevengeMultiplier(revenge);
	}

	float ForDestruction(Event event) {
		int objectIndex = event.GetInt("index");
		int upgradeLevel = GetEntProp(objectIndex, Prop_Send, "m_iUpgradeLevel");

		return upgradeLevel * g_Cvar_CurrencyOnObjectDestroyed.FloatValue;
	}

	// Deny payment
	bool Suicide(int killer, int victim) {
		return killer < 1 || killer == victim;
	}

	// Returns a ConVar-defined float on revenge, or 1 (which will have no effect)
	float RevengeMultiplier(bool revenge=false) {
		return revenge ? g_Cvar_RevengeMultiplier.FloatValue : 1.0;
	}

	// Revenge kills trigger a bonus multiplier
	// TODO: optimize by avoiding conversion from id to name, if possible
	bool RevengeKill(int killer, int victim) {
		// get killer name
		char dominatedName[MAX_NAME_LENGTH];
		GetClientName(killer, dominatedName, sizeof(dominatedName));

		// check if killer has been dominated, and if so, who the dominator was
		char storedDominator[MAX_NAME_LENGTH];
		bool revenge = this.RevengeTracker.GetString(dominatedName, storedDominator, sizeof(storedDominator));

		// bail early if killer hasn't actually been dominated
		if (!revenge) { return false; }

		// get victim name
		char dominatorName[MAX_NAME_LENGTH];
		GetClientName(victim, dominatorName, sizeof(dominatorName));

		// if the victim never dominated the player, this is not a revenge kill
		if (!StrEqual(dominatorName, storedDominator)) { return false; }

		// confirmed as a revenge kill - removing the record also returns true
		return this.RevengeTracker.Remove(dominatedName);
	}

	// Dying may incur a penalty (disabled by default)
	// Returns a positive value, should be inverted when called
	float DeathPenalty(int victim) {
		return this.bank.Balance(victim)
			 * g_Cvar_CurrencyDeathTax.FloatValue;
	}
}

// methodmap PaymentController < StringMap{
// 	public PaymentController() {
// 		// RevengeTracker = new StringMap();
// 		StringMap RevengeTracker = new StringMap();
// 		return view_as<PaymentController>(RevengeTracker);
// 	}

// 	public void Reset() {
// 		this.Clear();
// 	}

// 	public float ForKill(bool revenge=false) {
// 		return GetRandomFloat(
// 			g_Cvar_CurrencyOnKillMin.FloatValue,
// 			g_Cvar_CurrencyOnKillMax.FloatValue
// 		) * this.RevengeMultiplier(revenge);
// 	}

// 	public float ForDestruction(Event event) {
// 		int objectIndex = event.GetInt("index");
// 		int upgradeLevel = GetEntProp(objectIndex, Prop_Send, "m_iUpgradeLevel");

// 		return upgradeLevel * g_Cvar_CurrencyOnObjectDestroyed.FloatValue;
// 	}

// 	// Deny payment
// 	public bool Suicide(int killer, int victim) {
// 		return killer < 1 || killer == victim;
// 	}

// 	// Returns a ConVar-defined float on revenge, or 1 (which will have no effect)
// 	public float RevengeMultiplier(bool revenge=false) {
// 		return revenge ? g_Cvar_RevengeMultiplier.FloatValue : 1.0;
// 	}

// 	// Revenge kills trigger a bonus multiplier
// 	// TODO: optimize by avoiding conversion from id to name, if possible
// 	public bool RevengeKill(int killer, int victim) {
// 		// get killer name
// 		char dominatedName[MAX_NAME_LENGTH];
// 		GetClientName(killer, dominatedName, sizeof(dominatedName));

// 		// check if killer has been dominated, and if so, who the dominator was
// 		char storedDominator[MAX_NAME_LENGTH];
// 		bool revenge = this.GetString(dominatedName, storedDominator, sizeof(storedDominator));

// 		// bail early if killer hasn't actually been dominated
// 		if (!revenge) { return false; }

// 		// get victim name
// 		char dominatorName[MAX_NAME_LENGTH];
// 		GetClientName(victim, dominatorName, sizeof(dominatorName));

// 		// if the victim never dominated the player, this is not a revenge kill
// 		if (!StrEqual(dominatorName, storedDominator)) { return false; }

// 		// confirmed as a revenge kill - removing the record also returns true
// 		return this.Remove(dominatedName);
// 	}

// 	// Dying may incur a penalty (disabled by default)
// 	// Returns a positive value, should be inverted when called
// 	public float DeathPenalty(int victim) {
// 		return this.Balance(victim)
// 			 * g_Cvar_CurrencyDeathTax.FloatValue;
// 	}
// }

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

// higher level buildings yield more currency


void HookPaymentEvents() {
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
	HookEvent("object_destroyed", Event_ObjectDestroyed, EventHookMode_Post);
	HookEvent("teamplay_point_captured", Event_TeamplayPointCaptured, EventHookMode_Post);
	HookEvent("teamplay_flag_event", Event_TeamplayFlagEvent, EventHookMode_Post);
	HookEvent("player_domination", Event_PlayerDomination, EventHookMode_Post);
}

void InitPaymentConVars() {
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

// Kills earn currency
// Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
// 	int killer = GetClientOfUserId(event.GetInt("attacker"));
// 	int assister = GetClientOfUserId(event.GetInt("assister"));
// 	int victim = GetClientOfUserId(event.GetInt("userid"));

// 	// Opening the upgrades menu via chat leaves this prop with a value of 1
// 	// We need to reset it or the menu will immediately open on player spawn
// 	SetEntProp(victim, Prop_Send, "m_bInUpgradeZone", 0);

// 	if (Payment.Suicide(killer, victim)) { return Plugin_Continue; }

// 	bool revenge = Payment.RevengeKill(killer, victim)
// 				|| Payment.RevengeKill(assister, victim);

// 	float reward = Payment.ForKill(revenge);

// 	if (ValidClient(killer))   bank.Deposit(reward, killer);
// 	if (ValidClient(assister)) bank.Deposit(reward, assister);
// 	if (ValidClient(victim))   bank.Burn(Payment.DeathPenalty(victim), victim);

// 	return Plugin_Continue;
// }

// // Destroying an engineer building rewards increasing currency per building upgrade
// Action Event_ObjectDestroyed(Event event, const char[] name, bool dontBroadcast) {
// 	// Ignore sappers completely
// 	if (event.GetInt("objecttype") == view_as<int>(TFObject_Sapper)) return Plugin_Continue;

// 	// Play SFX when destroying an unfinished building
// 	// if (event.GetBool("was_building")) { ObjectDenied(event); }

// 	int attacker = GetClientOfUserId(event.GetInt("attacker"));
// 	int assister = GetClientOfUserId(event.GetInt("assister"));
// 	float reward = Payment.ForDestruction(event);

// 	if (ValidClient(attacker)) bank.Deposit(reward, attacker);
// 	if (ValidClient(assister)) bank.Deposit(reward, assister);

// 	return Plugin_Continue;
// }

// // Capturing a point earns team currency
// Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
// 	bank.DepositAll(
// 		g_Cvar_CurrencyOnCapturePoint.FloatValue,
// 		.team=view_as<TFTeam>(event.GetInt("team"))
// 	);

// 	return Plugin_Continue;
// }

// // Capturing a flag earns team currency
// Action Event_TeamplayFlagEvent(Event event, const char[] name, bool dontBroadcast) {
// 	// MAGIC NUMBER: 2 is "captured"
// 	if (event.GetInt("eventtype") != 2) { return Plugin_Continue; }

// 	// event.GetInt("team") won't work as some maps have inverted flag logic
// 	int player = event.GetInt("player");

// 	bank.DepositAll(
// 		g_Cvar_CurrencyOnCaptureFlag.FloatValue,
// 		.team=view_as<TFTeam>(GetClientTeam(player))
// 	);

// 	return Plugin_Continue;
// }

// // Dominations earn bonus currency (disabled by default) and provide data for revenge kills
// Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
// 	// Handle domination
//     int dominator = GetClientOfUserId(event.GetInt("dominator"));
// 	int dominated = GetClientOfUserId(event.GetInt("dominated"));
// 	if (ValidClient(dominator)) bank.Deposit(g_Cvar_CurrencyOnDomination.FloatValue, dominator);

// 	// Track revenge
// 	char dominatorName[MAX_NAME_LENGTH]; GetClientName(dominator, dominatorName, sizeof(dominatorName));
// 	char dominatedName[MAX_NAME_LENGTH]; GetClientName(dominated, dominatedName, sizeof(dominatedName));
// 	Payment.SetString(dominatedName, dominatorName);

// 	return Plugin_Continue;
// }
