### RPC and other APIs
- #19825 `setban` behavior changed: Banning a subnet that is a subset of a previous ban is a no-op instead of an rpc error.
- #19825 `setban` behavior changed: Unbanning a non-banned subnet is a no-op instead of an rpc error.