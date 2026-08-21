### Tools and Utilities

Command-line tools
------------------

- A dash-prefixed argument after a command is now parsed as an option (or an
  error if unrecognized) rather than silently collected as a positional
  argument (backwards compatibility). `bitcoin-cli` now supports GNU-style
  option parsing: options (such as `-rpcwallet=<wallet>`) may be specified
  after the command and its positional arguments, not only before. The `--`
  separator can be used to explicitly end option processing; all subsequent
  tokens are treated as positional arguments. `bitcoin-wallet` also benefits:
  options such as `-wallet=<name>` may now be placed after the subcommand
  (e.g. `bitcoin-wallet create -wallet=foo`).
