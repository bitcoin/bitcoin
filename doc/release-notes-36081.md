Updated RPCs
------------

- The `getmininginfo` RPC now returns a `bestblockhash` field. Together with
  the existing `next` object this lets mining software obtain the tip hash and
  the next block's `nBits` atomically from a single call, without a race
  against a concurrent tip change between `getblockchaininfo` and
  `getmininginfo`.
