P2P changes
-----------

The size of the set of transactions that peers have announced and we consider
for requests has been reduced from 100000 to 5000 (per peer), and further
announcements will be ignored when that limit is reached. If you need to
dump (very) large batches of transactions, exceptions can be made for trusted
peers using the "relay" network permission. For localhost for example it can
be enabled using the command line option `-whitelist=relay@127.0.0.1`.
