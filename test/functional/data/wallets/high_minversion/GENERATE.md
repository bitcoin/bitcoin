The wallet has been created by starting Bitcoin Core with the options
`-regtest -datadir=/tmp -nowallet -walletdir=$(pwd)/test/functional/data/wallets/`.

In the source code, `WalletFeature::FEATURE_LATEST` has been modified to be large, so that the minversion is too high
for a current build of the wallet.

The wallet has then been created with the RPC `createwallet high_minversion true true`, so that a blank wallet with
private keys disabled is created.
