New RPCs
--------

- The new `setprunelock` RPC adds or removes a manual prune lock, which keeps block
  data at or above a given height from being pruned. Locks set this way are stored in
  memory only and do not persist across restarts. (#34534)

- The new `listprunelocks` RPC returns all active prune locks, including the ones set
  internally by indexes. The `type` field of each lock tells the two apart. (#34534)

New settings
------------

- The new `-prunelock=<id>[:<height>]` setting creates a prune lock with the given id at
  startup and requires `-prune` to be set. Without a height the lock is placed at height
  0, so no block data is pruned until the lock is updated or removed with `setprunelock`
  using the same id. The setting can be specified multiple times. If the same id is
  specified multiple times, the lowest height is used. (#34534)
