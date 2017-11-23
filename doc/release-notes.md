(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Bitcoin Core version *version* is now available from:

  <https://bitcoin.org/bin/bitcoin-core-*version*/>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

The first time you run version 0.15.0, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0. Upgrading
directly from 0.7.x and earlier without redownloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

The chainstate database for this release is not compatible with previous
releases, so if you run 0.15 and then decide to switch back to any
older version, you will need to run the old release with the `-reindex-chainstate`
option to rebuild the chainstate data structures in the old format.

If your node has pruning enabled, this will entail re-downloading and
processing the entire blockchain.

Compatibility
==============

Bitcoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

GCC 4.8.x
--------------
The minimum version of GCC required to compile Bitcoin Core is now 4.8. No effort will be
made to support older versions of GCC. See discussion in issue #11732 for more information.

HD-wallets by default
---------------------
Due to a backward-incompatible change in the wallet database, wallets created
with version 0.16.0 will be rejected by previous versions. Also, version 0.16.0
will only create hierarchical deterministic (HD) wallets.

Custom wallet directories
---------------------
The ability to specify a directory other than the default data directory in which to store
wallets has been added. An existing directory can be specified using the `-walletdir=<dir>`
argument. Wallets loaded via `-wallet` arguments must be in this wallet directory. Care should be taken
when choosing a wallet directory location, as if  it becomes unavailable during operation,
funds may be lost.

Default wallet directory change
--------------------------
On new installations (if the data directory doesn't exist), wallets will now be stored in a
new `wallets/` subdirectory inside the data directory. If this `wallets/` subdirectory
doesn't exist (i.e. on existing nodes), the current datadir root is used instead, as it was.

Low-level RPC changes
----------------------
- The deprecated RPC `getinfo` was removed. It is recommended that the more specific RPCs are used:
  * `getblockchaininfo`
  * `getnetworkinfo`
  * `getwalletinfo`
  * `getmininginfo`
- The wallet RPC `getreceivedbyaddress` will return an error if called with an address not in the wallet.


Credits
=======

Thanks to everyone who directly contributed to this release:


As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
