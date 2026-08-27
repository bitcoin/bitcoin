Wallet
------

- PSBTs created by the wallet now carry the extended public keys of
  descriptors that have more than one, in the `PSBT_GLOBAL_XPUB` field
  defined by BIP 174. Hardware signers with no registered policy to work
  from use these to reconstruct a multisig. A descriptor with a single
  extended key publishes nothing, and the field is omitted along with the
  per-input derivation paths when `bip32derivs` is false. (#36154)
