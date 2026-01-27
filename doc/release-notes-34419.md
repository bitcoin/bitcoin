Updated RPCs
------------

- Added `coinbase_locktime` and `coinbase_sequence` to the `getblocktemplate`
  response to prepare for a possible BIP54 deployment in the future. There is
  no corresponding change for IPC clients, since `getCoinbaseTx()` already
  includes these fields.
