P2P and network changes
-----------------------

- The mempool now tracks whether transactions submitted via the wallet or RPCs
  have been successfully broadcast. Every 10-15 minutes, the node will try to
  announce unbroadcast transactions until a peer requests it via a `getdata`
  message or the transaction is removed from the mempool for other reasons.
  The node will not track the broadcast status of transactions submitted to the
  node using P2P relay. This version reduces the initial broadcast guarantees
  for wallet transactions submitted via P2P to a node running the wallet. (#18038)

Updated RPCs
------------

- `getmempoolinfo` now returns an additional `unbroadcastcount` field. The
  mempool tracks locally submitted transactions until their initial broadcast
  is acknowledged by a peer. This field returns the count of transactions
  waiting for acknowledgement.

- Mempool RPCs such as `getmempoolentry` and `getrawmempool` with
  `verbose=true` now return an additional `unbroadcast` field. This indicates
  whether initial broadcast of the transaction has been acknowledged by a
  peer. `getmempoolancestors` and `getmempooldescendants` are also updated.

Wallet
------

- To improve wallet privacy, the frequency of wallet rebroadcast attempts is
  reduced from approximately once every 15 minutes to once every 12-36 hours.
  To maintain a similar level of guarantee for initial broadcast of wallet
  transactions, the mempool tracks these transactions as a part of the newly
  introduced unbroadcast set. See the "P2P and network changes" section for
  more information on the unbroadcast set. (#18038)
