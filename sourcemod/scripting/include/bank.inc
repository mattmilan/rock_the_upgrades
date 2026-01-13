/**
 * Rock The Upgrades: Accounts Controller (aka Bank)
 * TODO: description
 */

#if defined _account_controller_included
 #endinput
#endif
#define _account_controller_included

#pragma newdecls required

#include <rock_the_upgrades/account>
#include <bank/natives>
// #include <bank/printer>
#include <rock_the_upgrades/team_bonus>
#include "json/helpers/metastringmap"
#include "bank/accounts"
#include "bank/connections"
#include "bank/transactions"

// TODO: isolate via longer name
// enum BANK_TRANSACTION {
// 	BANK_BANK_BALANCE,
// 	BANK_DEPOSIT,
// 	BANK_WITHDRAW,
// 	BANK_BURN,
// 	BANK_TRANSACTION_COUNT
// }

/*
	An Bank is a glorified MetaStringMap that maps SteamIDs (as
	  accountKey) to Accounts. These keys fall back to client names if Steam Auth
	  Servers are unreachable, and are restored once connectivity is restored
	It handles client connection, account management, map resets, and supports

*/

methodmap Bank < MetaStringMap {
	public Bank(float starting, float multiplier, float limit) {
		Bank self = view_as<Bank>(new MetaStringMap());

		// Clear on map end
		self.Keys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);
		self.Bonuses = new ArrayList(sizeof(TeamBonus));

		// from Convars
		self.Starting = starting;
		self.Multiplier = multiplier;
		self.Limit = limit;

		return self;
	}

	/* PROPERTIES */

	property bool Authable {
		public native get();
		public native set(bool val);
	}

	property float Multiplier {
		public native get();
		public native set(float val);
	}

	property float Limit {
		public native get();
		public native set(float val);
	}

	property float Starting {
		public native get();
		public native set(float val);
	}

	property ArrayList Keys {
		public native get();
		public native set(ArrayList val);
	}

	property ArrayList Bonuses {
		public native get();
		public native set(ArrayList val);
	}

	// TODO: native
	property TypedStringMap Accounts {
		public get() {
			return this.Data;
		}
	}

	/* STATE MANAGEMENT */

	// Client Name is used while auth servers are down - once restored, convert to SteamID
	public native void SetAuthServersReachable(bool reachable);
	//
	public native float StartingCurrency(TFTeam team);
	// Limit maximum earnable currency (disabled by default)
	public native float LimitCurrency(float amount);
	//
	public native float Bonus(TFTeam team);
	//
	public native void AddBonus(float amount, TFTeam team);
	// Call from `OnMapEnd`
	public native void Wipe();
	// Reset all accounts in the bank. Syncs connected clients only
	public native void ResetAccounts();
	//
	// public native BankPrinter Printer();

	/* CLIENT CONNECTION */

	// // Account link/creation happens JIT during OnPlayerSpawn
	// public native bool Connect(int client);
	// // Mark account as disconnected. May reconnect until account wipe at map end
	// public native void Disconnect(int client);
	// // Marks an account as connected or disconnected (for visibility)
	// // Disconnected accounts have their spending reset (retains earned currency)
	// public native void SetConnection(int client, bool connected);
	// // Maintain an ArrayList of client -> SteamID to limit external resource traffic
	// // Entries may be safely stomped while clients connect/reconnect
	// // While auth servers are down, Client Name is used instead (discouraged as spoofable)
	// // `SetAuthServersReachable` detects restoration and handles recovery of player name -> steamID
	// // Returns true if Steam Auth Servers were reachable
	// public native bool RegisterAccountKey(int client);
	// // Write string value to buffer param. Return false if empty
	// public native bool GetAccountKey(int client, char accountKey[MAX_AUTHID_LENGTH]);

	/* ACCOUNT MANAGEMENT */

	// // Establish account key and track auth server status
	// // Creates new accounts or reconnects existing ones
	// public native void FindOrCreateAccount(int client);

	// // Updates the underlying stringmap with a new account for the given Account Key
	// public native void CreateAccount(int client, char accountKey[MAX_AUTHID_LENGTH]);
	// // Sync data with game state
	// public native bool UpdateAccount(int client, TFClassType classType, TFTeam team);
	// // Account Lookup via Client -> Account Key(SteamID|Name) -> Account
	// // Follow with `SetArray` to save changes
	// public native bool Fetch(int client, Account a);

	/* PLAYER MANAGEMENT */

	// Get the balance for the client's current class
	public native float Balance(int client);
	// Synchronize bank values with actual client currency
	public native bool Sync(int clientID = -1);
	// Get the difference between bank balance and actual currency
	// Used to update the account `spent` value
	// Negative values indicates spending
	// Positive values indicates refunding
	// NOTE: Casting as int to simplify downstream zero comparison
	// TODO: handle nonrefundable currency (burnt)
	public native int Delta(int client);
	// Resolve account balance deltas arising from spent/refunded currency
	public native bool ResolveDelta(int client);
	// Get "the real" currency value for a client (not the bank balance)
	public native float GetClientCurrency(int client);
	// Update "the real" currency value for a client. TREAT AS PRIVATE
	public native bool SetClientCurrency(int client, float amount);
	// Update or create account
	public native bool OnPlayerSpawn(int client, TFClassType classType, TFTeam team);
	//
	public native bool NeedsRevert(int client);
	// Reverts spending for current class after a reset
	public native void Revert(int client);

	/* TRANSACTIONS */

	// Centralizes logic which had been duplicated among transaction-related methods (Deposit, Withdrawal, etc)
	// NOTE: Attempted and failed to execute callback functions defined within this methodmap
	public native void Transaction(float amount, int client, BTType transaction);
	// Used by an admin command - supports "all", "red", "blu", or a specific player
	public native void DepositTarget(char target[MAX_NAME_LENGTH], float amount, int replyTo=0);
	// Deposit money to all accounts whether connected or not. Optional team filter
	public native void DepositAll(float amount, TFTeam team=TFTeam_Unassigned);
	// Helper method for readability
	public native void Deposit(float amount, int client);
	// Helper method for readability
	public native void Withdraw(float amount, int client);
	// Helper method for readability
	public native void Burn(float amount, int client);
	// Call when refunding upgrades
	public native void Refund(float amount, int client);
	// Call to remove penalties or restore unrefundable currency (canteen purchases)
	public native void Forgive(float amount, int client);
}
