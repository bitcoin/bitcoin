Disk space
----------

- Periodic background chainstate compaction is now logged by default.
  Compaction can temporarily need about as much extra disk space as the chainstate itself.
  If there is not enough free space, it is deferred with a warning and obsolete data stays on disk.
