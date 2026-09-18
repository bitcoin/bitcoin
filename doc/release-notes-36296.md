Updated RPCs
------------

- The `getindexinfo` RPC now reports a `progress` field for each index: the
  fraction of the node's validated chain that the index covers, as a ratio of
  cumulative transactions rather than of block heights. It reaches 1 once the
  index is synced.
