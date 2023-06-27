Wallet
------

- Wallet loading has changed in this release. Wallets with some corrupted records that could be
  previously loaded (with warnings) may no longer load. For example, wallets with corrupted
  address book entries may no longer load. If this happens, it is recommended
  load the wallet in a previous version of Bitcoin Core and import the data into a new wallet.
  Please also report an issue to help improve the software and make wallet loading more robust
  in these cases.
