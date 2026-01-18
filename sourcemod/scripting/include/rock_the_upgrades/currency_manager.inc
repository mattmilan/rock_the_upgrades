#include <json/helpers/metastringmap>

static CurrencyManager g_CurrencyManager;

methodmap CurrencyManager {


    public static CurrencyManager Instance() {
        if (g_CurrencyManager == null) CurrencyManager.Init();

        return g_CurrencyManager;
    }

    public static void Init() {
        if (g_CurrencyManager != null) return;
        PrintToServer("[RTU][CurrencyManager] Initializing Currency Manager");

        g_CurrencyManager = view_as<CurrencyManager>(new MetaStringMap());
    }

    public static void Restart() {
        // No-op for now
    }

    public int Delta(int client) {
		float realCurrency = this.GetClientCurrency(client);
		float bankBalance = this.Balance(client);

		return RoundToNearest(bankBalance - realCurrency);
	}
	public bool ResolveDelta(int client) {
		int delta = this.Delta(client);
		if (delta == 0) return false;

		this.Withdraw(float(delta), client);

		return true;
	}

	public float GetClientCurrency(int client) {
		return ValidClient(client) ?
			float(GetEntProp(client, Prop_Send, "m_nCurrency")) :
			-1.0;
	}

	public bool SetClientCurrency(int client, float amount) {
		bool valid = ValidClient(client);
		if (valid) SetEntProp(client, Prop_Send, "m_nCurrency", RoundToCeil(amount));
		return valid;
	}
}
