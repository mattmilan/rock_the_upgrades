# Rock The Upgrades

A SourceMod plugin for Team Fortress 2 which enables MvM upgrades on any map - no mapscript.nut or map editing required.

## Overview

Players can chat `/rtu` to add a vote, similar to "Rock the Vote". Once enough votes are received

- the `m_nForceUpgrades` netprop is activated, enabling the upgrades system and currency HUD
- a `func_upgradestation` is added if not already present
- the touch events of each resupply locker (`func_regenerate`) are hooked to open/close the upgrades menu
- human clients are given 250 starting currency

After a vote passes, players can chat `/rtu` to open the upgrades menu.

Currency is earned individually from kills, assists, and destructions.

Currency is earned for the whole team from point and flag captures.

See the [Wiki](https://github.com/mattmilan/rock_the_upgrades/wiki) for more details.

## Setup

Copy the required files into the following locations

```cmd
../tf/addons/sourcemod/translations/rock_the_upgrades.phrases.txt
../tf/addons/sourcemod/plugins/rock_the_upgrades.smx
../tf/addons/sourcemod/plugins/sm_vscript_comms.smx
../tf/scripts/vscripts/sm-vscript-comms.nut
../tf/scripts/vscripts/sm_vscript_comms/custom_scripts.nut
```

then connect to your server and chat `/rtu` to see the plugin in action.

## Acknowledgements

Inspired by the community map [Freaky Fair](https://steamcommunity.com/sharedfiles/filedetails/?id=3326591381)

Designed for Engineer Fortress [Bangerz.tf](Bangerz.tf)

Depends on [SMVscriptComms](https://github.com/Bradasparky/sm_vscript_comms).

Created in collaboration with Dynamilk, owner and operator of [Bangerz.tf](Bangerz.tf)

## Future Plans

Custom Upgrades

Documentation

Code cleanup
