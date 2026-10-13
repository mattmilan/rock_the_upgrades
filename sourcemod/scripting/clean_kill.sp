#include <sourcemod>
#include <sdktools>
#include <sdkhooks>

public Plugin myinfo = {
    name = "Clean Kill",
    author = "murderousIntent",
    description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
    version = "0.1",
};

public void OnPluginStart()
{
    // CRITICAL: Hook players already in the server (handles mid-game reloads/late loads)
    for (int i = 1; i <= MaxClients; i++)
    {
        if (IsClientInGame(i))
        {
            RegisterDamageHook(i);
        }
    }
}

public void OnClientPutInServer(int client)
{
    // Hook players joining naturally later
    RegisterDamageHook(client);
}

void RegisterDamageHook(int client)
{
    // SDKHook_OnTakeDamageAlive is safer for evaluating lethal blows in TF2
    SDKHook(client, SDKHook_OnTakeDamageAlive, OnTakeDamageAlive);
    PrintToServer("[DEBUG] Damage hook successfully attached to client: %d", client);
}

public Action OnTakeDamageAlive(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
{
    PrintToServer("[DEBUG] Damage Hook fired on client %d! Damage: %f", victim, damage);

    // Is this blow going to kill the player?
    if (damage >= GetClientHealth(victim))
    {
        PrintToServer("[DEBUG] Blow is LETHAL. Overriding damage flags to stop edicts.");

        // Apply edict-blocking flags natively
        // (1 << 22) = DMG_REMOVENORAGDOLL -> Skips allocating a ragdoll edict
        // (1 << 26) = DMG_DISSOLVE -> Vaporizes model, skipping weapon/ammo edicts
        damagetype |= (1 << 22) | (1 << 26);
        PrintToServer("[%d] DEAD", GetEntityCount());
        return Plugin_Changed; // Force the engine to process our new flags
    }

    return Plugin_Continue;
}

public void OnEntityCreated(int entity, const char[] classname)
{
    PrintToServer("[%d]CREATED entity: %s", GetEntityCount(), classname);
    // Fast character switch on Linux x64
    switch (classname[3]) // Index 3 is the character right after "tf_"
    {
        case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
        case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
        case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
    }
}

// // public void OnClientPutInServer(int client)
// // {
// //     SDKHook(client, SDKHook_OnTakeDamage, OnTakeDamage);
// // }
// public void OnPluginStart() {
//     HookEvent("player_hurt", Event_PlayerHurt, EventHookMode_Pre);
// }

// //public Action Event_PlayerHurt(int victim, int &attacker, int &inflictor, float &damage, int &damagetype, int &weapon, float damageForce[3], float damagePosition[3])
// Action Event_PlayerHurt(Event event, const char[] name, bool dontBroadcast)
// {
//     int victim = event.GetInt("userid");
//     // int attacker = event.GetInt("attacker");
//     float damage = event.GetFloat("damage");
//     // int damagetype = event.GetInt("damagetype");
//     PrintToChatAll("[%d]DAMAGED", GetEntityCount());
//     // Check if this hit is going to kill the player
//     if (damage >= GetClientHealth(victim))
//     {
//         PrintToChatAll("[%d]KILLED", GetEntityCount());
//         // DMGMG_REMOVENORAGDOLL (1 << 22) tells the engine to completely skip ragdoll allocation
//         // DMG_DISSOLVE (1 << 26) forces energy death which cleanly vaporizes the player instantly without dropped entities
//         damagetype |= (1 << 22) | (1 << 26);
//     }

//     return Plugin_Changed;
// }

// public void OnEntityCreated(int entity, const char[] classname)
// {
//     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
//     // Fast character switch on Linux x64
//     switch (classname[3]) // Index 3 is the character right after "tf_"
//     {
//         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) AcceptEntityInput(entity, "Kill");
//         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) AcceptEntityInput(entity, "Kill");
//         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) AcceptEntityInput(entity, "Kill");
//     }
//     PrintToChatAll("[%d]AFTER", GetEntityCount());
// }

// // // This fires the instant any entity is created in the world
// // public void OnEntityCreated(int entity, const char[] classname) {
// //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// //     // 1. Prevent the server from spawning the ragdoll record
// //     if (StrEqual(classname, "tf_ragdoll")) {
// //         AcceptEntityInput(entity, "Kill");
// //     }

// //     // 2. Prevent active weapon items from falling to the ground
// //     else if (StrEqual(classname, "tf_dropped_weapon")) {
// //         AcceptEntityInput(entity, "Kill");
// //     }

// //     // 3. Prevent standard generic death boxes/ammo containers
// //     else if (StrEqual(classname, "tf_ammo_pack")) {
// //         AcceptEntityInput(entity, "Kill");
// //     }
// //     PrintToChatAll("[%d]AFTER", GetEntityCount());

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

// // public Action OnEntitySpawn(int entity)
// // {
// //     PrintToChatAll("[%d]BLOCKED spawn", GetEntityCount());
// //     // Handled safely deletes the object on x64 architectures
// //     return Plugin_Handled;
// // }

// // public void OnEntityCreated(int entity, const char[] classname)
// // {
// //     PrintToChatAll("[%d]CREATED entity: %s", GetEntityCount(), classname);
// //     // Fast character switch on Linux x64
// //     switch (classname[3]) // Index 3 is the character right after "tf_"
// //     {
// //         case 'r': if (strcmp(classname, "tf_ragdoll") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// //         case 'd': if (strcmp(classname, "tf_dropped_weapon") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// //         case 'a': if (strcmp(classname, "tf_ammo_pack") == 0) SDKHook(entity, SDKHook_Spawn, OnEntitySpawn);
// //     }
// // }

// // public Action OnEntitySpawn(int entity)
// // {
// //     PrintToChatAll("[%d]BLOCKED spawn", GetEntityCount());
// //     // Handled safely deletes the object on x64 architectures
// //     return Plugin_Handled;
// // }

// // #include <sourcemod>
// // #include <sdktools>

// // public Plugin myinfo = {
// //     name = "Clean Kill",
// //     author = "murderousIntent",
// //     description = "Reduce edict consumption from ragdolls and ammo drops that spawn on player death.",
// //     version = "0.1",
// // };

// // // Global variables to cache the Network Class IDs
// // int g_NetClass_Ragdoll = -1;
// // int g_NetClass_DroppedWeapon = -1;
// // int g_NetClass_AmmoPack = -1;

// // public void OnPluginStart()
// // {
// //     // If the plugin is loaded late (mid-game), cache IDs immediately
// //     CacheNetClassIDs();
// // }

// // public void OnMapStart()
// // {
// //     // Cache IDs at map start to ensure they are ready for entity spawning
// //     CacheNetClassIDs();
// // }

// // public void OnEntityCreated(int entity, const char[] classname)
// // {
// //     // // Get the entity's network class ID
// //     // int netClass = GetEntityNetClass(entity);

// //     // // Fallback: If IDs aren't cached yet, or the entity is invalid, stop
// //     // if (netClass <= 0) return;

// //     // // Fast integer switch statement
// //     // switch (netClass)
// //     // {
// //     //     case var if (var == g_NetClass_Ragdoll):
// //     //     {
// //     //         // Match: tf_ragdoll
// //     //     }
// //     //     case var if (var == g_NetClass_DroppedWeapon):
// //     //     {
// //     //         // Match: tf_dropped_weapon
// //     //     }
// //     //     case var if (var == g_NetClass_AmmoPack):
// //     //     {
// //     //         // Match: tf_ammo_pack
// //     //     }
// //     // }
// // }

// // /**
// //  * Iterates through the server's network classes to dynamically find and cache
// //  * the IDs matching our specific target entity classes.
// //  */
// // public void CacheNetClassIDs()
// // {
// //     // Reset caches
// //     g_NetClass_Ragdoll = -1;
// //     g_NetClass_DroppedWeapon = -1;
// //     g_NetClass_AmmoPack = -1;

// //     // Loop through all registered server classes to find our matches
// //     for (int i = 0; i < GetServerClassCount(); i++)
// //     {
// //         char serverClassname[64];
// //         if (!GetServerClassName(i, serverClassname, sizeof(serverClassname)))
// //         {
// //             continue;
// //         }

// //         // Note: Server class names match the engine/C++ class (e.g., CTFRagdoll, CTFDroppedWeapon)
// //         if (StrEqual(serverClassname, "CTFRagdoll"))
// //         {
// //             g_NetClass_Ragdoll = i;
// //         }
// //         else if (StrEqual(serverClassname, "CTFDroppedWeapon"))
// //         {
// //             g_NetClass_DroppedWeapon = i;
// //         }
// //         else if (StrEqual(serverClassname, "CTFAmmoPack") || StrEqual(serverClassname, "CAmmoPack"))
// //         {
// //             g_NetClass_AmmoPack = i;
// //         }
// //     }
// // }
