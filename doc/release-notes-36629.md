New RPCs
--------

- The new `getscripthistory` RPC returns transactions creating outputs for an
  address, output descriptor, or Electrum script hash, along with spentness and
  optional spending transactions. Enable the new `-txoscriptindex` option to use
  it. Confirmed spending transactions outside the matching history require
  `-txospenderindex`, as does the `asof` option for querying a fixed chain state.
  The script index supports pruning: unavailable blocks are reported in
  `pruned_blocks` and can be fetched with `getblockfrompeer` before retrying the
  query. This list may include false positives with no matching outputs.
  The RPC and index are experimental. The RPC interface and index database
  format may change in future releases.
