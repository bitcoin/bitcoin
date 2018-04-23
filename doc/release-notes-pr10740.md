Dynamic loading and creation of wallets
---------------------------------------

Previously, wallets could only be loaded or created at startup, by specifying `-wallet` parameters on the command line or in the bitcoin.conf file. It is now possible to load and create wallets dynamically at runtime:

- Existing wallets can be loaded by calling the `loadwallet` RPC. The wallet can be specified as file/directory basename (which must be located in the `walletdir` directory), or as an absolute path to a file/directory.
- New wallets can be created (and loaded) by calling the `createwallet` RPC. The provided name must not match a wallet file in the `walletdir` directory or the name of a wallet that is currently loaded.

This feature is currently only available through the RPC interface.
