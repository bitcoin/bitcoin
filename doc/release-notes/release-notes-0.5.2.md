Bitcoin version 0.5.2 is now available for download at:
http://sourceforge.net/projects/bitcoinrupee/files/Bitcoin/bitcoinrupee-0.5.2/

This is a bugfix-only release based on 0.5.1.

Please report bugs using the issue tracker at github:
https://github.com/bitcoinrupee/bitcoinrupee/issues

Stable source code is hosted at Gitorious:
http://gitorious.org/bitcoinrupee/bitcoinrupeed-stable/archive-tarball/v0.5.2#.tar.gz

BUG FIXES

Check all transactions in blocks after the last checkpoint (0.5.0 and 0.5.1 skipped checking ECDSA signatures during initial blockchain download).
Cease locking memory used by non-sensitive information (this caused a huge performance hit on some platforms, especially noticable during initial blockchain download; this was
not a security vulnerability).
Fixed some address-handling deadlocks (client freezes).
No longer accept inbound connections over the internet when Bitcoin is being used with Tor (identity leak).
Re-enable SSL support for the JSON-RPC interface (it was unintentionally disabled for the 0.5.0 and 0.5.1 release Linux binaries).
Use the correct base transaction fee of 0.0005 BTC for accepting transactions into mined blocks (since 0.4.0, it was incorrectly accepting 0.0001 BTC which was only meant to be relayed).
Don't show "IP" for transactions which are not necessarily IP transactions.
Add new DNS seeds (maintained by Pieter Wuille and Luke Dashjr).
