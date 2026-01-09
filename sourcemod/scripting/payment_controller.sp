/**
 *  Rock the Upgrades: Payment Controller
 *  Handles earning and losing currency from in-game actions
 *  TODO: details
 */

#include <sourcemod>
#include <tf2>
#include <tf2_stocks>
#include <rock_the_upgrades/shared>
#include <rock_the_upgrades/payment_controller>
#include <bank>

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

PaymentController Payment;
Bank bank;

native Bank TheBank();

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	PrepareLibrary();
	return APLRes_Success;
}

public void OnPluginStart() {
	Payment = new PaymentController();
	bank = TheBank();
    InitConVars();
    HookEvents();
}

public void OnPluginEnd() {
	Payment.Close();
}

// Kills earn currency
Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	int killer = GetClientOfUserId(event.GetInt("attacker"));
	int assister = GetClientOfUserId(event.GetInt("assister"));
	int victim = GetClientOfUserId(event.GetInt("userid"));

	// Opening the upgrades menu via chat leaves this prop with a value of 1
	// We need to reset it or the menu will immediately open on player spawn
	SetEntProp(victim, Prop_Send, "m_bInUpgradeZone", 0);

	if (Payment.Suicide(killer, victim)) { return Plugin_Continue; }

	bool revenge = Payment.RevengeKill(killer, victim)
				|| Payment.RevengeKill(assister, victim);

	float reward = Payment.ForKill(revenge);

	if (ValidClient(killer))   bank.Deposit(reward, killer);
	if (ValidClient(assister)) bank.Deposit(reward, assister);
	if (ValidClient(victim))   bank.Burn(Payment.DeathPenalty(victim), victim);

	return Plugin_Continue;
}

// Destroying an engineer building rewards increasing currency per building upgrade
Action Event_ObjectDestroyed(Event event, const char[] name, bool dontBroadcast) {
	// Ignore sappers completely
	if (event.GetInt("objecttype") == view_as<int>(TFObject_Sapper)) return Plugin_Continue;

	// Play SFX when destroying an unfinished building
	// if (event.GetBool("was_building")) { ObjectDenied(event); }

	int attacker = GetClientOfUserId(event.GetInt("attacker"));
	int assister = GetClientOfUserId(event.GetInt("assister"));
	float reward = Payment.ForDestruction(event);

	if (ValidClient(attacker)) bank.Deposit(reward, attacker);
	if (ValidClient(assister)) bank.Deposit(reward, assister);

	return Plugin_Continue;
}

// Capturing a point earns team currency
Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
	bank.DepositAll(
		Payment.ForPointCapture(),
		.team=view_as<TFTeam>(event.GetInt("team"))
	);

	return Plugin_Continue;
}

// Capturing a flag earns team currency
Action Event_TeamplayFlagEvent(Event event, const char[] name, bool dontBroadcast) {
	// MAGIC NUMBER: 2 is "captured"
	if (event.GetInt("eventtype") != 2) { return Plugin_Continue; }

	// event.GetInt("team") won't work as some maps have inverted flag logic
	int player = event.GetInt("player");

	bank.DepositAll(
		Payment.ForFlagCapture(),
		.team=view_as<TFTeam>(GetClientTeam(player))
	);

	return Plugin_Continue;
}

// Dominations earn bonus currency (disabled by default) and provide data for revenge kills
Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
	// Handle domination
    int dominator = GetClientOfUserId(event.GetInt("dominator"));
	int dominated = GetClientOfUserId(event.GetInt("dominated"));
	if (ValidClient(dominator)) bank.Deposit(Payment.ForDomination(), dominator);

	// Track revenge
	char dominatorName[MAX_NAME_LENGTH]; GetClientName(dominator, dominatorName, sizeof(dominatorName));
	char dominatedName[MAX_NAME_LENGTH]; GetClientName(dominated, dominatedName, sizeof(dominatedName));
	Payment.SetString(dominatedName, dominatorName);

	return Plugin_Continue;
}

public any Native_Payment(Handle plugin, int numParams) {
    return Payment;
}

public any Native_KillMin(Handle plugin, int numParams) {
    return g_Cvar_CurrencyOnKillMin.FloatValue;
}
public any Native_KillMax(Handle plugin, int numParams) {
	return g_Cvar_CurrencyOnKillMax.FloatValue;
}
public any Native_RevengeMultiplier(Handle plugin, int numParams) {
	return g_Cvar_RevengeMultiplier.FloatValue;
}
public any Native_Destruction(Handle plugin, int numParams) {
	return g_Cvar_CurrencyOnObjectDestroyed.FloatValue;
}
public any Native_Domination(Handle plugin, int numParams) {
	return g_Cvar_CurrencyOnDomination.FloatValue;
}
public any Native_CapturePoint(Handle plugin, int numParams) {
	return g_Cvar_CurrencyOnCapturePoint.FloatValue;
}
public any Native_CaptureFlag(Handle plugin, int numParams) {
	return g_Cvar_CurrencyOnCaptureFlag.FloatValue;
}
public any Native_DeathTax(Handle plugin, int numParams) {
	return g_Cvar_CurrencyDeathTax.FloatValue;
}


void PrepareLibrary() {
	RegPluginLibrary("payment");
    CreateNative("Payment", Native_Payment);
	CreateNative("PaymentController.KillMin", Native_KillMin);
	CreateNative("PaymentController.KillMax", Native_KillMax);
	CreateNative("PaymentController.RevengeMultiplier", Native_RevengeMultiplier);
	CreateNative("PaymentController.Destruction", Native_Destruction);
	CreateNative("PaymentController.Domination", Native_Domination);
	CreateNative("PaymentController.CapturePoint", Native_CapturePoint);
	CreateNative("PaymentController.CaptureFlag", Native_CaptureFlag);
	CreateNative("PaymentController.DeathTax", Native_DeathTax);
}

void HookEvents() {
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
	HookEvent("object_destroyed", Event_ObjectDestroyed, EventHookMode_Post);
	HookEvent("teamplay_point_captured", Event_TeamplayPointCaptured, EventHookMode_Post);
	HookEvent("teamplay_flag_event", Event_TeamplayFlagEvent, EventHookMode_Post);
	HookEvent("player_domination", Event_PlayerDomination, EventHookMode_Post);
}

void InitConVars() {
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
