- Mempool-related RPCs (`getrawmempool`, `getmempoolentry`, `testmempoolaccept`, `submitpackage`) now
clarify the difference between BIP-141 virtual size (`vsize_bip141`) and policy-adjusted virtual size
(`vsize`), which accounts for sigop-based adjustments. Additionally, `getrawtransaction` now includes
a `sigopsize` field and continues to report the unadjusted BIP-141 virtual size in its `vsize` field.
(#32800)
