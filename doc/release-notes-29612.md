RPC
---

- The `dumptxoutset` RPC now returns the UTXO set dump in a new and
  improved format. At the same time the `loadtxoutset` RPC now
  expects this new format in dumps it tries to load. Dumps with the
  old format are no longer supported and need to be recreated using
  the new format in order to be usable.
