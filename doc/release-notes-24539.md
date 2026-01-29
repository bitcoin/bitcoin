New settings
------------
- `-txospenderindex` enables the creation of a transaction output spender
  index that, if present, will be scanned by `gettxspendingprevout` if a
  spending transaction was not found in the mempool.
  (#24539)

Updated RPCs
------------
- `gettxspendingprevout` has 2 new optional arguments: `mempool_only` and `return_spending_tx`.
  If `mempool_only` is true it will limit scans to the mempoool even if `txospenderindex` is available.
  If `return_spending_tx` is true, the full spending tx will be returned.
  In addition if `txospenderindex` is available and a confirmed spending transaction is found,
  its block hash will be returned. (#24539)
