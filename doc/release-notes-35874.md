P2P and network changes
-----------------------

- BIP35 `mempool` requests are now ignored while a peer has transaction relay
  disabled, unless it has the `mempool` permission. Peers that send `fRelay=false`
  must enable transaction relay, for example by loading a BIP37 filter, before
  requesting mempool inventory. Default `-whitelist` permissions already include
  `mempool`.
