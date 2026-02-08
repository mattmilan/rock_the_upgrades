/**
 * Rock The Upgrades: Pocket Upgrades
 * TODO: description
 */

// applies to both global lock message and client-specific locks
// TODO: CReplyToCommand, RTU_BRAND and translations

enum struct PocketUpgrades {

    // Enable/disable feature via ConVar
    ConVar Cvar_PocketUpgrades;

    // Prevent re-initialization
    bool initialized;

    // Whether the feature is disabled via configuration
    bool disabled;

    // Whether the feature is temporarily locked for all
    // MAGIC NUMBER 32: arbitrary buffer for lock message
    char lockMessage[32];

    // Whether the feature is locked for a specific client
    CombatTimer combatTimer;

    void Init(CombatTimer timer) {
        if (this.initialized) return;

        this.initialized = true;
        this.Cvar_PocketUpgrades = CreateConVar("rtu_pocket_upgrades", "1", "Enable or disable upgrade menu access via chat commands [1, 0,1]", 0, true, 0.0, true, 1.0);
        this.combatTimer = timer;

        this.Reset();
    }

    // Double check if we should be disabled and clear global lock
    // TODO: we can watch the convar for changes instead of watching for it here
    void Reset() {
        this.disabled = this.Cvar_PocketUpgrades.IntValue == 0;
        this.lockMessage = "";
        // dont reset combatTimer here - it's done upstream
    }

    // Open the upgrades menu. Closing the menu is out of scope and handled in-game
    // NOTE: leaves the m_bInUpgradeZone prop set to 1 - requires special handling,
    //       otherwise the menu will not reopen (except on every respawn)
    bool Show(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        if (!this.Showable(client, accountKey)) return false;

        // m_bWasInZone may need to be reset, this is the first step towards that
        SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 0);

        // using a timer so that m_bWasInZone has time to update
        CreateTimer(0.1, Timer_PocketUpgradesOpenMenu, client)

        return true;
    }

    // Several validations must pass prior to showing the menu
    bool Showable(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        return ValidClient(client)
            && !this.Disabled(client)
            && !this.Locked(client)
            && !this.ClientLocked(client, accountKey);
    }

    // Disabled by convar
    bool Disabled(int client) {
        if (this.disabled) ReplyToCommand(client, "[RTU] Pocket Menu disabled by server configuration");

        return this.disabled;
    }

    // Disabled temporarily for all players due to game event
    bool Locked(int client) {
        // No message - No lock
        if (this.lockMessage[0] == '\0') return false;

        // Message present - report message to chat and report lock to caller
        ReplyToCommand(client, "[RTU] Pocket Menu is locked %s", this.lockMessage);
        return true;
    }

    // Disabled temporarily for specific player
    bool ClientLocked(int client, char accountKey[MAX_AUTHID_LENGTH]) {
        // CombatTimer message buffer
        char buffer[32];

        // Check for lock and message
        bool clientLocked = this.combatTimer.Locked(accountKey, buffer)

        // Report message if locked
        if (clientLocked) ReplyToCommand(client, "[RTU] Pocket Menu is Locked! (%s)", buffer);

        return clientLocked;
    }

    // Sets a lock message, locking the feature and informing players about the lock
    void Lock(char message[32]="but no reason was given") {
        strcopy(this.lockMessage, sizeof(this.lockMessage), message);
    }

    // Clears the lock message, unlocking the feature
    void Unlock() {
        strcopy(this.lockMessage, sizeof(this.lockMessage), "");
    }
}

// Timers must be static and cannot live within enum structs
Action Timer_PocketUpgradesOpenMenu(Handle timer, any client) {
	if (ValidClient(client)) SetEntProp(client, Prop_Send, "m_bInUpgradeZone", 1);

	return Plugin_Handled;
}
