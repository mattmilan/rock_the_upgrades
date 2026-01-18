#include <tf2>
#include <json/helpers/metastringmap>
#include "player_profile.inc"
#include "auth_keys"
#include "bank"
#include "upgrades"

static PlayerProfiles g_PlayerProfiles;

methodmap PlayerProfiles < MetaStringMap {
    public static PlayerProfiles Instance() {
        if (g_PlayerProfiles == null) PlayerProfiles.Init();

        return g_PlayerProfiles;
    }

    public static void Init() {
        if (g_PlayerProfiles != null) return;

        g_PlayerProfiles = view_as<PlayerProfiles>(new MetaStringMap());
    }

    public static bool Connect(int client) {
        return PlayerProfiles.FindOrCreate(client);
    }

    public static bool Disconnect(int client) {
        ProfileParams params;
        params.connected = 0; // (false)
        PlayerProfiles.Update(client, params);
    }

    public static bool FindOrCreate(int client) {
        PlayerProfile existing;

        return PlayerProfiles.Find(client, existing) ?
            PlayerProfiles.Instance().Reconnect(client) :
            PlayerProfiles.Create(client);
    }

    public static bool Find(int client, PlayerProfile out) {
        // Success - Found by Auth ID
        if (PlayerProfiles.FindByAuthId(client, out)) return true;

        // Failure - Not found by fallback (player name)
        if (!PlayerProfiles.FindByPlayerName(client, out)) return false;

        // Success - Found by fallback during fallback mode
        if (AuthKeys.Fallback()) return true;

        // Success - Fallback mode was reverted, fix outdated key
        return PlayerProfiles.UpdateKey(client, out);
    }

    public static bool FindByAuthId(int client, PlayerProfile out) {
        char key[MAX_NAME_LENGTH];
        AuthKeys.Get(client, key);

        // Write to buffer and report result
        bool found = PlayerProfiles.Instance().Data.GetArray(key, out, sizeof(PlayerProfile));

        return found;
    }

    public static bool FindByPlayerName(int client, PlayerProfile out) {
        char key[MAX_NAME_LENGTH];
        AuthKeys.Get(client, key, AuthModePlayerName);

        // Write to buffer and report result
        bool found = PlayerProfiles.Instance().Data.GetArray(key, out, sizeof(PlayerProfile));

        return found;
    }

    public static bool FindByKey(char key[MAX_NAME_LENGTH], PlayerProfile out) {
        return PlayerProfiles.
                   Instance().
                   Data.
                   GetArray(key, out, sizeof(PlayerProfile));
    }

    public static bool Create(int client) {
        char key[MAX_NAME_LENGTH];

        AuthKeys.Get(client, key);

        // Build
        PlayerProfile p; p.Init(client, key);

        // Save
        PlayerProfiles.Save(p);
    }

    public static bool UpdateKey(int client, PlayerProfile out) {
        AuthKey key;
        AuthKeys.GetKeyData(client, key);

        PlayerProfiles.FindByPlayerName(client, out);

        // Remove old entry
        PlayerProfiles.Instance().Data.Remove(out.key);

        // Update key
        AuthKeys.Get(key.client, out.key);

        // Save under new key
        PlayerProfiles.Save(out);

        return true;
    }

    public bool Reconnect(int client) {
        ProfileParams params;
        params.connected = 1; // (true)
        PlayerProfiles.Update(client, params);
    }

	// public bool OnPlayerSpawn(int client, TFClassType classType, TFTeam team) {
    public static bool OnPlayerSpawn(Event event) {
        // int client = GetClientOfUserId(event.GetInt("userid"));
        ProfileParams params;
        params.FromSpawnEvent(event);

        // if player isn't in game.
        if (!params.Valid()) return false;

        return PlayerProfiles.Update(params.client, params);
	}

    public static bool Update(int client, ProfileParams params) {
        PlayerProfile p;

        // Find
        if (!PlayerProfiles.Find(client, p)) return false;

        // Update
        params.Apply(p);

        // Save
        if (!PlayerProfiles.Save(p)) return false;



        // Indicate if this p should revert on next spawn
        return
    }

    public static bool Save(PlayerProfile p, char key[MAX_NAME_LENGTH]="") {
        if (key[0] == '\0') AuthKeys.Get(p.client, key);

        PlayerProfiles.Instance().SetArray(key, p, sizeof(PlayerProfile));

        if (p.Revert()) CreateTimer(0.1, Timer_RevertPlayer, p.client);
        else p.SyncEntity(); //PlayerProfiles.Sync(p);//Bank.Instance().Sync(p.client);
    }

    // Recursive
    public static void Sync(int client = -1) {
        // Buffer
        PlayerProfile p;

        // One or Many
        bool all = client < 0;
        int start  = all ? 1 : client;
        int finish = all ? MaxClients : client;

        // Iterate
        for (int i = start; i <= finish; i++) {
            // Bail if netprop not available
			if (!ValidClient(i)) continue;

            // Find and Update
            PlayerProfiles.Find(i, p);
            p.SyncEntity();
		}
    }

    public static void SyncAll() {
        for (int client = 1; client <= MaxClients; client++) {
            if (!ValidClient(client)) continue;

            PlayerProfiles.Sync(client);
        }
    }

	public static void ResetAll() {
		// Iteration vars
		StringMapSnapshot keys = PlayerProfiles.Instance().Snapshot();
		char key[MAX_NAME_LENGTH];
		PlayerProfile p;

		for (int i = 0; i < keys.Length; i++) {
			// Write key
			keys.GetKey(i, key, sizeof(key));

			// Lookup p
			bool found = PlayerProfiles.Instance().GetArray(key, p, sizeof(PlayerProfile));
            if (!found) continue;

			// Reset p
			p.Reset(Bank.Instance().StartingCurrency(p.currentTeam));

			// Save changes
			PlayerProfiles.Save(p);

            // Only sync connected clients (entity must exist)
            if (!p.connected) continue;

			p.SyncEntity();
		}
	}
}

enum struct ProfileParams {
    int client;
    int connected;
    TFTeam team;
    TFClassType classType;

   void FromSpawnEvent(Event event) {
        this.client    = GetClientOfUserId(event.GetInt("userid"));
        this.classType = view_as<TFClassType>(event.GetInt("class"));
        this.team      = view_as<TFTeam>(event.GetInt("team"));
        this.connected = 1; // (true)
    }

    void Apply(PlayerProfile p) {
        if (this.ValidClass())     p.currentClass = this.classType;
        if (this.ValidTeam())      p.currentTeam  = this.team;
        if (this.ValidConnected()) p.connected    = (this.connected > 0);

        p.Activate();
    }

    bool Valid() {
        return ValidPlayer(this.client, this.classType, this.team);
    }

    bool ValidClass() {
        return this.classType >= TFClass_Unknown
            && this.classType <= TFClass_Engineer;
    }

    bool ValidTeam() {
        return this.team >= TFTeam_Unassigned
            && this.team <= TFTeam_Red;
    }

    bool ValidConnected() {
        return this.connected == 0
            || this.connected == 1;
    }
}

// Must be called after a 100ms delay to give game state time to settle
Action Timer_RevertPlayer(Handle timer, any client) {
	Upgrades.ResetPlayer(client);
	Bank.Instance().Revert(client);

	return Plugin_Stop;
}