JSON-RPC
---

For RPC methods which accept `options` parameters ((`importmulti`, `listunspent`, `fundrawtransaction`, `bumpfee`, `send`, `sendall`, `walletcreatefundedpsbt`, `simulaterawtransaction`), it is now possible to pass the options as named parameters without the need for a nested object. (#26485)

This means it is possible make calls like:

```sh
src/bitcoin-cli -named bumpfee txid fee_rate=100
```

instead of

```sh
src/bitcoin-cli -named bumpfee txid options='{"fee_rate": 100}'
```
