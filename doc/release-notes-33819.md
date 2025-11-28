Mining IPC
----------

- Adds `getCoinbase()` which clients should use instead of `getCoinbaseTx()`. It
  contains all fields required to construct a coinbase transaction, and omits the
  dummy output which Bitcoin Core uses internally. (#33819)
