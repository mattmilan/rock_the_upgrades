/**
 * Rock The Upgrades: Includes
 * Centralized include file for all Rock The Upgrades components
 */

#if defined _rock_the_includes_included
 #endinput
#endif
#define _rock_the_includes_included

// Shared functions
#include "shared"

// Enable/Disable upgrade system, and easily reset entity upgrades
#include "upgrades_controller"

// Manages configurable currency gains and losses across various events
#include "payment_controller"



// Persistent, multi-target timer
#include "combat_timer"

// Allows upgrade menu access via bindable chat command
#include "pocket_upgrades"

// Enables voting and auto-enable of upgrade system
#include "voting"

// Allows defining custom upgrades via external file
#include "custom_upgrades"
