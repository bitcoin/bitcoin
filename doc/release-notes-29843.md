Updated settings
----------------

- On test networks, the `-acceptnonstdtxn` option now also allows
  transactions with non-standard scripts (that fail on
  `mempool-script-verify-flag-failed` checks) to be relayed and
  mined. Script flags that may be enforced at the consensus level are
  still applied, so transactions that could become consensus-invalid
  are still rejected. (#29843)
