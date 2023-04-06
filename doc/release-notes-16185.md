RPC changes
-----------
The `gettransaction` RPC now accepts a third (boolean) argument `verbose`. If
set to `true`, a new `decoded` field will be added to the response containing
the decoded transaction. This field is equivalent to RPC `decoderawtransaction`,
or RPC `getrawtransaction` when `verbose` is passed.
