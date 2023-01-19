Wallet
------

- Rescans for descriptor wallets are now significantly faster if compact
  block filters (BIP158) are available. Since those are not constructed
  by default, the configuration option "-blockfilterindex=1" has to be
  provided to take advantage of the optimization. This improves the
  performance of the RPC calls `rescanblockchain`, `importdescriptors`
  and `restorewallet`. (#25957)
