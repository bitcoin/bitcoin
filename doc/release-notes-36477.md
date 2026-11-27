P2P and network changes
-----------------------

- Transactions submitted via private broadcast are now retained after they are
  received back from the network. This allows inspecting the state of such
  transactions via the `getprivatebroadcastinfo` RPC. The information will now
  not vanish as soon as the transaction is echoed from the network. In addition
  `getprivatebroadcastinfo` returns a `received_by_us` object for echoed
  transactions, containing the peer `address` and `time` of the reception. They
  can be removed using the `abortprivatebroadcast` RPC, and are evicted oldest
  first if the queue is full and room is needed for new transactions. (#36477)
