Wallet `receivedby` RPCs now include coinbase transactions
-------------

Previously, the following wallet RPCs excluded coinbase transactions:

`getreceivedbyaddress`

`getreceivedbylabel`

`listreceivedbyaddress`

`listreceivedbylabel`

This release changes this behaviour and returns results accounting for received coins from coinbase outputs.

A new option, `include_immature_coinbase` (default=`false`), determines whether to account for immature coinbase transactions.
Immature coinbase transactions are coinbase transactions that have 100 or fewer confirmations, and are not spendable.

The previous behaviour can be restored using the configuration `-deprecatedrpc=exclude_coinbase`, but may be removed in a future release.
