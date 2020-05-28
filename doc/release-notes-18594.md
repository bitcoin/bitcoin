## CLI

The `bitcoin-cli -getinfo` command now displays the wallet name and balance for
each of the loaded wallets when more than one is loaded (e.g. in multiwallet
mode) and a wallet is not specified with `-rpcwallet`. (#18594)
