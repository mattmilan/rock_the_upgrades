#include <sourcemod>
#include <sourcescramble>

MemoryPatch g_pPatchRagdoll;
MemoryPatch g_pPatchAmmo;
MemoryPatch g_pPatchWeapon;

bool g_bRagdollsPatched = false;
bool g_bAmmoDropPatched = false;
bool g_bWeaponDropPatched = false;

// TODO: documentation
// TODO: make configurable and toggleable
// TODO: DRY with methodmaps

public Plugin myinfo =
{
    name = "Clean Kill",
    author = "MurderousIntent",
    description = "Disables ragdoll and ammo/weapon drops on death to reduce edict load",
    version = "1.0.0"
};

public void OnPluginStart() {
    CreatePatchesFromConf();
    EnablePatches();
}

void CreatePatchesFromConf() {
    Handle hGameConf = LoadGameConfigFile("tf2.clean_kill");
    if (hGameConf == null) SetFailState("Failed to load gamedata 'tf2.clean_kill'!");

    g_pPatchRagdoll = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::CreateRagdollEntity::SkipRagdollCreation");
    g_pPatchAmmo    = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::DropAmmoPack::SkipAmmoDrop");
    g_pPatchWeapon  = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::DropCurrentWeapon::SkipWeaponDrop");

    delete hGameConf;
}

void EnablePatches() {
    ToggleRagdolls(true);
    ToggleAmmo(true);
    ToggleWeapon(true);
}

void ToggleRagdolls(bool bEnable) {
    if (g_bRagdollsPatched == bEnable) {
        PrintToServer("[CLEAN_KILL] SkipRagdoll already set to %b.", bEnable);
    } else if (!bEnable) {
        g_pPatchRagdoll.Disable();
        PrintToServer("[CLEAN_KILL] SkipRagdoll disabled.");
    } else if (g_pPatchRagdoll.Enable())
        PrintToServer("[CLEAN_KILL] SkipRagdoll enabled.");
    else
        LogError("Failed to validate SkipRagdollCreation symbol patch.");

    g_bRagdollsPatched = bEnable;
}

void ToggleAmmo(bool bEnable) {
    if (g_bAmmoDropPatched == bEnable) {
        PrintToServer("[CLEAN_KILL] SkipAmmo already set to %b.", bEnable);
    } else if (!bEnable) {
        g_pPatchAmmo.Disable();
        PrintToServer("[CLEAN_KILL] SkipAmmo disabled.");
    } else if (g_pPatchAmmo.Enable())
        PrintToServer("[CLEAN_KILL] SkipAmmo enabled.");
    else
        LogError("Failed to validate SkipAmmoDrop symbol patch.");

    g_bAmmoDropPatched = bEnable;
}

void ToggleWeapon(bool bEnable) {
    if (g_bWeaponDropPatched == bEnable) {
        PrintToServer("[CLEAN_KILL] SkipWeapon already set to %b.", bEnable);
    } else if (!bEnable) {
        g_pPatchWeapon.Disable();
        PrintToServer("[CLEAN_KILL] SkipWeapon disabled.");
    } else if (g_pPatchWeapon.Enable())
        PrintToServer("[CLEAN_KILL] SkipWeapon enabled.");
    else
        LogError("Failed to validate SkipWeaponDrop symbol patch.");

    g_bWeaponDropPatched = bEnable;
}
