// Looks like ordering might be important
#include <sourcemod>
#include <tf2>
#include <tf2_stocks>
#include <rock_the_upgrades/shared>
#include <rock_the_upgrades/account_controller>

AccountController Bank;

ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
ConVar g_Cvar_CurrencyLimit;

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	RegPluginLibrary("account_controller");
    CreateNative("Bank", Native_Bank);
    CreateNative("AccountController.BaseStartingCurrency", Native_BaseStartingCurrency);
    CreateNative("AccountController.CurrencyMultiplier", Native_CurrencyMultiplier);
    CreateNative("AccountController.CurrencyLimit", Native_CurrencyLimit);
	return APLRes_Success;
}

public void OnPluginStart() {
    PrintToServer("[RTU][AccountController] (OnPluginStart)");
    Bank = new AccountController();

    // Starting currency
    g_Cvar_CurrencyStarting = CreateConVar("rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);

	// Global multiplier for all currency events
	g_Cvar_CurrencyMultiplier = CreateConVar("rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);

	// Limit max currency gain to encourage strategic spending
	g_Cvar_CurrencyLimit = CreateConVar("rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}

public any Native_Bank(Handle plugin, int numParams) {
    PrintToServer("[RTU](Native_Bank)");
    return Bank;
}

public any Native_BaseStartingCurrency(Handle plugin, int numParams) {
    return g_Cvar_CurrencyStarting.FloatValue
        * g_Cvar_CurrencyMultiplier.FloatValue;
}

public any Native_CurrencyMultiplier(Handle plugin, int numParams) {
    return g_Cvar_CurrencyMultiplier.FloatValue;
}

public any Native_CurrencyLimit(Handle plugin, int numParams) {
    return g_Cvar_CurrencyLimit.FloatValue;
}
