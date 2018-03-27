Chaincoin Core version 0.16.1 is now available from:

  <https://github.com/chaincoin/chaincoin/releases/tag/v0.16.1/>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/chaincoin/chaincoin/issues>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Chaincoin-Qt` (on Mac)
or `chaincoind`/`chaincoin-qt` (on Linux).

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0 or higher. Upgrading
directly from 0.7.x and earlier without re-downloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

Wallets created in 0.16 and later are not compatible with versions prior to 0.16
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Chaincoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.11+, and Windows 7 and newer (Windows XP is not supported).

Chaincoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Performance Improvements
------------------------

Version 0.16 contains a number of significant performance improvements, which make
Initial Block Download, startup, transaction and block validation much faster:

- The chainstate database (which is used for tracking UTXOs) has been changed
from a per-transaction model to a per-output model (See [PR 10195](https://github.com/bitcoin/bitcoin/pull/10195)). Advantages of this model
are that it:
- avoids the CPU overhead of deserializing and serializing the unused outputs;
- has more predictable memory usage;
- uses simpler code;
- is adaptable to various future cache flushing strategies.

As a result, validating the blockchain during Initial Block Download (IBD) and reindex
is ~30-40% faster, uses 10-20% less memory, and flushes to disk far less frequently.
The only downside is that the on-disk database is 15% larger. During the conversion from the previous format
a few extra gigabytes may be used.
- Earlier versions experienced a spike in memory usage while flushing UTXO updates to disk.
As a result, only half of the available memory was actually used as cache, and the other half was
reserved to accommodate flushing. This is no longer the case (See [PR 10148](https://github.com/bitcoin/bitcoin/pull/10148)), and the entirety of
the available cache (see `-dbcache`) is now actually used as cache. This reduces the flushing
frequency by a factor 2 or more.
- In previous versions, signature validation for transactions has been cached when the
transaction is accepted to the mempool. Version 0.15 extends this to cache the entire script
validity (See [PR 10192](https://github.com/bitcoin/bitcoin/pull/10192)). This means that if a transaction in a block has already been accepted to the
mempool, the scriptSig does not need to be re-evaluated. Empirical tests show that
this results in new block validation being 40-50% faster.
- LevelDB has been upgraded to version 1.20 (See [PR 10544](https://github.com/bitcoin/bitcoin/pull/10544)). This version contains hardware acceleration for CRC
on architectures supporting SSE 4.2. As a result, synchronization and block validation are now faster.
- SHA256 hashing has been optimized for architectures supporting SSE 4 (See [PR 10821](https://github.com/bitcoin/bitcoin/pull/10821)). SHA256 is around
50% faster on supported hardware, which results in around 5% faster IBD and block
validation. In version 0.15, SHA256 hardware optimization is disabled in release builds by
default, but can be enabled by using `--enable-experimental-asm` when building.
- Refill of the keypool no longer flushes the wallet between each key which resulted in a ~20x speedup in creating a new wallet. Part of this speedup was used to increase the default keypool to 1000 keys to make recovery more robust. (See [PR 10831](https://github.com/bitcoin/bitcoin/pull/10831)).

Miner block size removed
------------------------

The `-blockmaxsize` option for miners to limit their blocks' sizes was
deprecated in Bitcoin version 0.15.1, and has now been removed. Miners should use the
`-blockmaxweight` option if they want to limit the weight of their blocks'
weights.

Multi-wallet support
--------------------

Chaincoin Core now supports loading multiple, separate wallets (See [PR 8694](https://github.com/bitcoin/bitcoin/pull/8694), [PR 10849](https://github.com/bitcoin/bitcoin/pull/10849)). The wallets are completely separated, with individual balances, keys and received transactions.

Multi-wallet is enabled by using more than one `-wallet` argument when starting Chaincoin, either on the command line or in the Chaincoin config file.

**In Chaincoin-Qt, only the first wallet will be displayed and accessible for creating and signing transactions.** GUI selectable multiple wallets will be supported in a future version. However, even in 0.15 other loaded wallets will remain synchronized to the node's current tip in the background. This can be useful if running a pruned node, since loading a wallet where the most recent sync is beyond the pruned height results in having to download and revalidate the whole blockchain. Continuing to synchronize all wallets in the background avoids this problem.

Chaincoin Core 0.16.1 contains the following changes to the RPC interface and `chaincoin-cli` for multi-wallet:

* When running Chaincoin Core with a single wallet, there are **no** changes to the RPC interface or `chaincoin-cli`. All RPC calls and `chaincoin-cli` commands continue to work as before.
* When running Chaincoin Core with multi-wallet, all *node-level* RPC methods continue to work as before. HTTP RPC requests should be send to the normal `<RPC IP address>:<RPC port>` endpoint, and `chaincoin-cli` commands should be run as before. A *node-level* RPC method is any method which does not require access to the wallet.
* When running Chaincoin Core with multi-wallet, *wallet-level* RPC methods must specify the wallet for which they're intended in every request. HTTP RPC requests should be send to the `<RPC IP address>:<RPC port>/wallet/<wallet name>` endpoint, for example `127.0.0.1:8332/wallet/wallet1.dat`. `chaincoin-cli` commands should be run with a `-rpcwallet` option, for example `chaincoin-cli -rpcwallet=wallet1.dat getbalance`.
* A new *node-level* `listwallets` RPC method is added to display which wallets are currently loaded. The names returned by this method are the same as those used in the HTTP endpoint and for the `rpcwallet` argument.

Note that while multi-wallet is now fully supported, the RPC multi-wallet interface should be considered unstable for version 0.15.0, and there may backwards-incompatible changes in future versions.

Replace-by-fee control in the GUI
---------------------------------

Chaincoin Core has supported creating opt-in replace-by-fee (RBF) transactions
since version 0.12.0, and since version 0.14.0 has included a `bumpfee` RPC method to
replace unconfirmed opt-in RBF transactions with a new transaction that pays
a higher fee.

In version 0.15, creating an opt-in RBF transaction and replacing the unconfirmed
transaction with a higher-fee transaction are both supported in the GUI (See [PR 9592](https://github.com/bitcoin/bitcoin/pull/9592)).

Removal of Coin Age Priority
----------------------------

In previous versions of Chaincoin Core, a portion of each block could be reserved for transactions based on the age and value of UTXOs they spent. This concept (Coin Age Priority) is a policy choice by miners, and there are no consensus rules around the inclusion of Coin Age Priority transactions in blocks. In practice, only a few miners continue to use Coin Age Priority for transaction selection in blocks. Chaincoin Core 0.16.1 removes all remaining support for Coin Age Priority (See [PR 9602](https://github.com/bitcoin/bitcoin/pull/9602)). This has the following implications:

- The concept of *free transactions* has been removed. High Coin Age Priority transactions would previously be allowed to be relayed even if they didn't attach a miner fee. This is no longer possible since there is no concept of Coin Age Priority. The `-limitfreerelay` and `-relaypriority` options which controlled relay of free transactions have therefore been removed.
- The `-sendfreetransactions` option has been removed, since almost all miners do not include transactions which do not attach a transaction fee.
- The `-blockprioritysize` option has been removed.
- The `estimatepriority` and `estimatesmartpriority` RPCs have been removed.
- The `getmempoolancestors`, `getmempooldescendants`, `getmempoolentry` and `getrawmempool` RPCs no longer return `startingpriority` and `currentpriority`.
- The `prioritisetransaction` RPC no longer takes a `priority_delta` argument, which is replaced by a `dummy` argument for backwards compatibility with clients using positional arguments. The RPC is still used to change the apparent fee-rate of the transaction by using the `fee_delta` argument.
- `-minrelaytxfee` can now be set to 0. If `minrelaytxfee` is set, then fees smaller than `minrelaytxfee` (per kB) are rejected from relaying, mining and transaction creation. This defaults to 1000 satoshi/kB.
- The `-printpriority` option has been updated to only output the fee rate and hash of transactions included in a block by the mining code.

Mempool Persistence Across Restarts
-----------------------------------

Version 0.14 introduced mempool persistence across restarts (the mempool is saved to a `mempool.dat` file in the data directory prior to shutdown and restores the mempool when the node is restarted). Version 0.15 allows this feature to be switched on or off using the `-persistmempool` command-line option (See [PR 9966](https://github.com/bitcoin/bitcoin/pull/9966)). By default, the option is set to true, and the mempool is saved on shutdown and reloaded on startup. If set to false, the `mempool.dat` file will not be loaded on startup or saved on shutdown.

New RPC methods
---------------

Version 0.16 introduces several new RPC methods:

- `abortrescan` stops current wallet rescan, e.g. when triggered by an `importprivkey` call (See [PR 10208](https://github.com/bitcoin/bitcoin/pull/10208)).
- `combinerawtransaction` accepts a JSON array of raw transactions and combines them into a single raw transaction (See [PR 10571](https://github.com/bitcoin/bitcoin/pull/10571)).
- `estimaterawfee` returns raw fee data so that customized logic can be implemented to analyze the data and calculate estimates. See [Fee Estimation Improvements](#fee-estimation-improvements) for full details on changes to the fee estimation logic and interface.
- `getchaintxstats` returns statistics about the total number and rate of transactions
in the chain (See [PR 9733](https://github.com/bitcoin/bitcoin/pull/9733)).
- `listwallets` lists wallets which are currently loaded. See the *Multi-wallet* section
of these release notes for full details (See [Multi-wallet support](#multi-wallet-support)).
- `uptime` returns the total runtime of the `chaincoind` server since its last start (See [PR 10400](https://github.com/bitcoin/bitcoin/pull/10400)).

Low-level RPC changes
---------------------

- When using Chaincoin Core in multi-wallet mode, RPC requests for wallet methods must specify
the wallet that they're intended for. See [Multi-wallet support](#multi-wallet-support) for full details.

- The new database model no longer stores information about transaction
versions of unspent outputs (See [Performance improvements](#performance-improvements)). This means that:
- The `gettxout` RPC no longer has a `version` field in the response.
- The `gettxoutsetinfo` RPC reports `hash_serialized_2` instead of `hash_serialized`,
which does not commit to the transaction versions of unspent outputs, but does
commit to the height and coinbase information.
- The `getutxos` REST path no longer reports the `txvers` field in JSON format,
and always reports 0 for transaction versions in the binary format

- The `estimatefee` RPC is deprecated. Clients should switch to using the `estimatesmartfee` RPC, which returns better fee estimates. See [Fee Estimation Improvements](#fee-estimation-improvements) for full details on changes to the fee estimation logic and interface.

- The `gettxoutsetinfo` response now contains `disk_size` and `bogosize` instead of
`bytes_serialized`. The first is a more accurate estimate of actual disk usage, but
is not deterministic. The second is unrelated to disk usage, but is a
database-independent metric of UTXO set size: it counts every UTXO entry as 50 + the
length of its scriptPubKey (See [PR 10426](https://github.com/bitcoin/bitcoin/pull/10426)).

- `signrawtransaction` can no longer be used to combine multiple transactions into a single transaction. Instead, use the new `combinerawtransaction` RPC (See [PR 10571](https://github.com/bitcoin/bitcoin/pull/10571)).

- `fundrawtransaction` no longer accepts a `reserveChangeKey` option. This option used to allow RPC users to fund a raw transaction using an key from the keypool for the change address without removing it from the available keys in the keypool. The key could then be re-used for a `getnewaddress` call, which could potentially result in confusing or dangerous behaviour (See [PR 10784](https://github.com/bitcoin/bitcoin/pull/10784)).

- `estimatepriority` and `estimatesmartpriority` have been removed. See [Removal of Coin Age Priority](#removal-of-coin-age-priority).

- The `listunspent` RPC now takes a `query_options` argument (see [PR 8952](https://github.com/bitcoin/bitcoin/pull/8952)), which is a JSON object
containing one or more of the following members:
- `minimumAmount` - a number specifying the minimum value of each UTXO
- `maximumAmount` - a number specifying the maximum value of each UTXO
- `maximumCount` - a number specifying the minimum number of UTXOs
- `minimumSumAmount` - a number specifying the minimum sum value of all UTXOs

- The `getmempoolancestors`, `getmempooldescendants`, `getmempoolentry` and `getrawmempool` RPCs no longer return `startingpriority` and `currentpriority`. See [Removal of Coin Age Priority](#removal-of-coin-age-priority).

- The `dumpwallet` RPC now returns the full absolute path to the dumped wallet. It
used to return no value, even if successful (See [PR 9740](https://github.com/bitcoin/bitcoin/pull/9740)).

- In the `getpeerinfo` RPC, the return object for each peer now returns an `addrbind` member, which contains the ip address and port of the connection to the peer. This is in addition to the `addrlocal` member which contains the ip address and port of the local node as reported by the peer (See [PR 10478](https://github.com/bitcoin/bitcoin/pull/10478)).

- The `disconnectnode` RPC can now disconnect a node specified by node ID (as well as by IP address/port). To disconnect a node based on node ID, call the RPC with the new `nodeid` argument (See [PR 10143](https://github.com/bitcoin/bitcoin/pull/10143)).

- The second argument in `prioritisetransaction` has been renamed from `priority_delta` to `dummy` since Bitcoin Core no longer has a concept of coin age priority. The `dummy` argument has no functional effect, but is retained for positional argument compatibility. See [Removal of Coin Age Priority](#removal-of-coin-age-priority).

- The `resendwallettransactions` RPC throws an error if the `-walletbroadcast` option is set to false (See [PR 10995](https://github.com/bitcoin/bitcoin/pull/10995)).

- The second argument in the `submitblock` RPC argument has been renamed from `parameters` to `dummy`. This argument never had any effect, and the renaming is simply to communicate this fact to the user (See [PR 10191](https://github.com/bitcoin/bitcoin/pull/10191))
(Clients should, however, use positional arguments for `submitblock` in order to be compatible with BIP 22.)

- The `verbose` argument of `getblock` has been renamed to `verbosity` and now takes an integer from 0 to 2. Verbose level 0 is equivalent to `verbose=false`. Verbose level 1 is equivalent to `verbose=true`. Verbose level 2 will give the full transaction details of each transaction in the output as given by `getrawtransaction`. The old behavior of using the `verbose` named argument and a boolean value is still maintained for compatibility.

- Error codes have been updated to be more accurate for the following error cases (See [PR 9853](https://github.com/bitcoin/bitcoin/pull/9853)):
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
- `removeprunedfunds` now returns RPC_WALLET_ERROR if `chaincoind` is unable to remove
the transaction. Previously returned RPC_INTERNAL_ERROR.
- `removeprunedfunds` now returns RPC_INVALID_PARAMETER if the transaction does not
exist in the wallet. Previously returned RPC_INTERNAL_ERROR.
- `fundrawtransaction` now returns RPC_INVALID_ADDRESS_OR_KEY if an invalid change
address is provided. Previously returned RPC_INVALID_PARAMETER.
- `fundrawtransaction` now returns RPC_WALLET_ERROR if `chaincoind` is unable to create
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

Compact Block support (BIP 152)
-------------------------------

Support for block relay using the Compact Blocks protocol has been implemented
in PR 8068.

The primary goal is reducing the bandwidth spikes at relay time, though in many
cases it also reduces propagation delay. It is automatically enabled between
compatible peers.
[BIP 152](https://github.com/bitcoin/bips/blob/master/bip-0152.mediawiki)

As a side-effect, ordinary non-mining nodes will download and upload blocks
faster if those blocks were produced by miners using similar transaction
filtering policies. This means that a miner who produces a block with many
transactions discouraged by your node will be relayed slower than one with
only transactions already in your memory pool. The overall effect of such
relay differences on the network may result in blocks which include widely-
discouraged transactions losing a stale block race, and therefore miners may
wish to configure their node to take common relay policies into consideration.


Hierarchical Deterministic Key Generation
-----------------------------------------
Newly created wallets will use hierarchical deterministic key generation
according to BIP32 (keypath m/0'/0'/k').
Existing wallets will still use traditional key generation.

Backups of HD wallets, regardless of when they have been created, can
therefore be used to re-generate all possible private keys, even the
ones which haven't already been generated during the time of the backup.
**Attention:** Encrypting the wallet will create a new seed which requires
a new backup!

Wallet dumps (created using the `dumpwallet` RPC) will contain the deterministic
seed. This is expected to allow future versions to import the seed and all
associated funds, but this is not yet implemented.

HD key generation for new wallets can be disabled by `-usehd=0`. Keep in
mind that this flag only has affect on newly created wallets.
You can't disable HD key generation once you have created a HD wallet.

There is no distinction between internal (change) and external keys.

HD wallets are incompatible with older versions of Bitcoin Core.

[Pull request](https://github.com/bitcoin/bitcoin/pull/8035/files), [BIP 32](https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki)


Segregated Witness
------------------

The code preparations for Segregated Witness ("segwit"), as described in [BIP
141](https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki), [BIP
143](https://github.com/bitcoin/bips/blob/master/bip-0143.mediawiki), [BIP
144](https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki), and [BIP
145](https://github.com/bitcoin/bips/blob/master/bip-0145.mediawiki) are
finished and included in this release.  However, BIP 141 does not yet specify
activation parameters on mainnet, and so this release does not support segwit
use on mainnet.  Testnet use is supported, and after BIP 141 is updated with
proposed parameters, a future release of Bitcoin Core is expected that
implements those parameters for mainnet.

Furthermore, because segwit activation is not yet specified for mainnet,
version 0.13.0 will behave similarly as other pre-segwit releases even after a
future activation of BIP 141 on the network.  Upgrading from 0.13.0 will be
required in order to utilize segwit-related features on mainnet (such as signal
BIP 141 activation, mine segwit blocks, fully validate segwit blocks, relay
segwit blocks to other segwit nodes, and use segwit transactions in the
wallet, etc).

Removal of internal miner
--------------------------

As CPU mining has been useless for a long time, the internal miner has been
removed in this release, and replaced with a simpler implementation for the
test framework.

The overall result of this is that `setgenerate` RPC call has been removed, as
well as the `-gen` and `-genproclimit` command-line options.

For testing, the `generate` call can still be used to mine a block, and a new
RPC call `generatetoaddress` has been added to mine to a specific address. This
works with wallet disabled.

Governance system improvements
------------------------------

We do not use watchdogs since 0.16.x, instead we include all required information about sentinel into masternode
pings. With this update we add some additional information and cover everything with a signature to ensure that
masternode ping wasn't maleated by some intermediary node. All messages and logic related to watchdogs are
completely removed now. We also improved proposal message format, as well as proposal validation and processing,
which should lower network traffic and CPU usage. Handling of triggers was also improved slightly.


PrivateSend improvements
------------------------

PrivateSend collaterals are no longer required to be N times of the PrivateSend fee (PSF), instead any input
which is greater or equal 1 PSF but less or equal 4 PSF can be used as a collateral. Inputs that are greater or
equal 1 PSF but strictly less than 2 PSF will be used in collaterals with OP_RETURN outputs. Note that such
inputs will be consumed completely, with no change outputs at all. This should lower number of inputs wallet
would need to take care of, improve privacy by eliminating the case where user accidentally merge small
non-private inputs together and also decrease global UTXO set size.

It might feel that thanks to this change mixing fees are going to be slightly higher on average if have lots of
such small inputs. However, you need to keep in mind that creating new PrivateSend collaterals cost some fee too
and since such small inputs were not used at all you'd need more txes to create such collaterals. So in general,
we believe average mixing fees should stay mostly the same.

Support for pruned nodes in Lite Mode
-------------------------------------

It is now possible to run a pruned node which stores only some recent blocks and not the whole blockchain.
However this option is only available in so called Lite Mode. In this mode, Chaincoin specific features are disabled, meaning
that such nodes won't fully validate the blockchain (masternode payments and superblocks).
PrivateSend and InstantSend functions are also disabled on such nodes. Such nodes are comparable to SPV-like nodes
in terms of security and validation - it relies a lot on surrounding nodes, so please keep this in mind if you decide to
use it for something.
There are also some other minor fixes which should also slightly improve mixing process.

Removal of support for local masternodes
----------------------------------------

Keeping a wallet with 1000 CHC unlocked for 24/7 is definitely not a good idea
anymore. Because of this fact, it's also no longer reasonable to update and test
this feature, so it's completely removed now. If for some reason you were still
using it, please follow one of the guides and setup a remote masternode instead.

New Masternode Information Dialog
---------------------------------

You can now double-click on your masternode in `My Masternodes` list on `Masternodes` tab to reveal the new
Masternode Information dialog. It will show you some basic information as well as software versions reported by the
masternode. There is also a QR code now which encodes corresponding masternode private key (the one you set with
mnprivkey during MN setup and NOT the one that controls the 1000 CHC collateral) which should make the process of pairing with
mobile software allowing you to vote with your masternode a bit easier (this software is still in development).

Removal of SPORKs
---------------------------------

Chaincoin 0.9 was controlled by a central entity holding private keys to control the entire network. This has been concluded unwanted by the community and there for has been removed. All forks are depending on block validation from now on.

Removal of InstantSend
---------------------------------

Instant send is a functionality ported from DASH distributing a copy of desired txids to Masternodes which whill guard them to prevent a change. This was the way, the traceability of transactions has been ensured. Since Chaincoin is introducing Segregated Witness with Chaincoin Core 0.16.1, transaction malleability has been fixed and Instant Send became obsolete.

0.16.1 change log
------------------

### Policy
- #11423 `d353dd1` [Policy] Several transaction standardness rules (jl2012)

### Mining
- #12756 `e802c22` [config] Remove blockmaxsize option (jnewbery)

### Block and transaction handling
- #13199 `c71e535` Bugfix: ensure consistency of m_failed_blocks after reconsiderblock (sdaftuar)
- #13023 `bb79aaf` Fix some concurrency issues in ActivateBestChain() (skeees)

### P2P protocol and network code
- #12626 `f60e84d` Limit the number of IPs addrman learns from each DNS seeder (EthanHeilman)

### Wallet
- #13265 `5d8de76` Exit SyncMetaData if there are no transactions to sync (laanwj)
- #13030 `5ff571e` Fix zapwallettxes/multiwallet interaction. (jnewbery)

### GUI
- #12999 `1720eb3` Show the Window when double clicking the taskbar icon (ken2812221)
- #12650 `f118a7a` Fix issue: "default port not shown correctly in settings dialog" (251Labs)
- #13251 `ea487f9` Rephrase Bech32 checkbox texts, and enable it with legacy address default (fanquake)

### Build system
- #12474 `b0f692f` Allow depends system to support armv7l (hkjn)
- #12585 `72a3290` depends: Switch to downloading expat from GitHub (fanquake)
- #12648 `46ca8f3` test: Update trusted git root (MarcoFalke)
- #11995 `686cb86` depends: Fix Qt build with Xcode 9 (fanquake)
- #12636 `845838c` backport: #11995 Fix Qt build with Xcode 9 (fanquake)
- #12946 `e055bc0` depends: Fix Qt build with XCode 9.3 (fanquake)
- #12998 `7847b92` Default to defining endian-conversion DECLs in compat w/o config (TheBlueMatt)

### Tests and QA
- #12447 `01f931b` Add missing signal.h header (laanwj)
- #12545 `1286f3e` Use wait_until to ensure ping goes out (Empact)
- #12804 `4bdb0ce` Fix intermittent rpc_net.py failure. (jnewbery)
- #12553 `0e98f96` Prefer wait_until over polling with time.sleep (Empact)
- #12486 `cfebd40` Round target fee to 8 decimals in assert_fee_amount (kallewoof)
- #12843 `df38b13` Test starting chaincoind with -h and -version (jnewbery)
- #12475 `41c29f6` Fix python TypeError in script.py (MarcoFalke)
- #12638 `0a76ed2` Cache only chain and wallet for regtest datadir (MarcoFalke)
- #12902 `7460945` Handle potential cookie race when starting node (sdaftuar)
- #12904 `6c26df0` Ensure chaincoind processes are cleaned up when tests end (sdaftuar)
- #13049 `9ea62a3` Backports (MarcoFalke)
- #13201 `b8aacd6` Handle disconnect_node race (sdaftuar)

### Miscellaneous
- #12518 `a17fecf` Bump leveldb subtree (MarcoFalke)
- #12442 `f3b8d85` devtools: Exclude patches from lint-whitespace (MarcoFalke)
- #12988 `acdf433` Hold cs_main while calling UpdatedBlockTip() signal (skeees)
- #12985 `0684cf9` Windows: Avoid launching as admin when NSIS installer ends. (JeremyRand)

### Documentation
- #12637 `60086dd` backport: #12556 fix version typo in getpeerinfo RPC call help (fanquake)
- #13184 `4087dd0` RPC Docs: `gettxout*`: clarify bestblock and unspent counts (harding)
- #13246 `6de7543` Bump to Ubuntu Bionic 18.04 in build-windows.md (ken2812221)
- #12556 `e730b82` Fix version typo in getpeerinfo RPC call help (tamasblummer)

RPC changes
------------

### Low-level changes

- The `createrawtransaction` RPC will now accept an array or dictionary (kept for compatibility) for the `outputs` parameter. This means the order of transaction outputs can be specified by the client.
- The `fundrawtransaction` RPC will reject the previously deprecated `reserveChangeKey` option.
- `sendmany` now shuffles outputs to improve privacy, so any previously expected behavior with regards to output ordering can no longer be relied upon.
- The new RPC `testmempoolaccept` can be used to test acceptance of a transaction to the mempool without adding it.
- JSON transaction decomposition now includes a `weight` field which provides
  the transaction's exact weight. This is included in REST /rest/tx/ and
  /rest/block/ endpoints when in json mode. This is also included in `getblock`
  (with verbosity=2), `listsinceblock`, `listtransactions`, and
  `getrawtransaction` RPC commands.

External wallet files
---------------------

The `-wallet=<path>` option now accepts full paths instead of requiring wallets
to be located in the -walletdir directory.

Newly created wallet format
---------------------------

If `-wallet=<path>` is specified with a path that does not exist, it will now
create a wallet directory at the specified location (containing a wallet.dat
data file, a db.log file, and database/log.?????????? files) instead of just
creating a data file at the path and storing log files in the parent
directory. This should make backing up wallets more straightforward than
before because the specified wallet path can just be directly archived without
having to look in the parent directory for transaction log files.

For backwards compatibility, wallet paths that are names of existing data files
in the `-walletdir` directory will continue to be accepted and interpreted the
same as before.

Low-level RPC changes
---------------------

- When bitcoin is not started with any `-wallet=<path>` options, the name of
  the default wallet returned by `getwalletinfo` and `listwallets` RPCs is
  now the empty string `""` instead of `"wallet.dat"`. If bitcoin is started
  with any `-wallet=<path>` options, there is no change in behavior, and the
  name of any wallet is just its `<path>` string.

### Logging

- The log timestamp format is now ISO 8601 (e.g. "2018-02-28T12:34:56Z").

Miner block size removed

The `-blockmaxsize` option for miners to limit their blocks' sizes was
deprecated in V0.15.1, and has now been removed. Miners should use the
`-blockmaxweight` option if they want to limit the weight of their blocks'
weights.

Python Support
--------------

Support for Python 2 has been discontinued for all test files and tools.

Credits
=======

Thanks to everyone who directly contributed to this release:


- MarcoFalke
- Matt Corallo
- Pieter Wuille



- 251
- ロハン ダル
- Ahmad Kazi
- aideca
- Akio Nakamura
- Alex Morcos
- Allan Doensen
- Andres G. Aragoneses
- Andrew Chow
- Angel Leon
- Awemany
- Ben Woosley
- Bob McElrath
- boostdmg01
- Brian McMichael
- BtcDrak
- Charlie Lee
- Chris Gavin
- Chris Stewart
- Chun Kuan Lee
- Cory Fields
- CryptAxe
- Dag Robole
- Daniel Aleksandersen
- Daniel Cousens
- darksh1ne
- David A. Harding
- Dimitris Tsapakidis
- e0
- Henrik Jonsson
- Eric Shaw
- Evan Klitzke
- fanquake
- Felix Weis
- flack
- Guido Vranken
- Greg Griffith
- Gregory Maxwell
- Gregory Sanders
- Ian Kelling
- Jack Grigg
- James Evans
- James Hilliard
- Jameson Lopp
- Jeremy Rubin
- JeremyRand
- Jesse Cohen
- Jimmy Song
- João Barbosa
- Johnathan Corgan
- John Newbery
- Johnson Lau
- Jonas Schnelli
- Jorge Timón
- Karl-Johan Alm
- kewde
- KibbledJiveElkZoo
- Kirit Thadaka
- kobake
- Kyle Honeycutt
- Lawrence Nahum
- lubuzzo
- Luke Dashjr
- Marco Falke
- Marcos Mayorga
- Marijn Stollenga
- Mario Dian
- Mark Friedenbach
- Marko Bencun
- Masahiko Hyuga
- Matt Corallo
- Matthew Zipkin
- Matthias Grundmann
- Michael Goldstein
- Michael Polzer
- Michael Rotarius
- Mikerah
- Mike van Rossum
- Mitchell Cash
- Nicolas Dorier
- Patrick Strateman
- Pavel Janík
- Pavlos Antoniou
- Pavol Rusnak
- Pedro Branco
- Peter Todd
- Pieter Wuille
- practicalswift
- René Nyffenegger
- Ricardo Velhote
- romanornr
- Russell Yanofsky
- Rusty Russell
- Ryan Havar
- seekthehex
- shaolinfry
- Shigeya Suzuki
- Simone Madeo
- Spencer Lievens
- Steven D. Lander
- Suhas Daftuar
- Takashi Mitsuta
- Tamas Blummer
- Thomas Snider
- Timothy Redaelli
- tintinweb
- tnaka
- Warren Togami
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/chaincoin/).
