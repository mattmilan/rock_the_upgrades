enum CONNECTION_STATUS {
    UNSET = 0,
    DISCONNECTED,
    CONNECTED
}

enum AUTH_MODE {
    AUTH_MODE_UNSET = 0,
    AUTH_MODE_STEAM_ID,
    AUTH_MODE_NAME
}

methodmap Connections __nullable__ {

    // public any Native_Bank_RegisterAccountKey(Handle plugin, int numParams) {
    public static CONNECTION_STATUS Authorize(Bank b, int client) {
        // Buffer
        char accountKey[MAX_AUTHID_LENGTH];

        // Attempt read/write SteamID from external resource
        bool authorized = GetClientAuthId(client, AuthId_SteamID64, accountKey, sizeof(accountKey));

        // Fallback to client name (discouraged as spoofable but necessary if auth servers are down)
        if (!authorized) GetClientName(client, accountKey, sizeof(accountKey));

        // Update mapping
        b.Keys.SetString(client, accountKey, MAX_AUTHID_LENGTH);

        // Advise caller if external resource was reachable
        return authorized ? CONNECTED : DISCONNECTED;
    }

    public static bool AccountKey(Bank b, int client, char accountKey[MAX_AUTHID_LENGTH]) {
        return (b.Keys.GetString(client, accountKey, MAX_AUTHID_LENGTH)) > 0;
    }

    public static CONNECTION_STATUS Connect(Bank b, int client) {
        // Maps client -> steamID (fallback: client name)
        CONNECTION_STATUS status = Connections.Authorize(b, client);

        // Track steam auth server outages and recoveries
        // StateManager.SetAuthServersReachable(b, authorized)
        Connections.SetStatus(b, client, status);

        // Create new accounts or reconnect existing ones
        Accounts.FindOrCreate(b, client);

        return status;
    }

    public static void Disconnect(Bank b, int client) {
        Connections.SetStatus(b, client, DISCONNECTED);
    }

    // SetConnection
    public static bool SetStatus(Bank b, int client, CONNECTION_STATUS status) {
        // Bail if not found
        Account a; if (!Accounts.Find(b, client, a)) return false;

        // Reset spending but retain earned currency on disconnect
        if (status == DISCONNECTED) a.Reset(a.Earned());

        // Convert enum to bool // TODO: teach account about connection status
        a.connected = status == CONNECTED ? true : false;

        // Save Changes
        return Accounts.Save(b, a);
    }

    public static void SetAuthMode(Bank b, bool authable) {
        // Servers Reactivated
        if (!b.Authable && authable) Connections.RestoreAuth(b);

        b.Authable = authable;
    }

    // During restoration, all account keys are player names - swap to SteamIDs
    public static void RestoreAuth(Bank b) {
        // Iterator buffers
        char nameKey[MAX_AUTHID_LENGTH];
        char authKey[MAX_AUTHID_LENGTH];
        Account account;

        for (int i = 0; i < b.Keys.Length; i++) {
            // Write name
            b.Keys.GetString(i, nameKey, MAX_AUTHID_LENGTH);

            // Skip missing (dont use Accounts.Find here)
            if (!b.Accounts.GetArray(nameKey, account, sizeof(Account))) continue;

            // Write authID
            GetClientAuthId(i, AuthId_SteamID64, authKey, MAX_AUTHID_LENGTH);

            // Update key map
            b.Keys.SetString(i, authKey, MAX_AUTHID_LENGTH);

            // Update account
            account.accountKey = authKey;

            // Swap index
            Accounts.Save(b, account);
            Accounts.Remove(b, nameKey);
        }
    }
}
