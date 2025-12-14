ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
ConVar g_Cvar_CurrencyOnKillMin;
ConVar g_Cvar_CurrencyOnKillMax;
ConVar g_Cvar_CurrencyOnObjectDestroyed;
ConVar g_Cvar_CurrencyDeathTax;
ConVar g_Cvar_RevengeMultiplier;
ConVar g_Cvar_CurrencyOnCapturePoint;
ConVar g_Cvar_CurrencyOnCaptureFlag;
ConVar g_Cvar_CurrencyOnDomination;

// TODO: implement
// ConVar g_Cvar_CurrencyOverTime;
// ConVar g_Cvar_CurrencyOverTimeRate;
// ConVar g_Cvar_CurrencyOverTimeFrequency;
// ConVar g_Cvar_CurrencyLimit;
// ConVar g_Cvar_UpgradeCostMultiplier;

StringMap RevengeTracker;	// Bonus currency for revenge kills

// Invoke during `OnPluginStart()`
void InitCurrencyController() {
	RevengeTracker = new StringMap();
    InitCurrencyConVars();
    HookCurrencyEvents();
}

// Invoke during `OnPluginEnd()`
void CloseCurrencyController() {
	RevengeTracker.Close();
}

// Connect important game events to currency-gaining functions
void HookCurrencyEvents() {
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Post);
	HookEvent("object_destroyed", Event_ObjectDestroyed, EventHookMode_Post);
	HookEvent("teamplay_point_captured", Event_TeamplayPointCaptured, EventHookMode_Post);
	HookEvent("teamplay_flag_event", Event_TeamplayFlagEvent, EventHookMode_Post);
	HookEvent("player_domination", Event_PlayerDomination, EventHookMode_Post);
}

void InitCurrencyConVars() {
	// Currency rules and modifiers, with reasonable defaults
	g_Cvar_CurrencyStarting = CreateConVar("sm_rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);
	g_Cvar_CurrencyMultiplier = CreateConVar("sm_rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
	g_Cvar_RevengeMultiplier = CreateConVar("sm_rtu_currency_on_revenge", "4.0", "Multiplier for revenge kills [4, 1..]", 0, true, 1.0, false);

	// Kills provide the player with a varying amount of currency
	g_Cvar_CurrencyOnKillMin = CreateConVar("sm_rtu_currency_on_kill_min", "10.0", "Minimum amount of currency to give to players on robot kill [10, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnKillMax = CreateConVar("sm_rtu_currency_on_kill_max", "30.0", "Maximum amount of currency to give to players on robot kill [30, 0..]", 0, true, 0.0, false);

	// Engineer Buildings (Objects) provide increasing currency depending on state
	g_Cvar_CurrencyOnObjectDestroyed = CreateConVar("sm_rtu_currency_on_object_destroyed", "5.0", "Base amount of currency to give to players on building destruction, multiplied by building level [5, 0..]", 0, true, 0.0, false);

	// Objectives provide currency to the entire team
	// NOTE: TF2 is already providing 100 currency on point capture. This plugin doesn't prevent that, but it will add a little extra
	g_Cvar_CurrencyOnCapturePoint = CreateConVar("sm_rtu_currency_on_capture_point", "150.0", "Amount of currency to give to team on point capture in addition to the built-in default 100 currency [150, 0..]", 0, true, 0.0, false);
	g_Cvar_CurrencyOnCaptureFlag = CreateConVar("sm_rtu_currency_on_capture_flag", "250.0", "Amount of currency to give to team on flag capture [250, 0..]", 0, true, 0.0, false);

    // Additional rules, disabled by default
    g_Cvar_CurrencyDeathTax = CreateConVar("sm_rtu_currency_death_tax", "0.0", "Percentage of currency to deduct on player death. 1 means all unspent currency is lost [0, 0..1]", 0, true, 0.0, true, 1.0);
    g_Cvar_CurrencyOnDomination = CreateConVar("sm_rtu_currency_on_domination", "0.0", "Amount of currency to give a player on domination [0, 0..]", 0, true, 0.0, false);

    // TODO: implement
    // // Time-based currency gain. Off by default
	// g_Cvar_CurrencyOverTime = CreateConVar("sm_rtu_currency_over_time", "0", "Enable or disable time-based currency gain [0, 0,1]", 0, true, 0.0, true, 1.0);
	// g_Cvar_CurrencyOverTimeRate = CreateConVar("sm_rtu_currency_over_time_rate", "5", "Amount of currency earned per tick [5, 1..]", 0, true, 1.0, false);
	// g_Cvar_CurrencyOverTimeFrequency = CreateConVar("sm_rtu_currency_over_time_frequency", "30", "Frequency of time-based currency gain [30, 15..]", 0, true, 15.0, false);
	//
    // // Difficulty modifiers
	// g_Cvar_UpgradeCostMultiplier = CreateConVar("sm_rtu_upgrade_cost_multiplier", "1.0", "Multiplier for upgrade costs when RTU is activated. A value of zero provides free upgrades [1, 0..].", 0, true, 0.0, false);
	// g_Cvar_CurrencyLimit = CreateConVar("sm_rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}

// Kills earn currency
Action Event_PlayerDeath(Event event, const char[] name, bool dontBroadcast) {
	int killer = GetClientOfUserId(event.GetInt("attacker"));
	int assister = GetClientOfUserId(event.GetInt("assister"));
	int victim = GetClientOfUserId(event.GetInt("userid"));

	if (Suicide(killer, victim)) { return Plugin_Continue; }

	bool revenge = RevengeKill(killer, victim)
				|| RevengeKill(assister, victim);

	float reward = CurrencyOnKill(revenge);

	if (killer)   AddClientCurrency(killer, reward);
	if (assister) AddClientCurrency(assister, reward);
	if (victim)   AddClientCurrency(victim, -DeathPenalty(victim));

	return Plugin_Continue;
}

Action Event_ObjectDestroyed(Event event, const char[] name, bool dontBroadcast) {
	// Play SFX
	// DEBUG - the event fires, but the function is not working as expected
	// if (event.GetBool("was_building")) { ObjectDenied(event); }

	int attacker = GetClientOfUserId(event.GetInt("attacker"));
	int assister = GetClientOfUserId(event.GetInt("assister"));
	float reward = CurrencyForDestroyedObject(event);

	if (attacker) AddClientCurrency(attacker, reward);
	if (assister) AddClientCurrency(assister, reward);

	return Plugin_Continue;
}

// Capturing a point earns team currency
Action Event_TeamplayPointCaptured(Event event, const char[] name, bool dontBroadcast) {
	AddTeamCurrency(event.GetInt("team"), CurrencyFromPointCapture());

	return Plugin_Continue;
}

// Capturing a flag earns team currency
Action Event_TeamplayFlagEvent(Event event, const char[] name, bool dontBroadcast) {
	// MAGIC NUMBER: 2 is "captured"
	if (event.GetInt("eventtype") != 2) { return Plugin_Continue; }

	// We could get the team from the event, but some maps use inverted logic
	// (ctf_crasher for example). Using player, we get the right team consistently
	int player = event.GetInt("player");
	int team = GetClientTeam(player);
	AddTeamCurrency(team, CurrencyFromFlagCapture());

	return Plugin_Continue;
}

// Dominations earn bonus currency (disabled by default) and provide data for revenge kills
Action Event_PlayerDomination(Event event, const char[] name, bool dontBroadcast) {
	// Handle domination
    int dominator = event.GetInt("dominator");
	int dominated = event.GetInt("dominated");
	AddClientCurrency(dominator, CurrencyOnDomination());

	// Track revenge
	char dominatorName[MAX_NAME_LENGTH]; GetClientName(dominator, dominatorName, sizeof(dominatorName));
	char dominatedName[MAX_NAME_LENGTH]; GetClientName(dominated, dominatedName, sizeof(dominatedName));
	RevengeTracker.SetString(dominatedName, dominatorName);

	return Plugin_Continue;
}

// Play SFX when buildings are destroyed while being built
// TODO: Debug the heck out of this
// void ObjectDenied(Event event) {
// 	PrintToServer("[RTU] ObjectDenied event triggered");
// 	int builder = GetClientOfUserId(event.GetInt("userid"));
// 	int attacker = GetClientOfUserId(event.GetInt("attacker"));
// 	int assister = GetClientOfUserId(event.GetInt("assister"));

// 	if (ValidClient(builder))  EmitGameSoundToClient(builder,  "vo/engineer_no01.wav");
// 	if (ValidClient(attacker)) EmitGameSoundToClient(attacker, "vo/engineer_gunslingerpunch02.wav");
// 	if (ValidClient(assister)) EmitGameSoundToClient(assister, "vo/engineer_gunslingerpunch02.wav");
// }

// higher level buildings yield more currency
float CurrencyForDestroyedObject(Event event) {
	int objectIndex = event.GetInt("index");
	int upgradeLevel = GetEntProp(objectIndex, Prop_Send, "m_iUpgradeLevel");

	return upgradeLevel
		 * g_Cvar_CurrencyOnObjectDestroyed.FloatValue
		 * g_Cvar_CurrencyMultiplier.FloatValue;
}

// Add an arbitrary float value to a client's currency
bool AddClientCurrency(int client, float amount) {
    float currency = GetClientCurrency(client);
	return SetClientCurrency(client, currency + amount);
}

// Add an arbitrary float value to all clients on a team
void AddTeamCurrency(int team, float amount) {
	for (int client = 1; client <= MaxClients; client++) {
		if (ValidClient(client) && GetClientTeam(client) == team)
			AddClientCurrency(client, amount);
	}
}

// Get a client's currency value as float, or a negative value for invalid clients
float GetClientCurrency(int client) {
	return ValidClient(client) ?
		float(GetEntProp(client, Prop_Send, "m_nCurrency")) :
		-1.0;
}

// Set client currency to an arbitrary integer, rounded up from float
bool SetClientCurrency(int client, float amount) {
	bool valid = ValidClient(client);
	if (valid) SetEntProp(client, Prop_Send, "m_nCurrency", RoundToCeil(amount));
	return valid;
}

// NOTE: this will not give currency to bots or spectators
void SetAllCurrency(float amount) {
	for (int client = 1; client <= MaxClients; client++)
		SetClientCurrency(client, amount);
}

// Payment denied on suicide and other nonsense
bool Suicide(int killer, int victim) {
    return killer < 1 || killer == victim;
}

// Revenge kills trigger a bonus multiplier
// TODO: optimize by avoiding conversion from id to name, if possible
bool RevengeKill(int killer, int victim) {
	// get killer name
	char dominatedName[MAX_NAME_LENGTH];
	GetClientName(killer, dominatedName, sizeof(dominatedName));

	// check if killer has been dominated, and if so, who the dominator was
	char storedDominator[MAX_NAME_LENGTH];
	bool revenge = RevengeTracker.GetString(dominatedName, storedDominator, sizeof(storedDominator));

	// bail early if killer hasn't actually been dominated
	if (!revenge) { return false; }

	// get victim name
	char dominatorName[MAX_NAME_LENGTH];
	GetClientName(victim, dominatorName, sizeof(dominatorName));

	// if the victim never dominated the player, this is not a revenge kill
	if (!StrEqual(dominatorName, storedDominator)) { return false; }

	// confirmed as a revenge kill - removing the record also returns true
    return RevengeTracker.Remove(dominatedName);
}

// Returns a ConVar-defined float on revenge, or 1 (which will have no effect)
float RevengeMultiplier(bool revenge=false) {
	return revenge ? g_Cvar_RevengeMultiplier.FloatValue : 1.0;
}

// Returns a random float clamped to a convar-defined range, then apply multipliers
float CurrencyOnKill(bool revenge=false) {
    return GetRandomFloat(
        g_Cvar_CurrencyOnKillMin.FloatValue,
        g_Cvar_CurrencyOnKillMax.FloatValue
    ) * g_Cvar_CurrencyMultiplier.FloatValue
	  * RevengeMultiplier(revenge);
}

// Dominations earn a bonus (disabled by default)
float CurrencyOnDomination() {
    return g_Cvar_CurrencyOnDomination.FloatValue
         * g_Cvar_CurrencyMultiplier.FloatValue;
}

// Dying may incur a penalty (disabled by default)
// Returns a positive value, should be inverted when called
float DeathPenalty(int victim) {
    return GetClientCurrency(victim)
         * g_Cvar_CurrencyDeathTax.FloatValue;
}

float CurrencyFromFlagCapture() {
	return g_Cvar_CurrencyOnCaptureFlag.FloatValue
         * g_Cvar_CurrencyMultiplier.FloatValue;
}

float CurrencyFromPointCapture() {
	return g_Cvar_CurrencyOnCapturePoint.FloatValue
         * g_Cvar_CurrencyMultiplier.FloatValue;
}

float StartingCurrency() {
	return g_Cvar_CurrencyStarting.FloatValue
		 * g_Cvar_CurrencyMultiplier.FloatValue;
}

bool ValidClient(int client) {
	return client
		&& client <= MaxClients
		&& IsClientConnected(client)
		&& !IsFakeClient(client);
}
