P2P and network changes
-----------------------

- A new `bip152hb` permission can be granted with `-whitelist` and
  `-whitebind`. Peers with this permission are asked for BIP152 high-bandwidth
  compact block announcements as soon as the connection is established, and
  remain high-bandwidth for the lifetime of the connection. For example,
  `-addnode=203.0.113.5 -whitelist=bip152hb,out@203.0.113.5` keeps the
  connection to that added node in high-bandwidth mode, including after
  reconnects.

  These peers are additional to the up to three high-bandwidth peers selected
  automatically, so this may increase redundant bandwidth usage. The
  permission is not implied by `all` and is incompatible with `-blocksonly`.
  (#35940)
