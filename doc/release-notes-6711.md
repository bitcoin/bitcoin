Updated settings
----------------

- BIP157 compact block filters are now automatically enabled for masternodes. This improves privacy for light clients
  connecting to masternodes and enables better support for pruned nodes. When a node is configured as a masternode
  (via `-masternodeblsprivkey`), both `-peerblockfilters` and `-blockfilterindex=basic` are automatically enabled.
  Regular nodes keep the previous defaults (disabled). Masternodes can still explicitly disable these features
  if needed by setting `-peerblockfilters=0` or `-blockfilterindex=0` (#6711).
