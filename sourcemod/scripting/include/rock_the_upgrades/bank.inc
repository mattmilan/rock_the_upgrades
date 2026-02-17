/**
 * Rock The Upgrades: Bank (Aka Accounts Controller)
 * TODO: description
 */

#pragma newdecls required
#include <rock_the_upgrades/account>
#include <rock_the_upgrades/bank_config>
#include <regex>
// TODO: isolate via longer name
enum BANK_TRANSACTION {
	BANK_BANK_BALANCE,
	BANK_DEPOSIT,
	BANK_WITHDRAW,
	BANK_BURN,
	BANK_TRANSACTION_COUNT
}

ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
ConVar g_Cvar_CurrencyLimit;

// Invoke during `OnMapStart`
void InitBankConvars() {
	//TODO: move these
	PrecacheSound("vo/mvm_bonus01.mp3", true);
	PrecacheSound("vo/halloween_merasmus/hall2015_reward_01.mp3", true);

	// Starting currency
	g_Cvar_CurrencyStarting = CreateConVar("rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);

	// Global multiplier for all currency events
	g_Cvar_CurrencyMultiplier = CreateConVar("rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);

	// Limit max currency gain to encourage strategic spending
	g_Cvar_CurrencyLimit = CreateConVar("rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}

/*
	A bank is a glorified StringMap that maps SteamIDs (as accountKey) to Accounts
	Account Keys fall back to client names if Steam Auth Servers are unreachable
 	The bank handles client connection and account management
 	The bank resets between maps, and supports reconnecting players
 	For performance, the bank caches SteamID lookups
	TODO: DRY the skipping of the 'config' key
*/
methodmap Bank < StringMap  {
	// Unassuming constructor
    public Bank() {
		// TODO: call if uncalled (requires guard)
		InitBankConvars();
		StringMap accounts = new StringMap();

		BankConfig config;
		config.Init();
		accounts.SetArray("config", config, sizeof(config));

		return view_as<Bank>(accounts);
    }

	// Establish account key and track auth server status
	// Account link/creation happens JIT during OnPlayerSpawn
	public bool Connect(int client) {
		// Maps client -> steamID (fallback: client name)
		bool authorized = this.RegisterAccountKey(client);

		// Track steam auth server outages and recoveries
		this.SetAuthServersReachable(authorized);

		// Create new accounts or reconnect existing ones
		// TODO: can't set starting currency for new accounts until first valid spawn when team is selected
		this.FindOrCreateAccount(client);

		return authorized;
	}

	// Creates new accounts or reconnects existing ones
	public void FindOrCreateAccount(int client) {
		char accountKey[MAX_AUTHID_LENGTH];
		this.GetAccountKey(client, accountKey);

		if (this.ContainsKey(accountKey)) this.SetConnection(client, true);
		else this.CreateAccount(client, accountKey);
	}

	// Updates the underlying stringmap with a new account for the given Account Key
	public void CreateAccount(int client, char accountKey[MAX_AUTHID_LENGTH]) {
		// buffer
		Account account;

		// initialize ID information (starting currency set on activation)
		account.Init(client, accountKey);

		// save changes
		this.SetArray(accountKey, account, sizeof(Account));
	}

	// Accounts track the identity, team, and class of the client
	public bool UpdateAccount(int client, TFClassType classType, TFTeam team) {
		// buffer
		Account account;

		// fetch existing account
		this.Fetch(client, account);

		// apply updates
		account.currentClass = classType;
		account.currentTeam = team;

		// grant one-time starting currency
		if (account.earned < 0.001) account.earned = this.StartingCurrency(account.currentTeam);

		// save changes
		this.SetArray(account.accountKey, account, sizeof(Account));

		// inform caller if revert is needed
		return account.NeedsRevert();
	}

	// Mark account as disconnected. Account persists for current map
	public void Disconnect(int client) {
		this.SetConnection(client, false);
	}

	// Client Name is used while auth servers are down - once restored, convert to SteamID
	public void SetAuthServersReachable(bool reachable) {
		BankConfig config;
		this.GetArray("config", config, sizeof(config));

		// When auth servers come back online
		if (!config.steamAuthAvailable && reachable) {
			char accountKey[MAX_AUTHID_LENGTH];

			// convert client names to client steamIDs
			for (int client = 1; client <= MaxClients; client++) {
				if (!ValidClient(client)) continue;

				bool authorized = GetClientAuthId(client, AuthId_SteamID64, accountKey, sizeof(accountKey));

				if (!authorized) continue;

				config.clientAccountKeys.SetString(client, accountKey, MAX_AUTHID_LENGTH);
			}
		}

		config.steamAuthAvailable = reachable;
		this.SetArray("config", config, sizeof(config));
	}

	// Maintain an ArrayList of client -> SteamID to limit external resource traffic
	// Entries may be safely stomped while clients connect/reconnect
	// While auth servers are down, Client Name is used instead (discouraged as spoofable)
	// `SetAuthServersReachable` detects restoration and handles recovery of player name -> steamID
	// Returns true if Steam Auth Servers were reachable
	public bool RegisterAccountKey(int client) {
		// Buffer
		char accountKey[MAX_AUTHID_LENGTH];

		// Attempt read/write SteamID from external resource
		bool authorized = GetClientAuthId(client, AuthId_SteamID64, accountKey, sizeof(accountKey));

		// Fallback to client name (discouraged as spoofable but necessary if auth servers are down)
		if (!authorized) GetClientName(client, accountKey, sizeof(accountKey));

		// Update and save changes to mapping
		BankConfig config;
		this.GetArray("config", config, sizeof(config));
		config.clientAccountKeys.SetString(client, accountKey, MAX_AUTHID_LENGTH);
		this.SetArray("config", config, sizeof(config));

		// Advise caller if external resource was reachable
		return authorized;
	}

	// Account Lookup via Client -> Account Key(SteamID|Name) -> Account
	// Follow with `SetArray` to save changes
	public bool Fetch(int client, Account a) {
		// Sometimes we get a -1 from DepositTarget
		if (client <= 0 || client > MaxClients) return false;

		// Buffer
		char keyName[MAX_AUTHID_LENGTH];

		// Primary Lookup: SteamID
		this.GetAccountKey(client, keyName)

		// Fallback: Client Name (discouraged as spoofable but necessary if auth servers are down)
		if (keyName[0] == '\0') GetClientName(client, keyName, sizeof(keyName));

		// Attempt to write account to param buffer and inform caller of success
		return this.GetArray(keyName, a, sizeof(Account));
	}

	// Write string value to buffer param. Return false if empty
	public bool GetAccountKey(int client, char accountKey[MAX_AUTHID_LENGTH]) {
		BankConfig config;
		this.GetArray("config", config, sizeof(config));
	 	return config.clientAccountKeys.GetString(client, accountKey, MAX_AUTHID_LENGTH) > 0;
	}

	public bool AccountExists(int client) {
		char accountKey[MAX_AUTHID_LENGTH];
		this.GetAccountKey(client, accountKey);
		return this.ContainsKey(accountKey);
	}

	// Marks an account as connected or disconnected (for visibility)
	// Disconnected accounts have their spending reset (retains earned currency)
	public void SetConnection(int client, bool connected) {
		Account a; if (!this.Fetch(client, a)) return;

		// Reconnecting clients will have a different client id
		if (!a.connected && connected) a.client = client;

		a.connected = connected;

		// Reset spending but retain earned currency on disconnect
		if (!connected) a.Reset(a.Earned());

		// Save Changes
		this.SetArray(a.accountKey, a, sizeof(Account));
	}

	// Update or create account
	public bool OnPlayerSpawn(int client, TFClassType classType, TFTeam team) {
		// Apply changes. Returns true if revert is needed
		bool revert = this.UpdateAccount(client, classType, team);

		// Sync currency
		this.Sync(client);

		return revert;
	}

	public bool NeedsRevert(int client) {
		Account a;
		this.Fetch(client, a);
		return a.NeedsRevert();
	}

	// Always returns true; saves us a few lines.
	public bool Transaction(float amount, Account account, BANK_TRANSACTION transaction) {
		// Apply global multiplier (default 1.0)
		amount *= g_Cvar_CurrencyMultiplier.FloatValue;

		switch (transaction) {
			// Currency gained from events or chat commands
			case BANK_DEPOSIT:
				account.earned += amount;
			// Purchasing or refunding upgrades
			case BANK_WITHDRAW:
				account.classData[account.currentClass].spent += amount;
			// Purchasing non-refundables
			case BANK_BURN:
				account.classData[account.currentClass].burnt += amount;
		}

		// Enforce currency limit (disabled by default)
		account.earned = this.LimitCurrency(account.earned);

		// Save changes before syncing. WARN:
		this.SetArray(account.accountKey, account, sizeof(Account));

		// Skip disconnected clients as there is no netprop to sync
		if (account.connected) this.Sync(account.client);

		return true;
	}

	// Helper method for readability
	public bool Deposit(float amount, int client) {
		Account account; if (!this.Fetch(client, account)) return false;

		return this.Transaction(amount, account, BANK_DEPOSIT);
	}

	// Helper method for readability
	public bool Withdraw(float amount, int client) {
		Account account; if (!this.Fetch(client, account)) return false;

		return this.Transaction(amount, account, BANK_WITHDRAW);
	}

	// Helper method for readability
	public bool Burn(float amount, int client) {
		Account account; if (!this.Fetch(client, account)) return false;

		return this.Transaction(amount, account, BANK_BURN);
	}

	// Call when refunding upgrades
	public bool Refund(float amount, int client) {
		return this.Withdraw(-amount, client);
	}

	// Call to remove penalties or restore unrefundable currency (canteen purchases)
	public bool Forgive(float amount, int client) {
		return this.Burn(-amount, client);
	}

	public void Revert(int client) {
		Account account; if (!this.Fetch(client, account)) return;
		account.Revert();
		this.SetArray(account.accountKey, account, sizeof(Account));
		this.Sync(client);
	}

	// Deposit money to all accounts whether connected or not. Optional team filter
	// Always returns true; reduces LoC
	public bool DepositAll(float amount, TFTeam team=TFTeam_Unassigned) {

		// Increase late-join starting currency
		BankConfig config;
		this.GetArray("config", config, sizeof(config));
		config.AddBonusStartingCurrency(amount, team);
		this.SetArray("config", config, sizeof(config));

		// Prepare iteration
		Account a;
		char accountKey[MAX_AUTHID_LENGTH];
		StringMapSnapshot keys = this.Snapshot();

		// Iterate accounts
		for(int i = 0; i < keys.Length; i++) {
			// Write key to buffer
			keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

			// TODO: convert bank to MetaStringMap to get config out of here
			if (strcmp(accountKey, "config") == 0) continue;

			// Write account to buffer
			this.GetArray(accountKey, a, sizeof(Account));

			// Optional team filter
			if (!this.ValidTeam(a.currentTeam, team)) continue;

			// Get paid
			this.Transaction(amount, a, BANK_DEPOSIT);
		}

		return true;
	}

	// Used by rtu_pay chat command. DepositAll restricted to Admins
	public bool DepositTarget(char target[MAX_NAME_LENGTH], float amount, int client, char targetName[MAX_NAME_LENGTH]) {
		// Identify potential target(s)
		TFTeam targetTeam = this.TeamFromTarget(target);
		int targetClient = 0;


		// Determine which target to use
		bool admin = client == 0 || CheckCommandAccess(client, "", ADMFLAG_GENERIC, true);
		bool depositAll = admin && targetTeam != TFTeam_Spectator;

		if (!depositAll) targetClient = FindTarget(client, target, true, false);

		// Edge case - non-admins cannot pay themselves
		if (!admin && targetClient == client) return false;

		// Determine target name and send upstream for reporting
		if (depositAll) {
			strcopy(targetName, MAX_NAME_LENGTH, target);
		} else if (targetClient > 0) {
			GetClientName(targetClient, targetName, MAX_NAME_LENGTH);
		}

		// Execute the chosen deposit strategy
		bool success = depositAll ?
			this.DepositAll(amount, targetTeam) :
			this.Deposit(amount, targetClient);

		char clientName[MAX_NAME_LENGTH];
		GetClientName(client, clientName, MAX_NAME_LENGTH);

		if (success) {
			if (depositAll) {
				// TODO: this should emit only to the proper team if it's filtered as such
				EmitSoundToAll("vo/mvm_bonus01.mp3");
				PrintCenterTextAll("%s has granted %d bonus currency to everyone!", clientName, RoundToCeil(amount));
			} else {
				EmitSoundToClient(targetClient, "vo/halloween_merasmus/hall2015_reward_01.mp3");
				PrintCenterText(targetClient, "%s has given you %d currency!", clientName, RoundToCeil(amount));
				// or mvm/mvm_money_pickup.wav
				// or vo/pauling/plng_contract_complete_allclass_09.mp3 "here you go"
			}
		}

		return success;
	}

	// TODO: move to shared helpers
	public TFTeam TeamFromTarget(char target[MAX_NAME_LENGTH]) {
			 if (StrEqual(target, "all",  false)) return TFTeam_Unassigned;
		else if (StrEqual(target, "blu",  false)) return TFTeam_Blue;
		else if (StrEqual(target, "blue", false)) return TFTeam_Blue;
		else if (StrEqual(target, "red",  false)) return TFTeam_Red;
		else 									  return TFTeam_Spectator;
	}

	// Limit maximum earnable currency (disabled by default)
	public float LimitCurrency(float amount) {
		float limit = g_Cvar_CurrencyLimit.FloatValue;

		// if no limit or under limit, amount is valid
		if (limit < 0 || amount < limit) return amount;

		return limit;
	}

	// Get the balance for the client's current class
	public float Balance(int client) {
		Account a;
		this.Fetch(client, a);
		return a.Balance();
	}

	public float Earned(int client) {
		Account a;
		this.Fetch(client, a);
		return a.Earned();
	}

	// Starting currency before bonuses
	public float BaseStartingCurrency() {
		return  g_Cvar_CurrencyStarting.FloatValue
			* g_Cvar_CurrencyMultiplier.FloatValue;
	}

	// Starting currency including bonuses
	public float StartingCurrency(TFTeam team) {
		BankConfig config; this.GetArray("config", config, sizeof(config));
		return this.BaseStartingCurrency()
			 + config.Bonus(team);
	}

	// Synchronize bank values with actual client currency
	public bool Sync(int clientID = -1) {
		// client validation has just happened upstream
		if (clientID > 0) {
			return this.SetClientCurrency(clientID, this.Balance(clientID));
		}

		for (int client = 1; client <= MaxClients; client++) {
			if (!this.ValidClient(client)) continue;
			if (!this.Sync(client)) return false;
		}

		return true;
	}

	// Get the difference between bank balance and actual currency
	// Used to update the account `spent` value
	// Negative values indicates spending
	// Positive values indicates refunding
	// NOTE: Casting as int to simplify downstream zero comparison
	// TODO: handle nonrefundable currency (burnt)
	public int Delta(int client) {
		float current = this.GetClientCurrency(client);
		float balance = this.Balance(client);

		int delta = RoundToCeil(balance - current);

		return delta;
	}

	// Resolve account balance deltas arising from spent/refunded currency
	public bool ResolveDelta(int client) {
		int delta = this.Delta(client);
		if (delta == 0) return false;

		this.Withdraw(float(delta), client);
		return true;
	}

	// Reset all accounts in the bank. Syncs connected clients only
	public void ResetAccounts() {
		// Revert starting currency bonuses earned from objectives
		BankConfig config;
		this.GetArray("config", config, sizeof(config));
		config.bonusStartingCurrencyRed = 0.0;
		config.bonusStartingCurrencyBlu = 0.0;
		this.SetArray("config", config, sizeof(config));
		// float startingCurrency = this.StartingCurrency();
		// Iteration vars
		StringMapSnapshot keys = this.Snapshot();
		char accountKey[MAX_AUTHID_LENGTH];
		Account account;

		// Iterate Accounts
		for(int i = 0; i < keys.Length; i++) {
			// Store Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

			// Skip config entry
			if (strcmp(accountKey, "config") == 0) continue;

			// Fetch account
			this.GetArray(accountKey, account, sizeof(Account));

			// Perform reset
			account.Reset(this.StartingCurrency(account.currentTeam));

			// Save Changes
			this.SetArray(accountKey, account, sizeof(Account));

			// Sync connected clients
			if (account.connected) this.Sync(account.client);
		}
	}

	// Get "the real" currency value for a client (not the bank balance)
	public float GetClientCurrency(int client) {
		return this.ValidClient(client) ?
			float(GetEntProp(client, Prop_Send, "m_nCurrency")) :
			-1.0;
	}

	// Update "the real" currency value for a client. TREAT AS PRIVATE
	public bool SetClientCurrency(int client, float amount) {
		bool valid = this.ValidClient(client);
		if (valid) SetEntProp(client, Prop_Send, "m_nCurrency", RoundToCeil(amount));
		return valid;
	}

	// Debug. Print all accounts in a table. Optional client filter
	public void PrintToConsole(int client) {
		if (this.Size == 0) {
			PrintToConsole(client, "[RTU][Bank] No accounts to display.");
			return;
		}

		PrintToConsole(client, "=================================================================================");
		PrintToConsole(client, "[RTU][Bank] Accounts [BONUS RED:%.2f] [BONUS BLU:%.2f]", this.StartingCurrency(TFTeam_Red), this.StartingCurrency(TFTeam_Blue));
		PrintToConsole(client, "---------------------------------------------------------------------------------");
		PrintToConsole(client, "Account Key           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
		PrintToConsole(client, "---------------------------------------------------------------------------------");
		if (this.Size == 0) PrintToConsole(client, "[RTU][Bank] No accounts to display.");
		else this.PrintAccounts(client);
		PrintToConsole(client, "=================================================================================");
	}

	// Debug. Print the account identified by client. -1 prints all accounts.
	public void PrintAccounts(int client) {
		// Iteration vars
		Account a;
		char accountKey[MAX_AUTHID_LENGTH];
		StringMapSnapshot keys = this.Snapshot();

		// Iterate Accounts
		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, sizeof(accountKey));
			if (strcmp(accountKey, "config") == 0) continue;
			bool accountFound = this.GetArray(accountKey, a, sizeof(Account));

			PrintToConsole(client,
				accountFound ?
					a.ToRow() :
					"[RTU][Bank] No account found for Account Key: %s", accountKey);
		}
	}

	// Debug
	public void PrintAccount(int client) {
		Account a;

		bool accountFound = this.Fetch(client, a);

		PrintToConsole(client,
			accountFound ?
				a.ToString() :
		 		"[RTU][Bank] No account found for Client: %d", client);
	}

	// Debug
	public void PrintAccountKeys() {
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			int size = keys.KeyBufferSize(i);
			char[] accountKey = new char[size];
			keys.GetKey(i, accountKey, size);
			if (strcmp(accountKey, "config") == 0) continue;
			PrintToConsole(client, "[RTU][Bank] Index: %d, Account Key: %s", i, accountKey);
		}
	}

	// Full compliment of standard client validations. NO BOTS!
	public bool ValidClient(int client) {
		return client
			&& client <= MaxClients
			&& IsClientConnected(client)
			&& IsClientInGame(client)
			&& !IsFakeClient(client);
	}

	// Optional team filter
	public bool ValidTeam(TFTeam team, TFTeam filter) {
		return filter == TFTeam_Unassigned || team == filter;
	}

	public TFTeam ClientTeam(int client) {
		Account a;
		this.Fetch(client, a);
		return a.currentTeam;
	}
}
