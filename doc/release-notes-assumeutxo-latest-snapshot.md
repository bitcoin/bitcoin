AssumeUTXO
---------

Only the newest AssumeUTXO commitment is retained on each production
network. Older snapshots can no longer be loaded, and existing snapshot
chainstates based on retired commitments cannot be restarted with this release.

Ordinary block validation verifies the provided AssumeUTXO hash when block connection
reaches the corresponding block, stopping the node on a mismatch. The check flushes pending
changes and hashes the UTXO set, which can take several minutes. AssumeUTXO import
verification remains mandatory.
