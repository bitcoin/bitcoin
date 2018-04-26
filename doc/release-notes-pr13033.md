Transaction index changes
-------------------------

The transaction index is now built separately from the main node procedure,
meaning the `-txindex` flag can be toggled without a full reindex. If bitcoind
is run with `-txindex` on a node that is already partially or fully synced
without one, the transaction index will be built in the background and become
available once caught up. When switching from running `-txindex` to running
without the flag, the transaction index database will *not* be deleted
automatically, meaning it could be turned back on at a later time without a full
resync.
