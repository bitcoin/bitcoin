Wallet
------

- PSBTs created or updated by the wallet now carry the extended public
  keys of the descriptors they use, in the `PSBT_GLOBAL_XPUB` field
  defined by BIP 174. Signers with no registered policy, such as some
  hardware wallets signing for a multisig, use those keys to rebuild the
  scripts. Whoever holds such a PSBT can derive the addresses of those
  descriptors. Setting `bip32derivs` to false in `walletcreatefundedpsbt`
  or `walletprocesspsbt` stops the wallet from adding the field, along
  with the derivation paths, and does not remove what a PSBT already
  carries. (#36154)
