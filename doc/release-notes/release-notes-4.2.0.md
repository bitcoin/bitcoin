4.2.0 Release Notes
===================

Syscoin Core version 4.2.0 is now available from:
DO NOT USE UNTIL BLOCK 1004200

  <https://github.com/syscoin/syscoin/releases/tag/v4.2.0>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: <https://syscoin.readme.io/v4.2.0/docs/syscoin-42-upgrade-guide>
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
the Linux kernel, macOS 10.12+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

A target block height of 1004200 has been set as the upgrade blockheight.
This will happen at approximately April 30th 2021, 8PM UTC.
A more accurate countdown counter can be found at [syscoin.org](https://www.syscoin.org). 

Nodes not upgraded to Syscoin Core 4.2 will NOT be able to sync up to the network past block 1004200
After the block height, 2 new changes will be activated on the network
1. The Syscoin Platform Token protocol will be on utxo model
2. Masternodes will be deterministic

This upgrade brought in upstream changes from Bitcoin from 0.20.0 - 0.21.0 and tracking on the latest commit on master branch.  For changelog brought in from upstream, please see the release notes from Bitcoin Core.
- [0.20.0](https://bitcoincore.org/en/releases/0.20.0/)
- [0.20.1](https://bitcoincore.org/en/releases/0.20.1/)
- [0.21.0](https://bitcoincore.org/en/releases/0.21.0/)

This upgrade also brought in upstream changes from Dash, including the following Dash improvement proposals.  Please refer to the DIP for more information
- [DIP-0003 Deterministic Masternode Lists](https://github.com/dashpay/dips/blob/master/dip-0003.md)
- [DIP-0004 Simplified Verification of Deterministic Masternode Lists](https://github.com/dashpay/dips/blob/master/dip-0004.md)
- [DIP-0006 Long-Living Masternode Quorums](https://github.com/dashpay/dips/blob/master/dip-0006.md)
- [DIP-0007 LLMQ Signing Requests / Sessions](https://github.com/dashpay/dips/blob/master/dip-0007.md)
- [DIP-0008 ChainLocks](https://github.com/dashpay/dips/blob/master/dip-0008.md)

Syscoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From Syscoin Core 4.1.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Syscoin Core does not yet change appearance
when macOS "dark mode" is activated.

Notable changes
===============

- rpc: deprecate addresses and reqSigs from rpc outputs [#20286](https://github.com/bitcoin/bitcoin/pull/20286).
    - The following RPC calls are affected: `gettxout`, `getrawtransaction`, `decoderawtransaction`, `decodescript`, `gettransaction`, and REST endpoints: `/rest/tx`, `/rest/getutxos`, `/rest/block`.
    - Two fields, `addresses` and `reqSigs`,  are removed from the output of the above commands. And new sane field `address` is added.
    - Adding `-deprecatedrpc=addresses` to startup argument will leave `reqSigs` and `addresses` intact


Syscoin Core Changes
--------------------
Bitcoin 0.21+ Core engine is now used to power consensus and security of Syscoin Core. The changeset is here in this Pull request: https://github.com/syscoin/syscoin/pull/421

### Syscoin Platform Token protocol change
- Syscoin platform token protocol has been migrated from previously account-based to UTXO-based
- [Compliance through notarization](https://github.com/syscoin/sips/blob/master/sip-0002.mediawiki)

### Masternode upgrade
- Masternode are now deterministic following dash's [DIP-0003](https://github.com/dashpay/dips/blob/master/dip-0003.md)

### Bridge Freeze
- We are in research to create turing complete contracts for Syscoin and will work on composability aspects to link external systems such as Ethereum with Syscoin through these much more flexible composable layer 2 rollups as described in <https://jsidhu.medium.com/a-design-for-an-efficient-coordinated-financial-computing-platform-ab27e8a825a0>.

### Auxpow ID change
- Auxpow ID has been changed to 16


Updated RPCs
------------
- All asset and masternode related RPCs have been updated to reflect the rewrite of the asset and masternode changes

New RPCs
--------
- listunspentasset: Helper function which just calls listunspent to find unspent UTXO's for an asset.
- signmessagebech32: Sign a message with bech32 (sys1) address.
- assettransactionnotarize: Update notary signature on an asset transaction. Will require re-signing transaction before submitting to network.
- getnotarysighash: Get sighash for notary to sign off on, use assettransactionnotarize to update the transaction after re-singing once sighash is used to create a notarized signature.

Tests
-----
- [Unit tests](https://github.com/syscoin/syscoin/tree/master/src/test)
- [Functional tests](https://github.com/syscoin/syscoin/tree/master/test/functional)

Credits
=======

Thanks to everyone for the continued support of Syscoin.  Special shout-out to c\_e\_d, Alternating and Johnp for their dedicated involvement with the 4.2 testnet.

