/**
 * Rock The Upgrades: Combat Timer
 * TODO: description
 */

enum struct CombatTimer {
    // Persistent timer, ticks down all combat times
    Handle CombatTimer;

    // Tracks remaining combat time per client
    StringMap Times;

    void Init() { this.Times = new StringMap(); }

    void Reset() { this.Times.Clear();}

    void Start() {
        if (this.CombatTimer == INVALID_HANDLE) {
            this.CombatTimer = CreateTimer(1.0, Timer_CombatTimerTicker, this, TIMER_REPEAT);
        } // else PrintToServer("[RTU][CombatTimer] Combat timer already running.");
    }

    void Stop() {
        this.Times.Clear();
        if (this.CombatTimer != INVALID_HANDLE) {
            CloseHandle(this.CombatTimer);
            this.CombatTimer = INVALID_HANDLE;
        } // else PrintToServer("[RTU][CombatTimer] Combat timer not running.");
    }

    void Tick() {
        // Interation vars
        StringMapSnapshot keys = this.Times.Snapshot();

        if (keys.Length == 0) return;

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

            // `Set` also removes the entry once time reaches 0
            this.Set(key, time);

            PrintToServer("[RTU][CombatTimer] (Tick) Key: %s, Time: %d", key, time);
        }
    }

    bool ContainsKey(char key[MAX_AUTHID_LENGTH]) {
        return this.Times.ContainsKey(key);
    }

    bool Get(char key[MAX_AUTHID_LENGTH], int &buffer) {
        if (this.Times.GetValue(key, buffer)) return true;

        buffer = 0;
        return false;
    }

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
