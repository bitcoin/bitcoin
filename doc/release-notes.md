Dash Core version 0.13.0.0
==========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new major version release, bringing new features, various bugfixes and other improvements.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation you will
have to reindex (start with -reindex-chainstate or -reindex) to make sure
your wallet has all the new data synced.

Downgrade warning
-----------------

### Downgrade to a version < 0.13.0.0

Downgrading to a version smaller than 0.13 is only supported as long as DIP2/DIP3
has not been activated. Activation will happen when enough miners signal compatibility
through a BIP9 (bit 3) deployment.

Notable changes
===============

DIP0002 - Special Transactions
------------------------------
Currently, new features and consensus mechanisms have to be implemented on top of the restrictions
imposed by the simple nature of transactions. Since classical transactions can only carry inputs
and outputs, they are most useful for financial transactions (i.e. transfers of quantities of Dash
between addresses on the distributed ledger). These inputs and outputs carry scripts and signatures
which are used to authorize and validate the transaction.

To implement new on-chain features and consensus mechanisms which do not fit into this concept of
financial transactions, it is often necessary to misuse the inputs/outputs and their corresponding
scripts to add additional data and meaning to a transaction. For example, new opcodes would have
to be introduced to mark a transaction as something special and add a payload. In other cases,
OP_RETURN has been misused to store data on-chain.

The introduction of special transactions will require the whole Dash ecosystem to perform a one-time
mandatory update of all the software and libraries involved. Software and libraries will have to be
changed so that they can differentiate between classical transactions and special transactions.
Deserialization of a classical transaction remains unchanged. Deserialization of a special transaction
requires the software/library to at least implement skipping and ignoring the extra_payload field.
Validation and processing of the inputs and outputs of a special transaction must remain identical to
classical transactions.

Read more: https://github.com/dashpay/dips/blob/master/dip-0002.md

DIP0003 - Deterministic Masternode Lists
----------------------------------------
This DIP provides on-chain consensus for masternode lists that in turn allow for deterministic quorum
derivation and service scoring of masternode rewards.

In the previous system, each node maintained its own individual masternode list. Masternodes gained
entry to that masternode list after the owner created a 1000 Dash UTXO and the masternode broadcast
a "masternode broadcast/announcement" P2P message. This in turn set the masternode to a PRE_ENABLED
state in the list maintained by each node. Masternodes then regularly broadcasted ping messages to
keep the masternode in ENABLED state.

The previous system was maintained with consensus mechanisms that predated Satoshi Nakamoto’s solution
to the Byzantine Generals Problem. This meant that each node needed to maintain their own individual
masternode list with P2P messages and not a blockchain based solution. Due to the nature of the P2P
system, there was no guarantee that nodes would come to the same conclusion on what the masternode
list ought to look like. Discrepancies might, for example, occur due to a different order of message
reception or if messages had not been received at all. This posed some risks in regard to consensus
and limited the possible uses of quorums by the system.

Additionally, the previous system required a complicated and failure prone "masternode sync" after
the initial startup of the node. After the blockchain was synced, the node would request the current
masternode list, the reward payment votes, and then verify the received list. This process tended to
take an unnecessarily long amount of time and sometimes resulted in failure.

In the new system, the masternode list is derived entirely from information found on-chain. New
masternodes are added by new special transactions called Provider Registration Transactions
(abbreviated as ProRegTx). They are only removed by spending the collateral. A ProRegTx is a special
transaction which includes either a 1000-Dash collateral payment or a reference to it, along with
other payload information (DIP0002).

The new system is going to be activated via combination of a BIP9-like deployment (bit 3) and new spork
(`SPORK_15_DETERMINISTIC_MNS_ENABLED`).

Read more: https://github.com/dashpay/dips/blob/master/dip-0003.md
Upgrade instructions: https://docs.dash.org/DIP3-masternode-upgrade

DIP0004 - Simplified Verification of Deterministic Masternode Lists
-------------------------------------------------------------------
A verifiable and correct masternode list is foundational to many Dash features, including verification
of an InstantSend transaction, mixing in PrivateSend and many features of Evolution. The deterministic
masternode lists introduced by DIP0003 enable full derivation and verification of a masternode list via
on-chain data. This, however, requires the full chain to be available to construct or verify this list.
A SPV client does not have the full chain and thus would have to rely on a list provided by one or more
nodes in the network. This provided list must be verifiable by the SPV client without needing the full
chain. This DIP proposes additions to the block’s coinbase transaction and new P2P messages to get and
update a masternode list with additional proof data.

Read more: https://github.com/dashpay/dips/blob/master/dip-0004.md

Mining
------
Please note that masternode payments in `getblocktemplate` rpc are now returned as an array and not as
a single object anymore. Make sure to apply corresponding changes to your pool software.

Also, deterministic masternodes can now set their payout address to a P2SH address. The most common use
case for P2SH is multisig but script can be pretty much anything. If your pool software doesn't recognize
P2SH addresses, the simplest way to fix it is to use `script` field which shows scriptPubKey for each
entry of masternode payments array in `getblocktemplate`.

And finally, after DIP0003 activation your pool software must be able to produce Coinbase Special
Transaction https://github.com/dashpay/dips/blob/master/dip-0004.md#coinbase-special-transaction.
Use `coinbase_payload` from `getblocktemplate` to get extra payload needed to construct this transaction.

PrivateSend
-----------
With further refactoring of PrivateSend code it became possible to implement mixing in few parallel
mixing sessions at once from one single wallet. You can set number of mixing sessions via
`privatesendsessions` cmd-line option or dash.conf. You can pick any number of sessions between 1 and 10,
default is 4 which should be good enough for most users. For this feature to work you should also make
sure that `privatesendmultisession` is set to `1` via cmd-line or `Enable PrivateSend multi-session` is
enabled in GUI.

Introducing parallel mixing sessions should speed mixing up which makes it reasonable to add a new
mixing denom (0.00100001 DASH) now while keeping all the old ones too. It also makes sense to allow more
mixing rounds now, so the new default number of rounds is 4 and the maximum number of rounds is 16 now.

You can also adjust rounds and amount via `setprivatesendrounds` and `setprivatesendamount` RPC commands
which override corresponding cmd-line params (`privatesendrounds` and `privatesendamount` respectively).

NOTE: Introducing the new denom and a couple of other changes made it incompatible with mixing on
masternodes running on pre-0.13 software. Please keep using 0.12.3 local wallet to mix your coins until
there is some significant number of masternodes running on version 0.13 to make sure you have enough
masternodes to choose from when the wallet picks one to mix funds on.

InstantSend
-----------
With further improvements of networking code it's now possible to handle more load, so we are changing
InstantSend to be always-on for so called "simple txes" - transactions with 4 or less inputs. Such
transactions will be automatically locked even if they only pay minimal fee. According to stats, this
means that up to 90% of currently observed transactions will became automatically locked via InstantSend
with no additional cost to end users or any additional effort from wallet developers or other service
providers.

This feature is going to be activated via combination of a BIP9-like deployment (we are reusing bit 3)
and new spork (`SPORK_16_INSTANTSEND_AUTOLOCKS`).

Historically, InstantSend transactions were shown in GUI and RPC with more confirmations than regular ones,
which caused quite a bit of confusion. This will no longer be the case, instead we are going to show real
blockchain confirmations only and a separate indicator to show if transaction was locked via InstantSend
or not. For GUI it's color highlight and a new column, for RPC commands - `instantlock` field and `addlocked`
param.

One of the issues with InstantSend adoption by SPV wallets (besides lack of Deterministic Masternode List)
was inability to filter all InstantSend messages the same way transactions are filtered. This should be
fixed now and SPV wallets should only get lock votes for transactions they are interested in.

Another popular request was to preserve information about InstantSend locks between wallet restarts, which
is now implemented. This data is stored in a new cache file `instantsend.dat`. You can safely remove it,
if you don't need information about recent transaction locks for some reason (NOTE: make sure it's not one
of your wallets!).

We also added new ZMQ notifications for double-spend attempts which try to override transactions locked
via InstantSend - `zmqpubrawinstantsenddoublespend` and `zmqpubhashinstantsenddoublespend`.

Sporks
------
There are a couple of new sporks introduced in this version `SPORK_15_DETERMINISTIC_MNS_ENABLED` (block
based) and `SPORK_16_INSTANTSEND_AUTOLOCKS` (timestamp based). There is aslo `SPORK_17_QUORUM_DKG_ENABLED`
(timestamp based) which is going to be used on testnet only for now.

Spork data is stored in a new cache file (`sporks.dat`) now.

Governance
----------
Introduction of Deterministic Masternodes requires replacing of the old masternode private key which was used
both for operating a MN and for voting on proposals with a set of separate keys, preferably fresh new ones.
This means that votes casted for proposals by Masternode Owners via the old system will no longer be valid
after DIP0003 activation and must be re-casted using the new voting key.

Also, you can now get notifications about governance objects or votes via new ZMQ notifications:
`zmqpubhashgovernancevote`, `zmqpubhashgovernanceobject`, `zmqpubrawgovernancevote` and
`zmqpubhashgovernanceobject`.

GUI changes
-----------
Masternodes tab has a new section dedicated to DIP0003 registered masternodes now. After DIP0003 activation
this will be the only section shown here, the two old sections for non-deterministic masternodes will no
longer be available.

There are changes in the way InstantSend transactions are displayed, see `InstantSend` section above.

Some other (mostly minor) issues were also fixed, see `GUI` part of `0.13.0.0 Change log` section below for
detailed list of fixes.

RPC changes
-----------
There are a few changes in existing RPC interfaces in this release:
- `gobject prepare` allows to send proposal transaction as an InstantSend one and also accepts an UTXO reference to spend;
- `masternode status` and `masternode list` show some DIP0003 related info now;
- `previousbits` and `coinbase_payload` fields were added in `getblocktemplate`;
- `getblocktemplate` now returns an array for masternode payments instead of a single object (miners and mining pools have to upgrade their software to support multiple masternode payees);
- masternode and superblock payments in `getblocktemplate` show payee scriptPubKey in `script` field in addition to payee address in `payee`;
- `getblockchaininfo` shows BIP9 deployment progress;
- `help command subCommand` should give detailed help for subcommands e.g. `help protx list`;
- `compressed` option in `masternode genkey`;
- `dumpwallet` shows info about dumped wallet and warns user about security issues;
- `instantlock` field added in output of `getrawmempool`, `getmempoolancestors`, `getmempooldescendants`, `getmempoolentry`,
`getrawtransaction`, `decoderawtransaction`, `gettransaction`, `listtransactions`, `listsinceblock`;
- `addlocked` param added to `getreceivedbyaddress`, `getreceivedbyaccount`, `getbalance`, `sendfrom`, `sendmany`,
`listreceivedbyaddress`, `listreceivedbyaccount`, `listaccounts`.

There are also new RPC commands:
- `protx` (`list`, `info`, `diff`, `register`, `register_fund`, `register_prepare`,
`register_submit`, `update_service`, `update_registrar`, `revoke`);
- `bls generate`;
- `setprivatesendrounds`;
- `setprivatesendamount`.

See `help command` in rpc for more info.

Command-line options
--------------------

New cmd-line options:
- `masternodeblsprivkey`;
- `minsporkkeys`;
- `privatesendsessions`;
- `zmqpubrawinstantsenddoublespend`;
- `zmqpubhashinstantsenddoublespend`;
- `zmqpubhashgovernancevote`;
- `zmqpubhashgovernanceobject`;
- `zmqpubrawgovernancevote`;
- `zmqpubhashgovernanceobject`.

Some of them are Devnet only:
- `budgetparams`;
- `minimumdifficultyblocks`;
- `highsubsidyblocks`;
- `highsubsidyfactor`.

Few cmd-line options are no longer supported:
- `instantsenddepth`;
- `mempoolreplacement`.

See `Help -> Command-line options` in Qt wallet or `dashd --help` for more info.

Lots of refactoring and bug fixes
---------------------------------

A lot of refactoring, code cleanups and other small fixes were done in this release.

0.13.0.0 Change log
===================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v0.12.3.4...dashpay:v0.13.0.0).

### Network
- [`03a6865d9`](https://github.com/dashpay/dash/commit/03a6865d9) Enforce correct port on mainnet for DIP3 MNs (#2576)
- [`3f26ed78c`](https://github.com/dashpay/dash/commit/3f26ed78c) Backport network checks missing in CActiveDeterministicMasternodeManager::Init() (#2572)
- [`7c7500864`](https://github.com/dashpay/dash/commit/7c7500864) Also stop asking other peers for a TX when ProcessTxLockRequest fails (#2529)
- [`19a6f718d`](https://github.com/dashpay/dash/commit/19a6f718d) Don't respond with getdata for legacy inv types when spork15 is active (#2528)
- [`22dcec71a`](https://github.com/dashpay/dash/commit/22dcec71a) Punish nodes which keep requesting and then rejecting blocks (#2518)
- [`a18ca49a2`](https://github.com/dashpay/dash/commit/a18ca49a2) Disconnect peers with version < 70212 after DIP3 activation via BIP9 (#2497)
- [`a57e9dea7`](https://github.com/dashpay/dash/commit/a57e9dea7) Fix filtering of the lock votes for SPV nodes. (#2468)
- [`c6cf4d9a4`](https://github.com/dashpay/dash/commit/c6cf4d9a4) Relay txes through MN network faster than through regular nodes (#2397)
- [`e66c4e184`](https://github.com/dashpay/dash/commit/e66c4e184) Don't revert to INV based block announcements when the previous block is the devnet genesis block (#2388)
- [`b5142ee2c`](https://github.com/dashpay/dash/commit/b5142ee2c) Implement RemoveAskFor to indicate that we're not interested in an item anymore (#2384)
- [`53e12b7b4`](https://github.com/dashpay/dash/commit/53e12b7b4) Don't bail out from ProcessMasternodeConnections in regtest (#2368)
- [`31759a44d`](https://github.com/dashpay/dash/commit/31759a44d) Fix tx inv throughput (#2300)
- [`9d90b4fa4`](https://github.com/dashpay/dash/commit/9d90b4fa4) Honor filterInventoryKnown for non-tx/non-block items (#2292)
- [`6764dafec`](https://github.com/dashpay/dash/commit/6764dafec) Skip initial masternode list sync if spork 15 is active
- [`fced9a4b8`](https://github.com/dashpay/dash/commit/fced9a4b8) Ban peers that send us MNLISTDIFF messages
- [`d3ac86206`](https://github.com/dashpay/dash/commit/d3ac86206) Implement GETMNLISTDIFF and MNLISTDIFF P2P messages
- [`40eee1775`](https://github.com/dashpay/dash/commit/40eee1775) Fix sync in regtest (again) (#2241)
- [`c4ee2c89e`](https://github.com/dashpay/dash/commit/c4ee2c89e) Fix mnsync in regtest (#2202)

### Mining
- [`f96563462`](https://github.com/dashpay/dash/commit/f96563462) Fix check for nTemporaryTestnetForkDIP3Height (#2508)
- [`80656038f`](https://github.com/dashpay/dash/commit/80656038f) Bump nTemporaryTestnetForkHeight to 274000 (#2498)
- [`1c25356ff`](https://github.com/dashpay/dash/commit/1c25356ff) Allow to use low difficulty and higher block rewards for devnet (#2369)
- [`3cc4ac137`](https://github.com/dashpay/dash/commit/3cc4ac137) Fix crash bug with duplicate inputs within a transaction (#2302)
- [`e6b699bc2`](https://github.com/dashpay/dash/commit/e6b699bc2) Enforce MN and superblock payments in same block
- [`c7f75afdd`](https://github.com/dashpay/dash/commit/c7f75afdd) Fix nulldummy tests by creating correct DIP4 coinbase transactions
- [`bcc071957`](https://github.com/dashpay/dash/commit/bcc071957) Calculate and enforce DIP4 masternodes merkle root in CbTx
- [`0a086898f`](https://github.com/dashpay/dash/commit/0a086898f) Implement and enforce CbTx with correct block height and deprecate BIP34

### RPC
- [`a22f1bffe`](https://github.com/dashpay/dash/commit/a22f1bffe) Remove support for "0" as an alternative to "" when the default is requested (#2622) (#2624)
- [`18e1edabf`](https://github.com/dashpay/dash/commit/18e1edabf) Backport 2618 to v0.13.0.x (#2619)
- [`0dce846d5`](https://github.com/dashpay/dash/commit/0dce846d5) Add an option to use specific address as a source of funds in protx rpc commands (otherwise use payoutAddress/operatorPayoutAddress) (#2581)
- [`e71ea29e6`](https://github.com/dashpay/dash/commit/e71ea29e6) Add ownerAddr and votingAddr to CDeterministicMNState::ToJson (#2571)
- [`999a51907`](https://github.com/dashpay/dash/commit/999a51907) Fix optional revocation reason parameter for "protx revoke" and a few help strings (#2568)
- [`c08926146`](https://github.com/dashpay/dash/commit/c08926146) Unify "protx list" options (#2559)
- [`e9f7142ed`](https://github.com/dashpay/dash/commit/e9f7142ed) Bump PROTOCOL_VERSION and DMN_PROTO_VERSION to 70213 (#2557)
- [`818f0f464`](https://github.com/dashpay/dash/commit/818f0f464) Allow consuming specific UTXO in gobject prepare command (#2482)
- [`1270b7122`](https://github.com/dashpay/dash/commit/1270b7122) Use a verbosity instead of two verbose parameters (#2506)
- [`f6f6d075d`](https://github.com/dashpay/dash/commit/f6f6d075d) Still support "protx list" and "protx diff" when wallet is disabled at compile time (#2511)
- [`5a3f64310`](https://github.com/dashpay/dash/commit/5a3f64310) Deserialize CFinalCommitmentTxPayload instead of CFinalCommitment in TxToJSON (#2510)
- [`5da4c9728`](https://github.com/dashpay/dash/commit/5da4c9728) Use "registered" as default for "protx list" (#2513)
- [`fc6d651c4`](https://github.com/dashpay/dash/commit/fc6d651c4) Fix crashes in "protx" RPCs when wallet is disabled (#2509)
- [`ba49a4a16`](https://github.com/dashpay/dash/commit/ba49a4a16) Trivial: protx fund_register RPC help corrections (#2502)
- [`45421b1a3`](https://github.com/dashpay/dash/commit/45421b1a3) Add IS parameter for gobject prepare (#2452)
- [`2ba1ff521`](https://github.com/dashpay/dash/commit/2ba1ff521) Use ParseFixedPoint instead of ParseDoubleV in "protx register" commands (#2458)
- [`e049f9c1e`](https://github.com/dashpay/dash/commit/e049f9c1e) fix protx register rpc help (#2461)
- [`eb2103760`](https://github.com/dashpay/dash/commit/eb2103760) trivail, clarifies help text for protx register (#2462)
- [`76e93c7d7`](https://github.com/dashpay/dash/commit/76e93c7d7) Corrections to incorrect syntax in RPC help (#2466)
- [`3c1f44c3a`](https://github.com/dashpay/dash/commit/3c1f44c3a) Make sure protx_update_registrar adds enough funds for the fees
- [`d130f25ac`](https://github.com/dashpay/dash/commit/d130f25ac) Fix check for number of params to protx_update_service (#2443)
- [`adf9c87e2`](https://github.com/dashpay/dash/commit/adf9c87e2) Fix protx/bls rpc help (#2438)
- [`579c83e88`](https://github.com/dashpay/dash/commit/579c83e88) Add coinbase_payload to getblocktemplate help (#2437)
- [`3685c85e7`](https://github.com/dashpay/dash/commit/3685c85e7) Show BIP9 progress in getblockchaininfo (#2435)
- [`da3e3db4d`](https://github.com/dashpay/dash/commit/da3e3db4d) Fix sub-command help for masternode, gobject and protx rpcs (#2425)
- [`adad3fcfe`](https://github.com/dashpay/dash/commit/adad3fcfe) RPC: protx help corrections (#2422)
- [`1d56dffda`](https://github.com/dashpay/dash/commit/1d56dffda) Unify help display logic for various "complex" rpc commands (#2415)
- [`02442673d`](https://github.com/dashpay/dash/commit/02442673d) Trivial: Correct protx diff RPC help (#2410)
- [`1f56600c4`](https://github.com/dashpay/dash/commit/1f56600c4) Trivial: Protx operator reward clarification (#2407)
- [`7011fec1b`](https://github.com/dashpay/dash/commit/7011fec1b) RPC: Add help details for the bls RPC (#2403)
- [`50f133ad0`](https://github.com/dashpay/dash/commit/50f133ad0) Add merkle tree and coinbase transaction to the `protx diff` rpc command (#2392)
- [`25b6dae9e`](https://github.com/dashpay/dash/commit/25b6dae9e) Code style and RPC help string cleanups for DIP2/DIP3 (#2379)
- [`0ad2906c5`](https://github.com/dashpay/dash/commit/0ad2906c5) Clarify addlocked description in getbalance RPC (#2364)
- [`547b81dd0`](https://github.com/dashpay/dash/commit/547b81dd0) log `gobject prepare` params (#2317)
- [`d932d2c4e`](https://github.com/dashpay/dash/commit/d932d2c4e) Add instantlock field to getrawtransaction rpc output (#2314)
- [`c3d6b0651`](https://github.com/dashpay/dash/commit/c3d6b0651) Remove redundant check for unknown commands in masternode RPC (#2279)
- [`44706dc88`](https://github.com/dashpay/dash/commit/44706dc88) Implement projection of MN reward winners in "masternode winners"
- [`2d8f1244c`](https://github.com/dashpay/dash/commit/2d8f1244c) Implement 'masternode info <proTxHash>' RPC
- [`e2a9dbbce`](https://github.com/dashpay/dash/commit/e2a9dbbce) Better "masternode status" for deterministic masternodes
- [`50ac6fb3a`](https://github.com/dashpay/dash/commit/50ac6fb3a) Throw exception when trying to invoke start-xxx RPC in deterministic mode
- [`58aa81364`](https://github.com/dashpay/dash/commit/58aa81364) Implement "protx revoke" RPC
- [`185416b97`](https://github.com/dashpay/dash/commit/185416b97) Implement "protx update_registrar" RPC
- [`32951f795`](https://github.com/dashpay/dash/commit/32951f795) Implement "protx update_service" RPC
- [`5e3abeca2`](https://github.com/dashpay/dash/commit/5e3abeca2) Implement "protx list" RPC
- [`c77242346`](https://github.com/dashpay/dash/commit/c77242346) Implement "protx register" RPC
- [`1c2565804`](https://github.com/dashpay/dash/commit/1c2565804) Refactor `masternode` and `gobject` RPCs to support `help command subCommand` syntax (#2240)
- [`fb4d301a2`](https://github.com/dashpay/dash/commit/fb4d301a2) Add extraPayloadSize/extraPayload fields to RPC help
- [`2997d6d26`](https://github.com/dashpay/dash/commit/2997d6d26) add compressed option to `masternode genkey` (#2232)
- [`98ed90cbb`](https://github.com/dashpay/dash/commit/98ed90cbb) adds rpc calls for `setprivatesendrounds` and `setprivatesendamount` (#2230)
- [`50eb98d90`](https://github.com/dashpay/dash/commit/50eb98d90) Prepare for DIP3 operator reward payments and switch to array in getblocktemplate (#2216)
- [`a959f60aa`](https://github.com/dashpay/dash/commit/a959f60aa) De-duplicate "gobject vote-alias" and "gobject "vote-many" code (#2217)
- [`566fa5ec3`](https://github.com/dashpay/dash/commit/566fa5ec3) Add support for "help command subCommand" (#2210)
- [`4cd969e3d`](https://github.com/dashpay/dash/commit/4cd969e3d) Add `previousbits` field to `getblocktemplate` output (#2201)
- [`ac30196bc`](https://github.com/dashpay/dash/commit/ac30196bc) Show some info about the wallet dumped via dumpwallet (#2191)

### LLMQ and Deterministic Masternodes
- [`a3b01dfbe`](https://github.com/dashpay/dash/commit/a3b01dfbe) Gracefully shutdown on evodb inconsistency instead of crashing (#2611) (#2620)
- [`3861c6a82`](https://github.com/dashpay/dash/commit/3861c6a82) Add BIP9 deployment for DIP3 on mainnet (#2585)
- [`587911b36`](https://github.com/dashpay/dash/commit/587911b36) Fix IsBlockPayeeValid (#2577)
- [`3c30a6aff`](https://github.com/dashpay/dash/commit/3c30a6aff) Add missing masternodeblsprivkey help text (#2569)
- [`378dadd0f`](https://github.com/dashpay/dash/commit/378dadd0f) Ensure EvoDB consistency for quorum commitments by storing the best block hash (#2537)
- [`2127a426b`](https://github.com/dashpay/dash/commit/2127a426b) Further refactoring of CQuorumBlockProcessor (#2545)
- [`1522656d6`](https://github.com/dashpay/dash/commit/1522656d6) Correctly handle spent collaterals for MNs that were registered in the same block (#2553)
- [`583035337`](https://github.com/dashpay/dash/commit/583035337) Track operator key changes in mempool and handle conflicts (#2540)
- [`88f7bf0d8`](https://github.com/dashpay/dash/commit/88f7bf0d8) Don't delete/add values to the unique property map when it's null (#2538)
- [`15414dac2`](https://github.com/dashpay/dash/commit/15414dac2) Refactor CQuorumBlockProcessor and CDeterministicMNManager (#2536)
- [`d9b28fe1a`](https://github.com/dashpay/dash/commit/d9b28fe1a) Introduce dummy (ping-like) contributions for the dummy DKG (#2542)
- [`df0d0cce7`](https://github.com/dashpay/dash/commit/df0d0cce7) Watch for changes in operator key and disable local MN (#2541)
- [`511dc3714`](https://github.com/dashpay/dash/commit/511dc3714) Remove ProTxs from mempool that refer to a ProRegTx for which the collateral was spent (#2539)
- [`225c2135e`](https://github.com/dashpay/dash/commit/225c2135e) Allow skipping of MN payments with zero duffs (#2534)
- [`60867978d`](https://github.com/dashpay/dash/commit/60867978d) Avoid printing DIP3/DIP4 related logs twice (#2525)
- [`7037f7c99`](https://github.com/dashpay/dash/commit/7037f7c99) Bail out from GetBlockTxOuts in case nBlockHeight is above tip+1 (#2523)
- [`022491420`](https://github.com/dashpay/dash/commit/022491420) Print the state object when ProcessSpecialTxsInBlock fails in ConnectBlock (#2516)
- [`812834dc5`](https://github.com/dashpay/dash/commit/812834dc5) Put height into mined commitments and use it instead of the special handling of quorumVvecHash (#2501)
- [`a4f5ba38b`](https://github.com/dashpay/dash/commit/a4f5ba38b)  Implement CDummyDKG and CDummyCommitment until we have the real DKG merged (#2492)
- [`66612cc4b`](https://github.com/dashpay/dash/commit/66612cc4b) Add 0 entry to vTxSigOps when adding quorum commitments (#2489)
- [`f5beeafa1`](https://github.com/dashpay/dash/commit/f5beeafa1) Also restart MNs which didn't have the collateral moved, but do it later (#2483)
- [`0123517b4`](https://github.com/dashpay/dash/commit/0123517b4) Implement PoSe based on information from LLMQ commitments (#2478)
- [`22b5952c5`](https://github.com/dashpay/dash/commit/22b5952c5) Implement and enforce DIP6 commitments (#2477)
- [`d40a5ce31`](https://github.com/dashpay/dash/commit/d40a5ce31)  Properly initialize confirmedHash in CSimplifiedMNListEntry (#2479)
- [`5ffc31bce`](https://github.com/dashpay/dash/commit/5ffc31bce) Forbid version=0 in special TXs (#2473)
- [`85157f9a9`](https://github.com/dashpay/dash/commit/85157f9a9) Few trivial fixes for DIP2/DIP3 (#2474)
- [`b5947f299`](https://github.com/dashpay/dash/commit/b5947f299) Implement BuildSimplifiedDiff in CDeterministicMNList
- [`6edad3745`](https://github.com/dashpay/dash/commit/6edad3745) Use ForEachMN and GetMN in BuildDiff instead of directly accessing mnMap
- [`83aac461b`](https://github.com/dashpay/dash/commit/83aac461b) Allow P2SH/multisig addresses for operator rewards
- [`f5864254c`](https://github.com/dashpay/dash/commit/f5864254c) Do not use keyIDCollateralAddress anymore when spork15 is active
- [`5ccf556f3`](https://github.com/dashpay/dash/commit/5ccf556f3) GetMasternodeInfo with payee argument should do nothing when DIP3 is active
- [`927e8bd79`](https://github.com/dashpay/dash/commit/927e8bd79) Also forbid reusing collateral key for owner/voting keys
- [`826e7d063`](https://github.com/dashpay/dash/commit/826e7d063) Move internal collateral check to the else branch of the external collateral check
- [`dc404e755`](https://github.com/dashpay/dash/commit/dc404e755) Allow P2SH for payout scripts
- [`9adf8ad73`](https://github.com/dashpay/dash/commit/9adf8ad73) Remove restriction that forced use of same addresses for payout and collateral
- [`7c1f11089`](https://github.com/dashpay/dash/commit/7c1f11089) Revert #2441 and retry fixing (#2444)
- [`6761fa49f`](https://github.com/dashpay/dash/commit/6761fa49f) More checks for tx type
- [`b84369663`](https://github.com/dashpay/dash/commit/b84369663) Be more specific about tx version in conditions
- [`c975a986b`](https://github.com/dashpay/dash/commit/c975a986b) no cs_main in specialtxes
- [`8bd5b231b`](https://github.com/dashpay/dash/commit/8bd5b231b) Log mempool payload errors instead of crashing via assert
- [`658b7afd1`](https://github.com/dashpay/dash/commit/658b7afd1) Make error messages re payload a bit more specific
- [`153afb906`](https://github.com/dashpay/dash/commit/153afb906) Restart MNs in DIP3 tests even if collateral has not moved (#2441)
- [`4ad2f647c`](https://github.com/dashpay/dash/commit/4ad2f647c) Use proTxHash instead of outpoint when calculating masternode scores (#2440)
- [`c27e62935`](https://github.com/dashpay/dash/commit/c27e62935) Allow reusing of external collaterals in DIP3 (#2427)
- [`9da9d575a`](https://github.com/dashpay/dash/commit/9da9d575a) Allow collaterals for non-DIP3 MNs which were created after DIP3/BIP9 activation (#2412)
- [`30a2b283a`](https://github.com/dashpay/dash/commit/30a2b283a) Sign ProRegTx collaterals with a string message instead of payload hash, split `protx register` into `prepare`/`submit` (#2395)
- [`e34701295`](https://github.com/dashpay/dash/commit/e34701295) Fix crash when deterministic MN list is empty and keep paying superblocks in this case (#2387)
- [`28a6007a4`](https://github.com/dashpay/dash/commit/28a6007a4) Prepare DIP3 for testnet and reuse DIP3 deployment for autoix deployment (#2389)
- [`e3df91082`](https://github.com/dashpay/dash/commit/e3df91082) Allow referencing other TX outputs for ProRegTx collateral (#2366)
- [`fdfb07742`](https://github.com/dashpay/dash/commit/fdfb07742) Update ProRegTx serialization order (#2378)
- [`eaa856eb7`](https://github.com/dashpay/dash/commit/eaa856eb7) Remove nProtocolVersion and add mode/type fields to DIP3 (#2358)
- [`c9d274518`](https://github.com/dashpay/dash/commit/c9d274518) Use BLS keys for the DIP3 operator key (#2352)
- [`eaef90202`](https://github.com/dashpay/dash/commit/eaef90202) Don't use boost range adaptors in CDeterministicMNList (#2327)
- [`6adc236d0`](https://github.com/dashpay/dash/commit/6adc236d0) Only use dataDir in CEvoDB when not in-memory (#2291)
- [`8a878bfcf`](https://github.com/dashpay/dash/commit/8a878bfcf) Call InitializeCurrentBlockTip and activeMasternodeManager->Init after importing has finished (#2286)
- [`6b3d65028`](https://github.com/dashpay/dash/commit/6b3d65028) After DIP3 activation, allow voting with voting keys stored in your wallet (#2281)
- [`d8247dfff`](https://github.com/dashpay/dash/commit/d8247dfff) Use refactored payment logic when spork15 is active
- [`60002b7dd`](https://github.com/dashpay/dash/commit/60002b7dd) Payout and enforce operator reward payments
- [`2c481f0f8`](https://github.com/dashpay/dash/commit/2c481f0f8) Implement deterministic version of CMasternodePayments::IsScheduled
- [`19fbf8ab7`](https://github.com/dashpay/dash/commit/19fbf8ab7) Move cs_main lock from CMasternode::UpdateLastPaid to CMasternodeMan
- [`dc7292afa`](https://github.com/dashpay/dash/commit/dc7292afa) Implement new MN payments logic and add compatibility code
- [`d4530eb7d`](https://github.com/dashpay/dash/commit/d4530eb7d) Put all masternodes in MASTERNODE_ENABLED state when spork15 is active
- [`31b4f8354`](https://github.com/dashpay/dash/commit/31b4f8354) Forbid starting of legacy masternodes with non matching ProTx collateral values
- [`5050a9205`](https://github.com/dashpay/dash/commit/5050a9205) Add compatibility code for FindRandomNotInVec and GetMasternodeScores
- [`cc73422f8`](https://github.com/dashpay/dash/commit/cc73422f8) Add methods to add/remove (non-)deterministic MNs
- [`7d14566bc`](https://github.com/dashpay/dash/commit/7d14566bc) Add compatibility code to CMasternodeMan so that old code is still compatible
- [`27e8b48a6`](https://github.com/dashpay/dash/commit/27e8b48a6) Stop executing legacy MN list code when spork 15 is activated
- [`d90b13996`](https://github.com/dashpay/dash/commit/d90b13996) Implement CActiveDeterministicMasternodeManager
- [`a5e65aa37`](https://github.com/dashpay/dash/commit/a5e65aa37) Erase mnListCache entry on UndoBlock (#2254)
- [`88e7888de`](https://github.com/dashpay/dash/commit/88e7888de) Try using cache in GetListForBlock before reading from disk (#2253)
- [`9653af2f3`](https://github.com/dashpay/dash/commit/9653af2f3) Classes, validation and update logic for CProUpRevTX
- [`1c68d1107`](https://github.com/dashpay/dash/commit/1c68d1107) Classes, validation and update logic for CProUpRegTX
- [`8aca3b040`](https://github.com/dashpay/dash/commit/8aca3b040) Also check duplicate addresses for CProUpServTX in CTxMemPool
- [`923fd6739`](https://github.com/dashpay/dash/commit/923fd6739) Implement CProUpServTx logic in CDeterministicMNManager
- [`6ec0d7aea`](https://github.com/dashpay/dash/commit/6ec0d7aea) Classes and basic validation of ProUpServTx
- [`255403e92`](https://github.com/dashpay/dash/commit/255403e92) Include proTx data in json formatted transactions
- [`25545fc1e`](https://github.com/dashpay/dash/commit/25545fc1e) Split keyIDMasternode into keyIDOwner/keyIDOperator/keyIDVoting (#2248)
- [`2c172873a`](https://github.com/dashpay/dash/commit/2c172873a) Don't allow non-ProTx masternode collaterals after DIP3 activation
- [`9e8a86714`](https://github.com/dashpay/dash/commit/9e8a86714) Implementation of deterministic MNs list
- [`76fd30894`](https://github.com/dashpay/dash/commit/76fd30894) Automatically lock ProTx collaterals when TX is added/loaded to wallet
- [`cdd723ede`](https://github.com/dashpay/dash/commit/cdd723ede) Conflict handling for ProRegTx in mempool
- [`958b84ace`](https://github.com/dashpay/dash/commit/958b84ace) Implementation of ProRegTx with basic validation (no processing)
- [`c9a72e888`](https://github.com/dashpay/dash/commit/c9a72e888) Introduce CEvoDB for all evo related things, e.g. DIP3
- [`4531f6b89`](https://github.com/dashpay/dash/commit/4531f6b89) Implement CDBTransaction and CScopedDBTransaction
- [`e225cebcd`](https://github.com/dashpay/dash/commit/e225cebcd) Use previous block for CheckSpecialTx (#2243)
- [`b92bd8997`](https://github.com/dashpay/dash/commit/b92bd8997) Fix mninfo search by payee (#2233)
- [`8af7f6223`](https://github.com/dashpay/dash/commit/8af7f6223) Account for extraPayload when calculating fees in FundTransaction
- [`b606bde9a`](https://github.com/dashpay/dash/commit/b606bde9a) Support version 3 transaction serialization in mininode.py
- [`61bbe54ab`](https://github.com/dashpay/dash/commit/61bbe54ab) Add Get-/SetTxPayload helpers
- [`cebf71bbc`](https://github.com/dashpay/dash/commit/cebf71bbc) Stubs for special TX validation and processing
- [`d6c5a72e2`](https://github.com/dashpay/dash/commit/d6c5a72e2) Basic validation of version 3 TXs in CheckTransaction
- [`a3c4ee3fd`](https://github.com/dashpay/dash/commit/a3c4ee3fd) DIP2 changes to CTransaction and CMutableTransaction
- [`d20100ecd`](https://github.com/dashpay/dash/commit/d20100ecd) DIP0003 deployment
- [`4d3518fe0`](https://github.com/dashpay/dash/commit/4d3518fe0) Refactor MN payee logic in preparation for DIP3 (#2215)
- [`d946f21bd`](https://github.com/dashpay/dash/commit/d946f21bd) Masternode related refactorings in preparation of DIP3 (#2212)

### PrivateSend
- [`07309f0ec`](https://github.com/dashpay/dash/commit/07309f0ec) Allow up to MASTERNODE_MAX_MIXING_TXES (5) DSTXes per MN in a row (#2552)
- [`ed53fce47`](https://github.com/dashpay/dash/commit/ed53fce47) Revert "Apply similar logic to vecMasternodesUsed" (#2503)
- [`69bffed72`](https://github.com/dashpay/dash/commit/69bffed72) Do not sort resulting vector in SelectCoinsGroupedByAddresses (#2493)
- [`bb11f1a63`](https://github.com/dashpay/dash/commit/bb11f1a63) Fix recent changes in DSA conditions (#2494)
- [`6480ad1d5`](https://github.com/dashpay/dash/commit/6480ad1d5) Should check dsq queue regardless of the mixing state (#2491)
- [`67483cd34`](https://github.com/dashpay/dash/commit/67483cd34) Fix dsq/dsa conditions (#2487)
- [`9d4df466b`](https://github.com/dashpay/dash/commit/9d4df466b) Fix CreateDenominated failure for addresses with huge amount of inputs (#2486)
- [`2b400f74b`](https://github.com/dashpay/dash/commit/2b400f74b) Base dsq/dstx thresholold on total number of up to date masternodes (#2465)
- [`262454791`](https://github.com/dashpay/dash/commit/262454791) Add 5th denom, drop deprecated logic and bump min PS version (#2318)
- [`23f169c44`](https://github.com/dashpay/dash/commit/23f169c44) Drop custom PS logic for guessing fees etc. from SelectCoins (#2371)
- [`f7b0b5759`](https://github.com/dashpay/dash/commit/f7b0b5759) Pick rounds with the most inputs available to mix first (#2278)
- [`727e940c0`](https://github.com/dashpay/dash/commit/727e940c0) Fix recently introduced PS bugs (#2330)
- [`85a958a36`](https://github.com/dashpay/dash/commit/85a958a36) Drop dummy copy constructors in CPrivateSend*Session (#2305)
- [`c6a0c5541`](https://github.com/dashpay/dash/commit/c6a0c5541) A couple of small fixes for mixing collaterals (#2294)
- [`d192d642f`](https://github.com/dashpay/dash/commit/d192d642f) Move heavy coin selection out of the loop in SubmitDenominate (#2274)
- [`28e0476f4`](https://github.com/dashpay/dash/commit/28e0476f4) Squash two logic branches in SubmitDenominate into one (#2270)
- [`9b6eb4765`](https://github.com/dashpay/dash/commit/9b6eb4765) Include inputs with max rounds in SelectCoinsDark/SelectCoinsByDenominations (#2277)
- [`55d7bb900`](https://github.com/dashpay/dash/commit/55d7bb900) Add an option to disable popups for PS mixing txes (#2272)
- [`38ccfef3b`](https://github.com/dashpay/dash/commit/38ccfef3b) Identify PS collateral payments in transaction list a bit more accurate (#2271)
- [`ad31dbbd7`](https://github.com/dashpay/dash/commit/ad31dbbd7) Add more variance to coin selection in PS mixing (#2261)
- [`8c9cb2909`](https://github.com/dashpay/dash/commit/8c9cb2909) Revert 2075 (#2259)
- [`b164bcc7a`](https://github.com/dashpay/dash/commit/b164bcc7a) Split PS into Manager and Session and allow running multiple mixing sessions in parallel (client side) (#2203)
- [`d4d11476a`](https://github.com/dashpay/dash/commit/d4d11476a) Fix typo and grammar in PS error message (#2199)
- [`a83ab5501`](https://github.com/dashpay/dash/commit/a83ab5501) Fix wallet lock check in DoAutomaticDenominating (#2196)
- [`30fa8bc33`](https://github.com/dashpay/dash/commit/30fa8bc33) Make sure pwalletMain is not null whenever it's used in PS client (#2190)
- [`3c89983db`](https://github.com/dashpay/dash/commit/3c89983db) Remove DarksendConfig (#2132)
- [`43091a3ef`](https://github.com/dashpay/dash/commit/43091a3ef) PrivateSend Enhancement: Up default round count to 4 and allow user to mix up to 16 rounds (#2128)

### InstantSend
- [`35550a3f9`](https://github.com/dashpay/dash/commit/35550a3f9) Add quorumModifierHash to instant send lock vote (#2505)
- [`fa8f4a10c`](https://github.com/dashpay/dash/commit/fa8f4a10c)  Include masternodeProTxHash in CTxLockVote (#2484)
- [`624e50949`](https://github.com/dashpay/dash/commit/624e50949) Remove few leftovers of `-instantsenddepth`
- [`733cd9512`](https://github.com/dashpay/dash/commit/733cd9512) Remove global fDIP0003ActiveAtTip and fix wrong use of VersionBitsState in auto IX (#2380)
- [`5454bea37`](https://github.com/dashpay/dash/commit/5454bea37) Automatic InstantSend locks for "simple" transactions (#2140)
- [`1e74bcace`](https://github.com/dashpay/dash/commit/1e74bcace) [ZMQ] Notify when an IS double spend is attempted (#2262)
- [`6bcd868de`](https://github.com/dashpay/dash/commit/6bcd868de) Fix lockedByInstantSend initialization (#2197)
- [`0a6f47323`](https://github.com/dashpay/dash/commit/0a6f47323) Remove dummy confirmations in RPC API and GUI for InstantSend transactions (#2040)
- [`ace980834`](https://github.com/dashpay/dash/commit/ace980834) Extend Bloom Filter support to InstantSend related messages (#2184)
- [`2c0d4c9d7`](https://github.com/dashpay/dash/commit/2c0d4c9d7) Save/load InstantSend cache (#2051)

### Sporks
- [`33f78d70e`](https://github.com/dashpay/dash/commit/33f78d70e) Do not accept sporks with nTimeSigned way too far into the future (#2578)
- [`6c4b3ed8d`](https://github.com/dashpay/dash/commit/6c4b3ed8d) Load sporks before checking blockchain (#2573)
- [`d94092b60`](https://github.com/dashpay/dash/commit/d94092b60) Fix spork propagation while in IBD and fix spork integration tests (#2533)
- [`43e757bee`](https://github.com/dashpay/dash/commit/43e757bee) Amend SERIALIZATION_VERSION_STRING string for spork cache (#2339)
- [`f7ab6c469`](https://github.com/dashpay/dash/commit/f7ab6c469) M-of-N-like sporks (#2288)
- [`c2958733e`](https://github.com/dashpay/dash/commit/c2958733e) CSporkManager::Clear() should not alter sporkPubKeyID and sporkPrivKey (#2313)
- [`8c0dca282`](https://github.com/dashpay/dash/commit/8c0dca282) Add versioning to spork cache (#2312)
- [`5461e92bf`](https://github.com/dashpay/dash/commit/5461e92bf) Add spork to control deterministic MN lists activation
- [`73c2ddde7`](https://github.com/dashpay/dash/commit/73c2ddde7) extract sporkmanager from sporkmessage (#2234)
- [`1767e3457`](https://github.com/dashpay/dash/commit/1767e3457) Save/load spork cache (#2206)
- [`075ca0903`](https://github.com/dashpay/dash/commit/075ca0903) Protect CSporkManager with critical section (#2213)

### Governance
- [`222e5b4f7`](https://github.com/dashpay/dash/commit/222e5b4f7) Remove proposal/funding votes from MNs that changed the voting key (#2570)
- [`5185dd5b7`](https://github.com/dashpay/dash/commit/5185dd5b7) Use correct time field when removing pre-DIP3 votes (#2535)
- [`d2ca9edde`](https://github.com/dashpay/dash/commit/d2ca9edde) Fix multiple issues with governance voting after spork15 activation (#2526)
- [`08dc17871`](https://github.com/dashpay/dash/commit/08dc17871) Drop pre-DIP3 votes from current votes per MN per object (#2524)
- [`0c1b683a0`](https://github.com/dashpay/dash/commit/0c1b683a0) Clear votes which were created before spork15 activation and use operator key for non-funding votes (#2512)
- [`da4b5fb16`](https://github.com/dashpay/dash/commit/da4b5fb16) Remove an unused function from governance object collateral code (#2480)
- [`8deb8e90f`](https://github.com/dashpay/dash/commit/8deb8e90f) Modernize Gov Methods (#2326)
- [`0471fa884`](https://github.com/dashpay/dash/commit/0471fa884) Drop MAX_GOVERNANCE_OBJECT_DATA_SIZE (and maxgovobjdatasize in rpc) (#2298)
- [`737353c84`](https://github.com/dashpay/dash/commit/737353c84) Fix IsBlockValueValid/IsOldBudgetBlockValueValid (#2276)
- [`a5643f899`](https://github.com/dashpay/dash/commit/a5643f899) Switch RequestGovernanceObjectVotes from pointers to hashes (#2189)
- [`0e689341d`](https://github.com/dashpay/dash/commit/0e689341d) Implement Governance ZMQ notification messages (#2160)

### GUI
- [`858bb52ad`](https://github.com/dashpay/dash/commit/858bb52ad) Show correct operator payee address in DIP3 MN list GUI (#2563)
- [`190863722`](https://github.com/dashpay/dash/commit/190863722) Remove legacy MN list tabs on spork15 activation (#2567)
- [`3e97b0cbd`](https://github.com/dashpay/dash/commit/3e97b0cbd) Make sure that we can get inputType and fUseInstantSend regardless of the way recipients are sorted (#2550)
- [`1a7c29b97`](https://github.com/dashpay/dash/commit/1a7c29b97) Revert "Sort recipients in SendCoins dialog via BIP69 rule (#2546)" (#2549)
- [`ca0aec2a3`](https://github.com/dashpay/dash/commit/ca0aec2a3) Match recipients with txouts by scriptPubKey in reassignAmounts() (#2548)
- [`09730e1c5`](https://github.com/dashpay/dash/commit/09730e1c5) Bail out from update methods in MasternodeList when shutdown is requested (#2551)
- [`18cd5965c`](https://github.com/dashpay/dash/commit/18cd5965c) Sort recipients in SendCoins dialog via BIP69 rule (#2546)
- [`9100c69eb`](https://github.com/dashpay/dash/commit/9100c69eb) Allow filtering by proTxHash on DIP3 MN tab (#2532)
- [`216119921`](https://github.com/dashpay/dash/commit/216119921) Fix wrong total MN count in UI and "masternode count" RPC (#2527)
- [`8f8878a94`](https://github.com/dashpay/dash/commit/8f8878a94) Add dummy/hidden column to carry the proTxHash per MN list entry... (#2530)
- [`a4ea816b2`](https://github.com/dashpay/dash/commit/a4ea816b2) use aqua gui theme (#2472)
- [`aa495405b`](https://github.com/dashpay/dash/commit/aa495405b) [GUI] Realign tx filter widgets (#2485)
- [`f4ef388de`](https://github.com/dashpay/dash/commit/f4ef388de) Update PS help text for the new denom (#2471)
- [`7cabbadef`](https://github.com/dashpay/dash/commit/7cabbadef) Implement context menu and extra info on double-click for DIP3 masternode list (#2459)
- [`9232a455c`](https://github.com/dashpay/dash/commit/9232a455c) Do not hold cs_main while emitting messages in WalletModel::prepareTransaction (#2463)
- [`cf2b547b7`](https://github.com/dashpay/dash/commit/cf2b547b7) Implement tab for DIP3 MN list (#2454)
- [`46462d682`](https://github.com/dashpay/dash/commit/46462d682) Add a column for IS lock status on Transactions tab (#2433)
- [`5ecd91b05`](https://github.com/dashpay/dash/commit/5ecd91b05) Fix ps collateral/denom creation tx category confusion (#2430)
- [`4a78b161f`](https://github.com/dashpay/dash/commit/4a78b161f) PrivateSend spending txes should have "outgoing" icon on overview screen (#2396)
- [`d7e210341`](https://github.com/dashpay/dash/commit/d7e210341) Fixes inaccurate round count in CoinControlDialog (#2137)

### Cleanups/Tests/Docs/Other
- [`b5670c475`](https://github.com/dashpay/dash/commit/b5670c475) Set CLIENT_VERSION_IS_RELEASE to true (#2591)
- [`a05eeb21e`](https://github.com/dashpay/dash/commit/a05eeb21e) Update immer to c89819df92191d6969a6a22c88c72943b8e25016 (#2626)
- [`10b3736bd`](https://github.com/dashpay/dash/commit/10b3736bd) [0.13.0.x] Translations201901 (#2592)
- [`34d2a6038`](https://github.com/dashpay/dash/commit/34d2a6038) Release notes 0.13.0.0 draft (#2583)
- [`c950a8f51`](https://github.com/dashpay/dash/commit/c950a8f51) Merge v0.12.3.4 commits into develop (#2582)
- [`6dfceaba5`](https://github.com/dashpay/dash/commit/6dfceaba5) Force FlushStateToDisk on ConnectTip/DisconnectTip while not in IBD (#2560)
- [`552d9089e`](https://github.com/dashpay/dash/commit/552d9089e) Update testnet seeds to point to MNs that are on the new chain (#2558)
- [`63b58b1e9`](https://github.com/dashpay/dash/commit/63b58b1e9) Reintroduce BLSInit to correctly set secure alloctor callbacks (#2543)
- [`cbd030352`](https://github.com/dashpay/dash/commit/cbd030352) Serialize the block header in CBlockHeader::GetHash() (#2531)
- [`3a6bd8d23`](https://github.com/dashpay/dash/commit/3a6bd8d23) Call ProcessTick every second, handle tick cooldown inside (#2522)
- [`973a7f6dd`](https://github.com/dashpay/dash/commit/973a7f6dd) Fix GUI warnings in debug.log (#2521)
- [`c248c48e4`](https://github.com/dashpay/dash/commit/c248c48e4) Try to fix a few sporadic instant send failures in DIP3 tests  (#2500)
- [`7a709b81d`](https://github.com/dashpay/dash/commit/7a709b81d) Perform less instant send tests in DIP3 tests (#2499)
- [`245c3220e`](https://github.com/dashpay/dash/commit/245c3220e) Sync blocks before creating TXs (#2496)
- [`65528e9e7`](https://github.com/dashpay/dash/commit/65528e9e7) Bump masternodeman cache version (#2467)
- [`6c190d1bb`](https://github.com/dashpay/dash/commit/6c190d1bb) Fix make deploy error on macos (#2475)
- [`df7d12b41`](https://github.com/dashpay/dash/commit/df7d12b41) Add univalue test for real numbers (#2460)
- [`614ff70b4`](https://github.com/dashpay/dash/commit/614ff70b4) Let ccache compress the cache by itself instead of compressing ccache.tar (#2456)
- [`40fa1bb49`](https://github.com/dashpay/dash/commit/40fa1bb49) Add platform dependent include_directories in CMakeLists.txt (#2455)
- [`12aba2592`](https://github.com/dashpay/dash/commit/12aba2592) Updating translations for de, es, fi, nl, pt, sk, zh_CN, zh_TW (#2451)
- [`52bf5a6b0`](https://github.com/dashpay/dash/commit/52bf5a6b0) Install libxkbcommon0 in gitian-linux.yml
- [`fefe34250`](https://github.com/dashpay/dash/commit/fefe34250) Update manpages
- [`c60687fe6`](https://github.com/dashpay/dash/commit/c60687fe6) Sleep longer between attempts in sync_blocks
- [`88498ba13`](https://github.com/dashpay/dash/commit/88498ba13) Apply suggestions from code review
- [`91af72b18`](https://github.com/dashpay/dash/commit/91af72b18) Allow to specify how log to sleep between attempts in wait_until
- [`f65e74682`](https://github.com/dashpay/dash/commit/f65e74682) Pass "-parallel=3" to reduce load on Travis nodes while testing
- [`3c99d9e35`](https://github.com/dashpay/dash/commit/3c99d9e35) Fix test_fail_create_protx in DIP3 tests
- [`4de70f0ac`](https://github.com/dashpay/dash/commit/4de70f0ac) Test P2SH/multisig payee addresses in DIP3 tests
- [`5fc4072ca`](https://github.com/dashpay/dash/commit/5fc4072ca) Parallel ASN resolve and allow passing of input file names to makeseeds.py (#2432)
- [`76a38f6ce`](https://github.com/dashpay/dash/commit/76a38f6ce)  Update defaultAssumeValid, nMinimumChainWork and checkpoints (#2428)
- [`0e9ad207a`](https://github.com/dashpay/dash/commit/0e9ad207a) Update hardcoded seeds (#2429)
- [`42ee369b1`](https://github.com/dashpay/dash/commit/42ee369b1) [Formatting] masternodelist.* clang+manual format (#2426)
- [`e961c7134`](https://github.com/dashpay/dash/commit/e961c7134) Translations 201811 (#2249)
- [`f0df5bffa`](https://github.com/dashpay/dash/commit/f0df5bffa) Clang evo folder and activemasternode.* (#2418)
- [`98bdf35f9`](https://github.com/dashpay/dash/commit/98bdf35f9) bump PS copyright (#2417)
- [`e9bb822c1`](https://github.com/dashpay/dash/commit/e9bb822c1) Clang format PrivateSend files (#2373)
- [`bea590958`](https://github.com/dashpay/dash/commit/bea590958) Fix auto-IS tests (#2414)
- [`f03629d6d`](https://github.com/dashpay/dash/commit/f03629d6d) Explicitly specify which branch of Wine to install (#2411)
- [`5e829a3b1`](https://github.com/dashpay/dash/commit/5e829a3b1) Update Chia bls-signature to latest version (#2409)
- [`51addf9a0`](https://github.com/dashpay/dash/commit/51addf9a0) Fix p2p-instantsend.py test (#2408)
- [`7e8f07bb9`](https://github.com/dashpay/dash/commit/7e8f07bb9) A couple of fixes for shutdown sequence (#2406)
- [`9c455caea`](https://github.com/dashpay/dash/commit/9c455caea) A couple of fixes for init steps (#2405)
- [`6560ac64b`](https://github.com/dashpay/dash/commit/6560ac64b) Properly escape $ in Jenkinsfile.gitian (#2404)
- [`70eb710b1`](https://github.com/dashpay/dash/commit/70eb710b1) Undefine DOUBLE after include Chia BLS headers (#2400)
- [`052af81b4`](https://github.com/dashpay/dash/commit/052af81b4) Ensure correct order of destruction for BLS secure allocator (#2401)
- [`c8804ea5a`](https://github.com/dashpay/dash/commit/c8804ea5a) Do not ignore patches in depends (#2399)
- [`13f2eb449`](https://github.com/dashpay/dash/commit/13f2eb449) Force fvisibility=hidden when compiling on macos (#2398)
- [`9eb9c99d5`](https://github.com/dashpay/dash/commit/9eb9c99d5) Bump version to 0.13.0 (#2386)
- [`8f9b004ca`](https://github.com/dashpay/dash/commit/8f9b004ca) Support "fast" mode when calling sync_masternodes (#2383)
- [`fcea333ba`](https://github.com/dashpay/dash/commit/fcea333ba) Rewrite handling of too long depends builds in .travis.yml (#2385)
- [`d1debfc26`](https://github.com/dashpay/dash/commit/d1debfc26) Implement mt_pooled_secure_allocator and use it for BLS secure allocation (#2375)
- [`0692de1c5`](https://github.com/dashpay/dash/commit/0692de1c5) Fix prepare_masternodes/create_masternodes in DashTestFramework (#2382)
- [`6433a944a`](https://github.com/dashpay/dash/commit/6433a944a) [Trivial] typo Groupped -> Grouped (#2374)
- [`59932401b`](https://github.com/dashpay/dash/commit/59932401b) Change internal references of Darksend to PrivateSend (#2372)
- [`e3046adb3`](https://github.com/dashpay/dash/commit/e3046adb3) Clear devNetParams and mimic behavior of other param types (#2367)
- [`de426e962`](https://github.com/dashpay/dash/commit/de426e962) Give tail calls enough time to print errors (#2376)
- [`0402240a2`](https://github.com/dashpay/dash/commit/0402240a2) Bump CMAKE_CXX_STANDARD to 14 in CMakeLists.txt (#2377)
- [`3c9237aa4`](https://github.com/dashpay/dash/commit/3c9237aa4) Use VersionBitsState instead of VersionBitsTipState to avoid cs_main lock (#2370)
- [`c4351fd32`](https://github.com/dashpay/dash/commit/c4351fd32) revert 737, DEFAULT_TRANSACTION_MAXFEE = 0.1 * COIN (#2362)
- [`1c9ed7806`](https://github.com/dashpay/dash/commit/1c9ed7806) GDB automation with Python script to measure memory usage in dashd (#1609)
- [`d998dc13e`](https://github.com/dashpay/dash/commit/d998dc13e) Add cmake to non-mac gitian descriptors (#2360)
- [`266dd3232`](https://github.com/dashpay/dash/commit/266dd3232) mkdir -p to allow re-start of failed chia build (#2359)
- [`11a0cbf84`](https://github.com/dashpay/dash/commit/11a0cbf84) InstantSend-related tests refactoring (#2333)
- [`3313bbd51`](https://github.com/dashpay/dash/commit/3313bbd51) Backport bitcoin #13623 Migrate gitian-build.sh to python (#2319)
- [`7b76bbb57`](https://github.com/dashpay/dash/commit/7b76bbb57)  Update Chia BLS libs to latest master (#2357)
- [`e2de632f8`](https://github.com/dashpay/dash/commit/e2de632f8) Move handling of `size != SerSize` into SetBuf/GetBuf (#2356)
- [`81d60bc28`](https://github.com/dashpay/dash/commit/81d60bc28) Fix the issue with transaction amount precision in IS tests (#2353)
- [`a45055384`](https://github.com/dashpay/dash/commit/a45055384) Fix qt configure to detect clang version correctly (#2344)
- [`b99d94a0f`](https://github.com/dashpay/dash/commit/b99d94a0f) Minor build documentation updates (#2343)
- [`464191698`](https://github.com/dashpay/dash/commit/464191698) Review fixes
- [`9c8e4ac76`](https://github.com/dashpay/dash/commit/9c8e4ac76) Move bls stuff from crypto/ to bls/
- [`bed1ded8b`](https://github.com/dashpay/dash/commit/bed1ded8b) Remove duplicated check (#2336)
- [`89f744d06`](https://github.com/dashpay/dash/commit/89f744d06) pack of small cleanup fixes / optimizations (#2334)
- [`9603c5290`](https://github.com/dashpay/dash/commit/9603c5290) Trivial: Codestyle fixes in InstantSend code (#2332)
- [`90ad75911`](https://github.com/dashpay/dash/commit/90ad75911) Fix auto-IS and tests (#2331)
- [`b3fc236af`](https://github.com/dashpay/dash/commit/b3fc236af) Fix mnodeman.cs vs cs_vPendingMasternodes vs cs_main deadlock (#2200)
- [`80fd096b0`](https://github.com/dashpay/dash/commit/80fd096b0) Add ECDSA benchmarks
- [`78675d9bb`](https://github.com/dashpay/dash/commit/78675d9bb) Add BLS and DKG benchmarks
- [`3ee27c168`](https://github.com/dashpay/dash/commit/3ee27c168) Add highly parallelized worker/helper for BLS/DKG calculations
- [`aa3b0aa8a`](https://github.com/dashpay/dash/commit/aa3b0aa8a) Add simple helpers/wrappers for BLS+AES based integrated encryption schemes (IES)
- [`9ccf6f584`](https://github.com/dashpay/dash/commit/9ccf6f584) Implement wrappers around Chia BLS lib
- [`3039d44d3`](https://github.com/dashpay/dash/commit/3039d44d3) Add Chia bls-signatures library to depends
- [`f3dcb6916`](https://github.com/dashpay/dash/commit/f3dcb6916) Add cmake to ci/Dockerfile.builder
- [`057d7445e`](https://github.com/dashpay/dash/commit/057d7445e) Add libgmp to depends
- [`b0d0093d7`](https://github.com/dashpay/dash/commit/b0d0093d7) Add helper to rename all threads of a ctpl::thread_pool
- [`47a162255`](https://github.com/dashpay/dash/commit/47a162255) Add ctpl header only library
- [`407baccec`](https://github.com/dashpay/dash/commit/407baccec) Remove obsolete build-openbsd.md (#2328)
- [`8a1b51356`](https://github.com/dashpay/dash/commit/8a1b51356) Backport: Fix Qt build with XCode (for depends) (#2325)
- [`a5aca049d`](https://github.com/dashpay/dash/commit/a5aca049d) rename vars in mnsync to make more sense (#2308)
- [`ee6a5a33b`](https://github.com/dashpay/dash/commit/ee6a5a33b) Gov cleanup + copyright bump (#2324)
- [`d7e5f02ea`](https://github.com/dashpay/dash/commit/d7e5f02ea) Update build documentation (#2323)
- [`bd8c54d12`](https://github.com/dashpay/dash/commit/bd8c54d12) A bit more verbosity for some critical errors (#2316)
- [`a4ff2a19a`](https://github.com/dashpay/dash/commit/a4ff2a19a) Fix some warnings and do a couple of other trivial cleanups (#2315)
- [`07208a4ae`](https://github.com/dashpay/dash/commit/07208a4ae) document spork system with doxygen comments (#2301)
- [`2c1a17909`](https://github.com/dashpay/dash/commit/2c1a17909) cleanup: remove unused vars, includes, functions (#2306)
- [`3d48824b4`](https://github.com/dashpay/dash/commit/3d48824b4) Update .clang-format to more accurately show the actual style (#2299)
- [`8ea40102c`](https://github.com/dashpay/dash/commit/8ea40102c) Remove leftover RBF code from BTC (#2297)
- [`76599aad3`](https://github.com/dashpay/dash/commit/76599aad3) Drop (pre-)70208 compatibility code (#2295)
- [`016681cd3`](https://github.com/dashpay/dash/commit/016681cd3) Add support for serialization of bitsets and tuples (#2293)
- [`2a95dd30c`](https://github.com/dashpay/dash/commit/2a95dd30c) Fix locking issues in DIP3 unit tests (#2285)
- [`47ca06ab3`](https://github.com/dashpay/dash/commit/47ca06ab3) DIP3 integration tests (#2280)
- [`ad6c2893c`](https://github.com/dashpay/dash/commit/ad6c2893c) Docs - Update Core version number in readme files (#2267)
- [`bc7924d41`](https://github.com/dashpay/dash/commit/bc7924d41) Add unit tests for DIP3 and DIP4
- [`d653ace99`](https://github.com/dashpay/dash/commit/d653ace99) Update CbTx in TestChainSetup
- [`9674be8f9`](https://github.com/dashpay/dash/commit/9674be8f9) Refactor TestChain100Setup to allow different test chain setups
- [`cb37c3972`](https://github.com/dashpay/dash/commit/cb37c3972) Bump PROTOCOL_VERSION to 70211, bump MIN_* protocols to 70210 (#2256)
- [`b99886532`](https://github.com/dashpay/dash/commit/b99886532) add link for developer-notes in contributing (#2260)
- [`fded838c9`](https://github.com/dashpay/dash/commit/fded838c9) RPC folder: Cleaned up brackets on if, while, for, BOOST_FOREACH. Some whitespace fixes (#2257)
- [`5295c78cc`](https://github.com/dashpay/dash/commit/5295c78cc) Fix typo in "penalty" (#2247)
- [`c566ce75d`](https://github.com/dashpay/dash/commit/c566ce75d) Update copyright in specialtx.h/cpp
- [`e002c50b0`](https://github.com/dashpay/dash/commit/e002c50b0) Add "immer" functional/immutable containers library (#2244)
- [`799e3c312`](https://github.com/dashpay/dash/commit/799e3c312) Perform Jenkins builds in /dash-src all the time to fix caching issues (#2242)
- [`b6896387a`](https://github.com/dashpay/dash/commit/b6896387a) Move DIP1 transaction size checks out of ContextualCheckBlock and use ContextualCheckTransaction instead (#2238)
- [`e415fd049`](https://github.com/dashpay/dash/commit/e415fd049) Revert CMasternodePayments::IsTransactionValid to the logic before the recent refactorings (#2237)
- [`8da88ecf6`](https://github.com/dashpay/dash/commit/8da88ecf6) Don't crash when formatting in logging throws exceptions (#2231)
- [`2e06f8133`](https://github.com/dashpay/dash/commit/2e06f8133) fix missed format parameter (#2229)
- [`3d654b981`](https://github.com/dashpay/dash/commit/3d654b981) Build bionic base image in Jenkinsfile.gitian & update docs (#2226)
- [`c09f57bd7`](https://github.com/dashpay/dash/commit/c09f57bd7) Backport move to Ubuntu Bionic and GCC7 in Gitian builds (#2225)
- [`7cf9572c2`](https://github.com/dashpay/dash/commit/7cf9572c2) Backport Bitcoin #11881: Remove Python2 support (#2224)
- [`633879cd2`](https://github.com/dashpay/dash/commit/633879cd2) Only use version 1 and 2 transactions for sighash_tests (#2219)
- [`2d4e18537`](https://github.com/dashpay/dash/commit/2d4e18537) Some useful commits from the DIP3 PR in regard to integration tests (#2218)
- [`106bab1ae`](https://github.com/dashpay/dash/commit/106bab1ae) Add new ParseXXX methods to easily parse UniValue values (#2211)
- [`c4c610783`](https://github.com/dashpay/dash/commit/c4c610783) Use C++14 standard when building (#2209)
- [`589a77013`](https://github.com/dashpay/dash/commit/589a77013) Correction to release date for 0.12.0 (#2205)
- [`96435288f`](https://github.com/dashpay/dash/commit/96435288f) Move block template specific stuff from CBlock to CBlockTemplate (#2195)
- [`3d002c946`](https://github.com/dashpay/dash/commit/3d002c946) Fix active masternode task schedule (#2193)
- [`65b904526`](https://github.com/dashpay/dash/commit/65b904526) Add helpers GetSentinelString() and GetDaemonString() to CMasternodePing (#2192)
- [`eb202e812`](https://github.com/dashpay/dash/commit/eb202e812) Use ccache in gitian builds (#2185)
- [`b47617325`](https://github.com/dashpay/dash/commit/b47617325) Install python3 in gitian builds (#2182)
- [`7a85e24c3`](https://github.com/dashpay/dash/commit/7a85e24c3) Remove deprecated gitian-rpi2.yml descriptor (#2183)
- [`1681d6366`](https://github.com/dashpay/dash/commit/1681d6366) Replace Dash-specific threads with Dash-specific scheduled tasks (#2043)
- [`dac090964`](https://github.com/dashpay/dash/commit/dac090964) remove dashpay.io dns seed entry (#2181)
- [`753c2436b`](https://github.com/dashpay/dash/commit/753c2436b) Fix MissingPropertyException on Jenkins when no cache was found (#2180)
- [`f3e380659`](https://github.com/dashpay/dash/commit/f3e380659) Move to in-docker CI builds and add Jenkins support (#2178)
- [`23dde9f12`](https://github.com/dashpay/dash/commit/23dde9f12) Remove a few annoying debug prints from CMasternodeMan (#2179)
- [`5036d7dfc`](https://github.com/dashpay/dash/commit/5036d7dfc) depends: Update Qt download url (#2177)
- [`e23339d6f`](https://github.com/dashpay/dash/commit/e23339d6f) use nullptr in Dash-specific code (#2166)
- [`42c193df0`](https://github.com/dashpay/dash/commit/42c193df0) replace map count/insert w/emplace in instantx.cpp (#2165)
- [`fd70a1eb9`](https://github.com/dashpay/dash/commit/fd70a1eb9) iterator cleanup in several places (#2164)
- [`df1be90ce`](https://github.com/dashpay/dash/commit/df1be90ce)  Update links to obsolete documentation (#2162)
- [`448e92f4a`](https://github.com/dashpay/dash/commit/448e92f4a) GetOutPointPrivateSendRounds readability (#2149)
- [`6da2837bd`](https://github.com/dashpay/dash/commit/6da2837bd) InstantSend Integration tests (#2141)
- [`8ee9333bc`](https://github.com/dashpay/dash/commit/8ee9333bc) remove boost dependency from Dash-specific code (#2072)
- [`a527845e4`](https://github.com/dashpay/dash/commit/a527845e4) Bump to 0.12.4.0 pre-release (#2167)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alexander Block
- UdjinM6
- PastaPastaPasta
- gladcow
- Nathan Marley
- thephez
- strophy
- PaulieD
- InhumanPerfection
- Spencer Lievens
- -k
- Salisbury
- Solar Designer
- Oleg Girko
- Anton Suprunchuk

As well as everyone that submitted issues, reviewed pull requests or helped translating on
[Transifex](https://www.transifex.com/projects/p/dash/).

Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Darkcoin was rebranded to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

- [v0.12.3.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.4.md) released Dec/14/2018
- [v0.12.3.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.3.md) released Sep/19/2018
- [v0.12.3.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.2.md) released Jul/09/2018
- [v0.12.3.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

