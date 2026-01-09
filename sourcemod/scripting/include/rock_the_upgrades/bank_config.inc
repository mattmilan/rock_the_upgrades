/**
 * Rock The Upgrades: BankConfig
 * Banks extend stringmap but their entries are reserved for accounts
 * BankConfig is a place for
 */

#if defined _bank_config_included
 #endinput
#endif
#define _bank_config_included

#include "team_bonus"
#include <tf2>

// Maps currently connected clients to their Account Keys (fallback to client names during outages)
static ArrayList clientAccountKeys;

// Tracks event-based increases to starting currency for new accounts
static ArrayList bonuses;

// enum struct BankConfigStruct {
// 	// Prevent re-initialization
// 	bool initialized;

// 	// Track auth server state. Allows recovery when restoration detected
// 	bool steamAuthAvailable;

// 	// New accounts start with this value plus bonuses earned mid-round
// 	float starting;

// 	// Scale all currency gains
// 	float multiplier;

// 	// Maximum currency for accounts
// 	float limit;

// 	// Maps currently connected clients to their Account Keys (fallback to client names during outages)
// 	ArrayList clientAccountKeys;

// 	// Tracks event-based increases to starting currency for new accounts
// 	ArrayList bonuses;
// }


methodmap BankConfig < StringMap {

	public BankConfig(float starting, float multiplier, float limit) {
		bonuses = new ArrayList(sizeof(TeamBonus));
		clientAccountKeys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);

		StringMap b = new StringMap();
		b.SetValue("starting", starting);
		b.SetValue("multiplier", multiplier);
		b.SetValue("limit", limit);
		b.SetValue("authable", true);

		return view_as<BankConfig>(b);
	}

	property ArrayList Keys {
		public get() {
			return clientAccountKeys;
		}
	}

	property ArrayList Bonuses {
		public get() {
			return bonuses;
		}
	}
	// New accounts start with this value plus bonuses earned mid-round
	property float Starting {
		public get() { float out; this.GetValue("starting", out); return out; }
		public set(float val) { this.SetValue("starting", val); }
	}

	// Scale all currency gains
	property float Multiplier {
		public get() { float out; this.GetValue("multiplier", out); return out; }
		public set(float val) { this.SetValue("multiplier", val); }
	}

	// Maximum currency for accounts
	property float Limit {
		public get() { float out; this.GetValue("limit", out); return out; }
		public set(float val) { this.SetValue("limit", val); }
	}

	// Track auth server state. Allows recovery when restoration detected
	property bool Authable { // steamAuthAvailable
		public get() { bool out; this.GetValue("steamAuthAvailable", out); return out; }
		public set(bool val) { this.SetValue("steamAuthAvailable", val); }
	}

	// Round Start
	public void Restart() {
		this.Authable = true;
		bonuses.Clear();
	}

	// Map End
	public void Reset() {
		clientAccountKeys.Clear();
		this.Restart();
	}

	// Plugin End
	public void Close() {
		bonuses.Close();
		clientAccountKeys.Close();
	}

	// Increases starting bonus for late joiners
	public void Add(TeamBonus bonus) {
		bonuses.Push(bonus);
	}

	// Returns total bonus for a given team
	// Unassigned bonuses are included with Red/Blu totals
	public float Bonus(TFTeam team) {
		// Ignored
		if (team == TFTeam_Spectator) return 0.0;

		// Buffer
		TeamBonus bonus;
		float result;

		// Iterate bonuses
		for (int i = 0; i < bonuses.Length; i++) {
			// Write buffer
			bonuses.GetArray(i, bonus);

			// Filter by team
			if (!bonus.AppliesTo(team)) continue;

			// Sum
			result += bonus.amount;
		}

		return result;
	}

	public float StartingCurrency(TFTeam team) {
		return (this.Starting * this.Multiplier) + this.Bonus(team);
	}

	public void Print() {
		PrintToServer("[RTU][BankConfig] Steam Auth Available: %d", this.Authable);
		PrintToServer("[RTU][BankConfig] Client Account Keys Count: %d", clientAccountKeys.Length);
		PrintToServer("[RTU][BankConfig] Bonuses Count: %d", bonuses.Length);
	}
}

// enum struct BankConfig {
// 	// Prevent re-initialization
// 	bool initialized;

// 	// Track auth server state. Allows recovery when restoration detected
// 	bool steamAuthAvailable;

// 	// New accounts start with this value plus bonuses earned mid-round
// 	float starting;

// 	// Scale all currency gains
// 	float multiplier;

// 	// Maximum currency for accounts
// 	float limit;

// 	// Maps currently connected clients to their Account Keys (fallback to client names during outages)
// 	ArrayList clientAccountKeys;

// 	// Tracks event-based increases to starting currency for new accounts
// 	ArrayList bonuses;

// 	void Init(float starting, float multiplier, float limit) {
// 		PrintToServer("[RTU][BankConfig] (Init)");
// 		// Prevent re-initialization
// 		// if (this.initialized) return; this.initialized = true;
// 		if (this.initialized) PrintToServer("[RTU][BankConfig] Already Initialized!");

// 		this.starting = starting;
// 		this.multiplier = multiplier;
// 		this.limit = limit;

// 		this.bonuses = new ArrayList(sizeof(TeamBonus));

// 		this.clientAccountKeys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);

// 		this.steamAuthAvailable = true;
// 		this.Print();
// 	}

// 	// Round Start
// 	void Restart() {
// 		PrintToServer("  **  ** BankConfig Restart");
// 		this.steamAuthAvailable = true;
// 		this.bonuses.Clear();
// 	}

// 	// Map End
// 	void Reset() {
// 		PrintToServer("  **  ** BankConfig Reset");
// 		this.clientAccountKeys.Clear();
// 		this.Restart();
// 	}

// 	// Plugin End
// 	void Close() {
// 		PrintToServer("  **  ** BankConfig Close");
// 		this.bonuses.Close();
// 		this.clientAccountKeys.Close();
// 	}

// 	// Increases starting bonus for late joiners
// 	void Add(TeamBonus bonus) { this.bonuses.Push(bonus); }

// 	// Returns total bonus for a given team
// 	// Unassigned bonuses are included with Red/Blu totals
// 	float Bonus(TFTeam team) {
// 		// Ignored
// 		if (team == TFTeam_Spectator) return 0.0;

// 		// Buffer
// 		TeamBonus bonus;
// 		float result;

// 		// Iterate bonuses
// 		for (int i = 0; i < this.bonuses.Length; i++) {
// 			// Write buffer
// 			this.bonuses.GetArray(i, bonus);

// 			// Filter by team
// 			if (!bonus.AppliesTo(team)) continue;

// 			// Sum
// 			result += bonus.amount;
// 		}

// 		return result;
// 	}

// 	float StartingCurrency(TFTeam team) {
// 		return (this.starting * this.multiplier) + this.Bonus(team);
// 	}
// 	void Print() {
// 		PrintToServer("[RTU][BankConfig] Steam Auth Available: %d", this.steamAuthAvailable);
// 		PrintToServer("[RTU][BankConfig] Client Account Keys Count: %d", this.clientAccountKeys.Length);
// 		PrintToServer("[RTU][BankConfig] Bonuses Count: %d", this.bonuses.Length);
// 	}
// }
