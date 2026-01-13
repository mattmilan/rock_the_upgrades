/**
 * Rock The Upgrades: Combat Timer
 * TODO: description
 */

#define DEFAULT_COMBAT_TIMEOUT 3

enum COMBAT_TIMER_STATUS {
        COMBAT_TIMER_UNSET=0,
        COMBAT_TIMER_READY,
        COMBAT_TIMER_STOPPED,
        COMBAT_TIMER_RUNNING,
}

enum struct CombatTimer {

    // Persistent timer, ticks down all combat times
    Handle CombatTimer;

    // Tracks remaining combat time per client
    StringMap Times;

    // Length of timeout in seconds. Set during `Init`
    int Timeout;

    COMBAT_TIMER_STATUS status;

    void Init(int timeOut) {
        this.Times = new StringMap();
        this.Timeout = timeOut;
        this.status = COMBAT_TIMER_READY;
    }

    // Clear times mid-map when advancing rounds
    void Reset() { this.Times.Clear();}

    // Start timer when map starts
    void Start() {
        if (this.status == COMBAT_TIMER_RUNNING) {
            PrintToServer("[RTU][CombatTimer] (Start) Combat timer already running.");
            return;
        }

        if (this.status != COMBAT_TIMER_READY) {
            PrintToServer("[RTU][CombatTimer] (Start) Combat timer not initialized - initializing with default timeout of 3 seoncds.");
            this.Init(DEFAULT_COMBAT_TIMEOUT);
        }

        this.status = COMBAT_TIMER_RUNNING;
        this.CombatTimer = CreateTimer(1.0, Timer_CombatTimerTicker, this, TIMER_REPEAT);
    }

    // Stop timer when map ends
    void Stop() {
        if (this.status != COMBAT_TIMER_RUNNING) {
            PrintToServer("[RTU][CombatTimer] (Stop) Combat timer not running.");
            return;
        }

        this.status = COMBAT_TIMER_STOPPED;
        this.Reset();

        CloseHandle(this.CombatTimer);
        this.CombatTimer = INVALID_HANDLE;
    }

    // Worker method for the `Timer_CombatTimerTicker` timer
    // Ticks down all combat timers by 1 second
    void Tick() {
        StringMapSnapshot keys = this.Times.Snapshot();

        // No keys == No work
        if (keys.Length == 0) return;

        // Iteration vars
        char key[MAX_AUTHID_LENGTH];
        int time;

        // Iterate keys
        for (int i = 0; i < keys.Length; i++) {
            // Grab key
            keys.GetKey(i, key, MAX_AUTHID_LENGTH);

            // Grab current time
            this.Get(key, time);

            // Tick
            time--;

            // `Set` also removes at 0
            this.Set(key, time);

            PrintToServer("[RTU][CombatTimer] (Tick) Key: %s, Time: %d", key, time);
        }
    }

    // Check lock status for a given client. Provide remaining lock time in buffer
    bool Locked(char key[MAX_AUTHID_LENGTH], char buffer[32]="") {
        int time;
        bool locked = this.Get(key, time);
        if (locked) Format(buffer, sizeof(buffer), "%d seconds remaining", time);

        return locked;
    }

    // Delegate
    bool ContainsKey(char key[MAX_AUTHID_LENGTH]) {
        return this.Times.ContainsKey(key);
    }

    // Delegate
    bool Get(char key[MAX_AUTHID_LENGTH], int &buffer) {
        if (this.Times.GetValue(key, buffer)) return true;

        buffer = 0;
        return false;
    }

    // Starts a new combat timer for the given client. Duration derived from Convar
    bool Add(char key[MAX_AUTHID_LENGTH]) {
        return this.Set(key, this.Timeout);
    }

    // Sets the given client's combat timer, removing at 0
    bool Set(char key[MAX_AUTHID_LENGTH], int time) {
        bool remove = time <= 0;

        return remove ?
            this.Times.Remove(key) :
            this.Times.SetValue(key, time);
    }
}

// Timers must be static and cannot live within enum structs
Action Timer_CombatTimerTicker(Handle timer, CombatTimer combatTimer) {
    combatTimer.Tick();

	return Plugin_Continue;
}
