4.4.0 Release Notes
======================

Syscoin Core version 4.4.0 is now available from:

  <https://github.com/syscoin/syscoin/releases/tag/v4.4.0>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: <https://syscoin.readme.io/v4.4.0/docs/syscoin-43-upgrade-guide>
Basic upgrade instructions below:

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux).

If you are upgrading from a version older than 4.2.0, PLEASE READ: <https://syscoin.readme.io/v4.2.0/docs/syscoin-42-upgrade-guide>

This should reindex your local node and synchronize with the network from genesis.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.15+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

A target block height of 1586000 has been set as the upgrade blockheight.
This will happen at approximately March 23rd 2023.
A more accurate countdown counter can be found at [syscoin.org](https://www.syscoin.org). 

Nodes not upgraded to Syscoin Core 4.4 will NOT be able to sync up to the network past block 1586000
After the block height, 1 new change will be activated on the network
1. Syscoin PODA will launch

This upgrade brought in upstream changes from Bitcoin from 22.1 - 24.0.1 and tracking on the latest commit on master branch.  For changelog brought in from upstream, please see the release notes from Bitcoin Core.
- [22.1](https://bitcoincore.org/en/releases/22.1/)
- [23.0](https://bitcoincore.org/en/releases/23.0/)
- [23.1](https://bitcoincore.org/en/releases/23.1/)
- [24.0.1](https://bitcoincore.org/en/releases/24.0.1/)

Syscoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From Syscoin Core 4.4.0 onwards, macOS versions earlier than 10.15 are no
longer supported. 

Syscoin Core Changes
--------------------
Bitcoin 24.0+ Core engine is now used to power consensus and security of Syscoin Core. The changeset is here in this Pull request

### Block Structure Change
- Syscoin PoDA activates

### Syscoin Core asset interface removed
- Calls to interact with Syscoin assets have been removed from Syscoin Core.  Assets can still be accessed using syscoinjs-lib or Pali Wallet.

New RPC
-------
- getnevmblobdata: Return NEVM blob information and status from a version hash.
- listnevmblobdata
- syscoincreaterawnevmblob

Removed RPC
-----------
- addressbalance
- assetallocationbalance
- assetallocationburn
- assetallocationmint
- assetallocationsend
- assetallocationsendmany
- assetnew
- assetsend
- assetsendmany
- assetupdate
- assettransfer
- listunspentasset
- sendfrom

Tests
-----
- [Unit tests](https://github.com/syscoin/syscoin/tree/master/src/test)
- [Functional tests](https://github.com/syscoin/syscoin/tree/master/test/functional)

Credits
=======

Thanks to everyone for the continued support of Syscoin

