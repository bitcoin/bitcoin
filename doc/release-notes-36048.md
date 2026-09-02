Wallet
------

* On non-Windows systems, an authenticated RPC caller allowed to create wallets
  could execute arbitrary commands as the node process account when
  `-walletnotify` was configured, by crafting a wallet name with regex
  replacement characters. Wallet notification placeholder replacement now
  treats wallet names literally. (#36048)
