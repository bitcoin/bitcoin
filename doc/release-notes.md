Bitcoin Core version 0.9.3 is now available from:

  https://bitcoin.org/bin/0.9.3/

This is a new minor version release, bringing only bug fixes. Upgrading to this
release is recommended.

Please report bugs using the issue tracker at github:

  https://github.com/bitcoin/bitcoin/issues

Upgrading and downgrading
==========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you run
0.9.3 your blockchain files will be re-indexed, which will take anywhere from 
30 minutes to several hours, depending on the speed of your machine.

Downgrading warnings
--------------------

The 'chainstate' for this release is not always compatible with previous
releases, so if you run 0.9.x and then decide to switch back to a
0.8.x release you might get a blockchain validation error when starting the
old release (due to 'pruned outputs' being omitted from the index of
unspent transaction outputs).

Running the old release with the -reindex option will rebuild the chainstate
data structures and correct the problem.

Also, the first time you run a 0.8.x release on a 0.9 wallet it will rescan
the blockchain for missing spent coins, which will take a long time (tens
of minutes on a typical machine).

0.9.3 Release notes
=======================

RPC:

Command-line options:

Block-chain handling and storage:

Protocol and network code:

Wallet:

Build system:

GUI:

Miscellaneous:


Credits
--------

Thanks to everyone who contributed to this release:

