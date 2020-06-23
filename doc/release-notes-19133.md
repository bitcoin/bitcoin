## CLI

A new `bitcoin-cli -generate` command, equivalent to RPC `generatenewaddress`
followed by `generatetoaddress`, can generate blocks for command line testing
purposes. This is a client-side version of the
[former](https://github.com/bitcoin/bitcoin/issues/14299) `generate` RPC. See
the help for details. (#19133)
