XBit version 0.7.1 is now available from:
  http://sourceforge.net/projects/xbit/files/XBit/xbit-0.7.1/

This is a bug-fix minor release.

Please report bugs using the issue tracker at github:
  https://github.com/xbit/xbit/issues

Project source code is hosted at github; you can get
source-only tarballs/zipballs directly from there:
  https://github.com/xbit/xbit/tarball/v0.7.1  # .tar.gz
  https://github.com/xbit/xbit/zipball/v0.7.1  # .zip

Ubuntu Linux users can use the "Personal Package Archive" (PPA)
maintained by Matt Corallo to automatically keep 
up-to-date.  Just type:
  sudo apt-add-repository ppa:xbit/xbit
  sudo apt-get update
in your terminal, then install the xbit-qt package:
  sudo apt-get install xbit-qt

KNOWN ISSUES
------------

Mac OSX 10.5 is no longer supported.

How to Upgrade
--------------

If you are running an older version, shut it down. Wait
until it has completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/XBit-Qt (on Mac) or xbitd/xbit-qt (on Linux).

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

New features
------------

* Added a boolean argument to the RPC 'stop' command, if true sets
  -detachdb to create standalone database .dat files before shutting down.

* -salvagewallet command-line option, which moves any existing wallet.dat
  to wallet.{timestamp}.dat and then attempts to salvage public/private
  keys and master encryption keys (if the wallet is encrypted) into
  a new wallet.dat. This should only be used if your wallet becomes
  corrupted, and is not intended to replace regular wallet backups.

* Import $DataDir/bootstrap.dat automatically, if it exists.

Dependency changes
------------------

* Qt 4.8.2 for Windows builds

* openssl 1.0.1c

Bug fixes
---------

* Clicking on a xbit: URI on Windows should now launch XBit-Qt properly.

* When running -testnet, use RPC port 18332 by default.

* Better detection and handling of corrupt wallet.dat and blkindex.dat files.
  Previous versions would crash with a DB_RUNRECOVERY exception, this
  version detects most problems and tells you how to recover if it
  cannot recover itself.

* Fixed an uninitialized variable bug that could cause transactions to
  be reported out of order.

* Fixed a bug that could cause occasional crashes on exit.

* Warn the user that they need to create fresh wallet backups after they
  encrypt their wallet.

----------------------------------------------------
Thanks to everybody who contributed to this release:

Gavin Andresen
Jeff Garzik
Luke Dashjr
Mark Friedenbach
Matt Corallo
Philip Kaufmann
Pieter Wuille
Rune K. Svendsen
Virgil Dupras
Wladimir J. van der Laan
fanquake
kjj2
xanatos
