/**
 * Rock The Upgrades: BankConfig
 * TODO: description
 */

enum struct BankConfig {
	// Prevent re-initialization
	bool initialized;

	// Track auth server state. Allows recovery when restoration detected
	bool steamAuthAvailable;

	// Tracks currency gained from objectives this round which will be combined with starting currency for late joiners
	int bonusStartingCurrency;

	// Maps currently connected clients to their Account Keys (fallback to client names during outages)
	ArrayList clientAccountKeys;

	void Init() {
		if (this.initialized) return;

		this.steamAuthAvailable = true;
		this.bonusStartingCurrency = 0;
		this.clientAccountKeys = new ArrayList(MAX_AUTHID_LENGTH, MaxClients + 1);
		this.initialized = true;
	}
}
