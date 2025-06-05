Updated settings
----------------

- BIP157 compact block filters are now enabled by default. This improves privacy for light clients
  and enables better support for pruned nodes. The `-blockfilterindex` option now defaults to `basic`
  instead of `0` (disabled), and the `-peerblockfilters` option now defaults to `true` instead of `false` (#6708).