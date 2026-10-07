Updated settings
----------------

- The transaction index (`-txindex`) can now be used with pruning (`-prune`).
  The index must be built from genesis; indexes built in a version prior to v31
  must be rebuilt first. (#36002)

Updated RPCs
------------

- With pruning enabled, `getrawtransaction` and `gettxoutproof` may return
  error -1 when a transaction may be found in one or more pruned blocks.
  Their hashes are returned in the error message and the
  `error.data.pruned_block_hashes` array. The blocks are not guaranteed to contain
  the transaction. This also applies without txindex for explicit block hashes
  and `gettxoutproof` lookups through the UTXO set. (#36002)

Updated REST API
----------------

- `/rest/tx/<TX-HASH>.<bin|hex|json>` now includes pruned block hashes in HTTP
  404 errors when the transaction may be found in those blocks. (#36002)
