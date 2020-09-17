Wallet
------

Bitcoin Core will no longer create an unnamed `""` wallet by default when no wallet is specified on the command line or in the configuration files.
For backwards compatibility, if an unnamed `""` wallet already exists and would have been loaded previously, then it will still be loaded.
Users without an unnamed `""` wallet and without any other wallets to be loaded on startup  will be prompted to either choose a wallet to load, or to create a new wallet.
