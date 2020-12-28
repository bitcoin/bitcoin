Updated RPCs
------------
- `getpeerinfo` no longer returns the following fields: `addnode`,
  and `whitelisted`, which were previously deprecated in v21. Instead of
  `addnode`, the `connection_type` field returns manual. Instead of
  `whitelisted`, the `permissions` field indicates if the peer has special
  privileges. (#20755)
