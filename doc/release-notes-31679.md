Install changes
---------------

The `test_bitcoin`, `test_bitcoin-qt`, `bench_bitcoin`, `bitcoin-chainstate`,
`bitcoin-node`, and `bitcoin-wallet` binaries are now installed in
`$PREFIX/libexec` rather than `$PREFIX/bin`. If you are using a binary release
or building from source with default build options, most of these programs are
not built by default anyway so these changes may not be noticable.

Goal of this change is to organize binaries better and not add binaries that
rarely need to be executed directly on the system PATH.
