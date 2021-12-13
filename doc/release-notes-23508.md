Updated RPCs
------------

- Information on soft fork status has been moved from `getblockchaininfo`
  to `getdeploymentinfo` which allows querying soft fork status at any
  block, rather than just at the chain tip. Inclusion of soft fork
  status in `getblockchaininfo` can currently be restored using the
  configuration `-deprecatedrpc=softforks`, but this will be removed in
  a future release. (#23508)
