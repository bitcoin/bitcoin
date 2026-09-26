Notifications
-------------

- If a connected block contains a transaction with the same txid as one in the
  mempool but a different witness, the mempool transaction is now removed as a
  conflict. `-zmqpubsequence` subscribers receive an `R` for that txid before
  the block's `C`. Wallets tracking the transaction may receive an extra
  `-walletnotify` before it is marked confirmed.

Updated RPCs
------------

- In verbose `estimatesmartfee` results,
  `mempool_health_statistics[].mempool_txs_weight` now includes only block
  transactions whose wtxids matched mempool transactions.
