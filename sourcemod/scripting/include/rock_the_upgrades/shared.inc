/**
 * Rock The Upgrades: Shared
 * Centralized shared functions and utilities
 */

#if defined _rock_the_upgrades_shared_included
 #endinput
#endif
#define _rock_the_upgrades_shared_included

enum BankTransactionType {
	Bank_Balance,
	Bank_Deposit,
	Bank_Withdraw,
	Bank_Burn,
	BankTransactionType_Count
}


// Txn is a common substitution for Transaction
enum struct BankTxn {
	ArrayList txnItems;

	void Init() {
		this.txnItems = new ArrayList(sizeof(BankTxnItem));
	}

	void AddDeposit(float amount, int client=-1, TFTeam team=TFTeam_Spectator) {
		this.AddTxnItem(amount, client, team, Bank_Deposit);
	}

	void AddWithdraw(float amount, int client=-1, TFTeam team=TFTeam_Spectator) {
		this.AddTxnItem(amount, client, team, Bank_Withdraw);
	}

	void AddBurn(float amount, int client=-1, TFTeam team=TFTeam_Spectator) {
		this.AddTxnItem(amount, client, team, Bank_Burn);
	}

	void AddTxnItem(float amount, int client=-1, TFTeam team=TFTeam_Spectator, BankTransactionType txnType) {
		BankTxnItem item;
		item.amount = amount;
		item.client = client;
		item.team = team;
		item.txnType = txnType;
		item.Init();
		this.txnItems.PushArray(item, sizeof(BankTxnItem));
	}
}

enum struct BankTxnItem {
	float amount;
	int client;
	ArrayList clients;
	TFTeam team;
	BankTransactionType txnType;

	// TODO: memoize red/blue/all client ids
	void Init() {
		this.clients = new ArrayList();

		if (this.client > 0)
			this.clients.Push(this.client);
		else
			this.GetClients();

	}

	void GetClients() {
		for (int i = 0; i <= MaxClients; i++) {
			if (!ValidClient(i)) continue;

			TFTeam team = view_as<TFTeam>(GetClientTeam(i));

			if (!FilterTeam(team, this.team)) continue;

			this.clients.Push(i);
		}
	}
}

bool ValidPlayer(int client, TFClassType classType, TFTeam team) {
	return ValidClient(client) &&
		   ValidClass(classType) &&
		   ValidTeam(team);
}

// // Duplicated in Bank module - consider centralizing
bool ValidClient(int client, bool checkConnected=true, bool checkInGame=true, bool checkFake=true) {
	// REQUIRED: within integer bounds
	if (client < 1 || client > MaxClients) return false;

	// OPTIONAL: connected
	if (checkConnected && !IsClientConnected(client)) return false;

	// OPTIONAL: in-game
	if (checkInGame && !IsClientInGame(client)) return false;

	// OPTIONAL: is human
	if (checkFake && IsFakeClient(client)) return false;

	return true;
}

// Not Unknown and within TF2 Class Range
bool ValidClass(TFClassType classType) {
	return classType > TFClass_Unknown && classType <= TFClass_Engineer;
}

// Not Unassigned or Spectator
bool ValidTeam(TFTeam team) {
	return team == TFTeam_Red || team == TFTeam_Blue;
}

bool FilterTeam(TFTeam clientTeam, TFTeam filter) {
	return filter == clientTeam
		|| filter == TFTeam_Unassigned;
}
