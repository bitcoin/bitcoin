# Rust wallet utility

This provides additional functionality that's not available in the
`bin/bitcoin-wallet` tool.

## Import

Import keys into a blank wallet.

### bip39 mnenomic

Use `import bip39` to import from a [bip39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) mnemonic.

Bitcoin Core does not support bip39. The mnemonic will not be stored in the
wallet, so be sure to back it up. Only standard derivation paths are supported
(BIP44, BIP49, BIP84 and BIP86).

First create a new blank wallet. From the GUI go to File -> Create Wallet and
select "Make Blank Wallet". Or using the RPC:

```sh
bitcoin-cli -named createwallet wallet_name=mywallet blank=true
```

Then run:

```sh
cargo run -- import bip39
```

Follow the instructions to enter your mnemonic and optional passphrase.

It will show you the command needed to import the wallet. Afterwards, use
the `rescanblockchain` RPC to find historical transactions.

