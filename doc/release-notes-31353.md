Updated RPCs
------------

- RPC getbalances has a new `total` field that provides the sum of all wallet
  balances returned by the RPC. (#31353)

- CLI -getinfo now displays wallet balances from RPC getbalances `total` instead
  of `mine.trusted` in order to include watchonly, reused, untrusted pending, and
  immature coinbase outputs in the balance shown. (#31353)
