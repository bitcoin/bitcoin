Bitcoin version 0.6.2 is now available for download at:
http://sourceforge.net/projects/bitcoinrupee/files/Bitcoin/bitcoinrupee-0.6.2/

This is a bug-fix and code-cleanup release, with no major new features.

Please report bugs using the github issue tracker at:
https://github.com/bitcoinrupee/bitcoinrupee/issues


NOTABLE CHANGES

Much faster shutdowns. However, the blkindex.dat file is no longer
portable to different data directories by default. If you need a
portable blkindex.dat file then run with the new -detachdb=1 option
or the "Detach databases at shutdown" GUI preference.

Fixed https://github.com/bitcoinrupee/bitcoinrupee/issues/1065, a bug that
could cause long-running nodes to crash.

Mac and Windows binaries are compiled against OpenSSL 1.0.1b (Linux
binaries are dynamically linked to the version of OpenSSL on the system).


CHANGE SUMMARY

Use 'git shortlog --no-merges v0.6.0..' for a summary of this release.

Source codebase changes:
- Many source code cleanups and warnings fixes.  Close to building with -Wall
- Locking overhaul, and several minor locking fixes
- Several source code portability fixes, e.g. FreeBSD

JSON-RPC interface changes:
- addmultisigaddress enabled for mainnet (previously only enabled for testnet)

Network protocol changes:
- protocol version 60001
- added nonce value to "ping" message (BIP 31)
- added new "pong" message (BIP 31)

Backend storage changes:
- Less redundant database flushing, especially during initial block download
- Shutdown improvements (see above)

Qt user interface:
- minor URI handling improvements
- progressbar improvements
- error handling improvements (show message box rather than console exception,
etc.)
- by popular request, make 4th bar of connection icon green
