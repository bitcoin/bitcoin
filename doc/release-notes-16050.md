RPC changes
-----------
The RPC `importmulti` has a new option `update_existing` to control whether
rescan updates existing transactions. When `false`, only new wallet transactions
are notified to the `-walletnotify` script.
