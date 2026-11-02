v32.x Release Notes
===================

Bitcoin Core version 32.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-32.x/>

This release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>


How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bin/` and `libexec/` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on the following operating systems or newer:
Linux Kernel 3.17, macOS 14, and Windows 10 (version 1903). Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### Wallet

- As of the introduction of PSBTv2 support in 32.1, Bitcoin Core is no longer
  compatible with external signer software that does not support PSBTv2. This does
  not mean users need to have hardware wallets that support PSBTv2, only that
  users that start bitcoind with an external signer set via `-signer=<cmd>`, will
  need to be using an external signing tool, like HWI, which supports PSBTv2. For
  users of HWI, this means a minimum version of 2.1.0 (released in 2022) is
  required. (https://github.com/bitcoin-core/HWI/releases/tag/2.1.0) (#36539)

### Gui

- #36539 gui: Use PSBT v0

### Guix

- #36615 guix: add --no-same-owner to codesign.sh TAR_OPTIONS

### Test

- #36405 test: look for the wtxid in p2p_blocksonly
- #36406 test: make p2p_orphan_handling's never-requested checks able to fail
- #36617 test: Fix intermittent issue in p2p_orphan_handling.py

### CI

- #36701 ci: Disable tests on ARM32 in GHA yaml

Credits
=======

Thanks to everyone who directly contributed to this release:

- David Gumberg
- fametrano
- maflcko
- marcofleon

As well as to everyone that helped with translations on
[Transifex](https://explore.transifex.com/bitcoin/bitcoin/).
