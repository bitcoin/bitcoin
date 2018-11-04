Low-level RPC changes
----------------------

`-usehd` was removed in version 0.16. From that version onwards, all new
wallets created are hierarchical deterministic wallets. Version 0.18 makes
specifying `-usehd` invalid config.

`ischange` field of boolean type that shows if an address was used for change
output was added to `getaddressinfo` method response.
