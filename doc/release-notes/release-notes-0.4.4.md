Bitcoin version 0.4.4 is now available for download at:
http://luke.dashjr.org/programs/bitcoinrupee/files/bitcoinrupeed-0.4.4/

This is a bugfix-only release based on 0.4.0.

Please note that the wxBitcoin GUI client is no longer maintained nor supported. If someone would like to step up to maintain this, they should contact Luke-Jr.

Please report bugs for the daemon only using the issue tracker at github:
https://github.com/bitcoinrupee/bitcoinrupee/issues

Stable source code is hosted at Gitorious:
http://gitorious.org/bitcoinrupee/bitcoinrupeed-stable/archive-tarball/v0.4.4#.tar.gz

BUG FIXES

Limit the number of orphan transactions stored in memory, to prevent a potential denial-of-service attack by flooding orphan transactions. Also never store invalid transactions at all.
Fix possible buffer overflow on systems with very long application data paths. This is not exploitable.
Resolved multiple bugs preventing long-term unlocking of encrypted wallets (issue #922).
Only send local IP in "version" messages if it is globally routable (ie, not private), and try to get such an IP from UPnP if applicable.
Reannounce UPnP port forwards every 20 minutes, to workaround routers expiring old entries, and allow the -upnp option to override any stored setting.
Various memory leaks and potential null pointer deferences have been
fixed.
Several shutdown issues have been fixed.
Check that keys stored in the wallet are valid at startup, and if not,
report corruption.
Various build fixes.
If no password is specified to bitcoinrupeed, recommend a secure password.
Update hard-coded fallback seed nodes, choosing recent ones with long uptime and versions at least 0.4.0.
Add checkpoint at block 168,000.

