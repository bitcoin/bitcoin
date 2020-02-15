Bitcoin version 0.7.2 is now available from:
  http://sourceforge.net/projects/bitcoin/files/Bitcoin/bitcoin-0.7.2

This is a bug-fix minor release.

Please report bugs using the issue tracker at github:
  https://github.com/bitcoin/bitcoin/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait
until it has completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Bitcoin-Qt (on Mac) or bitcoind/bitcoin-qt (on Linux).

If you were running on Linux with a version that might have been compiled
with a different version of Berkeley DB (for example, if you were using an
Ubuntu PPA version), then run the old version again with the -detachdb
argument and shut it down; if you do not, then the new version will not
be able to read the database files and will exit with an error.

Explanation of -detachdb (and the new "stop true" RPC command):
The Berkeley DB database library stores data in both ".dat" and
"log" files, so the database is always in a consistent state,
even in case of power failure or other sudden shutdown. The
format of the ".dat" files is portable between different
versions of Berkeley DB, but the "log" files are not-- even minor
version differences may have incompatible "log" files. The
-detachdb option moves any pending changes from the "log" files
to the "blkindex.dat" file for maximum compatibility, but makes
shutdown much slower. Note that the "wallet.dat" file is always
detached, and versions prior to 0.6.0 detached all databases
at shutdown.

Bug fixes
---------

* Prevent RPC 'move' from deadlocking. This was caused by trying to lock the
  database twice.

* Fix use-after-free problems in initialization and shutdown, the latter of
  which caused Bitcoin-Qt to crash on Windows when exiting.

* Correct library linking so building on Windows natively works.

* Avoid a race condition and out-of-bounds read in block creation/mining code.

* Improve platform compatibility quirks, including fix for 100% CPU utilization
  on FreeBSD 9.

* A few minor corrections to error handling, and updated translations.

* OSX 10.5 supported again

----------------------------------------------------
Thanks to everybody who contributed to this release:

Alex
dansmith
Gavin Andresen
Gregory Maxwell
Jeff Garzik
Luke Dashjr
Philip Kaufmann
Pieter Wuille
Wladimir J. van der Laan
grimd34th
