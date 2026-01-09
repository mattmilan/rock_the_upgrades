#if defined _bank_create_natives_included
 #endinput
#endif
#define _bank_create_natives_included

void CreateNatives() {
    /* PROPERTIES */
    CreateNative("Bank.Authable.get", Native_Bank_GetAuthable); CreateNative("Bank.Authable.set", Native_Bank_SetAuthable);
    CreateNative("Bank.Multiplier.get", Native_Bank_GetMultiplier); CreateNative("Bank.Multiplier.set", Native_Bank_SetMultiplier);
    CreateNative("Bank.Limit.get", Native_Bank_GetLimit); CreateNative("Bank.Limit.set", Native_Bank_SetLimit);
    CreateNative("Bank.Starting.get", Native_Bank_GetStarting); CreateNative("Bank.Starting.set", Native_Bank_SetStarting);
    CreateNative("Bank.Keys.get", Native_Bank_GetKeys); CreateNative("Bank.Keys.set", Native_Bank_SetKeys);
    CreateNative("Bank.Bonuses.get", Native_Bank_GetBonuses); CreateNative("Bank.Bonuses.set", Native_Bank_SetBonuses);

    /* STATE MANAGEMENT */

    CreateNative("Bank.StartingCurrency", Native_Bank_StartingCurrency);
    CreateNative("Bank.LimitCurrency", Native_Bank_LimitCurrency);
    CreateNative("Bank.Bonus", Native_Bank_Bonus);
    CreateNative("Bank.AddBonus", Native_Bank_AddBonus);
    CreateNative("Bank.Wipe", Native_Bank_Wipe);
    CreateNative("Bank.ResetAccounts", Native_Bank_ResetAccounts);

    /* CLIENT CONNECTION */
    // CreateNative("Bank.Connect", Native_Bank_Connect);
    // CreateNative("Bank.Disconnect", Native_Bank_Disconnect);
    // CreateNative("Bank.SetConnection", Native_Bank_SetConnection);
    // CreateNative("Bank.RegisterAccountKey", Native_Bank_RegisterAccountKey);
    // CreateNative("Bank.GetAccountKey", Native_Bank_GetAccountKey);
    // CreateNative("Bank.SetAuthServersReachable", Native_Bank_SetAuthServersReachable);

    /* ACCOUNT MANAGEMENT */
    // CreateNative("Bank.FindOrCreateAccount", Native_Bank_FindOrCreateAccount);
    // CreateNative("Bank.CreateAccount", Native_Bank_CreateAccount);
    // CreateNative("Bank.UpdateAccount", Native_Bank_UpdateAccount);
    // CreateNative("Bank.Fetch", Native_Bank_Fetch);

    /* PLAYER MANAGEMENT */
    CreateNative("Bank.Balance", Native_Bank_Balance);
    CreateNative("Bank.Sync", Native_Bank_Sync);
    CreateNative("Bank.Delta", Native_Bank_Delta);
    CreateNative("Bank.ResolveDelta", Native_Bank_ResolveDelta);
    CreateNative("Bank.GetClientCurrency", Native_Bank_GetClientCurrency);
    CreateNative("Bank.SetClientCurrency", Native_Bank_SetClientCurrency);
    CreateNative("Bank.OnPlayerSpawn", Native_Bank_OnPlayerSpawn);
    CreateNative("Bank.NeedsRevert", Native_Bank_NeedsRevert);
    CreateNative("Bank.Revert", Native_Bank_Revert);

    /* TRANSACTIONS */
    CreateNative("Bank.Transaction", Native_Bank_Transaction);
    CreateNative("Bank.DepositTarget", Native_Bank_DepositTarget);
    CreateNative("Bank.DepositAll", Native_Bank_DepositAll);
    CreateNative("Bank.Deposit", Native_Bank_Deposit);
    CreateNative("Bank.Withdraw", Native_Bank_Withdraw);
    CreateNative("Bank.Burn", Native_Bank_Burn);
    CreateNative("Bank.Refund", Native_Bank_Refund);
    CreateNative("Bank.Forgive", Native_Bank_Forgive);
}
