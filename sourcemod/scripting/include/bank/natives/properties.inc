#if defined _bank_properties_natives_included
 #endinput
#endif
#define _bank_properties_natives_included

/* PROPERTIES */

public any Native_Bank_GetAuthable(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("authable", out); return out;
}

public any Native_Bank_GetMultiplier(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("multiplier", out); return out;
}

public any Native_Bank_GetLimit(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("limit", out); return out;
}

public any Native_Bank_GetStarting(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	float out; b.Meta.GetValue("starting", out); return out;
}

public any Native_Bank_GetKeys(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	return view_as<ArrayList>(b.Meta.GetHandle("keys"));
}

public any Native_Bank_GetBonuses(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
	return view_as<ArrayList>(b.Meta.GetHandle("bonuses"));
}

// Prop Set
public void Native_Bank_SetAuthable(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    bool val = view_as<bool>(GetNativeCell(2));
	b.Meta.SetValue("authable", val);
}

public void Native_Bank_SetMultiplier(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("multiplier", val);
}

public void Native_Bank_SetLimit(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("limit", val);
}

public void Native_Bank_SetStarting(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    float val = view_as<float>(GetNativeCell(2));
	b.Meta.SetValue("starting", val);
}

public void Native_Bank_SetKeys(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    ArrayList val = view_as<ArrayList>(GetNativeCell(2));
	b.Meta.SetValue("keys", val);
}

public void Native_Bank_SetBonuses(Handle plugin, int numParams) {
	Bank b = view_as<Bank>(GetNativeCell(1));
    ArrayList val = view_as<ArrayList>(GetNativeCell(2));
	b.Meta.SetValue("bonuses", val);
}
