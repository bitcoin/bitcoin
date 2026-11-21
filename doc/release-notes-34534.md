New RPCs
--------

- The new `setprunelock` RPC adds or removes a manual prune lock, which keeps block
  data at or above a given height from being pruned. Locks set this way are stored in
  memory only and do not persist across restarts. (#34534)

- The new `listprunelocks` RPC returns all active prune locks, including the ones set
  internally by indexes. (#34534)

New settings
------------

- The new `-prunelock=<id>` setting creates a prune lock with the given id at startup
  and requires `-prune` to be set. The lock is placed at height 0, so no block data is
  pruned until the lock is updated or removed with `setprunelock` using the same id.
  The setting can be specified multiple times. (#34534)
