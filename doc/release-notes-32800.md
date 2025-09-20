- Mempool RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, `submitpackage`)
now include an additional field `vsize_bip141` that shows the raw BIP-141 virtual size,
for cases where the unadjusted consensus value is needed instead of the sigop-adjusted `vsize`
used for policy.
(#32800)
