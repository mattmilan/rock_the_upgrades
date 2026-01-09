#if defined _player_management_natives_included
 #endinput
#endif
#define _player_management_natives_included

public any Native_Bank_Balance(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    Account a; Accounts.Find(b, client, a);// b.Fetch(client, a);

    return a.Balance();
}

public any Native_Bank_Sync(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int clientID = view_as<int>(GetNativeCell(2));

    // client validation has just happened upstream
    if (clientID > 0) {
        return b.SetClientCurrency(clientID, b.Balance(clientID));
    }

    for (int client = 1; client <= MaxClients; client++) {
        if (!ValidClient(client)) continue;
        if (!b.Sync(client)) return false;
    }

    return true;
}

public int Native_Bank_Delta(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    float current = b.GetClientCurrency(client);
    float balance = b.Balance(client);

    return RoundToCeil(balance - current);
}

public any Native_Bank_ResolveDelta(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    int delta = b.Delta(client);
    if (delta == 0) return false;

    b.Withdraw(float(delta), client);
    return true;
}

public any Native_Bank_GetClientCurrency(Handle plugin, int numParams) {
    // Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    return ValidClient(client) ?
        float(GetEntProp(client, Prop_Send, "m_nCurrency")) :
        -1.0;
}

public any Native_Bank_SetClientCurrency(Handle plugin, int numParams) {
    // Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    bool valid = ValidClient(client);
    if (valid) SetEntProp(client, Prop_Send, "m_nCurrency", RoundToCeil(amount));
    return valid;
}

public any Native_Bank_OnPlayerSpawn(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    // Apply changes and indicate if revert is needed
    bool revert = Accounts.Update(
        b,
        client,
        view_as<TFClassType>(GetNativeCell(3)),
        view_as<TFTeam>(GetNativeCell(4))
    );

    // if reverting, a sync will soon be triggered downstream
    if (!revert) b.Sync(client);

    return revert;
}

public any Native_Bank_NeedsRevert(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    Account a; Accounts.Find(b, client, a);
    return a.NeedsRevert();
}

public void Native_Bank_Revert(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    Account a; Accounts.Find(b, client, a);
    a.Revert();
    Accounts.Save(b, a);

    b.Sync(client);
}
