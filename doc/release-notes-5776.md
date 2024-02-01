Added RPC
--------

- `getassetunlockstatuses` RPC allows fetching of Asset Unlock txs by their withdrawal index.
The RPC accepts an array of indexes and an optional block height.
The possible outcomes per each index are:
- `chainlocked`: If the Asset Unlock index is mined on a ChainLocked block or up to the given block height.
- `mined`: If no ChainLock information is available for the mined Asset Unlock index.
- `mempooled`: If the Asset Unlock index is in the mempool.
- `unknown`: If none of the above are valid.

Note: If a specific block height is passed on request, then only `chainlocked` and `unknown` outcomes are possible.

Note: This RPC is whitelisted for the Platform RPC user.
