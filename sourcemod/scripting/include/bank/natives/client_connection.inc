#if defined _bank_client_connections_natives_included
 #endinput
#endif
#define _bank_client_connections_natives_included

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
