RPC
---

- `getpeerinfo` now accepts an optional parameter to filter by peer ID(s).
  This can be a single ID or a list of IDs. When provided, only the specified
  peers are returned instead of all connected peers.
