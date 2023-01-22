Updated settings
----------------

- Setting `-blocksonly` will now reduce the maximum mempool memory
  to 5MB (users may still use `-maxmempool` to override). Previously,
  the default 300MB would be used, leading to unexpected memory usage
  for users running with `-blocksonly` expecting it to eliminate
  mempool memory usage.

  As unused mempool memory is shared with dbcache, this also reduces
  the dbcache size for users running with `-blocksonly`, potentially
  impacting performance.

