RPC
---

`invalidateblock` now returns an error if the block could not be invalidated,
for example when a block cannot be disconnected and the chain is left partially
rolled back, or when the genesis block is given. Previously it returned `null`
in these cases.

(#36555)
