RPC
---

Duplicate blocks submitted with `submitblock` will now persist their block data
even if it was previously pruned. If pruning is activated, the data will be
pruned again eventually once the block file it is persisted in is selected for
pruning. This is consistent with the behaviour of `getblockfrompeer` where the
block is persisted as well even when pruning.
