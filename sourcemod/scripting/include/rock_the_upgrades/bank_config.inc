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

	// Tracks currency gained from objectives this round which will be combined with starting currency for late joiners
	float bonusStartingCurrencyRed;
	float bonusStartingCurrencyBlu;

	// Maps currently connected clients to their Account Keys (fallback to client names during outages)
	ArrayList clientAccountKeys;

	void Init() {
		if (this.initialized) return;

		this.steamAuthAvailable = true;
		this.bonusStartingCurrencyRed = 0.0;
		this.bonusStartingCurrencyBlu = 0.0;
		this.clientAccountKeys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);
		this.initialized = true;
	}

	void AddBonusStartingCurrency(float amount, TFTeam team) {
		if (team == TFTeam_Red || team == TFTeam_Unassigned) this.bonusStartingCurrencyRed += amount;
		if (team == TFTeam_Blue || team == TFTeam_Unassigned) this.bonusStartingCurrencyBlu += amount;
	}

	float Bonus(TFTeam team) {
		return team == TFTeam_Red ?
			this.bonusStartingCurrencyRed :
			this.bonusStartingCurrencyBlu;
	}
}
