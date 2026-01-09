#include <rock_the_upgrades/bank>

methodmap BankPrinter < Bank {
    public BankPrinter(Bank b) {
        return view_as<BankPrinter>(b);
    }

    // Debug. Print all accounts in a table. Optional client filter
    public void PrintToServer() {
        if (this.Size == 0) {
            PrintToServer("[RTU][Accounts] No data");
            return;
        }

        PrintToServer("=================================================================================");
        PrintToServer("[RTU] Accounts [BONUS RED:%.2f] [BONUS BLU:%.2f]", this.StartingCurrency(TFTeam_Red), this.StartingCurrency(TFTeam_Blue));
        PrintToServer("---------------------------------------------------------------------------------");
        PrintToServer("Account Key           | Client  | Connected? |  Class  |  Earnt  |  Spent  |  Burnt  | Balance ");
        PrintToServer("---------------------------------------------------------------------------------");
        this.PrintAccounts();
        PrintToServer("=================================================================================");
    }

    public void PrintAccounts() {
        // Iteration vars
        Account a;
        char accountKey[MAX_AUTHID_LENGTH];
        StringMapSnapshot keys = this.Snapshot();
        if (keys.Length == 0) {
            PrintToServer("%20s", "No Data");
            return;
        }
        // Iterate Accounts
        for(int i = 0; i < keys.Length; i++) {
            // Grab Account Key (SteamID|Name)
            keys.GetKey(i, accountKey, sizeof(accountKey));

            bool accountFound = this.Data.GetArray(accountKey, a, sizeof(Account));

            PrintToServer(
                accountFound ?
                    a.ToRow() :
                    "[RTU][Accounts] No account found for Account Key: %s", accountKey);
        }
    }

    public void PrintAccount(int client) {
        Account a;

        bool accountFound = this.Fetch(client, a);

        ReplyToCommand(client,
            accountFound ?
                a.ToString() :
                "[RTU][Accounts] No account found for Client: %d", client);
    }

    public void PrintAccountKeys() {
        StringMapSnapshot keys = this.Snapshot();

        for(int i = 0; i < keys.Length; i++) {
            // Grab Account Key (SteamID|Name)
            int size = keys.KeyBufferSize(i);
            char[] accountKey = new char[size];
            keys.GetKey(i, accountKey, size);
            PrintToServer("[RTU][Accounts] Index: %d, Account Key: %s", i, accountKey);
        }
    }
}
