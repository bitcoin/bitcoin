- Mempool RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, `submitpackage`)
now include an additional field `vsize_adjusted` (which is the sigop-adjusted virtual size
used for policy) and `vsize_bip141` (which represents the raw BIP141 virtual size).
While `vsize` is marked as DEPRECATED, it was previously erroneously described as the BIP 141
vsize, but is actually sigops-adjusted vsize. Use `vsize_bip141` to actually get that behavior
or switch to the explicit `vsize_adjusted` for retained behavior.

- `getrawtransaction` RPC now includes an additional field `vsize_adjusted`, which is the
sigop-adjusted virtual size if the transaction is in the mempool.
