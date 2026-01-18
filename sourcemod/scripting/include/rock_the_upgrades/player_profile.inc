/**
 * Rock The Upgrades: PlayerProfile
 * TODO: description
 */

#if defined _playerProfile_included
 #endinput
#endif
#define _playerProfile_included
#define MAX_WEAPONS_PER_CLASS 16

#pragma newdecls required
#pragma semicolon 1

#include <tf2_stocks>
// Helps PlayerProfile to track spending at the class level
enum struct TFClassData {
	float spent; 		  				  // refundable
	float burnt; 		  				  // non-refundable
	bool markedForRevert; 				  // resetting non-active classes
	int weapons[MAX_WEAPONS_PER_CLASS];	  // resetting non-equipped weapons

	void Reset() {
		// Nothing to reset if nothing spent
		if (this.spent < 0.001) return;

		this.spent = 0.0;
		this.burnt = 0.0;

		// Only the active class is affected during reset events
		// This means non-active classes will wrongly retain upgrades
		// Setting this flag helps us address that during respawn
		this.markedForRevert = true;
	}
}

// A data structure used by bank to track player currency
enum struct PlayerProfile {
	// Player-unique identifier, useful when reconnecting
	char key[MAX_NAME_LENGTH];

    // Client ID for reference
    int client;

    // Track connection status for debugging
    bool connected;

	// Has starting currency been applied?
	bool activated;

	// track earnings independent of class
	float earned;

	// track refundable and non-refundable spending per class
	TFClassType currentClass;
	TFClassData tfClassData[10];

	// Track team to support team-based currency when clients are disconnected
	TFTeam currentTeam;

	void Init(int client, char key[MAX_NAME_LENGTH]) {
		strcopy(this.key, sizeof(this.key), key);
		this.client = client;
		this.connected = true;
	}

	void Activate() {
		if (this.activated) return;

		this.earned = Bank.Instance().StartingCurrency(this.currentTeam);
		this.activated = true;
	}

	float Earned() {
		return this.earned;
	}

	float Spent(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.tfClassData[classType].spent;
	}

	float Burned(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.tfClassData[classType].burnt;
	}

	bool NeedsRevert(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1)) classType = this.currentClass;
		return this.tfClassData[classType].markedForRevert;
	}

	bool Revert() {
		if (!this.NeedsRevert()) return false;

		this.tfClassData[this.currentClass].markedForRevert = false;
		this.tfClassData[this.currentClass].spent = 0.0;

		return true;
	}

	void SyncEntity() {
		SetEntProp(
			this.client,
			Prop_Send,
			"m_nCurrency",
			RoundToCeil(this.Balance())
		);
	}

	// Called by `ResetAll` during `teamplay_round_start`, `rtu_reset` and `rtu_disable`
	void Reset(float startingCurrency) {
		this.earned = startingCurrency;

		// for all classes
		for (TFClassType classType = TFClass_Scout; classType <= TFClass_Engineer; classType++) {
			// clear spending and mark for revert
			this.tfClassData[classType].Reset();
			if (classType == this.currentClass) {
				this.tfClassData[classType].markedForRevert = false;
			}
		}
	}

	void Refund() {
		this.tfClassData[this.currentClass].spent = 0.0;
	}

	// NOTE: I would use TFClass_Unknown but that might represent Spectators
	// TODO: i bet that using -1 will break since the enum doesnt contain that value
	float Balance(TFClassType classType = view_as<TFClassType>(-1)) {
		// Default to current class
		if (classType == view_as<TFClassType>(-1))
			 classType = this.currentClass;

		return this.earned
			 - this.tfClassData[classType].spent
			 - this.tfClassData[classType].burnt;
	}

	// Builds a multi-line string representing full playerProfile info
	char[] ToString() {
        char buffer[1024];
		char row[128];
		char className[MAX_NAME_LENGTH];
		char clientName[MAX_NAME_LENGTH]; GetClientName(this.client, clientName, sizeof(clientName));

		// PlayerProfile Header
		Format(
			buffer,
			sizeof(buffer),
			"[RTU][BANK] PlayerProfile Info\nClient %3d - %s\nSteamID: %s\nEarned: %.2f\n",
			this.client,
			clientName,
			this.key,
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
				// this.tfClassData[classType].played ? "true" : "",
				this.tfClassData[classType].markedForRevert ? "true" : "",
				this.tfClassData[classType].spent,
				this.tfClassData[classType].burnt,
				this.Balance(classType)
			);

			// Join
			StrCat(buffer, sizeof(buffer), row);
		}

        return buffer;
    }

	// Builds row with values matching the headers in `Bank.PrintPlayerProfiles`
	char[] ToRow() {
		char row[128];
		char className[MAX_NAME_LENGTH];

		TF2_ClassTypeToName(this.currentClass, className);

		Format(
			row,
			sizeof(row),
			"%21s|%9d|%12s|%9s|%9.2f|%9.2f|%9.2f|%9.2f\n",
			this.key,
			this.client,
			this.connected ? "true" : "false",
			className,
			this.earned,
			this.tfClassData[this.currentClass].spent,
			this.tfClassData[this.currentClass].burnt,
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
