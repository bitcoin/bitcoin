Updated RPCs
------------

- The `getpeerinfo` RPC now has additional `last_block` and `last_transaction`
  fields that return the UNIX epoch time of the last block and the last valid
  transaction received from each peer. (#19731)
