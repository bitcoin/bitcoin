RPC Wallet
----------

- The `gettransaction`, `listtransactions`, `listsinceblock` RPCs now return
  the `abandoned` field for all transactions. Previously, the "abandoned" field
  was only returned for sent transactions. (#25158)