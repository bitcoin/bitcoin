New RPCs
--------

- The `sendall` RPC spends specific UTXOs to one or more recipients
  without creating change. By default, the `sendall` RPC will spend
  every UTXO in the wallet. `sendall` is useful to empty wallets or to
  create a changeless payment from select UTXOs. When creating a payment
  from a specific amount for which the recipient incurs the transaction
  fee, continue to use the `subtractfeefromamount` option via the
  `send`, `sendtoaddress`, or `sendmany` RPCs. (#24118)
