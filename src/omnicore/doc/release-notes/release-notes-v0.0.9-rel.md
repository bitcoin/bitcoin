Master Core version 0.0.9-rel is available from:

- https://github.com/mastercoin-MSC/mastercore/releases/tag/v0.0.9

General
=======

- This release is consensus critical. You MUST update before block 342650. After block 342650 prior versions WILL provide incorrect data
- This release modifies the schema. You MUST reparse when upgrading (run once with the --startclean option)
- Integrators should regularly poll getinfo_MP for any alerts to ensure they are kept aprised of any urgent issues

Changelog
=========

- Mainnet Send To Owners transaction processing is live from block 342650
- STO support added to listtransactions_MP
- STO RPC calls getsto_MP and sendtoowners_MP added
- Added --startclean option to automate reparsing (removal of MP_ state folders etc)
- Alerting system added including evaluation and shutdown in event of new protocol messages being made live that are unsupported by client
- getinfo_MP RPC call added
- Numerous bugfixes