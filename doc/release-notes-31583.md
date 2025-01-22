Updated RPCs
---
- `getmininginfo` now returns `nBits` and the current target in the `target` field. It also returns a `next` object which specifies the `height`, `nBits`, `difficulty`, and `target` for the next block.
- `getblock` and `getblockheader` now return the current target in the `target` field
- `getblockchaininfo` and `getchainstates` now return `nBits` and the current target in the `target` field

REST interface
---
- `GET /rest/block/<BLOCK-HASH>.json` and `GET /rest/headers/<BLOCK-HASH>.json` now return the current target in the `target` field
