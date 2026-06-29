P2P and network changes
-----------------------

- Transactions submitted via private broadcast are now retained in memory after
  they are received back from the network, rather than being removed. They can
  still be removed manually using the `abortprivatebroadcast` RPC. The
  `getprivatebroadcastinfo` RPC now returns a `received_by_us` object for
  transactions that have been received back from the network, containing the
  peer `address` and `time` of reception. (#34707)
