RPC
---

- The RPC `addnode` and the `-addnode` bitcoind startup option now
optionally support adding manual peers that are block-relay-only.
On these connections, the node doesn't participate in transaction or
address relay. See bitcoind and RPC help documentation for parameter
semantics. (#24170)
