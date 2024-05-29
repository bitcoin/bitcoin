
Updated RPCs
------------

- The `getpeerinfo` RPC no longer returns the `addnode` field by default. This
  field will be fully removed in the next major release.  It can be accessed
  with the configuration option `-deprecatedrpc=getpeerinfo_addnode`. However,
  it is recommended to instead use the `connection_type` field (it will return
  `manual` when addnode is true). (#6033)

