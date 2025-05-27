New command line interface
--------------------------

A new `bitcoin` command line tool has been added to make features more
discoverable and convenient to use. The `bitcoin` tool just calls other
executables and does not implement any functionality on its own.  Specifically
`bitcoin node` is a synonym for `bitcoind`, `bitcoin gui` is a synonym for
`bitcoin-qt`, and `bitcoin rpc` is a synonym for `bitcoin-cli -named`. Other
commands and options can be listed with `bitcoin help`. The new tool does not
replace other tools, so all existing commands should continue working and there
are no plans to deprecate them.
