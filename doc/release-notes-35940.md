P2P and network changes
-----------------------

- Added persistent BIP152 high-bandwidth compact block announcement requests
  for selected outbound added-node connections. Append `=bip152-hb` to an
  `-addnode=<host>[:<port>]` value to request high-bandwidth announcements from
  connections opened for that added node.

  Configured peers are additional to the up to three peers selected
  automatically, so this may increase redundant bandwidth usage. The suffix is
  incompatible with `-blocksonly`, applies only to `-addnode` configuration,
  and does not change the `addnode` RPC or inbound connections.
