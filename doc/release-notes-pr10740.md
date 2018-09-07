Dynamic loading of wallets
--------------------------

Previously, wallets could only be loaded at startup, by specifying `-wallet` parameters on the command line or in the bitcoin.conf file. It is now possible to load wallets dynamically at runtime by calling the `loadwallet` RPC.

The wallet can be specified as file/directory basename (which must be located in the `walletdir` directory), or as an absolute path to a file/directory.

This feature is currently only available through the RPC interface. Wallets loaded in this way will not display in the bitcoin-qt GUI.
