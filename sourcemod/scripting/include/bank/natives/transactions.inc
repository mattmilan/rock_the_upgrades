#if defined _bank_transactions_natives_included
 #endinput
#endif
#define _bank_transactions_natives_included

public void Native_Bank_Transaction(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));
    BANK_TRANSACTION transaction = view_as<BANK_TRANSACTION>(GetNativeCell(4));

    // Fetch account
    Account a; Accounts.Find(b, client, a);

    // Adjust all gains by global multiplier
    amount *= b.Multiplier;

    // Apply amount to the correct field
    switch (transaction) {
        // Currency gained from events or chat commands
        case BANK_DEPOSIT:
            a.earned += amount;
        // Purchasing or refunding upgrades
        case BANK_WITHDRAW:
            a.classData[a.currentClass].spent += amount;
        // Purchasing non-refundables
        case BANK_BURN:
            a.classData[a.currentClass].burnt += amount;
    }

    // enforce currency limit (disabled by default)
    a.earned = b.LimitCurrency(a.earned);

    // Save changes before syncing
    // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
    b.Data.SetArray(a.accountKey, a, sizeof(Account));

    // Match the client's currency to account balance
    b.Sync(client);
}

public void Native_Bank_DepositTarget(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));

    // Target info
    char target[MAX_NAME_LENGTH]; /*=*/ GetNativeString(2, target, MAX_NAME_LENGTH);
    float amount = view_as<float>(GetNativeCell(3));
    int replyTo = view_as<int>(GetNativeCell(4));

    if (StrEqual(target, "all", false)) {
        b.DepositAll(amount);
    } else if (StrEqual(target, "red", false)) {
        b.DepositAll(amount, .team=TFTeam_Red);
    } else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
        b.DepositAll(amount, .team=TFTeam_Blue);
    } else {
        int targetClientId = FindTarget(replyTo, target, true);

        if (ValidClient(targetClientId)) b.Deposit(amount, targetClientId);
        else ReplyToCommand(replyTo, "[RTU] Could not find player %s", target);
    }
}

public void Native_Bank_DepositAll(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    float amount = view_as<float>(GetNativeCell(2));
    TFTeam team = view_as<TFTeam>(GetNativeCell(3));

    // Increase bonus starting currency for late joiners
    b.AddBonus(amount, team);

    // Iteration vars
    Account a;
    char accountKey[MAX_AUTHID_LENGTH];

    // Iterate by key
    StringMapSnapshot keys = b.Snapshot();

    // for all accounts
    for(int i = 0; i < keys.Length; i++) {
        // Grab Account Key (SteamID|Name)
        keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

        // Fetch account
        b.Data.GetArray(accountKey, a, sizeof(Account));

        // Get paid if team matches optional filter
        if (FilterTeam(a.currentTeam, team)) b.Deposit(amount, a.client);
    }
}

public void Native_Bank_Deposit(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_DEPOSIT);
}

public void Native_Bank_Withdraw(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_WITHDRAW);
}

public void Native_Bank_Burn(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Transaction(amount, client, BANK_BURN);
}

public void Native_Bank_Refund(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Withdraw(-amount, client);
}

public void Native_Bank_Forgive(Handle plugin, int numParams) {
    Bank b = view_as<Bank>(GetNativeCell(1));
    int client = view_as<int>(GetNativeCell(2));
    float amount = view_as<float>(GetNativeCell(3));

    b.Burn(-amount, client);
}
