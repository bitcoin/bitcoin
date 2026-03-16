Updated RPCs
------------

- The `dumptxoutset` RPC now supports writing to a named pipe
  on UNIX-like systems (see mkfifo(1) and mkfifo(3) man pages).
  This allows the raw UTXO set data to be consumed directly by
  another process (e.g., the `contrib/utxo-tools/utxo_to_sqlite.py`
  conversion script) without first writing it to disk. (#31560)
