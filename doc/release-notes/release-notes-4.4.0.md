4.4.0 Release Notes
======================

Syscoin Core version 4.4.0 is now available from:

  <https://github.com/syscoin/syscoin/releases/tag/v4.4.0>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Note: Masternode operators should upgrade their sentinel to v4.4.0 

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: <https://syscoin.readme.io/v4.4.0/docs/syscoin-44-upgrade-guide>
Basic upgrade instructions below:

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux).

If you are upgrading from a version older than 4.2.0, PLEASE READ: <https://syscoin.readme.io/v4.2.0/docs/syscoin-42-upgrade-guide>

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.15+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

A target block height of 1586000 has been set as the upgrade blockheight.
This will happen at approximately March 23rd 2023.

Nodes not upgraded to Syscoin Core 4.4 will NOT be able to sync up to the network past block 1586000
After the block height, 3 new changes will be activated on the network
1. Syscoin PODA will launch which adds logic to consensus code related to keccack based blobs and an NEVM precompile to access it read <https://syscoin.org/news/revealing-the-method-in-the-madness>
2. V19 BLS upgrade to the standard IETF spec <https://datatracker.ietf.org/doc/draft-irtf-cfrg-bls-signature/>
3. Specific NEVM upgrades:
  - EIP-3651 warm COINBASE <https://eips.ethereum.org/EIPS/eip-3651>
  - EIP-3855 PUSH0 instruction <https://eips.ethereum.org/EIPS/eip-3855>
  - EIP-3860 Limit and meter initcode <https://eips.ethereum.org/EIPS/eip-3860>

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
Bitcoin 24.0+ Core engine is now used to power consensus and security of Syscoin Core. The changeset is here in this Pull request. Note that wallet users now by default will get descriptor wallets enabled. If you are a masternode or a service that requires legacy wallets you may still create them by setting the `descriptor` field to `false` when creating wallets.

### Block Structure Change
- Syscoin PoDA activates

### Syscoin Core asset interface removed
- Calls to interact with Syscoin assets have been removed from Syscoin Core.  Assets can still be accessed using syscoinjs-lib or Pali Wallet.

V19 BLS Upgrade
----------------

- Once v19 hard fork is activated `protx_register`, `protx_register_fund`, `protx_register_prepare` RPCs will decode BLS operator public key using new basic BLS scheme. In order to support BLS public keys encoded in legacy BLS scheme a legacy parameter was added to those RPC functions.

`qcommit`
--------------------

Once the hard fork is activated, `quorumPublicKey` will be serialised using basic BLS scheme.
To support sync of old chain,`version` field indicates which scheme to use for serialisation of `quorumPublicKey`.

| Version | Version Description                                     |
|---------|---------------------------------------------------------|
| 1       | qcommit serialised using legacy BLS scheme              |
| 2       | qcommit serialised using basic BLS scheme               |

`MNLISTDIFF` P2P message
--------

Starting with protocol version 70017, the following field is added to the `MNLISTDIFF` message between `cbTx`.

| Field               | Type | Size | Description                       |
|---------------------| --- | --- |-----------------------------------|
| version             | uint16_t | 2 | Version of the `MNLISTDIFF` reply |

The `version` field indicates which BLS scheme is used to serialise all SML entrie's `pubKeyOperator` field of `mnList`.

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

`ProTx` txs family 
--------

`proregtx` and `proupregtx` will support a new `version` value:

| Version | Version Description                                       |
|---------|-----------------------------------------------------------|
| 1       | Serialisation of `pubKeyOperator` using legacy BLS scheme |
| 2       | Serialisation of `pubKeyOperator` using basic BLS scheme  |

`proupservtx` and `prouprevtx` will support a new `version` value:

| Version | Version Description                            |
|---------|------------------------------------------------|
| 1       | Serialisation of `sig` using legacy BLS scheme |
| 2       | Serialisation of `sig` using basic BLS scheme  |


BLS enforced scheme
--------
Once the hard fork is activated all remaining messages containing BLS public keys or signatures will serialise them using the new basic BLS scheme.

List of affected messages:
`mnauth`, `govobj`, `govobjvote`,  `qsigshare`, `qsigrec`, `clsig`, all DKG messages (`qfcommit`, `qcontrib`, `qcomplaint`, `qjustify`, `qpcommit`);

New RPC
-------
- getnevmblobdata: Return NEVM blob information and status from a version hash.
- listnevmblobdata: List existing NEVM blobs
- syscoincreatenevmblob: Create a NEVM blob

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

