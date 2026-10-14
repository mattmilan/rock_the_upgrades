#include <sourcemod>
#include <sourcescramble>

MemoryPatch g_pPatchRagdoll;
MemoryPatch g_pPatchAmmo;

public Plugin myinfo = {
    name = "Clean Kill",
    author = "murderousIntent",
    description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
    version = "0.1",
};

public void OnPluginStart()
{
    Handle hGameConf = LoadGameConfigFile("tf2.cleankill");
    if (hGameConf == null)
    {
        SetFailState("Failed to load gamedata 'tf2.cleankill'!");
    }

    // 1. Patch out Ragdolls
    g_pPatchRagdoll = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::CreateRagdollEntity()::SkipRagdollCreation");
    if (g_pPatchRagdoll && g_pPatchRagdoll.Validate())
    {
        g_pPatchRagdoll.Enable();
        PrintToServer("[CORE] SourceScramble cleanly patched out Ragdoll generation.");
    }
    else
    {
        LogError("Failed to validate SkipRagdollCreation patch. Signature might be broken.");
    }

    // 2. Patch out Ammo Packs
    g_pPatchAmmo = MemoryPatch.CreateFromConf(hGameConf, "CTFPlayer::DropAmmoPack()::SkipAmmoDrop");
    if (g_pPatchAmmo && g_pPatchAmmo.Validate())
    {
        g_pPatchAmmo.Enable();
        PrintToServer("[CORE] SourceScramble cleanly patched out Ammo Pack generation.");
    }
    else
    {
        LogError("Failed to validate SkipAmmoDrop patch. Signature might be broken.");
    }

    delete hGameConf;
}

// #include <sourcemod>
// #include <sourcescramble> // Requires SourceScramble include file

// MemoryPatch g_pPatchRagdoll;
// MemoryPatch g_pPatchDrop;

// public Plugin myinfo = {
//     name = "Clean Kill",
//     author = "murderousIntent",
//     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
//     version = "0.1",
// };

// public void OnPluginStart()
// {
//     Handle hGameConf = LoadGameConfigFile("tf2.cleankill");
//     if (hGameConf == null)
//     {
//         SetFailState("Failed to load gamedata 'tf2.cleankill'!");
//     }

//     // Initialize and enable the Ragdoll blocking patch
//     g_pPatchRagdoll = MemoryPatch.CreateFromConf(hGameConf, "SkipRagdollCreation");
//     if (g_pPatchRagdoll && g_pPatchRagdoll.Validate())
//     {
//         g_pPatchRagdoll.Enable();
//         PrintToServer("[CORE] SourceScramble successfully patched out Ragdoll generation.");
//     }

//     // Initialize and enable the Item/Ammo Drop blocking patch
//     g_pPatchDrop = MemoryPatch.CreateFromConf(hGameConf, "SkipWeaponDrop");
//     if (g_pPatchDrop && g_pPatchDrop.Validate())
//     {
//         g_pPatchDrop.Enable();
//         PrintToServer("[CORE] SourceScramble successfully patched out Weapon/Ammo Drops.");
//     }

//     delete hGameConf;
// }

// #include <sourcemod>
// #include <sdktools>
// #include <sdkhooks>

// public Plugin myinfo = {
//     name = "Clean Kill",
//     author = "murderousIntent",
//     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
//     version = "0.1",
// };
// // public void OnEntityDestroyed(int entity)
// // {
// //     char classname[64];
// //     GetEntityClassname(entity, classname, sizeof(classname));
// //     PrintToServer("[%d] [%d] OnEntityDestroyed - %s", GetRealEdictCount(), entity, classname);
// // }
// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToServer("[%d] [%d] OnEntityCreated PRE - %s", GetRealEdictCount(), entity, classname);
//     // Fast character index check on Linux x64 (Index 3 is the character after "tf_")
//     switch (classname[3])
//     {
//         case 'd': // Checks "tf_dropped_weapon"
//         {
//             if (strcmp(classname, "tf_dropped_weapon") == 0)
//             {
//                 RemoveEntity(entity);//SDKHook(entity, SDKHook_Spawn, OnTargetSpawn);
//             }
//         }
//         case 'a': // Checks "tf_ammo_pack"
//         {
//             if (strcmp(classname, "tf_ammo_pack") == 0)
//             {
//                  RemoveEntity(entity);//SDKHook(entity, SDKHook_Spawn, OnTargetSpawn);
//             }
//         }
//         case 'r': // Checks "tf_ragdoll"
//         {
//             if (strcmp(classname, "tf_ragdoll") == 0)
//             {
//                 RemoveEntity(entity);//SDKHook(entity, SDKHook_Spawn, OnTargetSpawn);
//             }
//         }
//     }
//     PrintToServer("[%d] [%d] OnEntityCreated POST - %s", GetRealEdictCount(), entity, classname);
// }

// // public Action OnTargetSpawn(int entity)
// // {
// //     char classname[64];
// //     GetEntityClassname(entity, classname, sizeof(classname));

// //     // 1. Force remove the entity from the engine container immediately
// //     RemoveEntity(entity);

// //     PrintToServer("[%d] [%d] OnTargetSpawn - %s", GetRealEdictCount(), entity, classname);

// //     // 2. Return Plugin_Handled to tell the engine to abort the spawn sequence
// //     return Plugin_Handled;
// // }

// int GetRealEdictCount()
// {
//     int edicts = 0;
//     // Loop through the absolute maximum networked capacity of the engine
//     for (int i = 0; i < GetMaxEntities(); i++)
//     {
//         if (IsValidEdict(i)) // Checks if the slot is an active networked edict
//         {
//             edicts++;
//         }
//     }
//     return edicts;
// }

// public void OnEntityDestroyed(int entity)
// {
//     char classname[64];
//     GetEntityClassname(entity, classname, sizeof(classname));
//     PrintToServer("[%d] [%d] OnEntityDestroyed - %s", GetRealEdictCount(), entity, classname);
// }

// public void OnEntityCreated(int entity, const char[] classname)
// {
//     // PrintToServer("[%d] [%d] OnEntityCreated PRE - %s", GetRealEdictCount(), entity, classname);

//     // Fast character index check on Linux x64 (Index 3 is the character after "tf_")
//     switch (classname[3])
//     {
//         case 'd': // Checks "tf_dropped_weapon"
//         {
//             if (strcmp(classname, "tf_dropped_weapon") == 0)
//             {
//                 // Instantly purge the entity inside the allocation loop.
//                 // This stops it from broadcasting to clients or spawning fully.
//                 RemoveEntity(entity);
//             }
//         }
//         case 'a': // Checks "tf_ammo_pack"
//         {
//             if (strcmp(classname, "tf_ammo_pack") == 0)
//             {
//                 RemoveEntity(entity);
//             }
//         }
//         case 'r': // Checks "tf_ragdoll"
//         {
//             if (strcmp(classname, "tf_ragdoll") == 0)
//             {
//                 RemoveEntity(entity);
//             }
//         }
//     }

//     PrintToServer("[%d] [%d] OnEntityCreated - %s", GetRealEdictCount(), entity, classname);
// }

// #include <sourcemod>
// #include <sdktools>

// public Plugin myinfo = {
//     name = "Clean Kill",
//     author = "murderousIntent",
//     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
//     version = "0.1",
// };

// public void OnPluginStart()
// {
//     // Hook player_death early enough to alter player data before entity allocation
//     HookEvent("player_death", Event_PlayerDeathPre, EventHookMode_Pre);
// }
// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToServer("[%d] [%d] OnEntityCreated - %s", GetEntityCount(), entity, classname);
// }
// public void OnEntityDestroyed(int entity)
// {
//     char classname[64];
//     GetEntityClassname(entity, classname, sizeof(classname));
//     PrintToServer("[%d] [%d] OnEntityDestroyed - %s", GetEntityCount(), entity, classname);
// }

// public Action Event_PlayerDeathPre(Event event, const char[] name, bool dontBroadcast)
// {
//     int userid = event.GetInt("userid");
//     int client = GetClientOfUserId(userid);
//     PrintToServer("PlayerDeathPre");
//     if (client > 0 && IsClientInGame(client))
//     {
//         // 1. Force death flags into the event structure to tell the client engine
//         // to dissolve the body and skip standard ragdoll rendering
//         event.SetInt("death_flags", event.GetInt("death_flags") | 0x0020); // TF_DEATHFLAG_DISSOLVE

//         // 2. Dissolve the weapon drop tracking.
//         // Setting the weapons to an empty state forces the engine factory to skip item allocation
//         for (int i = 0; i < 5; i++)
//         {
//             int weapon = GetPlayerWeaponSlot(client, i);
//             if (weapon > 0 && IsValidEntity(weapon))
//             {
//                 // Remove weapons from the player instantly so the death factory sees an empty inventory
//                 RemovePlayerItem(client, weapon);
//                 RemoveEntity(weapon);
//             }
//         }

//         // 3. Clear out the player's internal resource flags that trigger ammo pack drops
//         // (Tricking the engine into thinking the player has 0 reserve ammo to drop)
//         SetEntProp(client, Prop_Send, "m_iAmmo", 0, _, 1); // Primary
//         SetEntProp(client, Prop_Send, "m_iAmmo", 0, _, 2); // Secondary
//     }

//     return Plugin_Continue;
// }

// Keep your existing OnEntityCreated log framework active to verify if
// tf_ragdoll, tf_dropped_weapon, or tf_ammo_pack ever trigger again.

// #include <sourcemod>
// #include <sdktools>
// #include <sdkhooks>

// public Plugin myinfo = {
//     name = "Clean Kill",
//     author = "murderousIntent",
//     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
//     version = "0.1",
// };

// public void OnPluginStart()
// {
//     for (int i = 1; i <= MaxClients; i++)
//     {
//         if (IsClientInGame(i)) RegisterHooks(i);
//     }
// }

// public void OnClientPutInServer(int client)
// {
//     RegisterHooks(client);
// }

// void RegisterHooks(int client)
// {
//     SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);
// }
// //                 Action (int victim, int &attacker, int &inflictor, float &damage, int &damagetype);
// //                 Action (int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3]);
// //                 Action (int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3], int damagecustom);
// public Action OnTakeDamage(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
// {
//     PrintToServer("[%d] [DEBUG] Damage Hook fired on client %d! Damage: %f", GetEntityCount(), victim, damage);

//     if (damage >= GetClientHealth(victim))
//     {
//         PrintToServer("[%d] Blow is LETHAL. Overriding damage flags to stop edicts.", GetEntityCount());
//         // Force the damage flags early enough so the C++ death sequence catches them.
//         // (1 << 22) = DMG_REMOVENORAGDOLL -> Engine skips ragdoll allocation cleanly
//         // (1 << 26) = DMG_DISSOLVE -> Vaporizes base model data
//         damagetype |= (1 << 22) | (1 << 26);

//         // Clear the exact networked entity handle property directly on the player
//         if (HasEntProp(victim, Prop_Send, "m_hRagdoll"))
//         {
//             SetEntPropEnt(victim, Prop_Send, "m_hRagdoll", -1);
//         }
//         PrintToServer("[%d] DEAD", GetEntityCount());
//         return Plugin_Changed;
//     }
//     return Plugin_Continue;
// }

// public void OnEntityDestroyed(int entity)
// {
//     PrintToServer("[%d] [%d] OnEntityDestroyed", GetEntityCount(), entity);
// }
// // Fallback: Catch any weapons or ammo packs that circumvent the damage flags
// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToServer("[%d] [%d] OnEntityCreated - %s", GetEntityCount(), entity, classname);
//     // Fast character switch on Linux x64 (Index 3 is the letter after "tf_")
//     switch (classname[3])
//     {
//         case 'd': // Checks "tf_dropped_weapon"
//         {
//             if (strcmp(classname, "tf_dropped_weapon") == 0)
//             {
//                 RequestFrame(Frame_KillEntity, entity);
//             }
//         }
//         case 'a': // Checks "tf_ammo_pack"
//         {
//             if (strcmp(classname, "tf_ammo_pack") == 0)
//             {
//                 RequestFrame(Frame_KillEntity, entity);
//             }
//         }
//     }
// }

// void Frame_KillEntity(int entity)
// {
//     if (IsValidEntity(entity))
//     {
//         PrintToServer("[%d] [%d] Killing...", GetEntityCount(), entity);
//         RemoveEntity(entity);
//         PrintToServer("[%d] [%d] Killed.", GetEntityCount(), entity);
//     }
// }

// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToServer("[%d]CREATED entity: %s", GetEntityCount(), classname);
    // Fast character switch on Linux x64
    // switch (classname[3]) // Index 3 is the character right after "tf_"
    // {
    //     case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
    //     case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
    //     case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
    // }
// }

// #include <sourcemod>
// #include <sdktools>
// #include <sdkhooks>

// public Plugin myinfo = {
//     name = "Clean Kill",
//     author = "murderousIntent",
//     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
//     version = "0.1",
// };

// public void OnPluginStart()
// {
//     // CRITICAL: Hook players already in the server (handles mid-game reloads/late loads)
//     for (int i = 1; i <= MaxClients; i++)
//     {
//         if (IsClientInGame(i))
//         {
//             RegisterDamageHook(i);
//         }
//     }
// }

// public void OnClientPutInServer(int client)
// {
//     // Hook players joining naturally later
//     RegisterDamageHook(client);
// }

// void RegisterDamageHook(int client)
// {
//     // SDKHook_OnTakeDamageAlive is safer for evaluating lethal blows in TF2
//     SDKHook(client, SDKHook_OnTakeDamageAlive, OnTakeDamageAlive);
//     PrintToServer("[DEBUG] Damage hook successfully attached to client: %d", client);
// }

// public Action OnTakeDamageAlive(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
// {
//     PrintToServer("[DEBUG] Damage Hook fired on client %d! Damage: %f", victim, damage);

//     // Is this blow going to kill the player?
//     if (damage >= GetClientHealth(victim))
//     {
//         PrintToServer("[DEBUG] Blow is LETHAL. Overriding damage flags to stop edicts.");

//         // Apply edict-blocking flags natively
//         // (1 << 22) = DMG_REMOVENORAGDOLL -> Skips allocating a ragdoll edict
//         // (1 << 26) = DMG_DISSOLVE -> Vaporizes model, skipping weapon/ammo edicts
//         damagetype |= (1 << 22) | (1 << 26);
//         PrintToServer("[%d] DEAD", GetEntityCount());

//         return Plugin_Changed; // Force the engine to process our new flags
//     }

//     return Plugin_Continue;
// }

// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToServer("[%d]CREATED entity: %s", GetEntityCount(), classname);
//     // Fast character switch on Linux x64
//     switch (classname[3]) // Index 3 is the character right after "tf_"
//     {
//         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
//         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
//         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
//     }
// }

// // // public void OnClientPutInServer(int client)
// // // {
// // //     SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);
// // // }
// // public void OnPluginStart() {
// //     HookEvent("player_hurt", Event_PlayerHurt, EventHookMode_Pre);
// // }

// // //public Action Event_PlayerHurt(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
// // Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast)
// // {
// //     int victim = event.GetInt("userid");
// //     // int attacker = event.GetInt("attacker");
// //     float damage = event.GetFloat("damage");
// //     // int damagetype = event.GetInt("damagetype");
// //     PrintToChatAll("[%d]DAMAGED", GetEntityCount());
// //     // Check if this hit is going to kill the player
// //     if (damage >= GetClientHealth(victim))
// //     {
// //         PrintToChatAll("[%d]KILLED", GetEntityCount());
// //         // DMGMG_REMOVENORAGDOLL (1 << 22) tells the engine to completely skip ragdoll allocation
// //         // DMG_DISSOLVE (1 << 26) forces energy death which cleanly vaporizes the player instantly without dropped entities
// //         damagetype |= (1 << 22) | (1 << 26);
// //     }

// //     return Plugin_Changed;
// // }

// // public void OnEntityCreated(int entity, const char[] classname)
// // {
// //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// //     // Fast character switch on Linux x64
// //     switch (classname[3]) // Index 3 is the character right after "tf_"
// //     {
// //         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
// //         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
// //         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
// //     }
// //     PrintToChatAll("[%d]AFTER", GetEntityCount());
// // }

// // // // This fires the instant any entity is created in the world
// // // public void OnEntityCreated(int entity, const char[] classname) {
// // //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// // //     // 1. Prevent the server from spawning the ragdoll record
// // //     if (StrEqual(classname, "tf_ragdoll")) {
// // //         AcceptEntityInput(entity, "Kill");
// // //     }

// // //     // 2. Prevent active weapon items from falling to the ground
// // //     else if (StrEqual(classname, "tf_dropped_weapon")) {
// // //         AcceptEntityInput(entity, "Kill");
// // //     }

// // //     // 3. Prevent standard generic death boxes/ammo containers
// // //     else if (StrEqual(classname, "tf_ammo_pack")) {
// // //         AcceptEntityInput(entity, "Kill");
// // //     }
// // //     PrintToChatAll("[%d]AFTER", GetEntityCount());

// // // }

// // // public void OnEntityCreated(int entity, const char[] classname)
// // // {
// // //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// // //     // Fast character switch on Linux x64
// // //     switch (classname[3]) // Index 3 is the character right after "tf_"
// // //     {
// // //         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
// // //         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
// // //         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
// // //     }
// // //     PrintToChatAll("[%d]AFTER", GetEntityCount());
// // // }

// // // public Action OnEntitySpawn(int entity)
// // // {
// // //     PrintToChatAll("[%d]BLOCKED spawn", GetEntityCount());
// // //     // Handled safely deletes the object on x64 architectures
// // //     return Plugin_Handled;
// // // }

// // // public void OnEntityCreated(int entity, const char[] classname)
// // // {
// // //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// // //     // Fast character switch on Linux x64
// // //     switch (classname[3]) // Index 3 is the character right after "tf_"
// // //     {
// // //         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// // //         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// // //         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// // //     }
// // // }

// // // public Action OnEntitySpawn(int entity)
// // // {
// // //     PrintToChatAll("[%d]BLOCKED spawn", GetEntityCount());
// // //     // Handled safely deletes the object on x64 architectures
// // //     return Plugin_Handled;
// // // }

// // // #include <sourcemod>
// // // #include <sdktools>

// // // public Plugin myinfo = {
// // //     name = "Clean Kill",
// // //     author = "murderousIntent",
// // //     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
// // //     version = "0.1",
// // // };

// // // // Global variables to cache the Network Class IDs
// // // int g_NetClass_Ragdoll = -1;
// // // int g_NetClass_DroppedWeapon = -1;
// // // int g_NetClass_AmmoPack = -1;

// // // public void OnPluginStart()
// // // {
// // //     // If the plugin is loaded late (mid-game), cache IDs immediately
// // //     CacheNetClassIDs();
// // // }

// // // public void OnMapStart()
// // // {
// // //     // Cache IDs at map start to ensure they are ready for entity spawning
// // //     CacheNetClassIDs();
// // // }

// // // public void OnEntityCreated(int entity, const char[] classname)
// // // {
// // //     // // Get the entity's network class ID
// // //     // int netClass = GetEntityNetClass(entity);

// // //     // // Fallback: If IDs aren't cached yet, or the entity is invalid, stop
// // //     // if (netClass <= 0) return;

// // //     // // Fast integer switch statement
// // //     // switch (netClass)
// // //     // {
// // //     //     case var if (var == g_NetClass_Ragdoll):
// // //     //     {
// // //     //         // Match: tf_ragdoll
// // //     //     }
// // //     //     case var if (var == g_NetClass_DroppedWeapon):
// // //     //     {
// // //     //         // Match: tf_dropped_weapon
// // //     //     }
// // //     //     case var if (var == g_NetClass_AmmoPack):
// // //     //     {
// // //     //         // Match: tf_ammo_pack
// // //     //     }
// // //     // }
// // // }

// // // /**
// // //  * Iterates through the server's network classes to dynamically find and cache
// // //  * the IDs matching our specific target entity classes.
// // //  */
// // // public void CacheNetClassIDs()
// // // {
// // //     // Reset caches
// // //     g_NetClass_Ragdoll = -1;
// // //     g_NetClass_DroppedWeapon = -1;
// // //     g_NetClass_AmmoPack = -1;

// // //     // Loop through all registered server classes to find our matches
// // //     for (int i = 0; i < GetServerClassCount(); i++)
// // //     {
// // //         char serverClassname[64];
// // //         if (!GetServerClassName(i, serverClassname, sizeof(serverClassname)))
// // //         {
// // //             continue;
// // //         }

// // //         // Note: Server class names match the engine/C++ class (e.g., CTFRagdoll, CTFDroppedWeapon)
// // //         if (StrEqual(serverClassname, "CTFRagdoll"))
// // //         {
// // //             g_NetClass_Ragdoll = i;
// // //         }
// // //         else if (StrEqual(serverClassname, "CTFDroppedWeapon"))
// // //         {
// // //             g_NetClass_DroppedWeapon = i;
// // //         }
// // //         else if (StrEqual(serverClassname, "CTFAmmoPack") || StrEqual(serverClassname, "CAmmoPack"))
// // //         {
// // //             g_NetClass_AmmoPack = i;
// // //         }
// // //     }
// // // }
