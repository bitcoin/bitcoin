Updated RPCs
------------

- The `testmempoolaccept` RPC returns `vsize` and a `fee` object with the `base` fee
  if the transaction passes validation.
- The `testmempoolaccept` RPC now accepts multiple transactions (still experimental at the moment,
  API may be unstable). This is intended for testing transaction packages with dependency
  relationships; it is not recommended for batch-validating independent transactions. In addition to
  mempool policy, package policies apply: the list cannot contain more than 25 transactions or have a
  total size exceeding 101K virtual bytes, and cannot conflict with (spend the same inputs as) each other or
  the mempool. There are some known limitations to the accuracy of the test accept: it's possible for
  `testmempoolaccept` to return "allowed"=True for a group of transactions, but "too-long-mempool-chain"
  if they are actually submitted.
