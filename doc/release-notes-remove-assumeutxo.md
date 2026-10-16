# Validation

AssumeUTXO snapshot loading and background validation have been removed.
The `loadtxoutset` RPC reports that snapshot loading is no longer supported.
The `getchainstates` RPC is no longer available, and `getblockchaininfo` no longer returns `backgroundvalidation`.

Nodes with an unfinished snapshot must finish background validation and restart using a version with AssumeUTXO support before upgrading.
Alternatively, delete the datadir's `chainstate_snapshot` directory and restart with `-reindex` to discard the snapshot and rebuild the block index.
Using `-reindex-chainstate` alone is insufficient because it preserves validation flags from the snapshot in the block index.
Unfinished snapshots are also rejected when using `-reindex`, `-reindex-chainstate` or the kernel library.
Kernel chainstate-manager creation returns null for an unfinished snapshot.
Nodes that completed background validation and restarted before upgrading are unaffected.

The `dumptxoutset` RPC remains available.
The `-assumevalid` option is unchanged.
