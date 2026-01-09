/**
 * Rock The Upgrades: Currency Controller
 * TODO: description
 */

#if defined _payment_controller_included
 #endinput
#endif
#define _payment_controller_included

#include "shared"
#include <tf2>

// StringMap RevengeTracker; // Bonus currency for revenge kills

methodmap PaymentController < StringMap{
	// Returns a random float clamped to a convar-defined range, then apply multipliers
	// form. CurrencyOnKill
	public PaymentController() {
		// RevengeTracker = new StringMap();
		StringMap RevengeTracker = new StringMap();
		return view_as<PaymentController>(RevengeTracker);
	}

	public void Reset() {
		this.Clear();
	}

	public float ForKill(bool revenge=false) {
		return GetRandomFloat(
			this.KillMin(),
			this.KillMax()
		) * this.Revenge(revenge);
	}

	public float ForDestruction(Event event) {
		int objectIndex = event.GetInt("index");
		int upgradeLevel = GetEntProp(objectIndex, Prop_Send, "m_iUpgradeLevel");

		return upgradeLevel * this.Destruction();
	}

	// Deny payment
	public bool Suicide(int killer, int victim) {
		return killer < 1 || killer == victim;
	}

	// Returns a ConVar-defined float on revenge, or 1 (which will have no effect)
	public float Revenge(bool revenge=false) {
		return revenge ? this.RevengeMultiplier() : 1.0;
	}

	// Revenge kills trigger a bonus multiplier
	// TODO: optimize by avoiding conversion from id to name, if possible
	public bool RevengeKill(int killer, int victim) {
		// get killer name
		char dominatedName[MAX_NAME_LENGTH];
		GetClientName(killer, dominatedName, sizeof(dominatedName));

		// check if killer has been dominated, and if so, who the dominator was
		char storedDominator[MAX_NAME_LENGTH];
		bool revenge = this.GetString(dominatedName, storedDominator, sizeof(storedDominator));

		// bail early if killer hasn't actually been dominated
		if (!revenge) { return false; }

		// get victim name
		char dominatorName[MAX_NAME_LENGTH];
		GetClientName(victim, dominatorName, sizeof(dominatorName));

		// if the victim never dominated the player, this is not a revenge kill
		if (!StrEqual(dominatorName, storedDominator)) { return false; }

		// confirmed as a revenge kill - removing the record also returns true
		return this.Remove(dominatedName);
	}

	// Dominations earn a bonus (disabled by default)
	// form. CurrencyOnDomination
	public float ForDomination() {
		return this.Domination();
	}

	// Dying may incur a penalty (disabled by default)
	// Returns a positive value, should be inverted when called
	public float DeathPenalty(int victim) {
		return this.Balance(victim)
			 * this.DeathTax();
	}

	public float ForFlagCapture() {
		return this.CaptureFlag();
	}

	public float ForPointCapture() {
		return this.CapturePoint();
	}

	// TODO: Bank plugin detection, and fallback in it's absence
	public void Deposit(float amount, int client) {
		TheBank().Deposit(amount, client);
	}

	public void Burn(float amount, int client) {
		TheBank().Burn(amount, client);
	}

	public void DepositAll(float amount, TFTeam team = TFTeam_Unassigned) {
		TheBank().DepositAll(amount, team);
	}

	public float Balance(int client) {
		return TheBank().Balance(client);
	}


	public native float KillMin();
	public native float KillMax();
	public native float RevengeMultiplier();
	public native float Destruction();
	public native float Domination();
	public native float CapturePoint();
	public native float CaptureFlag();
	public native float DeathTax();
}

// Play SFX when buildings are destroyed while being built
// TODO: Debug the heck out of this
// void ObjectDenied(Event event) {
// 	PrintToServer("[RTU] ObjectDenied event triggered");
// 	int builder = GetClientOfUserId(event.GetInt("userid"));
// 	int attacker = GetClientOfUserId(event.GetInt("attacker"));
// 	int assister = GetClientOfUserId(event.GetInt("assister"));

// 	if (ValidClient(builder))  EmitGameSoundToClient(builder,  "vo/engineer_no01.wav");
// 	if (ValidClient(attacker)) EmitGameSoundToClient(attacker, "vo/engineer_gunslingerpunch02.wav");
// 	if (ValidClient(assister)) EmitGameSoundToClient(assister, "vo/engineer_gunslingerpunch02.wav");
// }

// higher level buildings yield more currency






