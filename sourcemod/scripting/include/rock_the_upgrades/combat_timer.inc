#if defined _combat_timer_included
 #endinput
#endif
#define _combat_timer_included

ConVar g_Cvar_CombatTimeout;

static CombatTimer g_CombatTimer;

/**
 * Rock The Upgrades: Combat Timer (TODO: rename omni_timer)
 *
 * A single timer with an interval of 1 second that ticks down 0 or more "Times"
 * The timer runs automatically when first called but may be started/stopped manually
 *
 * Times are stored in a TypedStringMap whose keys represent the object being timed
 * Arbitrary Times may be added via `Set`, while `Add` uses a Convar-derived time
 * An object's presence in the map can be checked with `Get` to drive timed behavior
 *
 * Usage:
 *
 * CombatTimer is designed as a Singleton:
 *  - Static methods are considered "public"
 *  - Public methods are considered "private"
 *  - Don't instantiate - call statics
 *
 * Example: Adding a timer to a player when they take damage
 *
 * Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast) {
 *     int client = GetClientOfUserId(event.GetInt("userid"));
 *     char timerKey[3]; IntToString(client, timerKey, sizeof(timerKey));
 *     CombatTimer.Add(timerKey);
 *     return Plugin_Continue;
 * }
 *
 * Example: Checking if a player is in combat before allowing an action
 *
 * bool TurnInvisible(int client) {
 *     char timerKey[3]; IntToString(client, timerKey, sizeof(timerKey));
 *     if (CombatTimer.Get(timerKey, _)) return false
 *     else ...
 * }
 */

methodmap CombatTimer < TypedStringMap {
    public static CombatTimer Instance() {
        if (g_CombatTimer == null) CombatTimer.Init();

        return g_CombatTimer;
    }

    public static void Init() {
        g_Cvar_CombatTimeout = CreateConVar("rtu_combat_timeout", "3.0", "Duration in seconds after taking or dealing damage that a player is considered 'in combat' and cannot open the upgrade menu. [3.0, 0..]", 0, true, 0.0, false);
        g_CombatTimer = view_as<CombatTimer>(new StringMap());
        CombatTimer.Start();
    }

    // Start timer when map starts
    public static void Start() {
        if (CombatTimer.Instance().Ticker != INVALID_HANDLE) return;

        CombatTimer.Instance().Ticker = CreateTimer(1.0, Timer_CombatTimerTicker, CombatTimer.Instance(), TIMER_REPEAT);
    }

    // Stop timer when map ends
    public static void Stop() {
        if (CombatTimer.Instance().Ticker == INVALID_HANDLE) return;

        // clear remaining times
        CombatTimer.Instance().Clear();
        // close ticker
        CloseHandle(CombatTimer.Instance().Ticker);
        CombatTimer.Instance().Ticker = INVALID_HANDLE;
    }

    // Check lock status for a given client. Provide remaining lock time in buffer
    public static bool Locked(char key[MAX_AUTHID_LENGTH], char buffer[32]="") {
        int time;
        bool locked = CombatTimer.Instance().GetValue(key, time);
        if (locked) Format(buffer, sizeof(buffer), "%d seconds remaining", time);

        return locked;
    }

   // Starts/Restarts a combat timer for the given client
    public static bool Add(char key[MAX_AUTHID_LENGTH]) {
        return CombatTimer.Instance().SetValue(key, CombatTimer.Instance().Timeout);
    }

    // Delegate
    public static bool Get(char key[MAX_AUTHID_LENGTH], int &buffer=0) {
        if (CombatTimer.Instance().GetValue(key, buffer)) return true;

        return false;
    }

    public static bool Drop(char key[MAX_AUTHID_LENGTH]) {
        return CombatTimer.Instance().Remove(key);
    }

    // private

    property Handle Ticker {
        public get() { return this.GetHandle("ticker"); }
        public set(Handle value) { this.SetHandle("ticker", value); }
    }

    property int Timeout {
        public get() { return RoundToNearest(g_Cvar_CombatTimeout.FloatValue); }
    }

    public void Tick() {
        StringMapSnapshot keys = this.Snapshot();

        // No keys == No work
        if (keys.Length == 0) return;

        this.DecrementTimes(keys);
    }

    public void DecrementTimes(StringMapSnapshot keys) {
        // Iteration vars
        char key[MAX_AUTHID_LENGTH];

        // Iterate keys
        for (int i = 0; i < keys.Length; i++) {
            // Grab key
            keys.GetKey(i, key, MAX_AUTHID_LENGTH);
            this.DecrementTime(key);
        }
    }

    public void DecrementTime(char key[MAX_AUTHID_LENGTH]) {
        this.SetInt(key, this.GetInt(key) - 1);
    }


    // Sets the given client's combat timer, removing at 0
    public bool Set(char key[MAX_AUTHID_LENGTH], int time) {
        bool remove = time <= 0;

        return remove ?
            this.Remove(key) :
            this.SetValue(key, time);
    }
}

// Timers must be static and cannot live within methodmaps
Action Timer_CombatTimerTicker(Handle timer, CombatTimer combatTimer) {
    combatTimer.Tick();

	return Plugin_Continue;
}
