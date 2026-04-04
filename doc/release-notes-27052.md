JSON-RPC
--------

The `getpeerinfo` RPC now returns an additional result field,
`last_block_announcement`, which indicates the most recent time
this peer was the first to announce a new block to the local node.
This timestamp, previously internal only, is used by the stale-tip
eviction logic.
