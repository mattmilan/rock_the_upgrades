#pragma newdecls required
#pragma semicolon 1
#include <tf2>
#include <tf2_stocks>

// A support for Account to track spending per class
enum struct ClassData {
	float spent;
	float burnt;

	// TODO: track weapons

	void Reset() {
		this.spent = 0.0;
		this.burnt = 0.0;
	}
}

// A data structure used by bank to track player currency
enum struct Account {
	// Player-unique identifier, useful when reconnecting
	char steamID[MAX_AUTHID_LENGTH + 1];

    // Client ID for reference
    int client;

    // Track connection status for debugging
    bool connected;

	// track earnings independent of class
	float earned;

	// track refundable and non-refundable spending per class
	TFClassType currentClass;
	ClassData classData[10]; // starts at 1; remember to offset

	void Init(int client, char steamID[MAX_AUTHID_LENGTH], float startingCurrency) {
		strcopy(this.steamID, sizeof(this.steamID), steamID);
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

	void Reset(float startingCurrency) {
		this.earned = startingCurrency;

		for (TFClassType classType = TFClass_Scout; classType <= TFClass_Engineer; classType++) {
			this.classData[classType].Reset();
		}
	}

	void SetClass(TFClassType classType) {
		if (classType == this.currentClass) return;

		char className[MAX_NAME_LENGTH];
		if (!TF2_ClassTypeToName(classType, this.currentClass, className)) {
			PrintToServer("[RTU][Bank] Warning: Attempted to set invalid class type %d for client %d", classType, this.client);
			return;
		}
		char currentClassName[MAX_NAME_LENGTH];
		TF2_ClassTypeToName(this.currentClass, this.currentClass, currentClassName);
		PrintToServer("[RTU][Bank] Changing class for client %d from %s to %s", this.client, currentClassName, className);
		this.currentClass = classType;
	}

	// NOTE: I would use TFClass_Unknown but that might represent Spectators
	// TODO: i bet that using -1 will break since the enum doesnt contain that value
	float Balance(TFClassType classType = view_as<TFClassType>(-1)) {
		if (classType == view_as<TFClassType>(-1))
			 classType = this.currentClass;

		return this.earned
			 - this.classData[classType].spent
			 - this.classData[classType].burnt;
	}

    // char[] ToString() {
    //     char buffer[256];
    //     Format(
    //         buffer,
    //         sizeof(buffer),
    //         "%18s|%9d|%12s|%9.2f|%9.2f|%9.2f|%9.2f",
    //         this.steamID,
    //         this.client,
    //         this.connected ? "true" : "false",
    //         this.earned,
    //         this.classData[this.currentClass].spent,
    //         this.classData[this.currentClass].burnt,
    //         this.Balance()
    //     );

    //     return buffer;
    // }
	char[] ToString() {
        char buffer[1024];
		char line[128];
		char className[MAX_NAME_LENGTH];
		for (TFClassType classType = TFClass_Scout; classType <= TFClass_Engineer; classType++) {
			TF2_ClassTypeToName(classType, this.currentClass, className);
			Format(
				line,
				sizeof(line),
				"%18s|%9d|%12s|%9s|%9.2f|%9.2f|%9.2f|%9.2f\n",
				this.steamID,
				this.client,
				this.connected ? "true" : "false",
				className,
				this.earned,
				this.classData[classType].spent,
				this.classData[classType].burnt,
				this.Balance(classType)
			);
			StrCat(buffer, sizeof(buffer), line);
		}
        return buffer;
    }
}

// Returns the class name string for a given TFClassType
bool TF2_ClassTypeToName(TFClassType classType, TFClassType current, char className[MAX_NAME_LENGTH]) {
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

	if (current == classType) StrCat(className, sizeof(className), "*");

	return true;
}
