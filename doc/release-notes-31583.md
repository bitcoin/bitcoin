Updated RPCs
---
- `getmininginfo` now returns `nBits` and the current target in the `target` field. It also returns a `next` object which specifies the `height`, `nBits`, `difficulty`, and `target` for the next block.
- `getdifficulty` can now return the difficulty for the next block (rather than the current tip) when calling with the boolean `next` argument set to true.
- `getblock` and `getblockheader` now return the current target in the `target` field

New RPCs
---
- `gettarget` can be used to return the current target (for tip) or target for the next block (with the `next` argument)

REST interface
---
- `GET /rest/block/<BLOCK-HASH>` and `GET /rest/headers/<BLOCK-HASH>` now return the current target in the `target` field
