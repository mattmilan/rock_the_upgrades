/**
 * Rock The Upgrades: TeamBonus
 *
 */

#if defined _team_bonus_included
 #endinput
#endif
#define _team_bonus_included

#include <tf2>

enum struct TeamBonus {
	float amount;
	TFTeam team;
	char event[16]; // MAGIC NUMBER 32: arbitrary event name length

	void Set(float value, TFTeam team=TFTeam_Unassigned, const char[] reason = "unspecified") {
		this.amount = value;
		this.team = team;
		strcopy(this.event, sizeof(this.event), reason);
	}

	// Returns true if the bonus applies to the given team
	// Unassigned bonuses apply to all teams
	bool AppliesTo(TFTeam team) {
		return this.team == TFTeam_Unassigned || this.team == team;
	}
}
