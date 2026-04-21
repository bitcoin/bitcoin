Network changes
---------------

Custom signets now use separate data directories, with a suffix derived from the
network magic (message start), so multiple signets can be synced. The default
signet continues to use the unsuffixed directory for backward compatibility,
including when the default challenge is set explicitly via `-signetchallenge`.

Users currently running a custom signet will have their node select a new data
directory after upgrading. To avoid a resync, they should rename the existing
signet directory to the new signet_XXXXXXXX format, using the `getchainparams`
command from `bitcoin-util` to determine the suffix:

```
$ bitcoin-util -signet -signetchallenge=<challenge> getchainparams | jq -r .net.magic
```

(#34566)

