### RPC and other APIs
- #19825 `setban` behavior changed. Banning a subnet that is a subset of a previous ban is a no-op instead of an rpc error.