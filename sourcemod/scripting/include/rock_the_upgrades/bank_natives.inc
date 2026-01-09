#if defined _bank_natives_included
 #endinput
#endif
#define _bank_natives_included

void CreateNatives() {
    /* PRIMARY ACCESSOR */
    CreateNative("TheBank", Native_TheBank);

    /* PROPERTIES */
    CreateNative("Bank.Authable.get", Native_Bank_GetAuthable); CreateNative("Bank.Authable.set", Native_Bank_SetAuthable);
    CreateNative("Bank.Multiplier.get", Native_Bank_GetMultiplier); CreateNative("Bank.Multiplier.set", Native_Bank_SetMultiplier);
    CreateNative("Bank.Limit.get", Native_Bank_GetLimit); CreateNative("Bank.Limit.set", Native_Bank_SetLimit);
    CreateNative("Bank.Starting.get", Native_Bank_GetStarting); CreateNative("Bank.Starting.set", Native_Bank_SetStarting);
    CreateNative("Bank.Keys.get", Native_Bank_GetKeys); CreateNative("Bank.Keys.set", Native_Bank_SetKeys);
    CreateNative("Bank.Bonuses.get", Native_Bank_GetBonuses); CreateNative("Bank.Bonuses.set", Native_Bank_SetBonuses);

    /* STATE MANAGEMENT */
    CreateNative("Bank.SetAuthServersReachable", Native_Bank_SetAuthServersReachable);
    CreateNative("Bank.StartingCurrency", Native_Bank_StartingCurrency);
    CreateNative("Bank.LimitCurrency", Native_Bank_LimitCurrency);
    CreateNative("Bank.Bonus", Native_Bank_Bonus);
    CreateNative("Bank.AddBonus", Native_Bank_AddBonus);
    CreateNative("Bank.Wipe", Native_Bank_Wipe);
    CreateNative("Bank.ResetAccounts", Native_Bank_ResetAccounts);

    /* CLIENT CONNECTION */
    CreateNative("Bank.Connect", Native_Bank_Connect);
    CreateNative("Bank.Disconnect", Native_Bank_Disconnect);
    CreateNative("Bank.SetConnection", Native_Bank_SetConnection);
    CreateNative("Bank.RegisterAccountKey", Native_Bank_RegisterAccountKey);

    /* ACCOUNT MANAGEMENT */
    CreateNative("Bank.FindOrCreateAccount", Native_Bank_FindOrCreateAccount);
    CreateNative("Bank.GetAccountKey", Native_Bank_GetAccountKey);
    CreateNative("Bank.CreateAccount", Native_Bank_CreateAccount);
    CreateNative("Bank.UpdateAccount", Native_Bank_UpdateAccount);
    CreateNative("Bank.Fetch", Native_Bank_Fetch);

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

/* PROPERTIES */

public any Native_Bank_GetAuthable(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("authable", out); return out;
}

public any Native_Bank_GetMultiplier(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("multiplier", out); return out;
}

public any Native_Bank_GetLimit(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("limit", out); return out;
}

public any Native_Bank_GetStarting(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("starting", out); return out;
}

public any Native_Bank_GetKeys(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	return view_as<ArrayList>(b.Meta.GetHandle("keys"));
}

public any Native_Bank_GetBonuses(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	return view_as<ArrayList>(b.Meta.GetHandle("bonuses"));
}

// Prop Set
public void Native_Bank_SetAuthable(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    bool val = view_as<bool>(GetNativeCell(2));
	b.Meta.SetValue("authable", val);
}

public void Native_Bank_SetMultiplier(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("multiplier", val);
}

public void Native_Bank_SetLimit(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("limit", val);
}

public void Native_Bank_SetStarting(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("starting", val);
}

public void Native_Bank_SetKeys(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    ArrayList val = view_as<ArrayList>(GetNativeCell(2));
	b.Meta.SetValue("keys", val);
}

public void Native_Bank_SetBonuses(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    ArrayList val = view_as<ArrayList>(GetNativeCell(2));
	b.Meta.SetValue("bonuses", val);
}

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

/* CLIENT CONNECTION */
public any Native_Bank_Connect(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

	// Maps client -> steamID (fallback: client name)
		bool authorized = b.RegisterAccountKey(client);

		// Track steam auth server outages and recoveries
		b.SetAuthServersReachable(authorized);

		// Create new accounts or reconnect existing ones
		b.FindOrCreateAccount(client);

		return authorized;
}

public void Native_Bank_Disconnect(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    b.SetConnection(client, false);
}

public void Native_Bank_SetConnection(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    bool connected = view_as<bool>(GetNativeCell(3));

    Account a; /*=*/ b.Fetch(client, a);
    a.connected = connected;

    // Reset spending but retain earned currency on disconnect
    if (!connected) a.Reset(a.Earned());

    // Save Changes
    b.Data.SetArray(a.accountKey, a, sizeof(Account));
}

public any Native_Bank_RegisterAccountKey(Handle plugin, int numParams) {
    // Cells
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    // Buffer
    char accountKey[MAX_AUTHID_LENGTH];

    // Attempt read/write SteamID from external resource
    bool authorized = GetClientAuthId(client, AuthId_SteamID64, accountKey, sizeof(accountKey));

    // Fallback to client name (discouraged as spoofable but necessary if auth servers are down)
    if (!authorized) GetClientName(client, accountKey, sizeof(accountKey));

    // Update mapping
    b.Keys.SetString(client, accountKey, MAX_AUTHID_LENGTH);

    // Advise caller if external resource was reachable
    return authorized;
}

/* ACCOUNT MANAGEMENT */

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


    // Primary Lookup: SteamID
    b.GetAccountKey(client, keyName);

    // Fallback: Client Name (discouraged as spoofable but necessary if auth servers are down)
    if (keyName[0] == '\0') GetClientName(client, keyName, sizeof(keyName));

    // Attempt to write account to param buffer and inform caller of success
    return b.Data.GetArray(keyName, account, sizeof(Account));
}

/* PLAYER MANAGEMENT */

public any Native_Bank_Balance(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    Account a; /*=*/ b.Fetch(client, a);

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
    bool revert = b.UpdateAccount(
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

    Account a; /*=*/ b.Fetch(client, a);
    return a.NeedsRevert();
}

public void Native_Bank_Revert(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));

    Account a;
    b.Fetch(client, a);
    a.Revert();
    b.Data.SetArray(a.accountKey, a, sizeof(Account));
    b.Sync(client);
}

/* TRANSACTIONS */

public void Native_Bank_Transaction(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));
    BANK_TRANSACTION transaction = view_as<BANK_TRANSACTION>(GetNativeCell(4));

    // Fetch account
    Account a; b.Fetch(client, a);

    // Adjust all gains by global multiplier
    amount *= b.Multiplier;

    // Apply amount to the correct field
    switch (transaction) {
        // Currency gained from events or chat commands
        case BANK_DEPOSIT:
            a.earned += amount;
        // Purchasing or refunding upgrades
        case BANK_WITHDRAW:
            a.classData[a.currentClass].spent += amount;
        // Purchasing non-refundables
        case BANK_BURN:
            a.classData[a.currentClass].burnt += amount;
    }

    // enforce currency limit (disabled by default)
    a.earned = b.LimitCurrency(a.earned);

    // Save changes before syncing
    // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
    b.Data.SetArray(a.accountKey, a, sizeof(Account));

    // Match the client's currency to account balance
    b.Sync(client);
}

public void Native_Bank_DepositTarget(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    // Target info
    char target[MAX_NAME_LENGTH]; /*=*/ GetNativeString(2, target, MAX_NAME_LENGTH);
    float amount = view_as<float>(GetNativeCell(3));
    int replyTo = view_as<int>(GetNativeCell(4));

    if (StrEqual(target, "all", false)) {
        b.DepositAll(amount);
    } else if (StrEqual(target, "red", false)) {
        b.DepositAll(amount, .team=TFTeam_Red);
    } else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
        b.DepositAll(amount, .team=TFTeam_Blue);
    } else {
        int targetClientId = FindTarget(replyTo, target, true);

        if (ValidClient(targetClientId)) b.Deposit(amount, targetClientId);
        else ReplyToCommand(replyTo, "[RTU] Could not find player %s", target);
    }
}

public void Native_Bank_DepositAll(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    float amount = view_as<float>(GetNativeCell(2));
    TFTeam team = view_as<TFTeam>(GetNativeCell(3));

    // Increase bonus starting currency for late joiners
    b.AddBonus(amount, team);

    // Iteration vars
    Account a;
    char accountKey[MAX_AUTHID_LENGTH];

    // Iterate by key
    StringMapSnapshot keys = b.Snapshot();

    // for all accounts
    for(int i = 0; i < keys.Length; i++) {
        // Grab Account Key (SteamID|Name)
        keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

        // Fetch account
        b.Data.GetArray(accountKey, a, sizeof(Account));

        // Get paid if team matches optional filter
        if (FilterTeam(a.currentTeam, team)) b.Deposit(amount, a.client);
    }
}

public void Native_Bank_Deposit(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_DEPOSIT);
}

public void Native_Bank_Withdraw(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_WITHDRAW);
}

public void Native_Bank_Burn(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_BURN);
}

public void Native_Bank_Refund(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Withdraw(-amount, client);
}

public void Native_Bank_Forgive(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Burn(-amount, client);
}
