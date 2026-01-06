/**
 * Rock The Upgrades: Combat Timer
 * TODO: description
 */
enum COMBAT_TIMER_STATUS {
        COMBAT_TIMER_UNSET=0,
        COMBAT_TIMER_STOPPED,
        COMBAT_TIMER_RUNNING,
}
enum struct CombatTimer {

    // Persistent timer, ticks down all combat times
    Handle CombatTimer;

    // Tracks remaining combat time per client
    StringMap Times;

    COMBAT_TIMER_STATUS status;

    void Init() { this.Times = new StringMap(); }

    // Clear times mid-map when advancing rounds
    void Reset() { this.Times.Clear();}

    // Start timer when map starts
    void Start() {
        if (this.status == COMBAT_TIMER_RUNNING) {
            PrintToServer("[RTU][CombatTimer] (Start) Combat timer already running.");
            return;
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

    // Set, reset, or decrement a combat timer
    // MAGIC NUMBER 5: arbitrary lock time (TODO: convar)
    bool Set(char key[MAX_AUTHID_LENGTH], int time=5) {
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
