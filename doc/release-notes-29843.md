Updated settings
----------------

- On test networks, the `-acceptnonstdtxn` option now also allows
  transactions with non-standard scripts (that fail on
  `mempool-script-verify-flag-failed` checks) to be relayed and
  mined. Script flags that the node enforces at the consensus level (or
  may eventually enforce after a soft fork activates) are still applied,
  meaning that transactions that could become consensus-invalid are
  still rejected from the mempool. (#29843)
