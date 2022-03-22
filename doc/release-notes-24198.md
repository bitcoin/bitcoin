Updated RPCs
------------

- The `listtransactions`, `gettransaction`, and `listsinceblock`
  RPC methods now include a wtxid field (hash of serialized transaction,
  including witness data) for each transaction.