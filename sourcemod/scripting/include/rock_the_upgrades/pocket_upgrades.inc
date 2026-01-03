/**
 * Rock The Upgrades: Pocket Upgrades
 * TODO: description
 */

enum struct PocketUpgrades {
    // Enable/disable feature via ConVar
    ConVar Cvar_PocketUpgrades;

    // Prevent re-initialization
    bool initialized;

    // Whether the feature is disabled via configuration
    bool disabled;

    // Handle opening per client
    // ArrayList opens;

    // Replaces locks?
    CombatTimer combatTimer;

    void Init(CombatTimer timer) {
        if (this.initialized) return;
        this.initialized = true;

        this.Cvar_PocketUpgrades = CreateConVar("rtu_pocket_upgrades", "1", "Enable or disable upgrade menu access via chat commands [1, 0,1]", 0, true, 0.0, true, 1.0);
        // this.opens = new ArrayList(sizeof(Timer_PocketUpgradesOpenMenu), MaxClients + 1);
        this.combatTimer = timer;

        this.Reset();
    }

    void Reset() {
        this.disabled = this.Cvar_PocketUpgrades.IntValue == 0;

        // this.opens.Clear();
    }

    // Open the upgrades menu. Closing the menu is out of scope and handled in-game
    // NOTE: leaves the m_bInUpgradeZone prop set to 1 - requires special handling,
    //       otherwise the menu will not reopen (except on every respawn)
    void Show(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        if (!this.Showable(client, accountKey)) return;

        // m_bWasInZone may need to be reset, this is the first step towards that
        SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);

        // using a timer so that m_bWasInZone has time to update
        // this.opens.Set(client, CreateTimer(0.1, Timer_PocketUpgradesOpenMenu, client));
        CreateTimer(0.1, Timer_PocketUpgradesOpenMenu, client)
    }

    bool Disabled(int client) {
        if (this.disabled) ReplyToCommand(client, "[RTU] Pocket Menu disabled by server configuration");

        return this.disabled;
    }

    bool Locked(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        int time;

        this.combatTimer.Get(accountKey, time);

        if (time <= 0) return false;

        ReplyToCommand(client, "[RTU] Pocket Menu is Locked! (%d seconds remaining)", time);

        return true;
    }

    bool Opening(int client) {
        return false;

        // if (this.opens.Length == 0) return false;

        // bool opening = this.opens.Get(client) != INVALID_HANDLE;

        // if (opening) ReplyToCommand(client, "[RTU] Pocket Menu is opening");

        // return opening;
    }

    bool Showable(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        return ValidClient(client)
            && !this.Disabled(client)
            && !this.Locked(client, accountKey)
            && !this.Opening(client);
    }
}

// Timers must be static and cannot live within enum structs
Action Timer_PocketUpgradesOpenMenu(Handle timer, any client) {
	if (ValidClient(client)) SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 1);

	return Plugin_Handled;
}
