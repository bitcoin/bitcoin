- A new `importmempool` RPC has been added. It loads a valid `mempool.dat` file and attempts to
  add its contents to the mempool. This can be useful to import mempool data from another node
  without having to modify the datadir contents and without having to restart the node. (#27460)
    - Warning: Importing untrusted files is dangerous, especially if metadata from the file is taken over.
    - If you want to apply fee deltas, it is recommended to use the `getprioritisedtransactions` and
      `prioritisetransaction` RPCs instead of the `apply_fee_delta_priority` option to avoid
      double-prioritising any already-prioritised transactions in the mempool.
