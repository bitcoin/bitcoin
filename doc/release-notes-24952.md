Updated RPCs
------------

- The `dumptxoutset` RPC method now accepts an optional `format` parameter for the output file type.
  Possible values are "compact" (the default), or "sqlite" for a SQLite DB file with human-readable
  data in a `utxos` and `metadata` table. This new parameter requires compiling with the wallet or
  configuring with SQLite support. (#24952)
