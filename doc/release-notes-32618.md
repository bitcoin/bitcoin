Wallet
------

* Since descriptor wallets do not allow mixing watchonly and non-watchonly descriptors,
the `include_watchonly` option (and its variants in naming) are removed from all RPCs
that had it.
* The `iswatchonly` field is removed from any RPCs that returned it.
