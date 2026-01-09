#if defined _bank_state_management_natives_included
 #endinput
#endif
#define _bank_state_management_natives_included

/* STATE MANAGEMENT */
public void Native_Bank_SetAuthServersReachable(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    bool reachable = view_as<bool>(GetNativeCell(2));

    // Servers Reactivated
    if (reachable && !b.Authable) {
        // Iterator buffer
        char nameKey[MAX_AUTHID_LENGTH];
        char authKey[MAX_AUTHID_LENGTH];

        Account account;

        for (int key = 0; key < b.Keys.Length; key++) {
            // Write name to first buffer
            b.Keys.GetString(key, nameKey, sizeof(nameKey));

            // Presence
            if (!b.GetArray(nameKey, account, sizeof(Account)))
                continue;

            // Write AuthID to second buffer
            GetClientAuthId(key, AuthId_SteamID64, authKey, sizeof(authKey));

            // Update accountKey from name to auth
            b.Keys.SetString(key, authKey, sizeof(authKey));

            // Update account index from name for auth
            b.SetArray(authKey, account, sizeof(Account));
            b.Remove(nameKey);
        }
    }

    b.Authable = reachable;
}

public any Native_Bank_StartingCurrency(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    TFTeam team = view_as<TFTeam>(GetNativeCell(2));

    return b.Starting + b.Bonus(team);
}

public any Native_Bank_LimitCurrency(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    float amount = view_as<float>(GetNativeCell(2));

    // if no limit or under limit, amount is valid
    if (b.Limit < 0 || amount < b.Limit) return amount;

    return b.Limit;
}

public any Native_Bank_Bonus(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    TFTeam team = view_as<TFTeam>(GetNativeCell(2));

    if (team == TFTeam_Spectator) return 0.0;

    // Return val
    float result;

    // Iteration Buffer
    TeamBonus bonus;

    // Iterate bonuses
    for (int i = 0; i < b.Bonuses.Length; i++) {
        // Write buffer
        b.Bonuses.GetArray(i, bonus);

        // Filter by team and sum
        if (bonus.AppliesTo(team)) result += bonus.amount;
    }

    return result;
}

public void Native_Bank_AddBonus(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    TeamBonus bonus;

    bonus.Set(
        view_as<float>(GetNativeCell(2)),
        view_as<TFTeam>(GetNativeCell(3))
    );

    b.Bonuses.PushArray(bonus);
}

public void Native_Bank_Wipe(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    // wipe accounts
    b.Clear();

    // wipe acquisitions
    b.Bonuses.Clear();
    b.Keys.Clear();
}

public void Native_Bank_ResetAccounts(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    b.Bonuses.Clear();

    // Iteration vars
    StringMapSnapshot keys = b.Snapshot();
    char accountKey[MAX_AUTHID_LENGTH];
    Account account;

    // Iterate Accounts
    for(int i = 0; i < keys.Length; i++) {
        // Store Account Key (SteamID|Name)
        keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

        // Fetch account
        b.Data.GetArray(accountKey, account, sizeof(Account));

        // Perform reset
        account.Reset(b.StartingCurrency(account.currentTeam));

        // Save Changes
        b.Data.SetArray(accountKey, account, sizeof(Account));

        // Sync connected clients
        if (account.connected) b.Sync(account.client);
    }
}
