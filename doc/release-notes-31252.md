RPC
---

The `getrawtransaction` (verbosity=2), `getblock` (verbosity=2 and verbosity=3),
and the REST endpoint `/rest/block/` now include two new optional fields in each
transaction input when previous output data is available (i.e. for non-coinbase
transactions in non-pruned blocks):
- `redeemScript`: the decoded redeem script for P2SH and P2SH-P2WSH inputs.
- `witnessScript`: the decoded witness script for P2WSH and P2SH-P2WSH inputs.
