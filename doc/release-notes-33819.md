Mining IPC
----------

- The `getCoinbaseTx()` method is renamed to `getCoinbaseRawTx()` and deprecated.
  IPC clients do not use the function name, so they're not affected. (#33819)
- Adds `getCoinbaseTx()` which clients should use instead of `getCoinbaseRawTx()`. It
  contains all fields required to construct a coinbase transaction, and omits the
  dummy output which Bitcoin Core uses internally. (#33819)
