    // void OnPluginStart {
    //  Upgrades.InitSFXArrays();
    // }
    // void OnMApStart? {
    // Upgrades.Instance().PrecacheSFX();
    // }

    // void OnPluginEnd {
    // 		SFX_UPGRADES_ENABLE.Clear();
    //		SFX_UPGRADES_DISABLE.Clear();
    //}

#include <json/helpers/metastringmap>
#include <tf2>

// A random sound to play when upgrades are enabled
static ArrayList SFX_UPGRADES_ENABLE;
static char pathsForEnableSFX[][] = {
	"mvm/mvm_bought_in.wav",
	"vo/engineer_mvm_collect_credits01.mp3",
	"vo/heavy_mvm_collect_credits01.mp3",
	"vo/medic_mvm_collect_credits03.mp3",
	"vo/medic_mvm_collect_credits04.mp3",
	"vo/soldier_mvm_collect_credits01.mp3",
	"vo/soldier_mvm_collect_credits02.mp3"
};

// A random sound to play when upgrades are disabled
static ArrayList SFX_UPGRADES_DISABLE;
static char pathsForDisableSFX[][] = {
	"vo/announcer_sd_monkeynaut_end_crash01.mp3",
	"vo/demoman_sf12_badmagic07.mp3",
	"mvm/mvm_player_died.wav"
};

methodmap RTU_SFX < MetaStringMap {
	// Copy static string arrays into ArrayLists to facilitate random access
	public static void InitSFXArrays() {
		SFX_UPGRADES_ENABLE = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
		for (int i = 0; i < sizeof(pathsForEnableSFX); i++)
			SFX_UPGRADES_ENABLE.PushString(pathsForEnableSFX[i]);

		SFX_UPGRADES_DISABLE = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
		for (int i = 0; i < sizeof(pathsForDisableSFX); i++)
			SFX_UPGRADES_DISABLE.PushString(pathsForDisableSFX[i]);
	}

	// Needs to be called every OnMapStart when asset precaching occurs
	// TODO: Is this even required?
	public void PrecacheSFX() {
		// if (Upgrades.Instance().Ready) return;

		// Precache all sounds
		for (int i = 0; i < SFX_UPGRADES_ENABLE.Length; i++) {
			char sfx[PLATFORM_MAX_PATH];
			SFX_UPGRADES_ENABLE.GetString(i, sfx, sizeof(sfx));
			PrecacheSound(sfx, true);
		}

		for (int i = 0; i < SFX_UPGRADES_DISABLE.Length; i++) {
			char sfx[PLATFORM_MAX_PATH];
			SFX_UPGRADES_DISABLE.GetString(i, sfx, sizeof(sfx));
			PrecacheSound(sfx, true);
		}
	}

    public static void UpgradesEnabled() {
        RTU_SFX.PlayRandom(SFX_UPGRADES_ENABLE);
    }
    public static void UpgradesDisabled() {
        RTU_SFX.PlayRandom(SFX_UPGRADES_DISABLE);
    }
    	// Accompany state changes with appropriate SFX
	public static void PlayRandom(ArrayList soundArray) {
		int rand = GetRandomInt(0, soundArray.Length - 1)
		char sound[PLATFORM_MAX_PATH];
		soundArray.GetString(rand, sound, sizeof(sound));
		EmitSoundToAll(sound);
	}
}
