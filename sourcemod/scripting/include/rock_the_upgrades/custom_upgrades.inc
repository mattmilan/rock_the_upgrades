/**
 * Rock The Upgrades: Custom Upgrades
 * Handles loading and applying custom upgrades files per-map or globally
 *
 * Expects the files to be located on the server in "[gamedir]/tf/scripts/items"
 *
 * Files will be sent to clients at "[gamedir]/tf/download/scripts/items"
 *
 * WARN: Will not overwrite client files if they've already been downloaded
 *		 It's recommended to version the filename so that clients are forced
 *		 to redownload when changes are made.
 */

#if defined _custom_upgrades_included
 #endinput
#endif
#define _custom_upgrades_included

static const char UPGRADES_ROOT_PATH[] = "scripts/items";
static const char UPGRADES_PREFIX[]    = "bangerz";
static const char UPGRADES_SUFFIX[]    = "upgrades.txt";

static char mapName[MAX_NAME_LENGTH];
static char filePath[PLATFORM_MAX_PATH];
static bool fileFound = false;

// `OnMapStart`
void FindAndAddUpgradesFilesToDownloadsTable() {
	DetermineUpgradesFilePath();

	if (fileFound) AddFileToDownloadsTable(filePath);
}

// `OnMapStart`
void ApplyCustomUpgradesFile() {
	if (!fileFound) return;

	int entity = FindEntityByClassname(-1, "tf_gamerules");

	SetVariantString(filePath);

	AcceptEntityInput(entity, "SetCustomUpgradesFile");
}

// Map-specific, global override, or default
void DetermineUpgradesFilePath() {
	// Write mapname to static buffer
	GetCurrentMap(mapName, sizeof(mapName));

	// Write full path of map-specific file to buffer
	Format(filePath, sizeof(filePath), "%s/%s_upgrades.txt", UPGRADES_ROOT_PATH, mapName);

	// Use map-specific config if exists
	fileFound = FileExists(filePath);
	PrintToServer("[RTU] Checking for map upgrades file at %s ... %s", filePath, fileFound ? "found" : "not found");
	if (fileFound) return;

	// Fallback to global config
	Format(filePath, sizeof(filePath), "%s/%s_%s", UPGRADES_ROOT_PATH, UPGRADES_PREFIX, UPGRADES_SUFFIX);

	fileFound = FileExists(filePath);
	PrintToServer("[RTU] Checking for global upgrades file at %s ... %s", filePath, fileFound ? "found" : "not found");
	if (fileFound) return;

	fileFound = false;
	PrintToServer("[RTU] **WARNING** No custom upgrades file detected - default upgrades may lead to server instability!");
}
