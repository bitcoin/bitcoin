Wallet
------

The `addhdkey` RPC can now derive an HD key from a BIP 39 mnemonic through
the new `mnemonic` and `bip39_passphrase` arguments. Only lowercase words
from the English wordlist are supported, and passphrases are restricted
to ASCII characters. Neither value is stored; only the derived HD key is
stored.

`addhdkey` does not create active descriptors or scan for transactions.
To recover transactions, create the required descriptors with
`createwalletdescriptor`, then run `rescanblockchain`. An incorrect BIP 39
passphrase derives a different valid HD key and cannot be detected. Use
`bitcoin-cli -stdin -named addhdkey` to avoid placing secrets in
command-line arguments and shell history.
