New command line interface
--------------------------

A new `bitcoin` command line tool has been added to make features more
discoverable and convenient to use. The `bitcoin` tool just calls other
executables and does not implement any functionality on its own. Specifically
`bitcoin node` is a synonym for `bitcoind`, `bitcoin gui` is a synonym for
`bitcoin-qt`, and `bitcoin rpc` is a synonym for `bitcoin-cli -named`. Other
commands and options can be listed with `bitcoin help`. The new tool does not
replace other tools, so existing commands should continue working and there are
no plans to deprecate them.

Install changes
---------------

The `test_bitcoin` executable is now located in `libexec/` rather than `bin/`.
It can still be executed directly, or accessed through the new `bitcoin` command
line tool as `bitcoin test`.

Other executables which are only part of source releases and not built by
default: `test_bitcoin-qt`, `bench_bitcoin`, `bitcoin-chainstate`,
`bitcoin-node`, and `bitcoin-gui` are also now installed in `libexec/`
instead of `bin/` and can be accessed through the `bitcoin` command line tool.
See `bitcoin help` output for details.
