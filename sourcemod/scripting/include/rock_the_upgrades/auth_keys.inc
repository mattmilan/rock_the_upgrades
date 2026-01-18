#if defined _auth_manager_included
    #endinput
#endif
#define _auth_manager_included

#include "shared"
#include "auth_key"


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

    public static bool Fallback() {
        return authMode == AuthModePlayerName;
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

    public static bool Get(int client, char buffer[MAX_NAME_LENGTH], AuthMode mode = AuthModeUnset) {
        if (mode == AuthModeUnset) mode = authMode;

        AuthKey key;
        if (!AuthKeys.Instance().GetArray(client, key, sizeof(AuthKey))) return false;

        key.GetString(buffer, mode);
        return true;
    }

    public static bool GetKeyData(int client, AuthKey out) {
        return AuthKeys.Instance().GetArray(client, out, sizeof(AuthKey)) > 0;
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
