Network changes
---------------

Custom signets now use separate data directories, with a suffix derived from the
network magic (message start), so multiple signets can be synced. The default
signet continues to use the unsuffixed directory for backward compatibility,
including when the default challenge is set explicitly via `-signetchallenge`.
(#34566)
