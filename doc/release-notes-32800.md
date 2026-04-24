- Mempool RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, `submitpackage`)
now include an additional field `vsize_adjusted`, which is the sigop-adjusted virtual size
used for policy, while `vsize` now shows the raw BIP141 virtual size.

- `getrawtransaction` RPC now includes an additional field `vsize_adjusted`, which is the
sigop-adjusted virtual size if the transaction is in the mempool.
(#32800)
