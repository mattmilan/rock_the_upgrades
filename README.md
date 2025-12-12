# Rock The Upgrades
A SourceMod plugin for Team Fortress 2, which enables MvM upgrades on any map - no mapscript.nut or map editing required.

The plugin provides a chat command (!rtu) which triggers a vote, similar to the "Rock the Vote" SourceMod plugin (from which this plugin borrows, quite heavily). Several admin commands are also provided to manage state.

If the vote is successful, the upgrades system is activated, and a `func_upgradestation` is wrapped around every `func_regenerate`.

This allows players to open and close the upgrades menu by simply approaching any resupply locker.

Currency may be gained via a number of configurable strategies, including global multipliers to easily adjust difficulty without needing to manage each strategy individually.

## Usage
Install sourcemod, then move this plugin and its dependencies into the following folders
```cmd
../sourcemod/gamedata/tf2.attributes.txt
../sourcemod/plugins/rock_the_upgrades-x86.smx
../sourcemod/plugins/tf2attributes.smx
../sourcemod/translations/rock_the_upgrades.phrases.txt
```

then connect to your server and chat `!rtu` to see the plugin in action

## Acknowledgements

This plugin was inspired by the community map [Freaky Fair](https://steamcommunity.com/sharedfiles/filedetails/?id=3326591381), which quickly became a favorite among the community, and left them hungry for more.

This plugin is not a 1:1 port of the Freaky Fair functionality - which comes with a great deal of polish and flair, but rather a stripped down version focused on the upgrade system, with a strong desire to avoid any map editing and all the other issues associated with that approach.

Originally designed for the [Bangerz.tf](Bangerz.tf) Engineer Fortress game mode, it includes enough configurability to be a good fit for any game mode.

None of this would be possible without [SourceMod](https://github.com/alliedmodders/sourcemod) and [AlliedModders](https://forums.alliedmods.net/).

Reverting upgrades was greatly facilitated by [TF2Attributes](https://github.com/FlaminSarge/tf2attributes).

Finally a big thanks to Dynamilk of the  [Bangerz.tf](Bangerz.tf) community for his support and collaboration during development.

## Future Plans
x64 Compatibility (most likely pending Sourcemod 1.13 stable)

TBD
