Updated RPCs
------------

- `createpsbt`, `walletcreatepsbt`, `converttopsbt`, and `psbtbumpfee`
  will now default to creating version 2 PSBTs. An optional `psbt_version`
  argument is added to these RPCs which allows specifying the version of PSBT to create.
