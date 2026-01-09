
methodmap Accounts __nullable__ {
    public static bool FindOrCreate(Bank b, int client) {
        Account existing;

        return Accounts.Find(b, client, existing) ?
            Accounts.Connect(b, client) :
            Accounts.Create(b, client);
    }

    public static bool Find(Bank b, int client, Account out) {
        // Lookup Client's Account Key
        char accountKey[MAX_AUTHID_LENGTH];

        // Primary key: SteamID
        bool found = Connections.AccountKey(b, client, accountKey);

        // Backup key: Player Name
        if (!found) GetClientName(client, accountKey, MAX_AUTHID_LENGTH);

        // Write to buffer and report result
        return b.Accounts.GetArray(accountKey, out, sizeof(Account));
    }

    public static bool Create(Bank b, int client, char accountKey[MAX_AUTHID_LENGTH] = "") {
        // Default value performs a lookup - value should have been set upstream
        if (accountKey[0] == '\0') Connections.AccountKey(b, client, accountKey);

        // Build
        Account account; account.Init(client, accountKey);

        // Save
        Accounts.Save(b, account);
    }

    public static bool Update(Bank b, int client, TFClassType classType, TFTeam team, int connected=-1) {
        // Find
        Account account; Accounts.Find(b, client, account);
        // Update
        account.Update(classType, team, b.StartingCurrency(team), connected);
        // Save
        Accounts.Save(b, account);

        // Indicate if this account should revert on next spawn
        // TODO: is this a bad position for this?
        return account.NeedsRevert();
    }

    public static bool Remove(Bank b, char key[MAX_AUTHID_LENGTH]) {
        return b.Accounts.Remove(key);
    }

    public static bool Exists(Bank b, char key[MAX_AUTHID_LENGTH]) {
        return b.Accounts.HasKey(key);
    }

    public static bool Connect(Bank b, int client) {
        return Connections.SetStatus(b, client, CONNECTED);
    }

    public static bool Save(Bank b, Account account) {
        return b.Accounts.SetArray(account.accountKey, account, sizeof(Account));
    }
}
