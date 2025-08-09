Updated RPCs
------------
The following RPCs now contain a `version` parameter that allows
the user to create transactions of any standard version number (1-3):
- `createrawtransaction`
- `createpsbt`
- `send`
- `sendall`
- `walletcreatefundedpsbt`

Wallet
------
Support has been added for spending TRUC transactions received by the
wallet, as well as creating TRUC transactions. The wallet ensures that
TRUC policy rules are being met. The wallet will throw an error if the
user is trying to spend TRUC utxos with utxos of other versions.
Additionally, the wallet will treat unconfirmed TRUC sibling
transactions as mempool conflicts. The wallet will also ensure that
transactions spending TRUC utxos meet the required size restrictions.
