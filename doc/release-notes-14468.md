Wallet `generate` RPC method deprecated
---------------------------------------

The wallet's `generate` RPC method has been deprecated and will be fully
removed in v0.19.

`generate` is only used for testing. The RPC call reaches across multiple
subsystems (wallet and mining), so is deprecated to simplify the wallet-node
interface. Projects that are using `generate` for testing purposes should
transition to using the `generatetoaddress` call, which does not require or use
the wallet component. Calling `generatetoaddress` with an address returned by
`getnewaddress` gives the same functionality as the old `generate` method.

To continue using `generate` in v0.18, restart bitcoind with the
`-deprecatedrpc=generate` configuration.
