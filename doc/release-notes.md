Bitcoin Core version 0.14.2 is now available from:

  <https://bitcoin.org/bin/bitcoin-core-0.14.2/>

This is a new minor version release, including various bugfixes and
performance improvements, as well as updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

Compatibility
==============

Bitcoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later.

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
No attempt is made to prevent installing or running the software on Windows XP, you
can still do so at your own risk but be aware that there are known instabilities and issues.
Please do not report issues about Windows XP to the issue tracker.

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Low-level RPC changes
---------------------

- Error codes have been updated to be more accurate for the following error cases:
  - `getblock` now returns RPC_MISC_ERROR if the block can't be found on disk (for
  example if the block has been pruned). Previously returned RPC_INTERNAL_ERROR.
  - `pruneblockchain` now returns RPC_MISC_ERROR if the blocks cannot be pruned
  because the node is not in pruned mode. Previously returned RPC_METHOD_NOT_FOUND.
  - `pruneblockchain` now returns RPC_INVALID_PARAMETER if the blocks cannot be pruned
  because the supplied timestamp is too late. Previously returned RPC_INTERNAL_ERROR.
  - `pruneblockchain` now returns RPC_MISC_ERROR if the blocks cannot be pruned
  because the blockchain is too short. Previously returned RPC_INTERNAL_ERROR.
  - `setban` now returns RPC_CLIENT_INVALID_IP_OR_SUBNET if the supplied IP address
  or subnet is invalid. Previously returned RPC_CLIENT_NODE_ALREADY_ADDED.
  - `setban` now returns RPC_CLIENT_INVALID_IP_OR_SUBNET if the user tries to unban
  a node that has not previously been banned. Previously returned RPC_MISC_ERROR.
  - `removeprunedfunds` now returns RPC_WALLET_ERROR if bitcoind is unable to remove
  the transaction. Previously returned RPC_INTERNAL_ERROR.
  - `removeprunedfunds` now returns RPC_INVALID_PARAMETER if the transaction does not
  exist in the wallet. Previously returned RPC_INTERNAL_ERROR.
  - `fundrawtransaction` now returns RPC_INVALID_ADDRESS_OR_KEY if an invalid change
  address is provided. Previously returned RPC_INVALID_PARAMETER.
  - `fundrawtransaction` now returns RPC_WALLET_ERROR if bitcoind is unable to create
  the transaction. The error message provides further details. Previously returned
  RPC_INTERNAL_ERROR.
  - `bumpfee` now returns RPC_INVALID_PARAMETER if the provided transaction has
  descendants in the wallet. Previously returned RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_INVALID_PARAMETER if the provided transaction has
  descendants in the mempool. Previously returned RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction has
  has been mined or conflicts with a mined transaction. Previously returned
  RPC_INVALID_ADDRESS_OR_KEY.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction is not
  BIP 125 replaceable. Previously returned RPC_INVALID_ADDRESS_OR_KEY.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction has already
  been bumped by a different transaction. Previously returned RPC_INVALID_REQUEST.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction contains
  inputs which don't belong to this wallet. Previously returned RPC_INVALID_ADDRESS_OR_KEY.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction has multiple change
  outputs. Previously returned RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_WALLET_ERROR if the provided transaction has no change
  output. Previously returned RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_WALLET_ERROR if the fee is too high. Previously returned
  RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_WALLET_ERROR if the fee is too low. Previously returned
  RPC_MISC_ERROR.
  - `bumpfee` now returns RPC_WALLET_ERROR if the change output is too small to bump the
  fee. Previously returned RPC_MISC_ERROR.

miniupnp CVE-2017-8798
----------------------------

Bundled miniupnpc was updated to 2.0.20170509. This fixes an integer signedness error
(present in MiniUPnPc v1.4.20101221 through v2.0) that allows remote attackers
(within the LAN) to cause a denial of service or possibly have unspecified
other impact.

This only affects users that have explicitly enabled UPnP through the GUI
setting or through the `-upnp` option, as since the last UPnP vulnerability
(in Bitcoin Core 0.10.3) it has been disabled by default.

If you use this option, it is recommended to upgrade to this version as soon as
possible.

Known Bugs
==========

Since 0.14.0 the approximate transaction fee shown in Bitcoin-Qt when using coin
control and smart fee estimation does not reflect any change in target from the
smart fee slider. It will only present an approximate fee calculated using the
default target. The fee calculated using the correct target is still applied to
the transaction and shown in the final send confirmation dialog.

0.14.2 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### RPC and other APIs
- #10410 `321419b` Fix importwallet edge case rescan bug (ryanofsky)

### P2P protocol and network code
- #10424 `37a8fc5` Populate services in GetLocalAddress (morcos)
- #10441 `9e3ad50` Only enforce expected services for half of outgoing connections (theuni)

### Build system
- #10414 `ffb0c4b` miniupnpc 2.0.20170509 (fanquake)
- #10228 `ae479bc` Regenerate bitcoin-config.h as necessary (theuni)

### Miscellaneous
- #10245 `44a17f2` Minor fix in build documentation for FreeBSD 11 (shigeya)
- #10215 `0aee4a1` Check interruptNet during dnsseed lookups (TheBlueMatt)

### GUI
- #10231 `1e936d7` Reduce a significant cs_main lock freeze (jonasschnelli)

### Wallet
- #10294 `1847642` Unset change position when there is no change (instagibbs)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alex Morcos
- Cory Fields
- fanquake
- Gregory Sanders
- Jonas Schnelli
- Matt Corallo
- Russell Yanofsky
- Shigeya Suzuki
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).

