UTXO snapshot loading
---------------------

- The `loadtxoutset` RPC has been replaced by the startup option `-loadtxoutset=<file>`.
  The receiving chainstate must be at genesis and its block index must already contain the snapshot's base header.
  The node fails startup if these requirements are not met.
  Importing finishes before networking and normal RPC operation begin.
  Remove the option after the first successful import.
  Existing snapshot chainstates resume automatically without it, including background validation.
