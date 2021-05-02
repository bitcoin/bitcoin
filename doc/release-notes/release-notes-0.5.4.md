XBit version 0.5.4 is now available for download at:
http://sourceforge.net/projects/xbit/files/XBit/xbit-0.5.4/
NOTE: 0.5.4rc3 is being renamed to 0.5.4 final with no changes.

This is a bugfix-only release in the 0.5.x series, plus a few protocol updates.

Please report bugs using the issue tracker at github:
https://github.com/xbit/xbit/issues

Stable source code is hosted at Gitorious:
http://gitorious.org/xbit/xbitd-stable/archive-tarball/v0.5.4#.tar.gz

PROTOCOL UPDATES

BIP 16: Special-case "pay to script hash" logic to enable minimal validation of new transactions.
Support for validating message signatures produced with compressed public keys.

BUG FIXES

Build with thread-safe MingW libraries for Windows, fixing a dangerous memory corruption scenario when exceptions are thrown.
Fix broken testnet mining.
Stop excess inventory relay during initial block download.
When disconnecting a node, clear the received buffer so that we do not process any already received messages.
Yet another attempt at implementing "minimize to tray" that works on all operating systems.
Fix XBit-Qt notifications under Growl 1.3.
Increase required age of XBit-Qt's "not up to date" status from 30 to 90 minutes.
Implemented missing verifications that led to crash on entering some wrong passphrases for encrypted wallets.
Fix default filename suffixes in GNOME save dialog.
Make the "Send coins" tab use the configured unit type, even on the first attempt.
Print detailed wallet loading errors to debug.log when it is corrupt.
Allocate exactly the amount of space needed for signing transactions, instead of a fixed 10k buffer.
Workaround for improbable memory access violation.
Check wallet's minimum version before trying to load it.
Remove wxXBit properly when installing XBit-Qt over it. (Windows)
Detail reorganization information better in debug log.
Use a messagebox to display the error when -server is provided without configuring a RPC password.
Testing suite build now honours provided CXXFLAGS.
Removed an extraneous line-break in mature transaction tooltips.
Fix some grammatical errors in translation process documentation.
