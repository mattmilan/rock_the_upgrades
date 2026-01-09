#if defined _bank_printer_included
 #endinput
#endif
#define _bank_printer_included

methodmap BankPrinter < Handle {
    // Debug. Print all accounts in a table. Optional client filter
    public static void PrintToServer(Bank b) {
        if (b.Size == 0) {
            PrintToServer("[RTU][Accounts] No data");
            return;
        }

        PrintToServer("=================================================================================");
        PrintToServer("[RTU] Accounts [BONUS RED:%.2f] [BONUS BLU:%.2f]", b.StartingCurrency(TFTeam_Red), b.StartingCurrency(TFTeam_Blue));
        PrintToServer("---------------------------------------------------------------------------------");
        PrintToServer("Account Key           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
        PrintToServer("---------------------------------------------------------------------------------");
        BankPrinter.PrintAccounts(b);
        PrintToServer("=================================================================================");
    }

    public static void PrintAccounts(Bank b) {
        // Iteration vars
        Account a;
        char accountKey[MAX_AUTHID_LENGTH];
        StringMapSnapshot keys = b.Snapshot();
        if (keys.Length == 0) {
            PrintToServer("%20s", "No Data");
            return;
        }
        // Iterate Accounts
        for(int i = 0; i < keys.Length; i++) {
            // Grab Account Key (SteamID|Name)
            keys.GetKey(i, accountKey, sizeof(accountKey));

            bool accountFound = b.Data.GetArray(accountKey, a, sizeof(Account));

            PrintToServer(
                accountFound ?
                    a.ToRow() :
                    "[RTU][Accounts] No account found for Account Key: %s", accountKey);
        }
    }

    public static void PrintAccount(Bank b, int client) {
        Account a;

        bool accountFound = Accounts.Find(b, client, a);

        ReplyToCommand(client,
            accountFound ?
                a.ToString() :
                "[RTU][Accounts] No account found for Client: %d", client);
    }

    public static void PrintAccountKeys(Bank b) {
        StringMapSnapshot keys = b.Snapshot();

        for(int i = 0; i < keys.Length; i++) {
            // Grab Account Key (SteamID|Name)
            int size = keys.KeyBufferSize(i);
            char[] accountKey = new char[size];
            keys.GetKey(i, accountKey, size);
            PrintToServer("[RTU][Accounts] Index: %d, Account Key: %s", i, accountKey);
        }
    }
}
