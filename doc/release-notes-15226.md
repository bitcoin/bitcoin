Miscellaneous RPC changes
------------

- The RPC `createwallet` now has an optional `blank` argument that can be used to create a blank wallet.
Blank wallets do not have any keys or HD seed.
They cannot be opened in software older than 0.18.
Once a blank wallet has a HD seed set (by using `sethdseed`) or private keys, scripts, addresses, and other watch only things have been imported, the wallet is no longer blank and can be opened in 0.17.x.
Encrypting a blank wallet will also set a HD seed for it.
