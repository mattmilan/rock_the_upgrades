#if defined _bank_account_management_natives_included
 #endinput
#endif
#define _bank_account_management_natives_included

public void Native_Bank_FindOrCreateAccount(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    char accountKey[MAX_AUTHID_LENGTH];
    b.GetAccountKey(client, accountKey);

    if (b.ContainsKey(accountKey)) b.SetConnection(client, true);
    else b.CreateAccount(client, accountKey);
}

public any Native_Bank_GetAccountKey(Handle plugin, int numParams) {
    // Cells
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    // Buffer
    char accountKey[MAX_AUTHID_LENGTH];

    // Write buffer
    bool exists = (b.Keys.GetString(client, accountKey, MAX_AUTHID_LENGTH)) > 0;

    // Write buffer to param
    if (exists) SetNativeString(3, accountKey, MAX_AUTHID_LENGTH);

    return exists;
}

public void Native_Bank_CreateAccount(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    // Account Identity
    char accountKey[MAX_AUTHID_LENGTH];
    int client = view_as<int>(GetNativeCell(2));
    GetNativeString(3, accountKey, MAX_AUTHID_LENGTH);

    // Buffer
    Account account;

    // Establish account
    account.Init(client, accountKey);

    // Save
    b.Data.SetArray(accountKey, account, sizeof(Account));
}

public any Native_Bank_UpdateAccount(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    // Find account
    int client = view_as<int>(GetNativeCell(2));
    Account account;
    b.Fetch(client, account);

    // apply updates
    account.currentClass = view_as<TFClassType>(GetNativeCell(3));
    account.currentTeam = view_as<TFTeam>(GetNativeCell(4));

    // grant one-time starting currency
    // TODO: This is an initializer, not an update - move upstream?
    if (account.earned < 0.001) account.earned = b.StartingCurrency(account.currentTeam);

    // Save
    b.Data.SetArray(account.accountKey, account, sizeof(Account));

    // Indicate if this account should revert on next spawn
    return account.NeedsRevert();
}

public any Native_Bank_Fetch(Handle plugin, int numParams) {
    // Cells
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    Account account; /*=*/ GetNativeArray(3, account, sizeof(Account));

    // Buffer
    char keyName[MAX_AUTHID_LENGTH];

    // Write SteamID (or player name) to buffer
    if (!b.GetAccountKey(client, keyName))
        GetClientName(client, keyName, sizeof(keyName));

    // Read existing account into buffer
    bool found = b.Data.GetArray(keyName, account, sizeof(Account));

    // Write Account to param buffer
    if (found) SetNativeArray(3, account, sizeof(Account));

    // Report results
    return found;
}
