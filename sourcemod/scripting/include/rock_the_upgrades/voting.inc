/**
 * Rock The Upgrades: Voting
 * Handles player voting to enable the upgrade system
 */

#if defined _voting_included
 #endinput
#endif
#define _voting_included


typedef VoteVotedCallback = function void (VoteResult arg);

ConVar g_Cvar_VoteThreshold;        // A % of connected players
ConVar g_Cvar_AutoEnableThreshold;  // A flat number of players

static Votes instance;               // The Votes instance
static VoteState g_VoteState;     // The result of the last vote count


static VoteVotedCallback onVoteVoted;



/**
 * A votemap is a stringmap with KvP of ClientName -> 1. The value is arbitrary,
 * the key is unique, and most logic is driven by the size of the map.
 * TODO: decouple.
 */
methodmap Votes < MetaStringMap {
    public static Votes Instance() {
        if (instance == null) ThrowError("Votes instance not initialized. Call Votes.Init(callback) during OnPluginStart.");

        return instance;
    }

    // Unassuming constructor
    public static void Init(VoteVotedCallback voteCallback) {
        if (instance != null) ThrowError("Votes instance already initialized.");

        onVoteVoted = voteCallback;

        g_Cvar_VoteThreshold = CreateConVar("rtu_voting_threshold", "0.55", "Percentage of players needed to enable upgrades. A value of zero will start the round with upgrades enabled. [0.55, 0..1]", 0, true, 0.0, true, 1.0);
	    g_Cvar_AutoEnableThreshold = CreateConVar("rtu_auto_enable_threshold", "16", "Number of players required at end of waiting stage to auto-enable upgrades. A value of 0 disables auto-enable. [16, 0..]", 16, true, 0.0, false);
        instance = view_as<Votes>(new MetaStringMap());
    }

    // TF2_OnWaitingForPlayersEnd
    public static bool Start() {
        if (instance.Counted) return false;

        g_VoteState = Vote_Voting;

        // check auto-enable (0 for server-client)
        return Votes.Count(0, g_VoteState);
    }

    // TF2_OnWaitingForPlayersStart
    public static bool Wait() {
        if (instance.Counted) return false;

        g_VoteState = Vote_Waiting;

        return true;
    }

    // OnVoteReverted (ie OnUpgradesDisabled)
    public static void Revert() {
        // TODO: Dynamically detect PlayerCount?
        instance.Clear();
        g_VoteState = Vote_Voting;
    }

    // OnMapEnd
    public static void Reset() {
        instance.VoterCount = 0;

        Votes.Revert();
    }

    public static bool Register(int client) {
        instance.VoterCount++;
        return true;
    }

    public static bool Vote(int client) {
        VoteState state;

        if (instance.Votable(client, state) ) {
            Votes.Add(client)
            Votes.Count(client, state);
            return true;
        }

        return false;
    }

    public static bool Add(int client) {
        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));
        return instance.SetInt(clientName, 1);
    }

    // Call when a client disconnects. Should be followed by a count
    public static bool Drop(int client) {
        instance.VoterCount--;

        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));
        instance.Remove(clientName);

        return Votes.Count(client, g_VoteState);
    }

    // No voting during the waiting phase
    property bool Waiting {
        public get() {
            return g_VoteState == Vote_Waiting
                || this.VoterCount < 3
                || this.Needed < 2;
            }
    }

    // Holds the result of a successful count
    property bool Voting {
        public get() {
            return g_VoteState == Vote_Voting
                && this.VoterCount > 2
                && this.Needed > 1;
            }
    }

    property bool Counted {
        public get() { return g_VoteState > Vote_Voting; }
    }

    // VoterCount helps determine required number of votes
    property int VoterCount {
        public get()           { return this.GetInt("voterCount"); }//{ return VoterCount; }
        public set (int count) { this.SetInt("voterCount", count); }
    }

    public static bool Force(int client) {
        instance.Forced = true;

        return Votes.Count(client, g_VoteState);
    }

    // Count votes and determine if the threshold has been met
    // Only the first successful count will return true, at which point the
    // property `Counted` ought to be checked instead
    public static bool Count(int client, VoteState state) {
        if (instance.Countable && !instance.Counted) {
            state = instance.CheckStatus();
        }

        instance.OnVoteVoted(client, state);

        return true;
    }

    public VoteState CheckStatus() {
        VoteState result = this.GetStatus();
        this.SetInt("result", result);
        return result;
    }

    public VoteState GetStatus() {
        if (this.Forced)     return Vote_PassedByAdmin;
        if (this.PassByAuto) return Vote_PassedByAutoEnable;
        if (this.PassByVote) return Vote_PassedByVote;
                             return g_VoteState; // current state
    }

    public void OnVoteVoted(int client, VoteState state) {
        // Call_StartFunction needs to know which plugin contains the callback function
        // Currently this file is an include so we pass the containing function via `INVALID HANDLE`
        // If we go native, our signature changes, and `plugin` will be the first param, as `Handle plugin`

        VoteResult result;
        result.client = client;
        result.size = this.Size;
        result.needed = this.Needed;
        result.voteState = state;

        Call_StartFunction(INVALID_HANDLE, onVoteVoted);
        Call_PushArray(result, sizeof(VoteResult));
		Call_Finish();
    }

    property VoteState Status {
        public get()               { return view_as<VoteState>(this.GetInt("result")); }
    }

    property bool Countable {
        public get() {
            return this.Forced
                || (this.Voting
                 && this.VoterCount > 2
                 && this.Needed > 1);

        }
    }

    property bool Forced {
        public get()           { return this.GetBool("forced"); }
        public set (bool flag) { this.SetBool("forced", flag); }
    }

    // Check if it's safe to vote
    public bool Votable(int client, VoteState &state) {
        // No early voting
        if (this.Waiting) state = Vote_Waiting;

        // No need to vote
        else if (this.Counted) state = Vote_Counted;

        // No double voting
        else if (this.Voted(client)) state = Vote_Exists;

        else state = Vote_Voting;

        return state == Vote_Voting;
    }

    // Check if client has already voted
    public bool Voted(int client) {
        char clientName[MAX_NAME_LENGTH];
        GetClientName(client, clientName, sizeof(clientName));
        return this.ContainsKey(clientName);
    }

    // Get required number of votes from a percentage of connected player count. Ensure a minimum of 1 to prevent unintended activation
    property int Needed {
        public get() {
            if (this.VoterCount < 3) return 2;

            return RoundToCeil(
                float(this.VoterCount) * g_Cvar_VoteThreshold.FloatValue);
        }
    }

    property bool PassByAdmin {
        public get() {
            return g_VoteState == Vote_PassedByAdmin;
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
            return this.VoterCount >= g_Cvar_AutoEnableThreshold.IntValue;
        }
    }
}
enum VoteState {
    // Voting Denied
    Vote_Exists,
    Vote_Counted,
    Vote_Waiting,

    // Voting Allowed
    Vote_Voting,

    // Voting Finished
    Vote_PassedByVote,
    Vote_PassedByAutoEnable,
    Vote_PassedByAdmin,

    // Iteration
    Vote_Count
}

/*
 *  The result of calling Votes.Add
 *  Used by the calling plugin to report result
 */
enum struct VoteResult {
    int client;
    int size;
    int needed;

    VoteState voteState;

    bool Denied() { return this.voteState > Vote_Voting && this.voteState < Vote_Count; }
    bool Accepted() { return this.voteState == Vote_Voting; }
    bool Passed() { return this.voteState > Vote_Voting && this.voteState < Vote_Count; }
}
