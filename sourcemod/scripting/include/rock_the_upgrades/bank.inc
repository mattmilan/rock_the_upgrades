/**
 * Rock The Upgrades: Accounts Controller (aka Bank)
 * TODO: description
 */

#if defined _account_controller_included
 #endinput
#endif
#define _account_controller_included

#pragma newdecls required

#include "account"

#include "team_bonus"
#include "json/helpers/metastringmap"
#include "bank/printer"
// #include "bank/accounts"
// #include "bank/connections"
// #include "bank/transactions"


ConVar g_Cvar_CurrencyStarting;   // Currency for new accounts before bonuses
ConVar g_Cvar_CurrencyMultiplier; // Global multiplier for all currency gains
ConVar g_Cvar_CurrencyLimit;      // Optionally limit earnable currency


// TODO: isolate via longer name
enum BANK_TRANSACTION {
	BANK_BANK_BALANCE,
	BANK_DEPOSIT,
	BANK_WITHDRAW,
	BANK_BURN,
	BANK_TRANSACTION_COUNT
}

/*
	An Bank is a glorified MetaStringMap that maps SteamIDs (as
	  accountKey) to Accounts. These keys fall back to client names if Steam Auth
	  Servers are unreachable, and are restored once connectivity is restored
	It handles client connection, account management, map resets, and supports

*/
methodmap Bank < MetaStringMap {
	public Bank(float starting, float multiplier, float limit) {
		Bank self = view_as<Bank>(new MetaStringMap());
		PrintToServer("[RTU][Bank] Initializing Bank with Starting: %.2f, Multiplier: %.2f, Limit: %.2f", starting, multiplier, limit);
		// Clear on map end
		self.Keys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);
		self.Bonuses = new ArrayList(sizeof(TeamBonus));

		// from Convars
		self.Starting = starting;
		self.Multiplier = multiplier;
		self.Limit = limit;

		return self;
	}

	public void PrintAccount(int client) {
		BankPrinter.PrintAccount(this, client);
	}

	public void PrintToServer() {
		BankPrinter.PrintToServer(this);
	}

	public static Bank Init() {
		g_Cvar_CurrencyStarting = CreateConVar("rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);
		g_Cvar_CurrencyMultiplier = CreateConVar("rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyLimit = CreateConVar("rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);

		return new Bank(
 			g_Cvar_CurrencyStarting.FloatValue,
        	g_Cvar_CurrencyMultiplier.FloatValue,
        	g_Cvar_CurrencyLimit.FloatValue
		);
	}

	/* PROPERTIES */

	property bool Authable {
		public get()          	  { return this.Meta.GetBool("authable"); }
		public set(bool val) 	  { this.Meta.SetBool("authable", val); }
	}

	property float Multiplier {
		public get()          	  { return this.Meta.GetFloat("multiplier"); }
		public set(float val) 	  { this.Meta.SetFloat("multiplier", val); }
	}

	property float Limit {
		public get()          	  { return this.Meta.GetFloat("limit"); }
		public set(float val) 	  { this.Meta.SetFloat("limit", val); }
	}

	property float Starting {
		public get()          	  { return this.Meta.GetFloat("starting"); }
		public set(float val) 	  { this.Meta.SetFloat("starting", val); }
	}

	property ArrayList Keys {
		public get() 			  { return view_as<ArrayList>(this.Meta.GetHandle("keys")); }
		public set(ArrayList val) { this.Meta.SetHandle("keys", val); }
	}

	property ArrayList Bonuses {
		public get() 			  { return view_as<ArrayList>(this.Meta.GetHandle("bonuses")); }
		public set(ArrayList val) { this.Meta.SetHandle("bonuses", val); }
	}

	property TypedStringMap Accounts {
		public get()          	  { return this.Data; }
	}

	/* CLIENT CONNECTION */

	public bool Connect(int client) {
		// Maps client -> steamID (fallback: client name)
		bool authorized = this.RegisterAccountKey(client);

		// Track steam auth server outages and recoveries
		this.SetAuthServersReachable(authorized);

		// Create new accounts or reconnect existing ones
		this.FindOrCreateAccount(client);

		return authorized;
	}

	public void Disconnect(int client) {
		this.SetConnection(client, false);
	}

	public void SetConnection(int client, bool connected) {
		Account a; /*=*/ this.Fetch(client, a);
		a.connected = connected;

		// Reset spending but retain earned currency on disconnect
		if (!connected) a.Reset(a.Earned());

		// Save Changes
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
	}

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
	/* TRANSACTIONS */

	public void Transaction(float amount, int client, BANK_TRANSACTION transaction) {
		// Fetch account
		Account a; /*=*/ this.Fetch(client, a);

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
		// NOTE: metastringmap does not lift SetArray to Data - it points to Meta
		this.Data.SetArray(a.accountKey, a, sizeof(Account));

		// Match the client's currency to account balance
		this.Sync(client);
	}

	public void Deposit(float amount, int client) {
		this.Transaction(amount, client, BANK_DEPOSIT);
	}

	public void Withdraw(float amount, int client) {
		this.Transaction(amount, client, BANK_WITHDRAW);
	}

	public void Burn(float amount, int client) {
		this.Transaction(amount, client, BANK_BURN);
	}

	public void DepositAll(float amount, TFTeam team=TFTeam_Unassigned) {
		// Late-joiners receive increased starting currency
		this.AddBonus(amount, team);

		// Iteration vars
		Account a;
		char accountKey[MAX_AUTHID_LENGTH];
		StringMapSnapshot keys = this.Snapshot();

		// Iterate Accounts
		for(int i = 0; i < keys.Length; i++) {
			// Grab Account Key (SteamID|Name)
			keys.GetKey(i, accountKey, sizeof(accountKey));

			// Fetch account
			this.Data.GetArray(accountKey, a, sizeof(Account));

			// Get paid if team matches optional filter
			if (FilterTeam(a.currentTeam, team)) this.Deposit(amount, a.client);
		}
	}

	public void DepositTarget(float amount, char target[MAX_NAME_LENGTH], int replyTo) {
		if (StrEqual(target, "all", false)) {
			this.DepositAll(amount);
		} else if (StrEqual(target, "red", false)) {
			this.DepositAll(amount, .team=TFTeam_Red);
		} else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
			this.DepositAll(amount, .team=TFTeam_Blue);
		} else {
			int targetClientId = FindTarget(replyTo, target, true);

			if (ValidClient(targetClientId)) this.Deposit(amount, targetClientId);
			else ReplyToCommand(replyTo, "[RTU] Could not find player %s", target);
		}
	}
	/* ACCOUNT MANAGEMENT */

	public void FindOrCreateAccount(int client) {
		// Establish account key
		char accountKey[MAX_AUTHID_LENGTH];
		this.GetAccountKey(client, accountKey);

		// Create new account or reconnect existing one
		if (this.ContainsKey(accountKey)) this.SetConnection(client, true);
		else this.CreateAccount(client, accountKey);
	}

	public void CreateAccount(int client, char accountKey[MAX_AUTHID_LENGTH]) {
		// Buffer
		Account account;

		// Establish account
		account.Init(client, accountKey);

		// Save
		this.Data.SetArray(accountKey, account, sizeof(Account));
	}

	// TODO: should this check authable to switch between id/name lookup?
	public bool GetAccountKey(int client, char accountKey[MAX_AUTHID_LENGTH]) {
		// Write buffer
		bool exists = (this.Keys.GetString(client, accountKey, MAX_AUTHID_LENGTH)) > 0;

		return exists;
	}

	public bool UpdateAccount(int client, TFClassType classType, TFTeam team) {
		// Find account
		Account account;
		bool found = this.Fetch(client, account);
		if (!found) return false;

		// apply updates
		account.currentClass = classType;
		account.currentTeam = team;

		// TODO: find a better place for this
		if (!account.activated) account.Activate(this.StartingCurrency(team));

		// Save changes
		this.Data.SetArray(account.accountKey, account, sizeof(Account));
		return true;
	}

	public bool Fetch(int client, Account a) {
		// Get account key
		char accountKey[MAX_AUTHID_LENGTH];
		this.GetAccountKey(client, accountKey);

		// Lookup account
		return this.Data.GetArray(accountKey, a, sizeof(Account));
	}

	/* PLAYER MANAGEMENT */

	public float Balance(int client) {
		Account a; /*=*/ this.Fetch(client, a);
		return a.Balance();
	}

	public bool Sync(int clientID = -1) {
		// Single client sync
		if (clientID > 0) {
			return this.SetClientCurrency(clientID, this.Balance(clientID));
		}

		// Multi-client sync
		for (int client = 1; client <= MaxClients; client++) {
			if (!ValidClient(client)) continue;

			if (!this.Sync(client)) return false;
		}

		return true;
	}

	public int Delta(int client) {
		float realCurrency = this.GetClientCurrency(client);
		float bankBalance = this.Balance(client);

		return RoundToNearest(bankBalance - realCurrency);
	}
	public bool ResolveDelta(int client) {
		int delta = this.Delta(client);
		if (delta == 0) return false;

		this.Withdraw(float(delta), client);

		return true;
	}

	public float GetClientCurrency(int client) {
		return ValidClient(client) ?
			float(GetEntProp(client, Prop_Send, "m_nCurrency")) :
			-1.0;
	}

	public bool SetClientCurrency(int client, float amount) {
		bool valid = ValidClient(client);
		if (valid) SetEntProp(client, Prop_Send, "m_nCurrency", RoundToCeil(amount));
		return valid;
	}

	public bool OnPlayerSpawn(int client, TFClassType classType, TFTeam team) {
		// Update account
		bool revert = this.UpdateAccount(client, classType, team);
		if (!revert) this.Sync(client);

		return revert;
	}

	public bool NeedsRevert(int client) {
		Account a; /*=*/ this.Fetch(client, a);
		return a.NeedsRevert();
	}

	public void Revert(int client) {
		Account a; /*=*/ this.Fetch(client, a);
		a.Revert();

		// Save changes
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
		this.Sync(client);
	}
	/* STATE MANAGEMENT */

	public void SetAuthServersReachable(bool reachable) {
		// Servers Reactivated
		if (reachable && !this.Authable) this.RestoreAuth();

		this.Authable = reachable;
	}

	public void RestoreAuth() {
		// Connections.RestoreAuth(this);
		// Iterator buffers
        char nameKey[MAX_AUTHID_LENGTH];
        char authKey[MAX_AUTHID_LENGTH];
        Account account;

        for (int i = 0; i < this.Keys.Length; i++) {
            // Write name
            this.Keys.GetString(i, nameKey, MAX_AUTHID_LENGTH);

            // Skip missing (dont use fetch here)
            if (!this.Accounts.GetArray(nameKey, account, sizeof(Account))) continue;

            // Write authID
            GetClientAuthId(i, AuthId_SteamID64, authKey, MAX_AUTHID_LENGTH);

            // Update key map
            this.Keys.SetString(i, authKey, MAX_AUTHID_LENGTH);

            // Update account
            account.accountKey = authKey;

            // Swap index
            this.Accounts.SetArray(authKey, account, sizeof(Account));
            this.Accounts.Remove(nameKey);
        }
	}

	public float StartingCurrency(TFTeam team) {
		return this.Starting + this.Bonus(team);
	}

	public float LimitCurrency(float amount) {
		if (this.Limit < amount && this.Limit > 0.0)
			 return this.Limit;
		else return amount;
	}

	public float Bonus(TFTeam team) {
		float result;
		TeamBonus bonus;

		// Iterate bonuses
		for (int i = 0; i < this.Bonuses.Length; i++) {
			this.Bonuses.GetArray(i, bonus);

			if (bonus.AppliesTo(team)) result += bonus.amount;
		}

		return result;
	}

	public void AddBonus(float amount, TFTeam team) {
		TeamBonus bonus;

		bonus.Set(amount, team);

		this.Bonuses.PushArray(bonus);
	}

	public void Wipe() {
		// wipe accounts
		this.Clear();

		// wipe acquisitions
		this.Bonuses.Clear();
		this.Keys.Clear();
	}

	public void ResetAccounts() {
		this.Bonuses.Clear();

		// Iteration vars
		StringMapSnapshot keys = this.Snapshot();
		char accountKey[MAX_AUTHID_LENGTH];
		Account account;

		for (int i = 0; i < keys.Length; i++) {
			// Write key
			keys.GetKey(i, accountKey, sizeof(accountKey));

			// Lookup account
			if (!this.Data.GetArray(accountKey, account, sizeof(Account))) continue;

			// Reset account
			account.Reset(this.StartingCurrency(account.currentTeam));

			// Save changes
			this.Data.SetArray(accountKey, account, sizeof(Account));

			if (account.connected) this.Sync(account.client);
		}
	}
}
