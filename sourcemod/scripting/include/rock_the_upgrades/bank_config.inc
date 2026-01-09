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

enum struct BankConfig {
	// Prevent re-initialization
	bool initialized;

	// Track auth server state. Allows recovery when restoration detected
	bool steamAuthAvailable;

	// Maps currently connected clients to their Account Keys (fallback to client names during outages)
	ArrayList clientAccountKeys;

	// Tracks event-based increases to starting currency for new accounts
	ArrayList bonuses;

	void Init() {
		PrintToServer("[RTU][BankConfig] (Init)");
		// Prevent re-initialization
		if (this.initialized) return; this.initialized = true;

		// MAGIC NUMBER 5: most maps have less than 5 bonus-eligible events
		this.bonuses = new ArrayList(sizeof(TeamBonus), 5);

		this.clientAccountKeys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);

		this.steamAuthAvailable = true;
	}

	// Round Start
	void Restart() {
		this.steamAuthAvailable = true;
		this.bonuses.Clear();
	}

	// Map End
	void Reset() {
		this.clientAccountKeys.Clear();
		this.Restart();
	}

	// Plugin End
	void Close() {
		this.bonuses.Close();
		this.clientAccountKeys.Close();
	}

	// Increases starting bonus for late joiners
	void Add(TeamBonus bonus) { this.bonuses.Push(bonus); }

	// Returns total bonus for a given team
	// Unassigned bonuses are included with Red/Blu totals
	float Bonus(TFTeam team) {
		// Ignored
		if (team == TFTeam_Spectator) return 0.0;

		// Buffer
		TeamBonus bonus;
		float result;

		// Iterate bonuses
		for (int i = 0; i < this.bonuses.Length; i++) {
			// Write buffer
			this.bonuses.GetArray(i, bonus);

			// Filter by team
			if (!bonus.AppliesTo(team)) continue;

			// Sum
			result += bonus.amount;
		}

		return result;
	}

	void Print() {
		PrintToServer("[RTU][BankConfig] Steam Auth Available: %d", this.steamAuthAvailable);
		PrintToServer("[RTU][BankConfig] Client Account Keys Count: %d", this.clientAccountKeys.Length);
		PrintToServer("[RTU][BankConfig] Bonuses Count: %d", this.bonuses.Length);
	}
}
