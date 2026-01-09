/**
 * Rock The Upgrades: Shared
 * Centralized shared functions and utilities
 */

#if defined _rock_the_upgrades_shared_included
 #endinput
#endif
#define _rock_the_upgrades_shared_included

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
