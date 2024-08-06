RPC
---

- Previously when using the `sendrawtransaction` rpc and specifying outputs
  that are already in the UXTO set an RPC error code `-27` with RPC error
  text "Transaction already in block chain" was returned in response.
  The help text has been updated to "Transaction outputs already in utxo set"
  to more accurately describe the source of the issue.
