#if defined _bank_printer_included
 #endinput
#endif
#define _bank_printer_included

methodmap BankPrinter < Handle {
    // Debug. Print all PlayerProfiles in a table. Optional client filter
    public static void PrintToServer(Bank b) {
        if (b.Size == 0) {
            PrintToServer("[RTU][PlayerProfiles] No data");
            return;
        }

        PrintToServer("=================================================================================");
        PrintToServer("[RTU] PlayerProfiles [BONUS RED:%.2f] [BONUS BLU:%.2f]", b.StartingCurrency(TFTeam_Red), b.StartingCurrency(TFTeam_Blue));
        PrintToServer("---------------------------------------------------------------------------------");
        PrintToServer("PlayerProfile Key           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
        PrintToServer("---------------------------------------------------------------------------------");
        BankPrinter.PrintPlayerProfiles(b);
        PrintToServer("=================================================================================");
    }

    public static void PrintPlayerProfiles(Bank b) {
        // Iteration vars
        PlayerProfile playerProfile;
        char playerProfileKey[MAX_NAME_LENGTH];
        StringMapSnapshot keys = PlayerProfiles.Instance().Snapshot();
        if (keys.Length == 0) {
            PrintToServer("%20s", "No Data");
            return;
        }
        // Iterate PlayerProfiles
        for(int i = 0; i < keys.Length; i++) {
            // Grab PlayerProfile Key (SteamID|Name)
            keys.GetKey(i, playerProfileKey, sizeof(playerProfileKey));

            // bool playerProfileFound = PlayerProfiles.Find(playerProfileKey, playerProfile) > 0;
            bool playerProfileFound = PlayerProfiles.Instance().GetArray(playerProfileKey, playerProfile, sizeof(PlayerProfile));

            PrintToServer(
                playerProfileFound ?
                    playerProfile.ToRow() :
                    "[RTU][PlayerProfiles] No PlayerProfile found for PlayerProfile Key: %s", playerProfileKey);
        }
    }

    public static void PrintPlayerProfile(Bank bank, int client) {
        PlayerProfile playerProfile;

        bool playerProfileFound = PlayerProfiles.Find(client, playerProfile);

        ReplyToCommand(client,
            playerProfileFound ?
                playerProfile.ToString() :
                "[RTU][PlayerProfiles] No PlayerProfile found for Client: %d", client);
    }

    public static void PrintPlayerProfileKeys(Bank b) {
        StringMapSnapshot keys = b.Snapshot();

        for(int i = 0; i < keys.Length; i++) {
            // Grab PlayerProfile Key (SteamID|Name)
            int size = keys.KeyBufferSize(i);
            char[] playerProfileKey = new char[size];
            keys.GetKey(i, playerProfileKey, size);
            PrintToServer("[RTU][PlayerProfiles] Index: %d, PlayerProfile Key: %s", i, playerProfileKey);
        }
    }
}
