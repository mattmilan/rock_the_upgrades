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
#include "shared"
#include "team_bonus"
#include "json/helpers/metastringmap"
#include "bank/printer"
#include "auth_keys.inc"

ConVar g_Cvar_CurrencyStarting;   // Currency for new accounts before bonuses
ConVar g_Cvar_CurrencyMultiplier; // Global multiplier for all currency gains
ConVar g_Cvar_CurrencyLimit;      // Optionally limit earnable currency


// TODO: isolate via longer name
// enum BANK_TRANSACTION {
// 	BANK_BANK_BALANCE,
// 	BANK_DEPOSIT,
// 	BANK_WITHDRAW,
// 	BANK_BURN,
// 	BANK_TRANSACTION_COUNT
// }

static Bank g_Bank;

/*
	An Bank is a glorified MetaStringMap that maps SteamIDs (as
	  accountKey) to Accounts. These keys fall back to client names if Steam Auth
	  Servers are unreachable, and are restored once connectivity is restored
	It handles client connection, account management, map resets, and supports

*/
methodmap Bank < MetaStringMap {
	public static Bank Instance() {
		if (g_Bank == null) Bank.Init();

		return g_Bank;
	}

	public static void Init() {
		if (g_Bank != null) return;
		PrintToServer("[RTU][Bank] Initializing Bank");
		g_Cvar_CurrencyStarting = CreateConVar("rtu_currency_starting", "250.0", "Starting amount of currency for players. Negative values incur a debt. Don't blame me - blame Merasmus. [250, -inf..inf]", 0, false, 0.0, false);
		g_Cvar_CurrencyMultiplier = CreateConVar("rtu_currency_multiplier", "1.0", "Global multiplier for all currency gains when RTU is activated [1, 0..]", 0, true, 0.0, false);
		g_Cvar_CurrencyLimit = CreateConVar("rtu_currency_limit", "-1", "Maximum amount of currency a player can earn, -1 for unlimited [unlimited, -1..]", 0, true, -1.0, false);

		g_Bank = view_as<Bank>(new MetaStringMap());
		g_Bank.Bonuses = new ArrayList(sizeof(TeamBonus));
	}

	public static void Transact(BankTxn txn) {
		PrintToServer("[RTU][Bank] Processing Bank Transaction with %d items", txn.txnItems.Length);
		for (int i = 0; i < txn.txnItems.Length; i++) {
			BankTxnItem txnItem; txn.txnItems.GetArray(i, txnItem, sizeof(BankTxnItem));
			PrintToServer("[RTU][Bank] Processing transaction item %d: Amount %.2f, Type %d for %d clients", i, txnItem.amount, txnItem.txnType, txnItem.clients.Length);
			ArrayList clients = txnItem.clients;
			for (int j = 0; j < clients.Length; j++) {
				PrintToServer("[RTU][Bank] Executing transaction item %d for ClientID index %d", i, j);
				int client = clients.Get(j);
				Bank.Instance().Execute(txnItem.amount, client, txnItem.txnType);
			}
		}
	}

	public void PrintAccount(int client) {
		BankPrinter.PrintAccount(this, client);
	}

	public void PrintToServer() {
		BankPrinter.PrintToServer(this);
	}

	/* PROPERTIES */

	property bool Authable {
		public get()          	  { return this.Meta.GetBool("authable"); }
		public set(bool val) 	  { this.Meta.SetBool("authable", val); }
	}

	property float Multiplier {
		public get()          	  { return g_Cvar_CurrencyMultiplier.FloatValue; }
	}

	property float Limit {
		public get()          	  { return g_Cvar_CurrencyLimit.FloatValue; }
	}

	property float Starting {
		public get()          	  { return g_Cvar_CurrencyStarting.FloatValue; }
	}

	property ArrayList Bonuses {
		public get() 			  { return view_as<ArrayList>(this.Meta.GetHandle("bonuses")); }
		public set(ArrayList val) { this.Meta.SetHandle("bonuses", val); }
	}

	property TypedStringMap Accounts {
		public get()          	  { return this.Data; }
	}

	/* CLIENT CONNECTION */

	public void Connect(int client) {
		// Maps client -> steamID (fallback: client name)
		AuthKeys.Register(client);

		// Create new accounts or reconnect existing ones
		this.FindOrCreateAccount(client);
	}

	public void Disconnect(int client) {
		this.SetConnection(client, false);
	}

	public void SetConnection(int client, bool connected) {
		Account a; /*=*/ this.Fetch(a, client);
		a.connected = connected;

		// Reset spending but retain earned currency on disconnect
		if (!connected) a.Reset(a.Earned());

		// Save Changes
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
	}

	/* TRANSACTIONS */

	public void Execute(float amount, int client, BankTransactionType txn) {
		// Fetch account
		Account a; /*=*/ this.Fetch(a, client);

		// Adjust all gains by global multiplier
		amount *= this.Multiplier;
		PrintToServer("[RTU][Bank] Executing transaction for Client %d: Amount %.2f, Type %d (Post-Multiplier: %.2f)", client, amount, txn, amount);
		// Apply amount to the correct field
		switch (txn) {
			// Currency gained from events or chat commands
			case Bank_Deposit:
				a.earned += amount;
			// Purchasing or refunding upgrades
			case Bank_Withdraw:
				a.classData[a.currentClass].spent += amount;
			// Purchasing non-refundables
			case Bank_Burn:
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
		BankTxn txn; txn.Init(); txn.AddDeposit(amount, client);
		Bank.Transact(txn);
		// this.Execute(amount, client, Bank_Deposit);
	}

	public void Withdraw(float amount, int client) {
		BankTxn txn; txn.Init(); txn.AddWithdraw(amount, client);
		Bank.Transact(txn);
		// this.Execute(amount, client, Bank_Withdraw);
	}

	public void Burn(float amount, int client) {
		BankTxn txn; txn.Init(); txn.AddBurn(amount, client);
		Bank.Transact(txn);
		// this.Execute(amount, client, Bank_Burn);
	}

	public ArrayList AuthKeysByTeam(TFTeam team) {
		ArrayList teamAuthKeys = new ArrayList();
		switch (team) {
			case TFTeam_Red:		{} // red
			case TFTeam_Blue: 		{} // blu
			case TFTeam_Unassigned: {} // all
			default: 				{} // none
		}
		return teamAuthKeys;
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
		AuthKeys.Get(client, accountKey);

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

	public bool UpdateAccount(int client, TFClassType classType, TFTeam team) {
		// Find account
		Account account;
		bool found = this.Fetch(account, client);
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

	public bool Fetch(Account a, int client) {
		char accountKey[MAX_AUTHID_LENGTH]
		AuthKeys.Get(client, accountKey);

		// Primary lookup by SteamID - should pass 99.9% of the time
		if (this.Data.GetArray(accountKey, a, sizeof(Account))) return true;

		// Fallback to PlayerName
		char nameKey[MAX_AUTHID_LENGTH];
		AuthKeys.Get(client, nameKey, AuthModePlayerName);

		// Neither key exists - somewhat impossible
		if (!this.Data.GetArray(nameKey, a, sizeof(Account))) return false;

		// Auth servers are down - continue using PlayerName fallback
		if (AuthKeys.Mode() == AuthModePlayerName) return true;

		// Auth Servers restored - swap keys
		this.Data.SetArray(accountKey, a, sizeof(Account));
		this.Data.Remove(nameKey);

		return true;
	}

	/* PLAYER MANAGEMENT */

	public float Balance(int client) {
		Account a; /*=*/ this.Fetch(a, client);
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
		Account a; /*=*/ this.Fetch(a, client);
		return a.NeedsRevert();
	}

	public void Revert(int client) {
		Account a; /*=*/ this.Fetch(a, client);
		a.Revert();

		// Save changes
		this.Data.SetArray(a.accountKey, a, sizeof(Account));
		this.Sync(client);
	}

	/* STATE MANAGEMENT */

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
		// this.Keys.Clear();
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
