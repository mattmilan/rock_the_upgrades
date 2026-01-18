/**
 * Rock The Upgrades: Voting
 * Handles player voting to enable the upgrade system
 */

#if defined _voting_included
 #endinput
#endif
#define _voting_included

static int PlayerCount;             // Number of connected clients (excluding bots)
static bool WaitingForPlayers;      // Disallows voting while "Waiting for Players"
static bool g_VotingClosed;
static bool g_Forced;

ConVar g_Cvar_VoteThreshold;        // A % of connected players
ConVar g_Cvar_AutoEnableThreshold;  // A flat number of players

static Votes g_Votes;               // The Votes instance
static VoteResult g_VoteResult;     // The result of the last vote count

enum VoteResult {
    VoteResult_Waiting,
    VoteResult_Open,
    VoteResult_PassedByVote,
    VoteResult_PassedByAutoEnable,
    VoteResult_PassedByAdmin,
    VoteResult_Closed,
    VoteResult_Count
}

/**
 * A votemap is a stringmap with KvP of ClientName -> 1. The value is arbitrary,
 * the key is unique, and most logic is driven by the size of the map.
 */
methodmap Votes < StringMap {
    public static Votes Instance() {
        if (g_Votes == null) Votes.Init();
        return g_Votes;
    }
    // Unassuming constructor
    public static void Init() {
        g_Cvar_VoteThreshold = CreateConVar("rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);
	    g_Cvar_AutoEnableThreshold = CreateConVar("rtu_auto_enable_threshold", "16", "Number of players required at end of waiting stage to auto-enable upgrades. A value of 0 disables auto-enable. [16, 0..]", 16, true, 0.0, false);
        g_Votes = view_as<Votes>(new StringMap());
    }

    // Call on round start or map start
    public void Reset() {
        this.PlayerCount = 0;
        this.WaitingForPlayers = true;
        this.Revert();
    }

    // Call when upgrades are set to disabled
    public void Revert() {
        this.Clear();
        g_VoteResult = VoteResult_Open;
        g_VotingClosed = false;
    }

    // PlayerCount helps determine required number of votes
    property int PlayerCount {
        public get()           { return PlayerCount; }
        public set (int count) { PlayerCount = count; }
    }

    // No voting during the waiting phase
    property bool WaitingForPlayers {
        public get()              { return WaitingForPlayers;}
        public set (bool waiting) { WaitingForPlayers = waiting; }
    }

    // Holds the result of a successful count
    property bool Closed {
        public get()         { return g_VotingClosed; }
        public set(bool val) { g_VotingClosed = val; }
    }

    // Add a vote and trigger a count
    public bool Add(int client) {
        if (!this.Possible(client)) { return false; }

        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));

        this.SetValue(clientName, 1);

        PrintToChatAll("[RTU] %t", "RTU Requested", clientName, this.Size, this.Needed);

        return true;
    }

    // Call when a client disconnects. Should be followed by a count
    public void Drop(int client) {
        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));
        this.Remove(clientName);
        this.PlayerCount--;
    }

    // Count votes and determine if the threshold has been met
    // Only the first successful count will return true, at which point the
    // property `Closed` ought to be checked instead
    public static bool Count(bool force=false) {
        return Votes.Instance().Countable(force)
            && Votes.Instance().CheckResult(force);
    }

    public bool CheckResult(bool force=false) {
             if (force)           this.SetResult(VoteResult_PassedByAdmin);
        else if (this.PassByAuto) this.SetResult(VoteResult_PassedByAutoEnable);
        else if (this.PassByVote) this.SetResult(VoteResult_PassedByVote);

        return g_VoteResult > VoteResult_Open;
    }

    public void SetResult(VoteResult vr) {
        g_VoteResult = vr;
        this.Closed = true;
    }


    // Minimum sanity checks. Might be redundant. Except `!this.Closed`, that prevents multiple succesful count events
    public bool Countable(bool force=false) {
        if (this.Closed) return false;
        if (force) return true;

        return !WaitingForPlayers
            && PlayerCount > 0
            && this.Needed > 0;
    }

    // Check if it's safe to vote
    public bool Possible(int client) {
        // No early voting
        if (WaitingForPlayers) {
            ReplyToCommand(client, "[RTU] %t", "RTU Not Allowed");
            return false;
        }

        // No need to vote
        if (this.Closed) { // "RTU Already Enabled"
            ReplyToCommand(client, "[RTU] %t", "RTU Voting Closed");
            return false;
        }

        // No double voting
        if (this.Voted(client)) {
            ReplyToCommand(client, "[RTU] %t", "RTU Already Voted", this.Size, this.Needed);
            return false;
        }

        return true;
    }

    // Check if client has already voted
    public bool Voted(int client) {
        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));
        return this.ContainsKey(clientName);
    }

    property VoteResult Result {
        public get()              { return g_VoteResult; }
        public set(VoteResult vr) { g_VoteResult = vr; }
    }

    // Get required number of votes from a percentage of connected player count. Ensure a minimum of 1 to prevent unintended activation
    property int Needed {
        public get() {
            if (this.PlayerCount <= 1) return 1;

            return RoundToCeil(
                float(this.PlayerCount) * g_Cvar_VoteThreshold.FloatValue);
        }
    }

    property bool PassByAdmin {
        public get() {
            return g_VoteResult == VoteResult_PassedByAdmin;
        }
    }

    // One of two ways the vote can pass
    property bool PassByVote {
        public get() {
            return this.Size >= this.Needed;
        }
    }

    // The other way a vote can pass - without a single vote
    property bool PassByAuto {
        public get() {
            return this.AutoAllowed && this.AutoCountMet;
        }
    }

    // Whether or not Auto Pass is allowed by server config
    property bool AutoAllowed {
        public get() {
            return g_Cvar_AutoEnableThreshold.IntValue > 0;
        }
    }

    // Whether or not we have enough players to auto pass
    property bool AutoCountMet {
        public get() {
            return PlayerCount >= g_Cvar_AutoEnableThreshold.IntValue;
        }
    }
}
