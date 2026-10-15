Updated RPCs
------------

- The `getrawtransaction` and `gettxoutproof` RPCs now return an internal error
  when block files are corrupt or the transaction index is corrupt.
  Previously, they reported the transaction as missing.

- Similarly, the `utxoupdatepsbt` and `descriptorprocesspsbt` RPCs now return
  an internal error in this case, instead of silently continuing without the
  transaction data.

Updated REST APIs
-----------------

- The `/rest/tx/<TX-HASH>` endpoint now returns an internal error when block
  files are corrupt or the transaction index is corrupt.
  Previously, it reported the transaction as not found.
