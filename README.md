# Rock The Upgrades
A SourceMod plugin for Team Fortress 2, which enables MvM upgrades on any map - no mapscript.nut or map editing required.

The plugin provides a chat command (!rtu) which triggers a vote, similar to the "Rock the Vote" SourceMod plugin (from which this plugin borrows, quite heavily).

If the vote is successful…

- The upgrade system is activated
  - The currency hud element becomes visible
  - Currency may be gained
  - Upgrades may be purchased
- A `func_upgradestation` is added to each `func_regenerate` entity (resupply lockers)

… and as a result, approaching a resupply locker will open the upgrades menu.

Currency may be gained via a number of configurable strategies, to be developed.

## TODO
- Add configurable currency strategies
  - Gains
    - kills/damage/healing
    - time elapsed (w/wo dying)
    - headshots/crits
    - pickups
  - Losses
      - death
      - time elapsed without damage, healing, or killing
      - damage received (limit scope, ie ignore damage from sentry guns)
  - Limits
    - points captured
    - time elapsed
    - total team kills
    - arbitrary amounts (prevent players from fully maxing upgrades, forcing them to choose those most valuable)
    
