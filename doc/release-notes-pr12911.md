RPC changes
------------

### Low-level changes

- `signrawtransactionwithkey` and `signrawtransactionwithwallet` will now include a `fee` entry in the results,
  for cases where the fee is known.
