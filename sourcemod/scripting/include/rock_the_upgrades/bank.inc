#pragma newdecls required
#include <sourcemod>
#include <tf2>
#include <tf2_stocks>
#include "account.inc"

enum TRANSACTION {
	BALANCE,
	DEPOSIT,
	WITHDRAW,
	BURN,
	TRANSACTION_COUNT
}

ConVar g_Cvar_CurrencyStarting;
ConVar g_Cvar_CurrencyMultiplier;
// ConVar g_Cvar_CurrencyLimit;
// ConVar g_Cvar_UpgradeCostMultiplier;

// Maps currently connected clients to their SteamIDs
ArrayList clientSteamIDs;

// Invoke during `OnMapStart`
void InitBankGlobals() {
#if defined _bank_globals
	return;
#endif
#define _bank_globals
	PrintToServer("[RTU][Bank] Init");

	// A mapping of currently connected clients to their respective SteamIDs
	clientSteamIDs = new ArrayList(1, MaxClients + 1);

	// Starting currency
	g_Cvar_CurrencyStarting = CreateConVar("sm_rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);

	// Global multiplier for all currency events
	g_Cvar_CurrencyMultiplier = CreateConVar("sm_rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);

	// Misc modifiers
	// g_Cvar_UpgradeCostMultiplier = CreateConVar("sm_rtu_upgrade_cost_multiplier", "1.0", "Multiplier for upgrade costs when RTU is activated. A value of zero provides free upgrades [1, 0..].", 0, true, 0.0, false);
	// g_Cvar_CurrencyLimit = CreateConVar("sm_rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);
}

// A bank is a glorified StringMap that maps SteamIDs to Accounts
// The bank handles client connection and account management
// The bank resets between maps, and supports reconnecting players
// For performance, the bank maintains a separate mapping of Client -> SteamID
methodmap Bank < StringMap  {
	// Unassuming constructor
    public Bank() {
		// TODO: call if uncalled (requires guard)
		InitBankGlobals();
		StringMap accounts = new StringMap();
		return view_as<Bank>(accounts);
    }

	public void Connect(int client) {
		// Grab SteamID
		char steamID[MAX_AUTHID_LENGTH]; this.WriteSteamID(client, steamID);

		// Update Client SteamID mapping
		clientSteamIDs.Set(client, steamID);

		// Enroll new clients or reconnect existing ones
		if (!this.ContainsKey(steamID)) this.Enroll(client, steamID);
		else this.SetConnection(client, true);
	}

	public void Disconnect(int client) {
		this.SetConnection(client, false);
	}

	public void SetConnection(int client, bool connected) {
		Account a;
		this.Fetch(client, a);
		a.connected = connected;
		this.SetArray(a.steamID, a, sizeof(Account));
	}

	public void SetClass(int client, TFClassType classType) {
		Account a;
		this.Fetch(client, a);
		a.SetClass(classType);
		this.SetArray(a.steamID, a, sizeof(Account));
	}

	// Account Lookup via Client -> SteamID -> Account
	// Follow with `SetArray` to save changes
	public bool Fetch(int client, Account a) {
		char steamID[MAX_AUTHID_LENGTH];

		if (!this.WriteSteamID(client, steamID)) {
			PrintToServer("[RTU][Bank] Failed to fetch account for client %d", client);
			return false;
		}

		return this.GetArray(steamID, a, sizeof(Account));
	}

	// Write client SteamID to buffer param
	public bool WriteSteamID(int client, char steamID[MAX_AUTHID_LENGTH]) {
		if (!GetClientAuthId(client, AuthId_SteamID64, steamID, sizeof(steamID))) {
			PrintToServer("[RTU][Bank] Failed to get SteamID for client %d", client);
			return false;
		}

		return true;
	}

	// Enroll a new account for the given SteamID
	// Accounts help manage currency; banks simply manage accounts
	// TODO: Remember to set Account CurrentClass during class-related events
	public void Enroll(int client, char steamID[MAX_AUTHID_LENGTH]) {
		PrintToServer("[RTU][Bank] Initializing bank account for SteamID: %s", steamID);

		// Build the account
		Account account;
		account.Init(client, steamID, this.StartingCurrency());

		// Store the account
		this.SetArray(steamID, account, sizeof(Account));
		this.PrintAccount(steamID);
	}

	// Centralizes logic which had been duplicated among transaction-related methods (Deposit, Withdrawal, etc)
	public void Transaction(float amount, int client, TRANSACTION transaction) {
		Account a; this.Fetch(client, a);
		amount *= g_Cvar_CurrencyMultiplier.FloatValue;
		switch (transaction) {
			// Currency gained from events or chat commands
			case DEPOSIT:
				a.earned += amount;
			// Purchasing or refunding upgrades
			case WITHDRAW:
				a.classData[a.currentClass].spent += amount;
			// Purchasing non-refundables
			case BURN:
				a.classData[a.currentClass].burnt += amount;
		}
		// this.ResolveDelta(client);
		this.SetArray(a.steamID, a, sizeof(Account));
		this.Sync(client);
	}

	// Helper method for readability
	public void Deposit(float amount, int client) {
		PrintToServer("[RTU][Bank] Depositing %.2f to client %d", amount, client);
		this.Transaction(amount, client, DEPOSIT);
	}

	// Helper method for readability
	public void Withdraw(float amount, int client) {
		// PrintToServer("[RTU][Bank] Withdrawing %.2f from client %d", amount, client);
		this.Transaction(amount, client, WITHDRAW);
	}

	// Helper method for readability
	public void Burn(float amount, int client) {
		this.Transaction(amount, client, BURN);
	}

	// Call when refunding upgrades
	// public void Refund(float amount, int client) {
	// 	this.Withdraw(-amount, client);
	// }

	public void Refund(int client) {
		Account a; this.Fetch(client, a);
		float refundAmount = a.Spent();
		PrintToServer("[RTU][Bank] Refunding %.2f to client %d", refundAmount, client);
		this.Withdraw(-refundAmount, client);
	}

	// Call to remove penalties or restore unrefundable currency (canteen purchases)
	public void Forgive(float amount, int client) {
		this.Burn(-amount, client);
	}

	// Deposit money to all connects whether connected or not. Optional team filter
	public void DepositAll(float amount, int team=-1) {
		// Iteration vars
		Account a;
		int clientID;
		// char steamID[MAX_AUTHID_LENGTH];

		// Iterate over all accounts
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab SteamID (key)
			int size = keys.KeyBufferSize(i);
			char[] steamID = new char[size];
			keys.GetKey(i, steamID, size);

			// Fetch account
			this.GetArray(steamID, a, sizeof(Account));
			clientID = a.client;

			// Optional team filter
			if (!this.ValidTeam(clientID, team)) continue;

			// Get paid
			this.Deposit(amount, clientID);
		}
	}

	// Used by an admin command - converts it's argument to a group or an individual client
	public bool DepositTarget(char target[MAX_NAME_LENGTH], float amount, int replyTo=0) {
		if (StrEqual(target, "all", false)) {
			this.DepositAll(amount);
		} else if (StrEqual(target, "red", false)) {
			this.DepositAll(amount, .team=TFTeam_Red);
		} else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
			this.DepositAll(amount, .team=TFTeam_Blue);
		} else {
			int targetClientId = FindTarget(replyTo, target, true);
			if (targetClientId > 0) this.Deposit(amount, targetClientId);
			else return false;
		}

		return true;
	}

	// Get the balance for a client - will vary depending on client's class
	public float Balance(int client) {
		Account a;
		this.Fetch(client, a);
		return a.Balance();
	}

	// Used to set account currency during enrollments and resets
	public float StartingCurrency() {
		return g_Cvar_CurrencyStarting.FloatValue
			* g_Cvar_CurrencyMultiplier.FloatValue;
	}

	// Synchronize entity props with bank values
	public bool Sync(int clientID = -1) {
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
		// int delta = RoundToCeil(current - balance);
		int delta = RoundToCeil(balance - current);
		// PrintToServer("[RTU][Bank] Delta for client %d: %d (Current: %.2f, Balance: %.2f)", client, delta, current, balance);
		return delta;
		// return RoundToCeil(
		// 		this.GetClientCurrency(client)
		// 	  - this.Balance(client));
	}

	// Resolve currency deltas arising from purchases and refunds
	// Triggers a bank transaction which also syncs currency with balance
	// TODO: handle nonrefundable currency (burnt)
	public void ResolveDelta(int client) {
		// TODO: need to ensure Sync is called at least once prior to ResolveDelta
		// this.Sync(client);
		int delta = this.Delta(client);
		this.Withdraw(float(delta), client);
	}

	// Reset all accounts in the bank. Syncs connected clients
	public void ResetAccounts() {
		// Iterate over all accounts
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab SteamID (key)
			int size = keys.KeyBufferSize(i);
			char[] steamID = new char[size];
			keys.GetKey(i, steamID, size);

			// Write account to buffer
			Account a;
			this.GetArray(steamID, a, sizeof(Account));

			// Perform reset
			a.Reset(this.StartingCurrency());

			// Save Changes
			this.SetArray(steamID, a, sizeof(Account));

			// Sync connected clients
			if (a.connected) this.Sync(a.client);
		}
	}

	public void ResetAccount(int client) {
		Account a;
		this.Fetch(client, a);
		a.Reset(this.StartingCurrency());
		this.SetArray(a.steamID, a, sizeof(Account));
		this.Sync(client);
	}

	// Get "the real" currency value for a client
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
	public void PrintToServer(int client = -1) {
		if (this.Size == 0) {
			PrintToServer("[RTU][Bank] No accounts to display.");
			return;
		}

		PrintToServer("=================================================================================");
		PrintToServer("[RTU][Bank] Accounts");
		PrintToServer("---------------------------------------------------------------------------------");
		PrintToServer("SteamID           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
		PrintToServer("---------------------------------------------------------------------------------");
		this.PrintAccounts(client);
		PrintToServer("=================================================================================");
	}

	// Debug. Print the account identified by client. -1 prints all accounts.
	public void PrintAccounts(int client) {
		// Iteration var
		char steamID[MAX_AUTHID_LENGTH];

		// Single account
		if (client > 0) {
			this.WriteSteamID(client, steamID);
			this.PrintAccount(steamID);
			return;
		}

		// Iterate over all accounts
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab SteamID (key)
			keys.GetKey(i, steamID, sizeof(steamID));
			this.PrintAccount(steamID);
		}
	}
	// typedef SayChatHook = function Action (int client, const char[] text);
	// public void Keys(Action callback(Handle timer)) {
	// 	StringMapSnapshot keys = this.Snapshot();

	// 	for(int i = 0; i < keys.Length; i++) {
	// 		// Grab SteamID (key)
	// 		int size = keys.KeyBufferSize(i);
	// 		char[] steamID = new char[size];
	// 		keys.GetKey(i, steamID, size);
	// 		callback(steamID);
	// 	}
	// }


	// public void Keys(function callback(char steamID[MAX_AUTHID_LENGTH])) {
	// 	StringMapSnapshot keys = this.Snapshot();

	// 	for(int i = 0; i < keys.Length; i++) {
	// 		// Grab SteamID (key)
	// 		int size = keys.KeyBufferSize(i);
	// 		char[] steamID = new char[size];
	// 		keys.GetKey(i, steamID, size);
	// 		callback(steamID);
	// 	}
	// }
	// Debug
	public void PrintAccount(char steamID[MAX_AUTHID_LENGTH]) {
		Account a;
		bool accountFound = this.GetArray(steamID, a, sizeof(Account));
		PrintToServer(accountFound ?
					  a.ToString() :
		 			  "[RTU][Bank] No account found for SteamID: %s", steamID);
	}

	// Debug
	public void PrintSteamIDs() {
		StringMapSnapshot keys = this.Snapshot();

		for(int i = 0; i < keys.Length; i++) {
			// Grab SteamID (key)

			int size = keys.KeyBufferSize(i);
			char[] steamID = new char[size];
			keys.GetKey(i, steamID, size);
			PrintToServer("[RTU][Bank] Index: %d, SteamID: %s", i, steamID);
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

	// Optional team filter, default: pass
	public bool ValidTeam(int client, int team) {
		return team == -1 || GetClientTeam(client) == team;
	}
}
