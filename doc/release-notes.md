Bitcoin Core version 0.11.3 is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.11.3/>

This is a new minor version release, bringing bug fixes, the BIP65
(CLTV) consensus change, and relay policy preparation for BIP113. It is
recommended to upgrade to this version as soon as possible.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoin/bitcoin/issues>

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Downgrade warning
------------------

Because release 0.10.0 and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-0.10 versions of Bitcoin Core or other software:

* Blocks will be stored on disk out of order (in the order they are
received, really), which makes it incompatible with some tools or
other programs. Reindexing using earlier versions will also not work
anymore as a result of this.

* The block index database will now hold headers for which no block is
stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data
directory. Without this your node will need start syncing (or importing from
bootstrap.dat) anew afterwards. It is possible that the data from a completely
synchronised 0.10 node may be usable in older versions as-is, but this is not
supported and may break as soon as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility. There are no
known problems when downgrading from 0.11.x to 0.10.x.

Notable changes since 0.11.2
============================

Alert system
--------------

The P2P alert system is off by default. To turn on, use `-alert` with startup
configuration.

0.11.3 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.


Credits
=======

Thanks to everyone who directly contributed to this release:


And those who contributed additional code review and/or security research.

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
