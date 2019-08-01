RPC changes
-----------
`createwallet` now returns a warning if an empty string is used as an encryption password, and does not encrypt the wallet, instead of raising an error.
This makes it easier to disable encryption but also specify other options when using the `bitcoin-cli` tool.
