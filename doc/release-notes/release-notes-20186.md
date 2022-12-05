Wallet
------

### Automatic wallet creation removed

Dash Core will no longer automatically create new wallets on startup. It will
load existing wallets specified by `-wallet` options on the command line or in
`dash.conf` or `settings.json` files. And by default it will also load a
top-level unnamed ("") wallet. However, if specified wallets don't exist,
Dash Core will now just log warnings instead of creating new wallets with
new keys and addresses like previous releases did.

New wallets can be created through the GUI (which has a more prominent create
wallet option), through the `dash-cli createwallet` or `dash-wallet
create` commands, or the `createwallet` RPC.

