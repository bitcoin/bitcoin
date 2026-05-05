Wallet
------

- The `importdescriptors` RPC now reports missing or malformed `timestamp`
  fields as errors in the corresponding result array entries, instead of
  returning a top-level RPC error for the entire call. Other requests in the
  same batch can still be processed and reported. (#35185)
