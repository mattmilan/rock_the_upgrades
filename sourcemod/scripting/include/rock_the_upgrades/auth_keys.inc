#if defined _auth_manager_included
    #endinput
#endif
#define _auth_manager_included

#include "shared"

enum AuthMode {
    AuthModeUnset = 0,
    AuthModeSteamId,
    AuthModePlayerName
}

static AuthKeys authKeys;
static AuthMode authMode;

methodmap AuthKeys < ArrayList {
    public static AuthKeys Instance() {
        if (authKeys == null) AuthKeys.Init();

        return authKeys;
    }

    public static void Init() {
        ArrayList list = new ArrayList(sizeof(AuthKey), MaxClients + 1)
        authKeys = view_as<AuthKeys>(list);
    }

    public static AuthMode Mode() {
        return authMode;
    }

    // OnClientConnect, Reauthorize
    public static void Register(int client) {
        AuthKey authKey;
        AuthKeys.Instance().GetArray(client, authKey, sizeof(AuthKey));

        switch (authKey.mode) {
            // New -> Initialize
            case AuthModeUnset: authKey.Init(client);
            // Existing -> Need convert to SteamID
            case AuthModePlayerName: authKey.Authorize();
            // Existing -> No change needed
            case AuthModeSteamId: return;
        }

        // Save
        AuthKeys.Instance().SetArray(client, authKey, sizeof(AuthKey));

        // Track authmode globally
        AuthKeys.Instance().SetAuthMode(authKey.mode);
    }

    public static bool Get(int client, char buffer[MAX_AUTHID_LENGTH], AuthMode mode = AuthModeUnset) {
        if (mode == AuthModeUnset) mode = authMode;

        AuthKey key;
        if (!AuthKeys.Instance().GetArray(client, key, sizeof(AuthKey))) return false;

        key.GetString(buffer, mode);
        return true;
    }

    public void SetAuthMode(AuthMode mode) {
        if (this.AuthRestored(mode)) this.Reauthorize();

        authMode = mode;
    }

    public bool AuthRestored(AuthMode mode) {
        return (authMode != AuthModeSteamId && mode == AuthModeSteamId);
    }

    public void Reauthorize() {
        for (int i = 0; i < this.Length; i++) {
            if (!ValidClient(i)) continue;

            AuthKeys.Register(i);
        }
    }
}

// The only thing the world cares about is the GetString method
// Access to that value is provided by AuthKeys.Get()
// The rest of this struct is to support internal management
enum struct AuthKey {
    int client;
    char steamID[MAX_AUTHID_LENGTH];
    char playerName[MAX_AUTHID_LENGTH];
    AuthMode mode;

    // Derives values from client and returns auth mode
    AuthMode Init(int client) {
        this.client = client;
        GetClientName(client, this.playerName, MAX_AUTHID_LENGTH);
        return this.Authorize();
    }

    AuthMode Authorize() {
        bool authorized = GetClientAuthId(this.client, AuthId_SteamID64, this.steamID, MAX_AUTHID_LENGTH);
        this.mode = authorized ? AuthModeSteamId : AuthModePlayerName;
        return this.mode;
    }


    void GetString(char key[MAX_AUTHID_LENGTH], AuthMode mode) {
        switch (mode) {
            case AuthModeSteamId:
                strcopy(key, MAX_AUTHID_LENGTH, this.steamID);
            case AuthModePlayerName:
                strcopy(key, MAX_AUTHID_LENGTH, this.playerName);
            default:
                strcopy(key, MAX_AUTHID_LENGTH, "");
        }
    }
}
