#if defined _omni_timer_included
 #endinput
#endif
#define _omni_timer_included

// #define MAX_OMNITIMER_KEY_LENGTH 16
#define MAX_OMNITIMER_MESSAGE_LENGTH 32

#include <json/helpers/metastringmap>

ConVar g_Cvar_OmniTimeout;

static OmniTimer g_OmniTimer;

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
 * OmniTimer is designed as a Singleton:
 *  - Static methods are considered "public"
 *  - Public methods are considered "private"
 *  - Don't instantiate - call statics
 *
 * Example: Adding a timer to a player when they take damage
 *
 * HookEvent("player_hurt", Event_PlayerHurt, EventHookMode_Post);
 * ...
 * Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast) {
 *     int client = GetClientOfUserId(event.GetInt("userid"));
 *     char timerKey[MAX_NAME_LENGTH];
 *     GetClientName(client, timerKey, sizeof(timerKey));
 *     OmniTimer.Add(timerKey);
 *     return Plugin_Continue;
 * }
 *
 * Example: Checking if a player is in combat before allowing an action
 *
 * void TryTurnInvisible(int client) {
 *     char timerKey[MAX_NAME_LENGTH];
 *     GetClientName(client, timerKey, sizeof(timerKey));
 *     if (OmniTimer.Get(timerKey) > 0) return false;
 *     else TurnInvisible(client);
 * }
 */
methodmap OmniTimer < MetaStringMap {
    public static OmniTimer Instance() {
        if (g_OmniTimer == null) OmniTimer.Init();

        return g_OmniTimer;
    }

    // OnPluginStart
    public static void Init() {
        if (g_OmniTimer != null) return;

        g_Cvar_OmniTimeout = CreateConVar("omni_timer_timeout", "3.0", "Default duration in seconds of omni timer entries. [3.0, 0..]", 0, true, 0.0, false);
        g_OmniTimer = view_as<OmniTimer>(new MetaStringMap());
    }

    // OnMapStart
    public static void Start() {
        if (OmniTimer.Instance().Ticker != INVALID_HANDLE) return;

        OmniTimer.Instance().Ticker = CreateTimer(
            1.0,
            Timer_OmniTimerTicker,
            OmniTimer.Instance(),
            TIMER_REPEAT
        );
    }

    // OnMapEnd
    public static void Stop() {
        // clear remaining times
        OmniTimer.Instance().Clear();

        // bail if ticker not running
        if (OmniTimer.Instance().Ticker == INVALID_HANDLE) return;

        // close ticker
        CloseHandle(OmniTimer.Instance().Ticker);
        OmniTimer.Instance().Ticker = INVALID_HANDLE;
    }

   // Starts/Restarts a timer for the given key
    public static bool Add(char[] key, int time = 0) {
        if (!time) time = OmniTimer.Instance().DefaultTimeout;
        return OmniTimer.Instance().SetInt(key, time);
    }

    // Checks if a timer exists for the given key
    public static bool Get(char[] key) {
        return OmniTimer.Instance().GetInt(key, 0);
    }

    // Remove a time before it expires
    public static bool Drop(char[] key) {
        return OmniTimer.Instance().Remove(key);
    }

    // private

    // Decrements all entries on a 1-second interval
    property Handle Ticker {
        public get() { return this.GetHandle("ticker"); }
        public set(Handle value) { this.SetHandle("ticker", value); }
    }

    // Default duration of new entries in seconds
    property int DefaultTimeout {
        public get() { return RoundToNearest(g_Cvar_OmniTimeout.FloatValue); }
    }

    // Worker method that tics down all time entries in 1-second intervals
    public void Tick() {
        StringMapSnapshot keys = this.Snapshot();

        // No keys == No work
        if (keys.Length == 0) return;

        for (int i = 0; i < keys.Length; i++) {
            // Grab key
            int keySize = keys.KeyBufferSize(i);
            char[] key = new char[keySize];
            keys.GetKey(i, key, keySize);
            this.Tock(key);
        }
    }

    // Decrement (and remove expired) entries
    public bool Tock(char[] key) {
        int time = this.GetInt(key) - 1;

        time <= 0 ?
            this.Remove(key) :
            this.SetInt(key, time);
    }
}

// Timers must be static and cannot live within methodmaps
Action Timer_OmniTimerTicker(Handle timer, OmniTimer omniTimer) {
    omniTimer.Tick();

	return Plugin_Continue;
}

methodmap CombatTimer < OmniTimer {
    public static bool Add(int client, int time = 0) {
        char key[MAX_NAME_LENGTH];
        AuthKeys.Get(client, key);
        OmniTimer.Add(key, time);
    }
    // add
    // drop
    // start
    // stop

}