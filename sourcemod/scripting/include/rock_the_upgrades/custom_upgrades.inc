/**
 * Rock The Upgrades: Custom Upgrades
 * Handles loading and applying custom upgrades files per-map or globally
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

// Call during `OnMapStart`
void SendUpgradesFileToClients() {
	DetermineUpgradesFilePath();

	if (fileFound) AddFileToDownloadsTable(filePath);
}

// Map-specific, global override, or default
void DetermineUpgradesFilePath() {
	// Write mapname to static buffer
	GetCurrentMap(mapName, sizeof(mapName));

	// Write full path of map-specific file to buffer
	Format(filePath, sizeof(filePath), "%s/%s_upgrades.txt", UPGRADES_ROOT_PATH, mapName);

	// Use map-specific config if exists
	if (FileExists(filePath)) return;

	// Fallback to global config
	Format(filePath, sizeof(filePath), "%s/%s_%s", UPGRADES_ROOT_PATH, UPGRADES_PREFIX, UPGRADES_SUFFIX);

	if (FileExists(filePath)) return;

	fileFound = false;
	PrintToServer("[RTU] **WARNING** No custom upgrades file detected - default upgrades may lead to server instability!");
}

// TODO: ensure no conflicts with particular maps (FF, VIP, etc)
void ApplyCustomUpgradesFile() {
	if (!fileFound) return;

	int entity = FindEntityByClassname(-1, "tf_gamerules");

	SetVariantString(filePath);
	AcceptEntityInput(entity, "SetCustomUpgradesFile");
}
