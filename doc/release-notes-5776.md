Added RPC
--------

- `getassetunlockstatuses` RPC allows fetching of Asset Unlock txs by their withdrawal index. The RPC accepts an array of indexes and returns status for each index.
The possible outcomes per each index are:
- "chainlocked": If the Asset Unlock index is mined on a ChainLocked block.
- "mined": If no ChainLock information is available, and the Asset Unlock index is mined.
- "mempooled": If the Asset Unlock index is in the mempool.
- "unknown": If none of the above are valid.

Note: This RPC is whitelisted for the Platform RPC user.
