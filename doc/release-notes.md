v31.x Release Notes
===================

Bitcoin Core version 31.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-31.x/>

This release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the installer
(on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS) or
`bin/` and `libexec/` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be
migrated. Old wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or
newer: Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin Core
should also work on most other Unix-like systems but is not as frequently tested
on them. It is not recommended to use Bitcoin Core on unsupported systems.

Notable changes
===============

### Net

- #36894 net_processing: set fDisconnect instead of calling DisconnectNode

### Build

- #36889 cmake: Switch include_directories -> target_include_directories

### Misc

- #36869 refactor, memusage: Make fields of STL helper structs public

Credits
=======

Thanks to everyone who directly contributed to this release:

- Eugene Siegel
- hebasto

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
