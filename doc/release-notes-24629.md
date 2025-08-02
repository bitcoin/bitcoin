Updated RPCs
------------

- The return value of the `pruneblockchain` method had an off-by-one bug,
  returning the height of the block *after* the most recent pruned. This has
  been corrected, and it now returns the height of the last pruned block as
  documented.
