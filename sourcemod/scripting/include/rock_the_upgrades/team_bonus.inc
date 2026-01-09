/**
 *  Rock The Upgrades: TeamBonus
 *  Dataset and utilties for team-based currency events
 */

#if defined _team_bonus_included
 #endinput
#endif
#define _team_bonus_included
#define MAX_TEAM_BONUS_EVENT_LENGTH 16

#include <tf2>

enum struct TeamBonus {
	float amount;
	TFTeam team;
	char event[MAX_TEAM_BONUS_EVENT_LENGTH];

	void Set(float value, TFTeam team=TFTeam_Unassigned, const char[] reason = "unspecified") {
		this.amount = value;
		this.team = team;
		strcopy(this.event, sizeof(this.event), reason);
	}

	// Returns true if the bonus applies to the given team
	// Unassigned bonuses apply to both RED and BLU
	bool AppliesTo(TFTeam team) {
		return this.team == TFTeam_Unassigned || this.team == team;
	}

	// Debug
	void Print() {
		PrintToServer(
			"[RTU][TeamBonus] Amount: %.2f, Team: %d, Event: %s",
			this.amount,
			this.team,
			this.event
		);
	}
}
