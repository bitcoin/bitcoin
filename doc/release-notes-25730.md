RPC Wallet
----------

- RPC `listunspent` now has a new argument `include_immature_coinbase`
  to include coinbase UTXOs that don't meet the minimum spendability
  depth requirement (which before were silently skipped). (#25730)