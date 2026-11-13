> [!IMPORTANT]
> The repository has been made private due to abuse. To request access, open an issue and use the `Request Access` label.

# Rock The Upgrades

A SourceMod plugin for Team Fortress 2 which enables MvM upgrades on any map - no mapscript.nut or map editing required.

## Usage

RTU may be activated by votes or by server configuration. Voting starts after the waiting phase has ended.

Voting is done with `/rtu`, which will also open the upgrades menu after the vote has passed.

Players will be given starting currency (defaults to 250). Two systems are provided for earning currency:

- `K/D/A` kills, destructions, and assists
- `Score` any increase to player's score

Regardless of which system is active, bonus currency is granted for objectives.

Several admin commands and convars are provided for management and fine tuning.

## Installation

RTU depends on [SMVscriptComms](https://github.com/Bradasparky/sm_vscript_comms).

Ensure the plugin and dependencies are copied as follows:

```
/tf
  /scripts
    /vscripts
      sm-vscript-comms.nut
      /sm-vscript-comms
        custom_scripts.nut
  /addons
    /sourcemod
      /plugins
        rock_the_upgrades.smx
        sm_vscript_comms.smx
      /translations
        rock_the_upgrades.phrases.txt
```
*custom_scripts.nut is not required but sm-vscript-comms.nut might bark if it's missing

## Limitations

Given that this was designed specifically for Engineer Fortress, it's compatibility with other games modes has not been tested.

There should be no platform limitations as there is no reliance on specific architecture or signatures.

## Acknowledgements

Inspired by the community map [Freaky Fair](https://steamcommunity.com/sharedfiles/filedetails/?id=3326591381)

Designed for Engineer Fortress in collaboration with it's owner Dynamilk [Bangerz.tf](https://bangerz.tf/) ([Discord](https://discord.gg/bangerz))

Rescued from several blockers by numerous members of [AlliedModders](https://forums.alliedmods.net/) ([Discord](https://discord.com/invite/CXeUtHbxzH))

Depends on [SMVscriptComms](https://github.com/Bradasparky/sm_vscript_comms)
