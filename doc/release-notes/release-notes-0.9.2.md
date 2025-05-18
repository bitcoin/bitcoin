Tortoisecoin Core version 0.9.2 is now available from:

  https://tortoisecoin.org/bin/0.9.2/

This is a new minor version release, bringing mostly bug fixes and some minor
improvements. OpenSSL has been updated because of a security issue (CVE-2014-0224).
Upgrading to this release is recommended.

Please report bugs using the issue tracker at github:

  https://github.com/tortoisecoin/tortoisecoin/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Tortoisecoin-Qt (on Mac) or
tortoisecoind/tortoisecoin-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you run
0.9.2 your blockchain files will be re-indexed, which will take anywhere from 
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

Important changes
==================

Gitian OSX build
-----------------

The deterministic build system that was already used for Windows and Linux
builds is now used for OSX as well. Although the resulting executables have
been tested quite a bit, there could be possible regressions. Be sure to report
these on the Github bug tracker mentioned above.

Compatibility of Linux build
-----------------------------

For Linux we now build against Qt 4.6, and filter the symbols for libstdc++ and glibc.
This brings back compatibility with

- Debian 6+ / Tails
- Ubuntu 10.04
- CentOS 6.5

0.9.2 Release notes
=======================

The OpenSSL dependency in the gitian builds has been upgraded to 1.0.1h because of CVE-2014-0224.

RPC:

- Add `getwalletinfo`, `getblockchaininfo` and `getnetworkinfo` calls (will replace hodge-podge `getinfo` at some point)
- Add a `relayfee` field to `getnetworkinfo`
- Fix RPC related shutdown hangs and leaks
- Always show syncnode in `getpeerinfo`
- `sendrawtransaction`: report the reject code and reason, and make it possible to re-send transactions that are already in the mempool
- `getmininginfo` show right genproclimit

Command-line options:

- Fix `-printblocktree` output
- Show error message if ReadConfigFile fails

Block-chain handling and storage:

- Fix for GetBlockValue() after block 13,440,000 (BIP42)
- Upgrade leveldb to 1.17

Protocol and network code:

- Per-peer block download tracking and stalled download detection
- Add new DNS seed from bitnodes.io
- Prevent socket leak in ThreadSocketHandler and correct some proxy related socket leaks
- Use pnode->nLastRecv as sync score (was the wrong way around)

Wallet:

- Make GetAvailableCredit run GetHash() only once per transaction (performance improvement)
- Lower paytxfee warning threshold from 0.25 BTC to 0.01 BTC
- Fix importwallet nTimeFirstKey (trigger necessary rescans)
- Log BerkeleyDB version at startup
- CWallet init fix

Build system:

- Add OSX build descriptors to gitian
- Fix explicit --disable-qt-dbus
- Don't require db_cxx.h when compiling with wallet disabled and GUI enabled
- Improve missing boost error reporting
- Upgrade miniupnpc version to 1.9
- gitian-linux: --enable-glibc-back-compat for binary compatibility with old distributions
- gitian: don't export any symbols from executable
- gitian: build against Qt 4.6
- devtools: add script to check symbols from Linux gitian executables
- Remove build-time no-IPv6 setting

GUI:

- Fix various coin control visual issues
- Show number of in/out connections in debug console
- Show weeks as well as years behind for long timespans behind
- Enable and disable the Show and Remove buttons for requested payments history based on whether any entry is selected.
- Show also value for options overridden on command line in options dialog
- Fill in label from address book also for URIs
- Fixes feel when resizing the last column on tables (issue #2862)
- Fix ESC in disablewallet mode
- Add expert section to wallet tab in optionsdialog
- Do proper boost::path conversion (fixes unicode in datadir)
- Only override -datadir if different from the default (fixes -datadir in config file)
- Show rescan progress at start-up
- Show importwallet progress
- Get required locks upfront in polling functions (avoids hanging on locks)
- Catch Windows shutdown events while client is running
- Optionally add third party links to transaction context menu
- Check for !pixmap() before trying to export QR code (avoids crashes when no QR code could be generated)
- Fix "Start tortoisecoin on system login"

Miscellaneous:

- Replace non-threadsafe C functions (gmtime, strerror and setlocale)
- Add missing cs_main and wallet locks
- Avoid exception at startup when system locale not recognized
- Changed bitrpc.py's raw_input to getpass for passwords to conceal characters during command line input
- devtools: add a script to fetch and postprocess translations

Credits
--------

Thanks to everyone who contributed to this release:

- Addy Yeow
- Altoidnerd
- Andrea D'Amore
- Andreas Schildbach
- Bardi Harborow
- Brandon Dahler
- Bryan Bishop
- Chris Beams
- Christian von Roques
- Cory Fields
- Cozz Lovan
- daniel
- Daniel Newton
- David A. Harding
- ditto-b
- duanemoody
- Eric S. Bullington
- Fabian Raetz
- Gavin Andresen
- Gregory Maxwell
- gubatron
- Haakon Nilsen
- harry
- Hector Jusforgues
- Isidoro Ghezzi
- Jeff Garzik
- Johnathan Corgan
- jtimon
- Kamil Domanski
- langerhans
- Luke Dashjr
- Manuel Araoz
- Mark Friedenbach
- Matt Corallo
- Matthew Bogosian
- Meeh
- Michael Ford
- Michagogo
- Mikael Wikman
- Mike Hearn
- olalonde
- paveljanik
- peryaudo
- Philip Kaufmann
- philsong
- Pieter Wuille
- R E Broadley
- richierichrawr
- Rune K. Svendsen
- rxl
- shshshsh
- Simon de la Rouviere
- Stuart Cardall
- super3
- Telepatheic
- Thomas Zander
- Torstein Husebø
- Warren Togami
- Wladimir J. van der Laan
- Yoichi Hirai
