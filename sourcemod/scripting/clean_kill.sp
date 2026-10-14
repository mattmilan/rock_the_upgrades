#include <sourcemod>
#include <sourcescramble>

MemoryPatch g_pPatchRagdoll;
MemoryPatch g_pPatchAmmo;
MemoryPatch g_pPatchWeapon;

public Plugin myinfo =
{
    name = "Clean Kill Core",
    author = "MurderousIntent",
    description = "Disables ragdoll and ammo/weapon drops on death to reduce edict load",
    version = "1.0.0"
};

public void OnPluginStart()
{
    Handle hGameConf = LoadGameConfigFile("tf2.cleankill");
    if (hGameConf == null)
    {
        SetFailState("Failed to load gamedata 'tf2.cleankill'!");
    }

    g_pPatchRagdoll = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::CreateRagdollEntity::SkipRagdollCreation");
    if (g_pPatchRagdoll && g_pPatchRagdoll.Validate())
    {
        g_pPatchRagdoll.Enable();
        PrintToServer("[CORE] SourceScramble cleanly patched out Ragdoll generation.");
    }
    else
    {
        LogError("Failed to validate SkipRagdollCreation symbol patch.");
    }

    g_pPatchAmmo = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::DropAmmoPack::SkipAmmoDrop");
    if (g_pPatchAmmo && g_pPatchAmmo.Validate())
    {
        g_pPatchAmmo.Enable();
        PrintToServer("[CORE] SourceScramble cleanly patched out Ammo Pack generation.");
    }
    else
    {
        LogError("Failed to validate SkipAmmoDrop symbol patch.");
    }

    // 3. Enable Weapon Drop Patch
    g_pPatchWeapon = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::DropCurrentWeapon::SkipWeaponDrop");
    if (g_pPatchWeapon && g_pPatchWeapon.Validate())
    {
        g_pPatchWeapon.Enable();
        PrintToServer("[CORE] SourceScramble cleanly patched out Weapon drop generation.");
    }
    else
    {
        LogError("Failed to validate SkipWeaponDrop symbol patch.");
    }

    delete hGameConf;
}
