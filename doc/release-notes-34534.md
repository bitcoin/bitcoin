New RPCs
--------

- The new `setprunelock` RPC adds or removes a manual prune lock, which keeps block
  data at or above a given height from being pruned. Locks set this way are stored in
  memory only and do not persist across restarts. (#34534)

- The new `listprunelocks` RPC returns all active prune locks, including the ones set
  internally by indexes. (#34534)

New settings
------------

- The new `-prunelockheight=<n>` setting creates a prune lock at the given block
  height at startup and requires `-prune` to be set. If no value is given, the lock is
  placed at the chain tip height. (#34534)
