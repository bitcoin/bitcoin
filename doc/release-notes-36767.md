Wallet
------

- `createwallet` now returns `RPC_WALLET_ERROR` (-4), with a message starting
  with "Wallet creation failed.", when an external signer fails, is not
  configured or more than one is found while the new wallet is being set up,
  for example when the signer returns an invalid descriptor. Such failures
  previously returned `RPC_MISC_ERROR` (-1) and made `bitcoin-qt` abort.
- When creating a wallet fails, the directory and `wallet.dat` that the
  attempt created are now removed, so that the name can be used again. Files
  that existed before the attempt are kept. Files left behind by earlier
  versions still have to be removed by hand.
