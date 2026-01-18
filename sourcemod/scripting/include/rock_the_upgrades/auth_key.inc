enum AuthMode {
    AuthModeUnset = 0,
    AuthModeSteamId,
    AuthModePlayerName
}

enum struct AuthKey {
    int client;
    char steamID[MAX_NAME_LENGTH];
    char playerName[MAX_NAME_LENGTH];
    AuthMode mode;

    // Derives values from client and returns auth mode
    AuthMode Init(int client) {
        this.client = client;
        GetClientName(client, this.playerName, MAX_NAME_LENGTH);
        return this.Authorize();
    }

    AuthMode Authorize() {
        bool authorized = GetClientAuthId(this.client, AuthId_SteamID64, this.steamID, MAX_NAME_LENGTH);
        this.mode = authorized ? AuthModeSteamId : AuthModePlayerName;
        return this.mode;
    }

    void GetString(char key[MAX_NAME_LENGTH], AuthMode mode) {
        switch (mode) {
            case AuthModeSteamId:
                strcopy(key, MAX_NAME_LENGTH, this.steamID);
            case AuthModePlayerName:
                strcopy(key, MAX_NAME_LENGTH, this.playerName);
            default:
                strcopy(key, MAX_NAME_LENGTH, "");
        }
    }
}
