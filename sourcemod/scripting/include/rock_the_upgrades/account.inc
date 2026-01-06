/**
 * Rock The Upgrades: Account
 * TODO: description
 */

#pragma newdecls required
#pragma semicolon 1

// A support for Account to track spending per class
enum struct ClassData {
	float spent;
	float burnt;

	// Indicate whether class was played and may need a reset
	// bool played;

	// whether or not we believe this class has any upgrades to refund
	// bool upgraded;

	// Revert upgrades for current class after next spawn. Set on every reset
	bool markedForRevert;

	// TODO: track weapons upgraded per class
	// MAGIC NUMBER 16: max # of weapons a single class could juggle?
	// int weaponsUpgraded[16];


	void Reset() {
		// Nothing to reset if nothing spent
		if (this.spent < 0.001) return;

		this.spent = 0.0;
		this.burnt = 0.0;
		// this.played = false;
		// this.upgraded = false;
		this.markedForRevert = true;
	}
}

// A data structure used by bank to track player currency
enum struct Account {
	// Player-unique identifier, useful when reconnecting
	char accountKey[MAX_AUTHID_LENGTH + 1];

    // Client ID for reference
    int client;

    // Track connection status for debugging
    bool connected;

	// track earnings independent of class
	float earned;

	// track refundable and non-refundable spending per class. For interation:
	TFClassType currentClass;
	ClassData classData[10];

	void Init(int client, char accountKey[MAX_AUTHID_LENGTH], float startingCurrency) {
		strcopy(this.accountKey, sizeof(this.accountKey), accountKey);
		this.client = client;
		this.earned = startingCurrency;
		this.connected = true;
	}

	float Earned() {
		return this.earned;
	}

	float Spent(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.classData[classType].spent;
	}

	float Burned(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.classData[classType].burnt;
	}

	bool NeedsRevert(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.classData[classType].markedForRevert;
	}

	bool Revert() {
		this.classData[this.currentClass].markedForRevert = false;
		this.classData[this.currentClass].spent = 0.0;

		return true;
	}

	void Reset(float startingCurrency) {
		this.earned = startingCurrency;

		for (TFClassType classType = TFClass_Scout; classType <= TFClass_Engineer; classType++) {
			this.classData[classType].Reset();
			if (classType == this.currentClass) {
				this.classData[classType].markedForRevert = false;
			}
		}
	}

	void SetClass(TFClassType classType) {
		this.currentClass = classType;
	}

	void Refund() {
		this.classData[this.currentClass].spent = 0.0;
	}

	// NOTE: I would use TFClass_Unknown but that might represent Spectators
	// TODO: i bet that using -1 will break since the enum doesnt contain that value
	float Balance(TFClassType classType = view_as<TFClassType>(-1)) {
		// Default to current class
		if (classType == view_as<TFClassType>(-1))
			 classType = this.currentClass;

		return this.earned
			 - this.classData[classType].spent
			 - this.classData[classType].burnt;
	}

	// Builds a multi-line string representing full account info
	char[] ToString() {
        char buffer[1024];
		char row[128];
		char className[MAX_NAME_LENGTH];
		char clientName[MAX_NAME_LENGTH]; GetClientName(this.client, clientName, sizeof(clientName));

		// Account Header
		Format(
			buffer,
			sizeof(buffer),
			"[RTU][BANK] Account Info\nClient %3d - %s\nSteamID: %s\nEarned: %.2f\n",
			this.client,
			clientName,
			this.accountKey,
			this.earned
		);

		// Sub Header
		StrCat( buffer, sizeof(buffer), "---------------------------------------------------------\n");
		StrCat( buffer, sizeof(buffer), "Class    | Revert? | Spent  |  Burnt  | Balance \n");
		StrCat( buffer, sizeof(buffer), "---------------------------------------------------------\n");

		// Rows
		for (TFClassType classType = TFClass_Scout; classType <= TFClass_Engineer; classType++) {
			TF2_ClassTypeToName(classType, className, this.currentClass);
			Format(
				row,
				sizeof(row),
				"%9s|%8s|%9.2f|%9.2f|%9.2f\n",
				className,
				// this.classData[classType].played ? "true" : "",
				this.classData[classType].markedForRevert ? "true" : "",
				this.classData[classType].spent,
				this.classData[classType].burnt,
				this.Balance(classType)
			);

			// Join
			StrCat(buffer, sizeof(buffer), row);
		}

        return buffer;
    }

	// Builds row with values matching the headers in `Bank.PrintAccounts`
	char[] ToRow() {
		char row[128];
		char className[MAX_NAME_LENGTH];

		TF2_ClassTypeToName(this.currentClass, className);

		Format(
			row,
			sizeof(row),
			"%21s|%9d|%12s|%9s|%9.2f|%9.2f|%9.2f|%9.2f\n",
			this.accountKey,
			this.client,
			this.connected ? "true" : "false",
			className,
			this.earned,
			this.classData[this.currentClass].spent,
			this.classData[this.currentClass].burnt,
			this.Balance()
		);

		return row;
	}
}

// Returns the class name string for a given TFClassType
bool TF2_ClassTypeToName(TFClassType classType, char className[MAX_NAME_LENGTH], TFClassType current=view_as<TFClassType>(-1)) {
    if (classType < TFClass_Scout || classType > TFClass_Engineer) {
		className = "INVALID";
		return false;
	}

	switch (classType) {
        case TFClass_Scout:      className = "scout";
        case TFClass_Soldier:    className = "soldier";
        case TFClass_Pyro:       className = "pyro";
        case TFClass_DemoMan:    className = "demoman";
        case TFClass_Heavy:      className = "heavy";
        case TFClass_Engineer:   className = "engineer";
        case TFClass_Medic:      className = "medic";
        case TFClass_Sniper:     className = "sniper";
        case TFClass_Spy:        className = "spy";
        default:                 return false; // TODO: handle spectator?
    }

	// Decorate output to indicate active class during iterations
	if (current != view_as<TFClassType>(-1) && current == classType) StrCat(className, sizeof(className), "*");

	return true;
}
