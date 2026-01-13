/**
 * Bank
 *
 * Provides bank-like functionality for player currency management
 * Player Data is represented by the `Account` enum struct in account.inc
 * Accounts persist through disconnects and are cleared on map change
 * Reconnecting players will be fully refunded as upgrades were lost
 *
 * Installation (in your plugin.sp):
 *   - `#include <rock_the_upgrades/account_controller>`
 *   - `native Bank Bank();`
 *
 * Usage:
 *   - void Method() { Bank().Deposit(client, amount); Bank().PrintToServer(); }
 *   -
 */

#include <sourcemod>
#include <tf2>
#include <rock_the_upgrades/shared>
#include <bank>
#include <bank/natives>
#include <bank/create_natives>
#include <bank/printer>

static Bank bank;                 // single source of truth, provided via native
ConVar g_Cvar_CurrencyStarting;   // Currency for new accounts before bonuses
ConVar g_Cvar_CurrencyMultiplier; // Global multiplier for all currency gains
ConVar g_Cvar_CurrencyLimit;      // Optionally limit earnable currency

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
	RegPluginLibrary("bank");

    // Primary
    CreateNative("TheBank", Native_TheBank);

    // Secondary
    CreateNatives();

	return APLRes_Success;
}

public void OnPluginStart() {
    InitConvars();
    InitBank();
}

void InitConvars() {
    g_Cvar_CurrencyStarting = CreateConVar("rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);
	g_Cvar_CurrencyMultiplier = CreateConVar("rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyLimit = CreateConVar("rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}

void InitBank() {
    bank = new Bank(
        g_Cvar_CurrencyStarting.FloatValue,
        g_Cvar_CurrencyMultiplier.FloatValue,
        g_Cvar_CurrencyLimit.FloatValue
    );
}

// TODO: Naming. TheBank? CentralBank? GetBank? Shame we cant use Bank. Maybe if we rename this to `AccountManager`.
public any Native_TheBank(Handle plugin, int numParams) {
    return bank;
}
