*This is a draft!*

0.12.0 Release notes
====================


Dash Core version 0.11.2 is now available from:

  https://dashpay.io/downloads

Please report bugs using the issue tracker at github:

  https://github.com/dashpay/dash/issues


How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

Downgrade warning
------------------

Because release 0.12.0 and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-0.12 versions of Dash Core or other software:

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

This does not affect wallet forward or backward compatibility.


0.12.0 changelog
----------------

Switched to Bitcoin Core version 0.10
https://bitcoin.org/en/release/v0.10.0

Credits
--------

Thanks to who contributed to this release, at least:

- *to do ..*

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/dash/).
