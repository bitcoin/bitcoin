Updated RPCs
------------

The `utxoupdatepsbt` RPC method has been updated to take a `descriptors`
argument. When provided, input and output scripts and keys will be filled in
when known, and P2SH-witness inputs will be filled in from the UTXO set when a
descriptor is provided that shows they're spending segwit outputs.

See the RPC help text for full details.
