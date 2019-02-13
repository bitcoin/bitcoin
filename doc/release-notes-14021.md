Miscellaneous RPC Changes
-------------------------
- Descriptors with key origin information imported through `importmulti` will have their key origin information stored in the wallet for use with creating PSBTs.
- If `bip32derivs` of both `walletprocesspsbt` and `walletcreatefundedpsbt` is set to true but the key metadata for a public key has not been updated yet, then that key will have a derivation path as if it were just an independent key (i.e. no derivation path and its master fingerprint is itself)

Miscellaneous Wallet changes
----------------------------

- The key metadata will need to be upgraded the first time that the HD seed is available.
For unencrypted wallets this will occur on wallet loading.
For encrypted wallets this will occur the first time the wallet is unlocked.
