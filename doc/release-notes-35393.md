Mining
------

- The IPC mining interface `BlockCreateOptions` now has an
  `alwaysAddCoinbaseCommitment` option. It defaults to `false`, so empty block
  templates and templates without SegWit spends no longer include a dummy
  coinbase witness and SegWit OP_RETURN.
  It's important that IPC clients DO NOT unconditionally add a witness to the
  coinbase transaction they pass to `submitSolution()`. Clients must only do so
  when the `witness` field of the `getCoinbaseTx()` result is set.
  IPC clients that need the previous behavior can set this option to `true`.
  Previous Bitcoin Core releases do not understand this option and will ignore it. (#35393)
