### Tools and Utilities

Command-line tools
------------------

- Command-line options may now be specified after non-option arguments. For example, `bitcoin-cli getblockchaininfo -rpcwait` now behaves the same as `bitcoin-cli -rpcwait getblockchaininfo`, matching common GNU-style option parsing behavior.

- Options beginning with a double dash (`--`) that appear after the RPC method name are interpreted as RPC named parameters. Other option prefixes, such as single dashes (`-`) and Windows-style forward slashes (`/`), continue to be interpreted as command-line options.
