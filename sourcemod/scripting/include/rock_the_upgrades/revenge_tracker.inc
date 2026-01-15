#if defined _revenge_tracker_included
 #endinput
#endif
#define _revenge_tracker_included

static RevengeTracker g_RevengeTracker;

methodmap RevengeTracker < TypedStringMap {
	// public RevengeTracker() {
	// 	RevengeTracker self = view_as<RevengeTracker>(new StringMap());
	// 	return self;
	// }

	public static RevengeTracker Instance() {
		if (g_RevengeTracker == null) {
			g_RevengeTracker = view_as<RevengeTracker>(new StringMap());
		}

		return g_RevengeTracker;
	}

	//public void Track(char dominator[MAX_AUTHID_LENGTH], char dominated[MAX_AUTHID_LENGTH]) {
	public void Track(int dominator, int dominated) {
		// buffer
		RevengeEntry entry;

		// key buffers
		char dominatorKey[MAX_AUTHID_LENGTH]; AuthKeys.Get(dominator, dominatorKey);
		char dominatedKey[MAX_AUTHID_LENGTH]; AuthKeys.Get(dominated, dominatedKey);

		// find or build entry
		bool existing = this.GetArray(dominatorKey, entry, sizeof(RevengeEntry));
		if (!existing) entry.Init();

		// track
		entry.Track(dominatedKey);

		// save
		this.SetArray(dominatorKey, entry, sizeof(RevengeEntry));
	}

	public bool Check(int dominator, int dominated) {
		// buffer
		RevengeEntry entry;

		// key buffers
		char dominatorKey[MAX_AUTHID_LENGTH]; AuthKeys.Get(dominator, dominatorKey);
		char dominatedKey[MAX_AUTHID_LENGTH]; AuthKeys.Get(dominated, dominatedKey);

		// false unless found
		bool found = this.GetArray(dominatorKey, entry, sizeof(RevengeEntry));
		if (!found) return false;

		// false unless match
		bool revenge = entry.Check(dominatedKey);

		// update or remove empty entry
		if (entry.Empty()) this.Remove(dominatorKey);
		else this.SetArray(dominatorKey, entry, sizeof(RevengeEntry));

		return revenge;
	}
}

enum struct RevengeEntry {
	ArrayList dominatedSteamIds;

	void Init() {
		this.dominatedSteamIds = new ArrayList();
	}

	// Player has just dominated another player
	void Track(char victim[MAX_AUTHID_LENGTH]) {
		this.dominatedSteamIds.Push(victim);
	}

	// Player is dominating another player
	bool Check(char victim[MAX_AUTHID_LENGTH]) {
		int index = this.dominatedSteamIds.FindValue(victim);
		return this.Remove(index);
	}

	bool Remove(int index) {
		bool found = index > -1;
		if (found) this.dominatedSteamIds.Erase(index);
		return found;
	}

	bool Empty() {
		return this.dominatedSteamIds.Length == 0;
	}
}
