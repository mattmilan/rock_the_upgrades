/**
 * Rock The Upgrades: BankConfig
 * Composition for AccountController
 * Except for initializers, functionally should be acount-independent
 */

#if defined _bank_config_included
 #endinput
#endif
#define _bank_config_included

// #include "team_bonus"
// #include "account_controller"
// #include <tf2>

// TODO: extract from AccountController

// methodmap BankConfig __nullable__ {
//     public BankConfig() {
//         return view_as<BankConfig>(INVALID_HANDLE);
//     }

//     property bool Authable {
// 		public get() { bool out; this.Meta.GetValue("authable", out); return out; }
// 		public set(bool val) { this.Meta.SetValue("authable", val); }
// 	}

// 	property float Multiplier {
// 		public get() { float out; this.Meta.GetValue("multiplier", out); return out; }
// 		public set(float val) { this.Meta.SetValue("multiplier", val); }
// 	}

// 	property float Limit {
// 		public get() { float out; this.Meta.GetValue("limit", out); return out; }
// 		public set(float val) { this.Meta.SetValue("limit", val); }
// 	}

// 	property float Starting {
// 		public get() { float out; this.Meta.GetValue("starting", out); return out; }
// 		public set(float val) { this.Meta.SetValue("starting", val); }
// 	}

// 	property ArrayList Keys {
// 		public get() { return view_as<ArrayList>(this.Meta.GetHandle("keys")); }
// 		public set(ArrayList val) { this.Meta.SetValue("keys", val); }
// 	}

// 	property ArrayList Bonuses {
// 		public get() { return view_as<ArrayList>(this.Meta.GetHandle("bonuses")); }
// 		public set(ArrayList val) { this.Meta.SetValue("bonuses", val); }

// 	}
// }