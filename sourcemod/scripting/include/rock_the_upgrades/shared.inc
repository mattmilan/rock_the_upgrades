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

// Playing with natives. having no fun

// public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max) {
//     CreateNative("ValidClient", Native_ValidClient);
//     return APLRes_Success;
// }

// native bool ValidClient(int client, bool checkConnected=true, bool checkInGame=true, bool checkFake=true);
// any Native_ValidClient(Handle plugin, int numParams) {
// 	// REQUIRED: within integer bounds
// 	int client = GetNativeCell(1);
// 	if (client < 1 || client > MaxClients) return false;

// 	// OPTIONAL: connected
// 	bool checkConnected = GetNativeCell(2) != 0;
// 	if (checkConnected && !IsClientConnected(client)) return false;

// 	// OPTIONAL: in-game
// 	bool checkInGame = GetNativeCell(3) != 0;
// 	if (checkInGame && !IsClientInGame(client)) return false;

// 	// OPTIONAL: is human
// 	bool checkFake = GetNativeCell(4) != 0;
// 	if (checkFake && IsFakeClient(client)) return false;

// 	return true;
// }