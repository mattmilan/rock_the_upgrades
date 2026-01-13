#if defined _bank_transactions_natives_included
 #endinput
#endif
#define _bank_transactions_natives_included

enum BTType {
    BTBalance,
    BTWithdrawl,
    BTDeposit,
    BTBurn,
    BTCount
}

enum struct BTData {
    BTType type;
    float amount;
    int client;

    void Set(int client, BTType type, float amount) {
        this.client = client;
        this.type = type;
        this.amount = amount;
    }
}


methodmap BankTransaction {
    public BankTransaction(Bank bank, BTData data) {
        BankTransaction self = view_as<BankTransaction>(new TypedStringMap());
        return self;
    }

    public BankTransaction Execute() {
        // Fetch account
        // Account a; Accounts.Find(view_as<Bank>(this.bank), this.data.client, a);

        // // Adjust all gains by global multiplier
        // this.data.amount *= view_as<Bank>(this.bank).Multiplier;

        // // Apply amount to the correct field
        // switch (this.data.type) {
        //     // Currency gained from events or chat commands
        //     case BTDeposit:
        //         a.earned += this.data.amount;
        //     // Purchasing or refunding upgrades
        //     case BTWithdrawl:
        //         a.classData[a.currentClass].spent += this.data.amount;
        //     // Purchasing non-refundables
        //     case BTBurn:
        //         a.classData[a.currentClass].burnt += this.data.amount;
        // }

        // // enforce currency limit (disabled by default)
        // a.earned = view_as<Bank>(this.bank).LimitCurrency(a.earned);

        // // Save changes before syncing
        // // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
        // view_as<Bank>(this.bank).Data.SetArray(a.accountKey, a, sizeof(Account));

        // // Match the client's currency to account balance
        // view_as<Bank>(this.bank).Sync(this.data.client);
        return this;
    }
}

methodmap Withdrawl < BankTransaction {
    public Withdrawl(Bank bank, BTData data) {

            // static BTData d = data;


        Withdrawl self = view_as<Withdrawl>(new TypedStringMap());
        // self.Data = bank;

        return self;
    }

    public Withdrawl Execute() {}
}

methodmap Deposit < BankTransaction {
    public Deposit(Bank bank, BTData data) {
        BankTransaction(bank, data);
        Deposit self = view_as<Deposit>(new TypedStringMap());
        // self.Data = bank;
        return self;
    }
}

void Example() {
    BTData btData;
    btData.Set(.client = 1, .type = BTWithdrawl, .amount = 500.0);

    Bank b = new Bank(100.0, 1.0, 0.0);
    Withdrawl(b, btData).Execute();
    Deposit(b, btData).Execute();
}

// methodmap BankTransaction < MetaStringMap {
//     // public BankTransaction(Bank bank, BTData data) {
//     //     BankTransaction self = view_as<BankTransaction>(new MetaStringMap());
//     //     self.Data = bank;
//     //     return self
//     // }
//     public BankTransaction(Bank bank, BTData data) {
//         BankTransaction self = view_as<BankTransaction>(new MetaStringMap());
//         self.Data = bank;
//         return self
//     }

//     public BankTransaction Execute() {
//         // Account a; Accounts.Find(view_as<Bank>(this), this.client, a);

//         // // Adjust all gains by global multiplier
//         // this.amount *= view_as<Bank>(this).Multiplier;

//         // // Apply amount to the correct field
//         // switch (this.bankTransactionType) {
//         //     // Currency gained from events or chat commands
//         //     case Account_Deposit:
//         //         a.earned += this.amount;
//         //     // Purchasing or refunding upgrades
//         //     case Account_Withdrawl:
//         //         a.classData[a.currentClass].spent += this.amount;
//         //     // Purchasing non-refundables
//         //     case Account_Burn:
//         //         a.classData[a.currentClass].burnt += this.amount;
//         // }

//         // // enforce currency limit (disabled by default)
//         // a.earned = view_as<Bank>(this).LimitCurrency(a.earned);

//         // // Save changes before syncing
//         // // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
//         // view_as<Bank>(this).Data.SetArray(a.accountKey, a, sizeof(Account));

//         // // Match the client's currency to account balance
//         // view_as<Bank>(this).Sync(this.client);
//         return this;
//     }
// }



// static Bank b;
// static BTData tData;

// methodmap BankTransaction < BTData {
//     public BankTransaction(Bank bank, BTData data) {

//         tData = data;
//         b = bank;

//         return view_as<BankTransaction>(bank);
//     }

//     public static BankTransaction Build() {
//         return BankTransaction(b,tData);
//     }

//     public BankTransaction Execute() {
//          // Fetch account
//         Account a; Accounts.Find(b, tData.client, a);

//         // Adjust all gains by global multiplier
//         tData.amount *= b.Multiplier;


//         // Apply amount to the correct field
//         switch (tData.bankTransactionType) {
//             // Currency gained from events or chat commands
//             case Transaction_Deposit:
//                 a.earned += tData.amount;
//             // Purchasing or refunding upgrades
//             case Transaction_Withdraw:
//                 a.classData[a.currentClass].spent += tData.amount;
//             // Purchasing non-refundables
//             case Transaction_Burn:
//                 a.classData[a.currentClass].burnt += tData.amount;
//         }

//         // enforce currency limit (disabled by default)
//         a.earned = b.LimitCurrency(a.earned);

//         // Save changes before syncing
//         // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
//         b.Data.SetArray(a.accountKey, a, sizeof(Account));

//         // Match the client's currency to account balance
//         view_as<Bank>(this).Sync(tData.client);
//         return this;
//     }
// }



// static BTData tData;

// methodmap BankTransaction __nullable__ {
//     public BankTransaction(Bank bank, BTData data) {
//         // static BTData tata;
//         // tData = data;
//         return view_as<BankTransaction>(bank);
//     }
//     // public Transaction(float amount, int client, BANK_TRANSACTION bankTransactionType) {
//     //     BTData tData;

//     //     tData.amount = amount;
//     //     tData.client = client;
//     //     tData.bankTransactionType = bankTransactionType;

//     //     return view_as<Transaction>(tData);
//     // }

//     // public static BankTransaction Build(float amount, int client, BANK_TRANSACTION bankTransactionType) {
//     //     tData.amount = amount;
//     //     tData.client = client;
//     //     tData.bankTransactionType = bankTransactionType;

//     //     return new BankTransaction();
//     // }

//     public static BankTransaction Execute(Bank b, BTData data) {
//         // Fetch account
//         Account a; Accounts.Find(b, data.client, a);

//         // Adjust all gains by global multiplier
//         data.amount *= b.Multiplier;

//         // Apply amount to the correct field
//         switch (data.bankTransactionType) {
//             // Currency gained from events or chat commands
//             case BANK_DEPOSIT:
//                 a.earned += data.amount;
//             // Purchasing or refunding upgrades
//             case BANK_WITHDRAW:
//                 a.classData[a.currentClass].spent += data.amount;
//             // Purchasing non-refundables
//             case BANK_BURN:
//                 a.classData[a.currentClass].burnt += data.amount;
//         }

//         // enforce currency limit (disabled by default)
//         a.earned = b.LimitCurrency(a.earned);

//         // Save changes before syncing
//         // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
//         b.Data.SetArray(a.accountKey, a, sizeof(Account));

//         // Match the client's currency to account balance
//         b.Sync(data.client);
//     }

// }


// public void Native_Bank_Transaction(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));
//     BANK_TRANSACTION transaction = view_as<BANK_TRANSACTION>(GetNativeCell(4));

//     // Fetch account
//     Account a; Accounts.Find(b, client, a);

//     // Adjust all gains by global multiplier
//     amount *= b.Multiplier;

//     // Apply amount to the correct field
//     switch (transaction) {
//         // Currency gained from events or chat commands
//         case BANK_DEPOSIT:
//             a.earned += amount;
//         // Purchasing or refunding upgrades
//         case BANK_WITHDRAW:
//             a.classData[a.currentClass].spent += amount;
//         // Purchasing non-refundables
//         case BANK_BURN:
//             a.classData[a.currentClass].burnt += amount;
//     }

//     // enforce currency limit (disabled by default)
//     a.earned = b.LimitCurrency(a.earned);

//     // Save changes before syncing
//     // NOTE: metastringmap does not lift SetArray to Data - it points to Meta
//     b.Data.SetArray(a.accountKey, a, sizeof(Account));

//     // Match the client's currency to account balance
//     b.Sync(client);
// }

// public void Native_Bank_DepositTarget(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));

//     // Target info
//     char target[MAX_NAME_LENGTH]; /*=*/ GetNativeString(2, target, MAX_NAME_LENGTH);
//     float amount = view_as<float>(GetNativeCell(3));
//     int replyTo = view_as<int>(GetNativeCell(4));

//     if (StrEqual(target, "all", false)) {
//         b.DepositAll(amount);
//     } else if (StrEqual(target, "red", false)) {
//         b.DepositAll(amount, .team=TFTeam_Red);
//     } else if (StrEqual(target, "blu", false) || StrEqual(target, "blue", false)) {
//         b.DepositAll(amount, .team=TFTeam_Blue);
//     } else {
//         int targetClientId = FindTarget(replyTo, target, true);

//         if (ValidClient(targetClientId)) b.Deposit(amount, targetClientId);
//         else ReplyToCommand(replyTo, "[RTU] Could not find player %s", target);
//     }
// }

// public void Native_Bank_DepositAll(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     float amount = view_as<float>(GetNativeCell(2));
//     TFTeam team = view_as<TFTeam>(GetNativeCell(3));

//     // Increase bonus starting currency for late joiners
//     b.AddBonus(amount, team);

//     // Iteration vars
//     Account a;
//     char accountKey[MAX_AUTHID_LENGTH];

//     // Iterate by key
//     StringMapSnapshot keys = b.Snapshot();

//     // for all accounts
//     for(int i = 0; i < keys.Length; i++) {
//         // Grab Account Key (SteamID|Name)
//         keys.GetKey(i, accountKey, MAX_AUTHID_LENGTH);

//         // Fetch account
//         b.Data.GetArray(accountKey, a, sizeof(Account));

//         // Get paid if team matches optional filter
//         if (FilterTeam(a.currentTeam, team)) b.Deposit(amount, a.client);
//     }
// }

// public void Native_Bank_Deposit(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));

//     b.Transaction(amount, client, BANK_DEPOSIT);
// }

// public void Native_Bank_Withdraw(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));

//     b.Transaction(amount, client, BANK_WITHDRAW);
// }

// public void Native_Bank_Burn(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));

//     b.Transaction(amount, client, BANK_BURN);
// }

// public void Native_Bank_Refund(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));

//     b.Withdraw(-amount, client);
// }

// public void Native_Bank_Forgive(Handle plugin, int numParams) {
//     Bank b = view_as<Bank>(GetNativeCell(1));
//     int client = view_as<int>(GetNativeCell(2));
//     float amount = view_as<float>(GetNativeCell(3));

//     b.Burn(-amount, client);
// }
char[] TransactionTypeString(BTType tType) {
    char buffer[16];
    switch (tType) {
        case BTBalance: buffer = "Balance";
        case BTWithdrawl: buffer = "Withdrawl";
        case BTDeposit: buffer ="Deposit";
        case BTBurn: buffer ="Burn";
        default: buffer = "Unknown";
    }
    return buffer;
}