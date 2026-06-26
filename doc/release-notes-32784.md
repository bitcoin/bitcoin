Wallet
------

- A new `derivehdkey` RPC is available to obtain an xpub or xprv for a
  derivation path with at least one hardened step from an HD key known to the
  wallet. This can be used to coordinate a multisig setup, where each signer
  shares an xpub using a
  different derivation path than the default single-signature descriptors. The
  example in `doc/multisig-tutorial.md` is updated to use this RPC. (#32784)
