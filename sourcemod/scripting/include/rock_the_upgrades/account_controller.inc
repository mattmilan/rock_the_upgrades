/**
 * Rock The Upgrades: Accounts Controller (aka Bank)
 * TODO: description
 */

#if defined _account_controller_included
 #endinput
#endif
#define _account_controller_included

#pragma newdecls required

#include "account.inc"
#include "bank_config.inc"
#include "team_bonus.inc"
#include "json/helpers/metastringmap"

// TODO: isolate via longer name
enum BANK_TRANSACTION {
	BANK_BANK_BALANCE,
	BANK_DEPOSIT,
	BANK_WITHDRAW,
	BANK_BURN,
	BANK_TRANSACTION_COUNT
}

// BankConfig config;

/*
	An AccountController is a glorified StringMap that maps SteamIDs (as
	accountKey) to Accounts. These keys fall back to client names if Steam Auth
	Servers are unreachable.
	It handles client connection, account management, map resets, and supports
 	reconnecting clients. For performance, it caches SteamID lookups
*/

methodmap AccountController < MetaStringMap {
	public AccountController(float starting, float multiplier, float limit) {
		AccountController self = view_as<AccountController>(new MetaStringMap());

		self.Keys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);
		self.Bonuses = new ArrayList(sizeof(TeamBonus));

		self.Starting = starting;
		self.Multiplier = multiplier;
		self.Limit = limit;

		return self;
	}

	property bool Authable {
		public get() { bool out; this.Meta.GetValue("authable", out); return out; }
		public set(bool val) { this.Meta.SetValue("authable", val); }
	}

	property float Multiplier {
		public get() { float out; this.Meta.GetValue("multiplier", out); return out; }
		public set(float val) { this.Meta.SetValue("multiplier", val); }
	}

	property float Limit {
		public get() { float out; this.Meta.GetValue("limit", out); return out; }
		public set(float val) { this.Meta.SetValue("limit", val); }
	}

	property float Starting {
		public get() { float out; this.Meta.GetValue("starting", out); return out; }
		public set(float val) { this.Meta.SetValue("starting", val); }
	}

	property ArrayList Keys {
		public get() { return view_as<ArrayList>(this.Meta.GetHandle("keys")); }
		public set(ArrayList val) { this.Meta.SetValue("keys", val); }
	}

	property ArrayList Bonuses {
		public get() { return view_as<ArrayList>(this.Meta.GetHandle("bonuses")); }
		public set(ArrayList val) { this.Meta.SetValue("bonuses", val); }

	}

	// Establish account key and track auth server status
	// Account link/creation happens JIT during OnPlayerSpawn
	public bool Connect(int client) {
		// Maps client -> steamID (fallback: client name)
		bool authorized = this.RegisterAccountKey(client);

		// Track steam auth server outages and recoveries
		this.SetAuthServersReachable(authorized);

		// Create new accounts or reconnect existing ones
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

		// Establish account
		account.Init(client, accountKey);

		// Save
		this.Data.SetArray(accountKey, account, sizeof(Account));
	}

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
		this.Data.SetArray(account.accountKey, account, sizeof(Account));

		// inform caller if revert is needed
		return account.NeedsRevert();
	}

	// Mark account as disconnected. Account persists for current map
	public void Disconnect(int client) {
		this.SetConnection(client, false);
	}

	// Client Name is used while auth servers are down - once restored, convert to SteamID
	public void SetAuthServersReachable(bool reachable) {
		// When auth servers come back online
		if (!this.Authable && reachable) {
			char accountKey[MAX_AUTHID_LENGTH];

			// convert client names to client steamIDs
			for (int client = 1; client <= MaxClients; client++) {
				if (!ValidClient(client)) continue;

				bool authorized = GetClientAuthId(client, AuthId_SteamID64, accountKey, sizeof(accountKey));

				if (!authorized) continue;

				this.Keys.SetString(client, accountKey, MAX_AUTHID_LENGTH);
			}
		}

		this.Authable = reachable;
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

		// Update mapping
		this.Keys.SetString(client, accountKey, MAX_AUTHID_LENGTH);

		// Advise caller if external resource was reachable
		return authorized;
	}

	// Account Lookup via Client -> Account Key(SteamID|Name) -> Account
	// Follow with `SetArray` to save changes
	public bool Fetch(int client, Account a) {
		// Buffer
		char keyName[MAX_AUTHID_LENGTH];

		// Primary Lookup: SteamID
		this.GetAccountKey(client, keyName)

		// Fallback: Client Name (discouraged as spoofable but necessary if auth servers are down)
		if (keyName[0] == '\0') GetClientName(client, keyName, sizeof(keyName));

		// Attempt to write account to param buffer and inform caller of success
		return this.Data.GetArray(keyName, a, sizeof(Account));
	}

	// Write string value to buffer param. Return false if empty
	public bool GetAccountKey(int client, char accountKey[MAX_AUTHID_LENGTH]) {
	 	return this.Keys.GetString(client, accountKey, MAX_AUTHID_LENGTH) > 0;
	}

	public bool AccountExists(int client) {
		char accountKey[MAX_AUTHID_LENGTH];
		this.GetAccountKey(client, accountKey);
		return this.ContainsKey(accountKey);
	}

	// Marks an account as connected or disconnected (for visibility)
	// Disconnected accounts have their spending reset (retains earned currency)
	public void SetConnection(int client, bool connected) {
		Account a;
		this.Fetch(client, a);
		a.connected = connected;

		// Reset spending but retain earned currency on disconnect
		if (!connected) a.Reset(a.Earned());

		// Save Changes
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
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

	// Centralizes logic which had been duplicated among transaction-related methods (Deposit, Withdrawal, etc)
	// NOTE: Attempted and failed to execute callback functions defined within this methodmap
	public void Transaction(float amount, int client, BANK_TRANSACTION transaction) {
		// Fetch account
		Account a; this.Fetch(client, a);

		// Adjust all gains by global multiplier
		amount *= this.Multiplier;

		// Apply amount to the correct field
		switch (transaction) {
			// Currency gained from events or chat commands
			case BANK_DEPOSIT:
				a.earned += amount;
			// Purchasing or refunding upgrades
			case BANK_WITHDRAW:
				a.classData[a.currentClass].spent += amount;
			// Purchasing non-refundables
			case BANK_BURN:
				a.classData[a.currentClass].burnt += amount;
		}

		// enforce currency limit (disabled by default)
		a.earned = this.LimitCurrency(a.earned);

		// Save changes before syncing
		this.Data.SetArray(a.accountKey, a, sizeof(Account));

		// Match the client's currency to account balance
		this.Sync(client);
	}

	// Helper method for readability
	public void Deposit(float amount, int client) {
		this.Transaction(amount, client, BANK_DEPOSIT);
	}

	// Helper method for readability
	public void Withdraw(float amount, int client) {
		this.Transaction(amount, client, BANK_WITHDRAW);
	}

	// Helper method for readability
	public void Burn(float amount, int client) {
		this.Transaction(amount, client, BANK_BURN);
	}

	// Call when refunding upgrades
	public void Refund(float amount, int client) {
		this.Withdraw(-amount, client);
	}

	// Call to remove penalties or restore unrefundable currency (canteen purchases)
	public void Forgive(float amount, int client) {
		this.Burn(-amount, client);
	}

	public void Revert(int client) {
		Account a;
		this.Fetch(client, a);
		a.Revert();
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
		this.Sync(client);
	}

	// Deposit money to all accounts whether connected or not. Optional team filter
	public void DepositAll(float amount, TFTeam team=TFTeam_Unassigned) {
		// Increase bonus starting currency for late joiners
		this.AddBonus(amount, team);

		// Iteration vars
		Account a;
		char accountKey[MAX_AUTHID_LENGTH];

		// Iterate by key
		StringMapSnapshot keys = this.Snapshot();

		// for all accounts
		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

			// Fetch account
			this.Data.GetArray(accountKey, a, sizeof(Account));

			// Optional team filter
			if (!this.ValidTeam(a.client, team)) continue;

			// Get paid
			this.Deposit(amount, a.client);
		}
	}

	public void AddBonus(float amount, TFTeam team) {
		TeamBonus bonus;
		bonus.Set(amount, team);
		this.Bonuses.PushArray(bonus);
	}

	// Used by an admin command - supports "all", "red", "blu", or a specific player
	public void DepositTarget(char target[MAX_NAME_LENGTH], float amount, int replyTo=0) {
		if (StrEqual(target, "all", false)) {
			this.DepositAll(amount);
		} else if (StrEqual(target, "red", false)) {
			this.DepositAll(amount, .team=TFTeam_Red);
		} else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
			this.DepositAll(amount, .team=TFTeam_Blue);
		} else {
			int targetClientId = FindTarget(replyTo, target, true);

			if (ValidClient(targetClientId)) this.Deposit(amount, targetClientId); // if (targetClientId > 0) this.Deposit(amount, targetClientId);
			else ReplyToCommand(replyTo, "[RTU] Could not find player %s", target);
		}
	}

	// Limit maximum earnable currency (disabled by default)
	public float LimitCurrency(float amount) {
		// if no limit or under limit, amount is valid
		if (this.Limit < 0 || amount < this.Limit) return amount;

		return this.Limit;
	}

	// Get the balance for the client's current class
	public float Balance(int client) {
		Account a;
		this.Fetch(client, a);
		return a.Balance();
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

		return RoundToCeil(balance - current);
	}

	// Resolve account balance deltas arising from spent/refunded currency
	public bool ResolveDelta(int client) {
		int delta = this.Delta(client);
		if (delta == 0) return false;

		this.Withdraw(float(delta), client);
		return true;
	}

	// Call from `OnMapEnd`
	public void Wipe() {
		// wipe accounts
		this.Clear();

		// wipe acquisitions
		this.Bonuses.Clear();
		this.Keys.Clear();
	}

	// Reset all accounts in the bank. Syncs connected clients only
	public void ResetAccounts() {
		this.Bonuses.Clear();

		// Iteration vars
		StringMapSnapshot keys = this.Snapshot();
		char accountKey[MAX_AUTHID_LENGTH];
		Account account;

		// Iterate Accounts
		for(int i = 0; i < keys.Length; i++) {
			// Store Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

			// Fetch account
			this.Data.GetArray(accountKey, account, sizeof(Account));

			// Perform reset
			account.Reset(this.StartingCurrency(account.currentTeam));

			// Save Changes
			this.Data.SetArray(accountKey, account, sizeof(Account));

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
	public void PrintToServer() {
		if (this.Size == 0) {
			PrintToServer("[RTU][Accounts] No data");
			return;
		}

		PrintToServer("=================================================================================");
		PrintToServer("[RTU] Accounts [BONUS RED:%.2f] [BONUS BLU:%.2f]", this.StartingCurrency(TFTeam_Red), this.StartingCurrency(TFTeam_Blue));
		PrintToServer("---------------------------------------------------------------------------------");
		PrintToServer("Account Key           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
		PrintToServer("---------------------------------------------------------------------------------");
		this.PrintAccounts();
		PrintToServer("=================================================================================");
	}

	// Debug. Print the account identified by client. -1 prints all accounts.
	public void PrintAccounts() {
		// Iteration vars
		Account a;
		char accountKey[MAX_AUTHID_LENGTH];
		StringMapSnapshot keys = this.Snapshot();
		if (keys.Length == 0) {
			PrintToServer("%20s", "No Data");
			return;
		}
		// Iterate Accounts
		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, sizeof(accountKey));

			bool accountFound = this.Data.GetArray(accountKey, a, sizeof(Account));

			PrintToServer(
				accountFound ?
					a.ToRow() :
					"[RTU][Accounts] No account found for Account Key: %s", accountKey);
		}
	}

	// Debug
	public void PrintAccount(int client) {
		Account a;

		bool accountFound = this.Fetch(client, a);

		ReplyToCommand(client,
			accountFound ?
				a.ToString() :
		 		"[RTU][Accounts] No account found for Client: %d", client);
	}

	// Debug
	public void PrintAccountKeys() {
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			int size = keys.KeyBufferSize(i);
			char[] accountKey = new char[size];
			keys.GetKey(i, accountKey, size);
			PrintToServer("[RTU][Accounts] Index: %d, Account Key: %s", i, accountKey);
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
	public bool ValidTeam(int client, TFTeam team) {
		return team == TFTeam_Unassigned || this.ClientTeam(client) == team;
	}

	public TFTeam ClientTeam(int client) {
		Account a;
		this.Fetch(client, a);
		return a.currentTeam;
	}

	public float StartingCurrency(TFTeam team) {
		return this.Starting + this.Bonus(team);
	}

	public float Bonus(TFTeam team) {
		// Ignored
		if (team == TFTeam_Spectator) return 0.0;

		// Return val
		float result;

		// Iteration Buffer
		TeamBonus bonus;

		// Iterate bonuses
		for (int i = 0; i < this.Bonuses.Length; i++) {
			// Write buffer
			this.Bonuses.GetArray(i, bonus);

			// Filter by team and sum
			if (bonus.AppliesTo(team)) result += bonus.amount;
		}

		return result;
	}
}
