22.0 Release Notes
==================

Tortoisecoin Core version 22.0 is now available from:

  <https://tortoisecoincore.org/bin/tortoisecoin-core-22.0/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://tortoisecoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Tortoisecoin-Qt` (on Mac)
or `tortoisecoind`/`tortoisecoin-qt` (on Linux).

Upgrading directly from a version of Tortoisecoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Tortoisecoin Core are generally supported.

Compatibility
==============

Tortoisecoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.14+, and Windows 7 and newer.  Tortoisecoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Tortoisecoin Core on
unsupported systems.

From Tortoisecoin Core 22.0 onwards, macOS versions earlier than 10.14 are no longer supported.

Notable changes
===============

P2P and network changes
-----------------------
- Added support for running Tortoisecoin Core as an
  [I2P (Invisible Internet Project)](https://en.wikipedia.org/wiki/I2P) service
  and connect to such services. See [i2p.md](https://github.com/tortoisecoin/tortoisecoin/blob/22.x/doc/i2p.md) for details. (#20685)
- This release removes support for Tor version 2 hidden services in favor of Tor
  v3 only, as the Tor network [dropped support for Tor
  v2](https://blog.torproject.org/v2-deprecation-timeline) with the release of
  Tor version 0.4.6.  Henceforth, Tortoisecoin Core ignores Tor v2 addresses; it
  neither rumors them over the network to other peers, nor stores them in memory
  or to `peers.dat`.  (#22050)

- Added NAT-PMP port mapping support via
  [`libnatpmp`](https://miniupnp.tuxfamily.org/libnatpmp.html). (#18077)

New and Updated RPCs
--------------------

- Due to [BIP 350](https://github.com/tortoisecoin/bips/blob/master/bip-0350.mediawiki)
  being implemented, behavior for all RPCs that accept addresses is changed when
  a native witness version 1 (or higher) is passed. These now require a Bech32m
  encoding instead of a Bech32 one, and Bech32m encoding will be used for such
  addresses in RPC output as well. No version 1 addresses should be created
  for mainnet until consensus rules are adopted that give them meaning
  (as will happen through [BIP 341](https://github.com/tortoisecoin/bips/blob/master/bip-0341.mediawiki)).
  Once that happens, Bech32m is expected to be used for them, so this shouldn't
  affect any production systems, but may be observed on other networks where such
  addresses already have meaning (like signet). (#20861)

- The `getpeerinfo` RPC returns two new boolean fields, `bip152_hb_to` and
  `bip152_hb_from`, that respectively indicate whether we selected a peer to be
  in compact blocks high-bandwidth mode or whether a peer selected us as a
  compact blocks high-bandwidth peer. High-bandwidth peers send new block
  announcements via a `cmpctblock` message rather than the usual inv/headers
  announcements. See BIP 152 for more details. (#19776)

- `getpeerinfo` no longer returns the following fields: `addnode`, `banscore`,
  and `whitelisted`, which were previously deprecated in 0.21. Instead of
  `addnode`, the `connection_type` field returns manual. Instead of
  `whitelisted`, the `permissions` field indicates if the peer has special
  privileges. The `banscore` field has simply been removed. (#20755)

- The following RPCs:  `gettxout`, `getrawtransaction`, `decoderawtransaction`,
  `decodescript`, `gettransaction`, and REST endpoints: `/rest/tx`,
  `/rest/getutxos`, `/rest/block` deprecated the following fields (which are no
  longer returned in the responses by default): `addresses`, `reqSigs`.
  The `-deprecatedrpc=addresses` flag must be passed for these fields to be
  included in the RPC response. This flag/option will be available only for this major release, after which
  the deprecation will be removed entirely. Note that these fields are attributes of
  the `scriptPubKey` object returned in the RPC response. However, in the response
  of `decodescript` these fields are top-level attributes, and included again as attributes
  of the `scriptPubKey` object. (#20286)

- When creating a hex-encoded tortoisecoin transaction using the `tortoisecoin-tx` utility
  with the `-json` option set, the following fields: `addresses`, `reqSigs` are no longer
  returned in the tx output of the response. (#20286)

- The `listbanned` RPC now returns two new numeric fields: `ban_duration` and `time_remaining`.
  Respectively, these new fields indicate the duration of a ban and the time remaining until a ban expires,
  both in seconds. Additionally, the `ban_created` field is repositioned to come before `banned_until`. (#21602)

- The `setban` RPC can ban onion addresses again. This fixes a regression introduced in version 0.21.0. (#20852)

- The `getnodeaddresses` RPC now returns a "network" field indicating the
  network type (ipv4, ipv6, onion, or i2p) for each address.  (#21594)

- `getnodeaddresses` now also accepts a "network" argument (ipv4, ipv6, onion,
  or i2p) to return only addresses of the specified network.  (#21843)

- The `testmempoolaccept` RPC now accepts multiple transactions (still experimental at the moment,
  API may be unstable). This is intended for testing transaction packages with dependency
  relationships; it is not recommended for batch-validating independent transactions. In addition to
  mempool policy, package policies apply: the list cannot contain more than 25 transactions or have a
  total size exceeding 101K virtual bytes, and cannot conflict with (spend the same inputs as) each other or
  the mempool, even if it would be a valid BIP125 replace-by-fee. There are some known limitations to
  the accuracy of the test accept: it's possible for `testmempoolaccept` to return "allowed"=True for a
  group of transactions, but "too-long-mempool-chain" if they are actually submitted. (#20833)

- `addmultisigaddress` and `createmultisig` now support up to 20 keys for
  Segwit addresses. (#20867)

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

Build System
------------

- Release binaries are now produced using the new `guix`-based build system.
  The [/doc/release-process.md](/doc/release-process.md) document has been updated accordingly.

Files
-----

- The list of banned hosts and networks (via `setban` RPC) is now saved on disk
  in JSON format in `banlist.json` instead of `banlist.dat`. `banlist.dat` is
  only read on startup if `banlist.json` is not present. Changes are only written to the new
  `banlist.json`. A future version of Tortoisecoin Core may completely ignore
  `banlist.dat`. (#20966)

New settings
------------

- The `-natpmp` option has been added to use NAT-PMP to map the listening port.
  If both UPnP and NAT-PMP are enabled, a successful allocation from UPnP
  prevails over one from NAT-PMP. (#18077)

Updated settings
----------------

Changes to Wallet or GUI related settings can be found in the GUI or Wallet section below.

- Passing an invalid `-rpcauth` argument now cause tortoisecoind to fail to start.  (#20461)

Tools and Utilities
-------------------

- A new CLI `-addrinfo` command returns the number of addresses known to the
  node per network type (including Tor v2 versus v3) and total. This can be
  useful to see if the node knows enough addresses in a network to use options
  like `-onlynet=<network>` or to upgrade to this release of Tortoisecoin Core 22.0
  that supports Tor v3 only.  (#21595)

- A new `-rpcwaittimeout` argument to `tortoisecoin-cli` sets the timeout
  in seconds to use with `-rpcwait`. If the timeout expires,
  `tortoisecoin-cli` will report a failure. (#21056)

Wallet
------

- External signers such as hardware wallets can now be used through the new RPC methods `enumeratesigners` and `displayaddress`. Support is also added to the `send` RPC call. This feature is experimental. See [external-signer.md](https://github.com/tortoisecoin/tortoisecoin/blob/22.x/doc/external-signer.md) for details. (#16546)

- A new `listdescriptors` RPC is available to inspect the contents of descriptor-enabled wallets.
  The RPC returns public versions of all imported descriptors, including their timestamp and flags.
  For ranged descriptors, it also returns the range boundaries and the next index to generate addresses from. (#20226)

- The `bumpfee` RPC is not available with wallets that have private keys
  disabled. `psbtbumpfee` can be used instead. (#20891)

- The `fundrawtransaction`, `send` and `walletcreatefundedpsbt` RPCs now support an `include_unsafe` option
  that when `true` allows using unsafe inputs to fund the transaction.
  Note that the resulting transaction may become invalid if one of the unsafe inputs disappears.
  If that happens, the transaction must be funded with different inputs and republished. (#21359)

- We now support up to 20 keys in `multi()` and `sortedmulti()` descriptors
  under `wsh()`. (#20867)

- Taproot descriptors can be imported into the wallet only after activation has occurred on the network (e.g. mainnet, testnet, signet) in use. See [descriptors.md](https://github.com/tortoisecoin/tortoisecoin/blob/22.x/doc/descriptors.md) for supported descriptors.

GUI changes
-----------

- External signers such as hardware wallets can now be used. These require an external tool such as [HWI](https://github.com/tortoisecoin-core/HWI) to be installed and configured under Options -> Wallet. When creating a new wallet a new option "External signer" will appear in the dialog. If the device is detected, its name is suggested as the wallet name. The watch-only keys are then automatically imported. Receive addresses can be verified on the device. The send dialog will automatically use the connected device. This feature is experimental and the UI may freeze for a few seconds when performing these actions.

Low-level changes
=================

RPC
---

- The RPC server can process a limited number of simultaneous RPC requests.
  Previously, if this limit was exceeded, the RPC server would respond with
  [status code 500 (`HTTP_INTERNAL_SERVER_ERROR`)](https://en.wikipedia.org/wiki/List_of_HTTP_status_codes#5xx_server_errors).
  Now it returns status code 503 (`HTTP_SERVICE_UNAVAILABLE`). (#18335)

- Error codes have been updated to be more accurate for the following error cases (#18466):
  - `signmessage` now returns RPC_INVALID_ADDRESS_OR_KEY (-5) if the
    passed address is invalid. Previously returned RPC_TYPE_ERROR (-3).
  - `verifymessage` now returns RPC_INVALID_ADDRESS_OR_KEY (-5) if the
    passed address is invalid. Previously returned RPC_TYPE_ERROR (-3).
  - `verifymessage` now returns RPC_TYPE_ERROR (-3) if the passed signature
    is malformed. Previously returned RPC_INVALID_ADDRESS_OR_KEY (-5).

Tests
-----

22.0 change log
===============

A detailed list of changes in this version follows. To keep the list to a manageable length, small refactors and typo fixes are not included, and similar changes are sometimes condensed into one line.

### Consensus
- tortoisecoin/tortoisecoin#19438 Introduce deploymentstatus (ajtowns)
- tortoisecoin/tortoisecoin#20207 Follow-up extra comments on taproot code and tests (sipa)
- tortoisecoin/tortoisecoin#21330 Deal with missing data in signature hashes more consistently (sipa)

### Policy
- tortoisecoin/tortoisecoin#18766 Disable fee estimation in blocksonly mode (by removing the fee estimates global) (darosior)
- tortoisecoin/tortoisecoin#20497 Add `MAX_STANDARD_SCRIPTSIG_SIZE` to policy (sanket1729)
- tortoisecoin/tortoisecoin#20611 Move `TX_MAX_STANDARD_VERSION` to policy (MarcoFalke)

### Mining
- tortoisecoin/tortoisecoin#19937, tortoisecoin/tortoisecoin#20923 Signet mining utility (ajtowns)

### Block and transaction handling
- tortoisecoin/tortoisecoin#14501 Fix possible data race when committing block files (luke-jr)
- tortoisecoin/tortoisecoin#15946 Allow maintaining the blockfilterindex when using prune (jonasschnelli)
- tortoisecoin/tortoisecoin#18710 Add local thread pool to CCheckQueue (hebasto)
- tortoisecoin/tortoisecoin#19521 Coinstats Index (fjahr)
- tortoisecoin/tortoisecoin#19806 UTXO snapshot activation (jamesob)
- tortoisecoin/tortoisecoin#19905 Remove dead CheckForkWarningConditionsOnNewFork (MarcoFalke)
- tortoisecoin/tortoisecoin#19935 Move SaltedHashers to separate file and add some new ones (achow101)
- tortoisecoin/tortoisecoin#20054 Remove confusing and useless "unexpected version" warning (MarcoFalke)
- tortoisecoin/tortoisecoin#20519 Handle rename failure in `DumpMempool(…)` by using the `RenameOver(…)` return value (practicalswift)
- tortoisecoin/tortoisecoin#20749, tortoisecoin/tortoisecoin#20750, tortoisecoin/tortoisecoin#21055, tortoisecoin/tortoisecoin#21270, tortoisecoin/tortoisecoin#21525, tortoisecoin/tortoisecoin#21391, tortoisecoin/tortoisecoin#21767, tortoisecoin/tortoisecoin#21866 Prune `g_chainman` usage (dongcarl)
- tortoisecoin/tortoisecoin#20833 rpc/validation: enable packages through testmempoolaccept (glozow)
- tortoisecoin/tortoisecoin#20834 Locks and docs in ATMP and CheckInputsFromMempoolAndCache (glozow)
- tortoisecoin/tortoisecoin#20854 Remove unnecessary try-block (amitiuttarwar)
- tortoisecoin/tortoisecoin#20868 Remove redundant check on pindex (jarolrod)
- tortoisecoin/tortoisecoin#20921 Don't try to invalidate genesis block in CChainState::InvalidateBlock (theStack)
- tortoisecoin/tortoisecoin#20972 Locks: Annotate CTxMemPool::check to require `cs_main` (dongcarl)
- tortoisecoin/tortoisecoin#21009 Remove RewindBlockIndex logic (dhruv)
- tortoisecoin/tortoisecoin#21025 Guard chainman chainstates with `cs_main` (dongcarl)
- tortoisecoin/tortoisecoin#21202 Two small clang lock annotation improvements (amitiuttarwar)
- tortoisecoin/tortoisecoin#21523 Run VerifyDB on all chainstates (jamesob)
- tortoisecoin/tortoisecoin#21573 Update libsecp256k1 subtree to latest master (sipa)
- tortoisecoin/tortoisecoin#21582, tortoisecoin/tortoisecoin#21584, tortoisecoin/tortoisecoin#21585 Fix assumeutxo crashes (MarcoFalke)
- tortoisecoin/tortoisecoin#21681 Fix ActivateSnapshot to use hardcoded nChainTx (jamesob)
- tortoisecoin/tortoisecoin#21796 index: Avoid async shutdown on init error (MarcoFalke)
- tortoisecoin/tortoisecoin#21946 Document and test lack of inherited signaling in RBF policy (ariard)
- tortoisecoin/tortoisecoin#22084 Package testmempoolaccept followups (glozow)
- tortoisecoin/tortoisecoin#22102 Remove `Warning:` from warning message printed for unknown new rules (prayank23)
- tortoisecoin/tortoisecoin#22112 Force port 0 in I2P (vasild)
- tortoisecoin/tortoisecoin#22135 CRegTestParams: Use `args` instead of `gArgs` (kiminuo)
- tortoisecoin/tortoisecoin#22146 Reject invalid coin height and output index when loading assumeutxo (MarcoFalke)
- tortoisecoin/tortoisecoin#22253 Distinguish between same tx and same-nonwitness-data tx in mempool (glozow)
- tortoisecoin/tortoisecoin#22261 Two small fixes to node broadcast logic (jnewbery)
- tortoisecoin/tortoisecoin#22415 Make `m_mempool` optional in CChainState (jamesob)
- tortoisecoin/tortoisecoin#22499 Update assumed chain params (sriramdvt)
- tortoisecoin/tortoisecoin#22589 net, doc: update I2P hardcoded seeds and docs for 22.0 (jonatack)

### P2P protocol and network code
- tortoisecoin/tortoisecoin#18077 Add NAT-PMP port forwarding support (hebasto)
- tortoisecoin/tortoisecoin#18722 addrman: improve performance by using more suitable containers (vasild)
- tortoisecoin/tortoisecoin#18819 Replace `cs_feeFilter` with simple std::atomic (MarcoFalke)
- tortoisecoin/tortoisecoin#19203 Add regression fuzz harness for CVE-2017-18350. Add FuzzedSocket (practicalswift)
- tortoisecoin/tortoisecoin#19288 fuzz: Add fuzzing harness for TorController (practicalswift)
- tortoisecoin/tortoisecoin#19415 Make DNS lookup mockable, add fuzzing harness (practicalswift)
- tortoisecoin/tortoisecoin#19509 Per-Peer Message Capture (troygiorshev)
- tortoisecoin/tortoisecoin#19763 Don't try to relay to the address' originator (vasild)
- tortoisecoin/tortoisecoin#19771 Replace enum CConnMan::NumConnections with enum class ConnectionDirection (luke-jr)
- tortoisecoin/tortoisecoin#19776 net, rpc: expose high bandwidth mode state via getpeerinfo (theStack)
- tortoisecoin/tortoisecoin#19832 Put disconnecting logs into BCLog::NET category (hebasto)
- tortoisecoin/tortoisecoin#19858 Periodically make block-relay connections and sync headers (sdaftuar)
- tortoisecoin/tortoisecoin#19884 No delay in adding fixed seeds if -dnsseed=0 and peers.dat is empty (dhruv)
- tortoisecoin/tortoisecoin#20079 Treat handshake misbehavior like unknown message (MarcoFalke)
- tortoisecoin/tortoisecoin#20138 Assume that SetCommonVersion is called at most once per peer (MarcoFalke)
- tortoisecoin/tortoisecoin#20162 p2p: declare Announcement::m_state as uint8_t, add getter/setter (jonatack)
- tortoisecoin/tortoisecoin#20197 Protect onions in AttemptToEvictConnection(), add eviction protection test coverage (jonatack)
- tortoisecoin/tortoisecoin#20210 assert `CNode::m_inbound_onion` is inbound in ctor, add getter, unit tests (jonatack)
- tortoisecoin/tortoisecoin#20228 addrman: Make addrman a top-level component (jnewbery)
- tortoisecoin/tortoisecoin#20234 Don't bind on 0.0.0.0 if binds are restricted to Tor (vasild)
- tortoisecoin/tortoisecoin#20477 Add unit testing of node eviction logic (practicalswift)
- tortoisecoin/tortoisecoin#20516 Well-defined CAddress disk serialization, and addrv2 anchors.dat (sipa)
- tortoisecoin/tortoisecoin#20557 addrman: Fix new table bucketing during unserialization (jnewbery)
- tortoisecoin/tortoisecoin#20561 Periodically clear `m_addr_known` (sdaftuar)
- tortoisecoin/tortoisecoin#20599 net processing: Tolerate sendheaders and sendcmpct messages before verack (jnewbery)
- tortoisecoin/tortoisecoin#20616 Check CJDNS address is valid (lontivero)
- tortoisecoin/tortoisecoin#20617 Remove `m_is_manual_connection` from CNodeState (ariard)
- tortoisecoin/tortoisecoin#20624 net processing: Remove nStartingHeight check from block relay (jnewbery)
- tortoisecoin/tortoisecoin#20651 Make p2p recv buffer timeout 20 minutes for all peers (jnewbery)
- tortoisecoin/tortoisecoin#20661 Only select from addrv2-capable peers for torv3 address relay (sipa)
- tortoisecoin/tortoisecoin#20685 Add I2P support using I2P SAM (vasild)
- tortoisecoin/tortoisecoin#20690 Clean up logging of outbound connection type (sdaftuar)
- tortoisecoin/tortoisecoin#20721 Move ping data to `net_processing` (jnewbery)
- tortoisecoin/tortoisecoin#20724 Cleanup of -debug=net log messages (ajtowns)
- tortoisecoin/tortoisecoin#20747 net processing: Remove dropmessagestest (jnewbery)
- tortoisecoin/tortoisecoin#20764 cli -netinfo peer connections dashboard updates 🎄 ✨ (jonatack)
- tortoisecoin/tortoisecoin#20788 add RAII socket and use it instead of bare SOCKET (vasild)
- tortoisecoin/tortoisecoin#20791 remove unused legacyWhitelisted in AcceptConnection() (jonatack)
- tortoisecoin/tortoisecoin#20816 Move RecordBytesSent() call out of `cs_vSend` lock (jnewbery)
- tortoisecoin/tortoisecoin#20845 Log to net debug in MaybeDiscourageAndDisconnect except for noban and manual peers (MarcoFalke)
- tortoisecoin/tortoisecoin#20864 Move SocketSendData lock annotation to header (MarcoFalke)
- tortoisecoin/tortoisecoin#20965 net, rpc:  return `NET_UNROUTABLE` as `not_publicly_routable`, automate helps (jonatack)
- tortoisecoin/tortoisecoin#20966 banman: save the banlist in a JSON format on disk (vasild)
- tortoisecoin/tortoisecoin#21015 Make all of `net_processing` (and some of net) use std::chrono types (dhruv)
- tortoisecoin/tortoisecoin#21029 tortoisecoin-cli: Correct docs (no "generatenewaddress" exists) (luke-jr)
- tortoisecoin/tortoisecoin#21148 Split orphan handling from `net_processing` into txorphanage (ajtowns)
- tortoisecoin/tortoisecoin#21162 Net Processing: Move RelayTransaction() into PeerManager (jnewbery)
- tortoisecoin/tortoisecoin#21167 make `CNode::m_inbound_onion` public, initialize explicitly (jonatack)
- tortoisecoin/tortoisecoin#21186 net/net processing: Move addr data into `net_processing` (jnewbery)
- tortoisecoin/tortoisecoin#21187 Net processing: Only call PushAddress() from `net_processing` (jnewbery)
- tortoisecoin/tortoisecoin#21198 Address outstanding review comments from PR20721 (jnewbery)
- tortoisecoin/tortoisecoin#21222 log: Clarify log message when file does not exist (MarcoFalke)
- tortoisecoin/tortoisecoin#21235 Clarify disconnect log message in ProcessGetBlockData, remove send bool (MarcoFalke)
- tortoisecoin/tortoisecoin#21236 Net processing: Extract `addr` send functionality into MaybeSendAddr() (jnewbery)
- tortoisecoin/tortoisecoin#21261 update inbound eviction protection for multiple networks, add I2P peers (jonatack)
- tortoisecoin/tortoisecoin#21328 net, refactor: pass uint16 CService::port as uint16 (jonatack)
- tortoisecoin/tortoisecoin#21387 Refactor sock to add I2P fuzz and unit tests (vasild)
- tortoisecoin/tortoisecoin#21395 Net processing: Remove unused CNodeState.address member (jnewbery)
- tortoisecoin/tortoisecoin#21407 i2p: limit the size of incoming messages (vasild)
- tortoisecoin/tortoisecoin#21506 p2p, refactor: make NetPermissionFlags an enum class (jonatack)
- tortoisecoin/tortoisecoin#21509 Don't send FEEFILTER in blocksonly mode (mzumsande)
- tortoisecoin/tortoisecoin#21560 Add Tor v3 hardcoded seeds (laanwj)
- tortoisecoin/tortoisecoin#21563 Restrict period when `cs_vNodes` mutex is locked (hebasto)
- tortoisecoin/tortoisecoin#21564 Avoid calling getnameinfo when formatting IPv4 addresses in CNetAddr::ToStringIP (practicalswift)
- tortoisecoin/tortoisecoin#21631 i2p: always check the return value of Sock::Wait() (vasild)
- tortoisecoin/tortoisecoin#21644 p2p, bugfix: use NetPermissions::HasFlag() in CConnman::Bind() (jonatack)
- tortoisecoin/tortoisecoin#21659 flag relevant Sock methods with [[nodiscard]] (vasild)
- tortoisecoin/tortoisecoin#21750 remove unnecessary check of `CNode::cs_vSend` (vasild)
- tortoisecoin/tortoisecoin#21756 Avoid calling `getnameinfo` when formatting IPv6 addresses in `CNetAddr::ToStringIP` (practicalswift)
- tortoisecoin/tortoisecoin#21775 Limit `m_block_inv_mutex` (MarcoFalke)
- tortoisecoin/tortoisecoin#21825 Add I2P hardcoded seeds (jonatack)
- tortoisecoin/tortoisecoin#21843 p2p, rpc: enable GetAddr, GetAddresses, and getnodeaddresses by network (jonatack)
- tortoisecoin/tortoisecoin#21845 net processing: Don't require locking `cs_main` before calling RelayTransactions() (jnewbery)
- tortoisecoin/tortoisecoin#21872 Sanitize message type for logging (laanwj)
- tortoisecoin/tortoisecoin#21914 Use stronger AddLocal() for our I2P address (vasild)
- tortoisecoin/tortoisecoin#21985 Return IPv6 scope id in `CNetAddr::ToStringIP()` (laanwj)
- tortoisecoin/tortoisecoin#21992 Remove -feefilter option (amadeuszpawlik)
- tortoisecoin/tortoisecoin#21996 Pass strings to NetPermissions::TryParse functions by const ref (jonatack)
- tortoisecoin/tortoisecoin#22013 ignore block-relay-only peers when skipping DNS seed (ajtowns)
- tortoisecoin/tortoisecoin#22050 Remove tor v2 support (jonatack)
- tortoisecoin/tortoisecoin#22096 AddrFetch - don't disconnect on self-announcements (mzumsande)
- tortoisecoin/tortoisecoin#22141 net processing: Remove hash and fValidatedHeaders from QueuedBlock (jnewbery)
- tortoisecoin/tortoisecoin#22144 Randomize message processing peer order (sipa)
- tortoisecoin/tortoisecoin#22147 Protect last outbound HB compact block peer (sdaftuar)
- tortoisecoin/tortoisecoin#22179 Torv2 removal followups (vasild)
- tortoisecoin/tortoisecoin#22211 Relay I2P addresses even if not reachable (by us) (vasild)
- tortoisecoin/tortoisecoin#22284 Performance improvements to ProtectEvictionCandidatesByRatio() (jonatack)
- tortoisecoin/tortoisecoin#22387 Rate limit the processing of rumoured addresses (sipa)
- tortoisecoin/tortoisecoin#22455 addrman: detect on-disk corrupted nNew and nTried during unserialization (vasild)

### Wallet
- tortoisecoin/tortoisecoin#15710 Catch `ios_base::failure` specifically (Bushstar)
- tortoisecoin/tortoisecoin#16546 External signer support - Wallet Box edition (Sjors)
- tortoisecoin/tortoisecoin#17331 Use effective values throughout coin selection (achow101)
- tortoisecoin/tortoisecoin#18418 Increase `OUTPUT_GROUP_MAX_ENTRIES` to 100 (fjahr)
- tortoisecoin/tortoisecoin#18842 Mark replaced tx to not be in the mempool anymore (MarcoFalke)
- tortoisecoin/tortoisecoin#19136 Add `parent_desc` to `getaddressinfo` (achow101)
- tortoisecoin/tortoisecoin#19137 wallettool: Add dump and createfromdump commands (achow101)
- tortoisecoin/tortoisecoin#19651 `importdescriptor`s update existing (S3RK)
- tortoisecoin/tortoisecoin#20040 Refactor OutputGroups to handle fees and spending eligibility on grouping (achow101)
- tortoisecoin/tortoisecoin#20202 Make BDB support optional (achow101)
- tortoisecoin/tortoisecoin#20226, tortoisecoin/tortoisecoin#21277, - tortoisecoin/tortoisecoin#21063 Add `listdescriptors` command (S3RK)
- tortoisecoin/tortoisecoin#20267 Disable and fix tests for when BDB is not compiled (achow101)
- tortoisecoin/tortoisecoin#20275 List all wallets in non-SQLite and non-BDB builds (ryanofsky)
- tortoisecoin/tortoisecoin#20365 wallettool: Add parameter to create descriptors wallet (S3RK)
- tortoisecoin/tortoisecoin#20403 `upgradewallet` fixes, improvements, test coverage (jonatack)
- tortoisecoin/tortoisecoin#20448 `unloadwallet`: Allow specifying `wallet_name` param matching RPC endpoint wallet (luke-jr)
- tortoisecoin/tortoisecoin#20536 Error with "Transaction too large" if the funded tx will end up being too large after signing (achow101)
- tortoisecoin/tortoisecoin#20687 Add missing check for -descriptors wallet tool option (MarcoFalke)
- tortoisecoin/tortoisecoin#20952 Add BerkeleyDB version sanity check at init time (laanwj)
- tortoisecoin/tortoisecoin#21127 Load flags before everything else (Sjors)
- tortoisecoin/tortoisecoin#21141 Add new format string placeholders for walletnotify (maayank)
- tortoisecoin/tortoisecoin#21238 A few descriptor improvements to prepare for Taproot support (sipa)
- tortoisecoin/tortoisecoin#21302 `createwallet` examples for descriptor wallets (S3RK)
- tortoisecoin/tortoisecoin#21329 descriptor wallet: Cache last hardened xpub and use in normalized descriptors (achow101)
- tortoisecoin/tortoisecoin#21365 Basic Taproot signing support for descriptor wallets (sipa)
- tortoisecoin/tortoisecoin#21417 Misc external signer improvement and HWI 2 support (Sjors)
- tortoisecoin/tortoisecoin#21467 Move external signer out of wallet module (Sjors)
- tortoisecoin/tortoisecoin#21572 Fix wrong wallet RPC context set after #21366 (ryanofsky)
- tortoisecoin/tortoisecoin#21574 Drop JSONRPCRequest constructors after #21366 (ryanofsky)
- tortoisecoin/tortoisecoin#21666 Miscellaneous external signer changes (fanquake)
- tortoisecoin/tortoisecoin#21759 Document coin selection code (glozow)
- tortoisecoin/tortoisecoin#21786 Ensure sat/vB feerates are in range (mantissa of 3) (jonatack)
- tortoisecoin/tortoisecoin#21944 Fix issues when `walletdir` is root directory (prayank23)
- tortoisecoin/tortoisecoin#22042 Replace size/weight estimate tuple with struct for named fields (instagibbs)
- tortoisecoin/tortoisecoin#22051 Basic Taproot derivation support for descriptors (sipa)
- tortoisecoin/tortoisecoin#22154 Add OutputType::BECH32M and related wallet support for fetching bech32m addresses (achow101)
- tortoisecoin/tortoisecoin#22156 Allow tr() import only when Taproot is active (achow101)
- tortoisecoin/tortoisecoin#22166 Add support for inferring tr() descriptors (sipa)
- tortoisecoin/tortoisecoin#22173 Do not load external signers wallets when unsupported (achow101)
- tortoisecoin/tortoisecoin#22308 Add missing BlockUntilSyncedToCurrentChain (MarcoFalke)
- tortoisecoin/tortoisecoin#22334 Do not spam about non-existent spk managers (S3RK)
- tortoisecoin/tortoisecoin#22379 Erase spkmans rather than setting to nullptr (achow101)
- tortoisecoin/tortoisecoin#22421 Make IsSegWitOutput return true for taproot outputs (sipa)
- tortoisecoin/tortoisecoin#22461 Change ScriptPubKeyMan::Upgrade default to True (achow101)
- tortoisecoin/tortoisecoin#22492 Reorder locks in dumpwallet to avoid lock order assertion (achow101)
- tortoisecoin/tortoisecoin#22686 Use GetSelectionAmount in ApproximateBestSubset (achow101)

### RPC and other APIs
- tortoisecoin/tortoisecoin#18335, tortoisecoin/tortoisecoin#21484 cli: Print useful error if tortoisecoind rpc work queue exceeded (LarryRuane)
- tortoisecoin/tortoisecoin#18466 Fix invalid parameter error codes for `{sign,verify}message` RPCs (theStack)
- tortoisecoin/tortoisecoin#18772 Calculate fees in `getblock` using BlockUndo data (robot-visions)
- tortoisecoin/tortoisecoin#19033 http: Release work queue after event base finish (promag)
- tortoisecoin/tortoisecoin#19055 Add MuHash3072 implementation (fjahr)
- tortoisecoin/tortoisecoin#19145 Add `hash_type` MUHASH for gettxoutsetinfo (fjahr)
- tortoisecoin/tortoisecoin#19847 Avoid duplicate set lookup in `gettxoutproof` (promag)
- tortoisecoin/tortoisecoin#20286 Deprecate `addresses` and `reqSigs` from RPC outputs (mjdietzx)
- tortoisecoin/tortoisecoin#20459 Fail to return undocumented return values (MarcoFalke)
- tortoisecoin/tortoisecoin#20461 Validate `-rpcauth` arguments (promag)
- tortoisecoin/tortoisecoin#20556 Properly document return values (`submitblock`, `gettxout`, `getblocktemplate`, `scantxoutset`) (MarcoFalke)
- tortoisecoin/tortoisecoin#20755 Remove deprecated fields from `getpeerinfo` (amitiuttarwar)
- tortoisecoin/tortoisecoin#20832 Better error messages for invalid addresses (eilx2)
- tortoisecoin/tortoisecoin#20867 Support up to 20 keys for multisig under Segwit context (darosior)
- tortoisecoin/tortoisecoin#20877 cli: `-netinfo` user help and argument parsing improvements (jonatack)
- tortoisecoin/tortoisecoin#20891 Remove deprecated bumpfee behavior (achow101)
- tortoisecoin/tortoisecoin#20916 Return wtxid from `testmempoolaccept` (MarcoFalke)
- tortoisecoin/tortoisecoin#20917 Add missing signet mentions in network name lists (theStack)
- tortoisecoin/tortoisecoin#20941 Document `RPC_TRANSACTION_ALREADY_IN_CHAIN` exception (jarolrod)
- tortoisecoin/tortoisecoin#20944 Return total fee in `getmempoolinfo` (MarcoFalke)
- tortoisecoin/tortoisecoin#20964 Add specific error code for "wallet already loaded" (laanwj)
- tortoisecoin/tortoisecoin#21053 Document {previous,next}blockhash as optional (theStack)
- tortoisecoin/tortoisecoin#21056 Add a `-rpcwaittimeout` parameter to limit time spent waiting (cdecker)
- tortoisecoin/tortoisecoin#21192 cli: Treat high detail levels as maximum in `-netinfo` (laanwj)
- tortoisecoin/tortoisecoin#21311 Document optional fields for `getchaintxstats` result (theStack)
- tortoisecoin/tortoisecoin#21359 `include_unsafe` option for fundrawtransaction (t-bast)
- tortoisecoin/tortoisecoin#21426 Remove `scantxoutset` EXPERIMENTAL warning (jonatack)
- tortoisecoin/tortoisecoin#21544 Missing doc updates for bumpfee psbt update (MarcoFalke)
- tortoisecoin/tortoisecoin#21594 Add `network` field to `getnodeaddresses` (jonatack)
- tortoisecoin/tortoisecoin#21595, tortoisecoin/tortoisecoin#21753 cli: Create `-addrinfo` (jonatack)
- tortoisecoin/tortoisecoin#21602 Add additional ban time fields to `listbanned` (jarolrod)
- tortoisecoin/tortoisecoin#21679 Keep default argument value in correct type (promag)
- tortoisecoin/tortoisecoin#21718 Improve error message for `getblock` invalid datatype (klementtan)
- tortoisecoin/tortoisecoin#21913 RPCHelpMan fixes (kallewoof)
- tortoisecoin/tortoisecoin#22021 `bumpfee`/`psbtbumpfee` fixes and updates (jonatack)
- tortoisecoin/tortoisecoin#22043 `addpeeraddress` test coverage, code simplify/constness (jonatack)
- tortoisecoin/tortoisecoin#22327 cli: Avoid truncating `-rpcwaittimeout` (MarcoFalke)

### GUI
- tortoisecoin/tortoisecoin#18948 Call setParent() in the parent's context (hebasto)
- tortoisecoin/tortoisecoin#20482 Add depends qt fix for ARM macs (jonasschnelli)
- tortoisecoin/tortoisecoin#21836 scripted-diff: Replace three dots with ellipsis in the ui strings (hebasto)
- tortoisecoin/tortoisecoin#21935 Enable external signer support for GUI builds (Sjors)
- tortoisecoin/tortoisecoin#22133 Make QWindowsVistaStylePlugin available again (regression) (hebasto)
- tortoisecoin-core/gui#4 UI external signer support (e.g. hardware wallet) (Sjors)
- tortoisecoin-core/gui#13 Hide peer detail view if multiple are selected (promag)
- tortoisecoin-core/gui#18 Add peertablesortproxy module (hebasto)
- tortoisecoin-core/gui#21 Improve pruning tooltip (fluffypony, TortoisecoinErrorLog)
- tortoisecoin-core/gui#72 Log static plugins meta data and used style (hebasto)
- tortoisecoin-core/gui#79 Embed monospaced font (hebasto)
- tortoisecoin-core/gui#85 Remove unused "What's This" button in dialogs on Windows OS (hebasto)
- tortoisecoin-core/gui#115 Replace "Hide tray icon" option with positive "Show tray icon" one (hebasto)
- tortoisecoin-core/gui#118 Remove BDB version from the Information tab (hebasto)
- tortoisecoin-core/gui#121 Early subscribe core signals in transaction table model (promag)
- tortoisecoin-core/gui#123 Do not accept command while executing another one (hebasto)
- tortoisecoin-core/gui#125 Enable changing the autoprune block space size in intro dialog (luke-jr)
- tortoisecoin-core/gui#138 Unlock encrypted wallet "OK" button bugfix (mjdietzx)
- tortoisecoin-core/gui#139 doc: Improve gui/src/qt README.md (jarolrod)
- tortoisecoin-core/gui#154 Support macOS Dark mode (goums, Uplab)
- tortoisecoin-core/gui#162 Add network to peers window and peer details (jonatack)
- tortoisecoin-core/gui#163, tortoisecoin-core/gui#180 Peer details: replace Direction with Connection Type (jonatack)
- tortoisecoin-core/gui#164 Handle peer addition/removal in a right way (hebasto)
- tortoisecoin-core/gui#165 Save QSplitter state in QSettings (hebasto)
- tortoisecoin-core/gui#173 Follow Qt docs when implementing rowCount and columnCount (hebasto)
- tortoisecoin-core/gui#179 Add Type column to peers window, update peer details name/tooltip (jonatack)
- tortoisecoin-core/gui#186 Add information to "Confirm fee bump" window (prayank23)
- tortoisecoin-core/gui#189 Drop workaround for QTBUG-42503 which was fixed in Qt 5.5.0 (prusnak)
- tortoisecoin-core/gui#194 Save/restore RPCConsole geometry only for window (hebasto)
- tortoisecoin-core/gui#202 Fix right panel toggle in peers tab (RandyMcMillan)
- tortoisecoin-core/gui#203 Display plain "Inbound" in peer details (jonatack)
- tortoisecoin-core/gui#204 Drop buggy TableViewLastColumnResizingFixer class (hebasto)
- tortoisecoin-core/gui#205, tortoisecoin-core/gui#229 Save/restore TransactionView and recentRequestsView tables column sizes (hebasto)
- tortoisecoin-core/gui#206 Display fRelayTxes and `bip152_highbandwidth_{to, from}` in peer details (jonatack)
- tortoisecoin-core/gui#213 Add Copy Address Action to Payment Requests (jarolrod)
- tortoisecoin-core/gui#214 Disable requests context menu actions when appropriate (jarolrod)
- tortoisecoin-core/gui#217 Make warning label look clickable (jarolrod)
- tortoisecoin-core/gui#219 Prevent the main window popup menu (hebasto)
- tortoisecoin-core/gui#220 Do not translate file extensions (hebasto)
- tortoisecoin-core/gui#221 RPCConsole translatable string fixes and improvements (jonatack)
- tortoisecoin-core/gui#226 Add "Last Block" and "Last Tx" rows to peer details area (jonatack)
- tortoisecoin-core/gui#233 qt test: Don't bind to regtest port (achow101)
- tortoisecoin-core/gui#243 Fix issue when disabling the auto-enabled blank wallet checkbox (jarolrod)
- tortoisecoin-core/gui#246 Revert "qt: Use "fusion" style on macOS Big Sur with old Qt" (hebasto)
- tortoisecoin-core/gui#248 For values of "Bytes transferred" and "Bytes/s" with 1000-based prefix names use 1000-based divisor instead of 1024-based (wodry)
- tortoisecoin-core/gui#251 Improve URI/file handling message (hebasto)
- tortoisecoin-core/gui#256 Save/restore column sizes of the tables in the Peers tab (hebasto)
- tortoisecoin-core/gui#260 Handle exceptions isntead of crash (hebasto)
- tortoisecoin-core/gui#263 Revamp context menus (hebasto)
- tortoisecoin-core/gui#271 Don't clear console prompt when font resizing (jarolrod)
- tortoisecoin-core/gui#275 Support runtime appearance adjustment on macOS (hebasto)
- tortoisecoin-core/gui#276 Elide long strings in their middle in the Peers tab (hebasto)
- tortoisecoin-core/gui#281 Set shortcuts for console's resize buttons (jarolrod)
- tortoisecoin-core/gui#293 Enable wordWrap for Services (RandyMcMillan)
- tortoisecoin-core/gui#296 Do not use QObject::tr plural syntax for numbers with a unit symbol (hebasto)
- tortoisecoin-core/gui#297 Avoid unnecessary translations (hebasto)
- tortoisecoin-core/gui#298 Peertableview alternating row colors (RandyMcMillan)
- tortoisecoin-core/gui#300 Remove progress bar on modal overlay (brunoerg)
- tortoisecoin-core/gui#309 Add access to the Peers tab from the network icon (hebasto)
- tortoisecoin-core/gui#311 Peers Window rename 'Peer id' to 'Peer' (jarolrod)
- tortoisecoin-core/gui#313 Optimize string concatenation by default (hebasto)
- tortoisecoin-core/gui#325 Align numbers in the "Peer Id" column to the right (hebasto)
- tortoisecoin-core/gui#329 Make console buttons look clickable (jarolrod)
- tortoisecoin-core/gui#330 Allow prompt icon to be colorized (jarolrod)
- tortoisecoin-core/gui#331 Make RPC console welcome message translation-friendly (hebasto)
- tortoisecoin-core/gui#332 Replace disambiguation strings with translator comments (hebasto)
- tortoisecoin-core/gui#335 test: Use QSignalSpy instead of QEventLoop (jarolrod)
- tortoisecoin-core/gui#343 Improve the GUI responsiveness when progress dialogs are used (hebasto)
- tortoisecoin-core/gui#361 Fix GUI segfault caused by tortoisecoin/tortoisecoin#22216 (ryanofsky)
- tortoisecoin-core/gui#362 Add keyboard shortcuts to context menus (luke-jr)
- tortoisecoin-core/gui#366 Dark Mode fixes/portability (luke-jr)
- tortoisecoin-core/gui#375 Emit dataChanged signal to dynamically re-sort Peers table (hebasto)
- tortoisecoin-core/gui#393 Fix regression in "Encrypt Wallet" menu item (hebasto)
- tortoisecoin-core/gui#396 Ensure external signer option remains disabled without signers (achow101)
- tortoisecoin-core/gui#406 Handle new added plurals in `tortoisecoin_en.ts` (hebasto)

### Build system
- tortoisecoin/tortoisecoin#17227 Add Android packaging support (icota)
- tortoisecoin/tortoisecoin#17920 guix: Build support for macOS (dongcarl)
- tortoisecoin/tortoisecoin#18298 Fix Qt processing of configure script for depends with DEBUG=1 (hebasto)
- tortoisecoin/tortoisecoin#19160 multiprocess: Add basic spawn and IPC support (ryanofsky)
- tortoisecoin/tortoisecoin#19504 Bump minimum python version to 3.6 (ajtowns)
- tortoisecoin/tortoisecoin#19522 fix building libconsensus with reduced exports for Darwin targets (fanquake)
- tortoisecoin/tortoisecoin#19683 Pin clang search paths for darwin host (dongcarl)
- tortoisecoin/tortoisecoin#19764 Split boost into build/host packages + bump + cleanup (dongcarl)
- tortoisecoin/tortoisecoin#19817 libtapi 1100.0.11 (fanquake)
- tortoisecoin/tortoisecoin#19846 enable unused member function diagnostic (Zero-1729)
- tortoisecoin/tortoisecoin#19867 Document and cleanup Qt hacks (fanquake)
- tortoisecoin/tortoisecoin#20046 Set `CMAKE_INSTALL_RPATH` for native packages (ryanofsky)
- tortoisecoin/tortoisecoin#20223 Drop the leading 0 from the version number (achow101)
- tortoisecoin/tortoisecoin#20333 Remove `native_biplist` dependency (fanquake)
- tortoisecoin/tortoisecoin#20353 configure: Support -fdebug-prefix-map and -fmacro-prefix-map (ajtowns)
- tortoisecoin/tortoisecoin#20359 Various config.site.in improvements and linting (dongcarl)
- tortoisecoin/tortoisecoin#20413 Require C++17 compiler (MarcoFalke)
- tortoisecoin/tortoisecoin#20419 Set minimum supported macOS to 10.14 (fanquake)
- tortoisecoin/tortoisecoin#20421 miniupnpc 2.2.2 (fanquake)
- tortoisecoin/tortoisecoin#20422 Mac deployment unification (fanquake)
- tortoisecoin/tortoisecoin#20424 Update univalue subtree (MarcoFalke)
- tortoisecoin/tortoisecoin#20449 Fix Windows installer build (achow101)
- tortoisecoin/tortoisecoin#20468 Warn when generating man pages for binaries built from a dirty branch (tylerchambers)
- tortoisecoin/tortoisecoin#20469 Avoid secp256k1.h include from system (dergoegge)
- tortoisecoin/tortoisecoin#20470 Replace genisoimage with xorriso (dongcarl)
- tortoisecoin/tortoisecoin#20471 Use C++17 in depends (fanquake)
- tortoisecoin/tortoisecoin#20496 Drop unneeded macOS framework dependencies (hebasto)
- tortoisecoin/tortoisecoin#20520 Do not force Precompiled Headers (PCH) for building Qt on Linux (hebasto)
- tortoisecoin/tortoisecoin#20549 Support make src/tortoisecoin-node and src/tortoisecoin-gui (promag)
- tortoisecoin/tortoisecoin#20565 Ensure PIC build for bdb on Android (BlockMechanic)
- tortoisecoin/tortoisecoin#20594 Fix getauxval calls in randomenv.cpp (jonasschnelli)
- tortoisecoin/tortoisecoin#20603 Update crc32c subtree (MarcoFalke)
- tortoisecoin/tortoisecoin#20609 configure: output notice that test binary is disabled by fuzzing (apoelstra)
- tortoisecoin/tortoisecoin#20619 guix: Quality of life improvements (dongcarl)
- tortoisecoin/tortoisecoin#20629 Improve id string robustness (dongcarl)
- tortoisecoin/tortoisecoin#20641 Use Qt top-level build facilities (hebasto)
- tortoisecoin/tortoisecoin#20650 Drop workaround for a fixed bug in Qt build system (hebasto)
- tortoisecoin/tortoisecoin#20673 Use more legible qmake commands in qt package (hebasto)
- tortoisecoin/tortoisecoin#20684 Define .INTERMEDIATE target once only (hebasto)
- tortoisecoin/tortoisecoin#20720 more robustly check for fcf-protection support (fanquake)
- tortoisecoin/tortoisecoin#20734 Make platform-specific targets available for proper platform builds only (hebasto)
- tortoisecoin/tortoisecoin#20936 build fuzz tests by default (danben)
- tortoisecoin/tortoisecoin#20937 guix: Make nsis reproducible by respecting SOURCE-DATE-EPOCH (dongcarl)
- tortoisecoin/tortoisecoin#20938 fix linking against -latomic when building for riscv (fanquake)
- tortoisecoin/tortoisecoin#20939 fix `RELOC_SECTION` security check for tortoisecoin-util (fanquake)
- tortoisecoin/tortoisecoin#20963 gitian-linux: Build binaries for 64-bit POWER (continued) (laanwj)
- tortoisecoin/tortoisecoin#21036 gitian: Bump descriptors to focal for 22.0 (fanquake)
- tortoisecoin/tortoisecoin#21045 Adds switch to enable/disable randomized base address in MSVC builds (EthanHeilman)
- tortoisecoin/tortoisecoin#21065 make macOS HOST in download-osx generic (fanquake)
- tortoisecoin/tortoisecoin#21078 guix: only download sources for hosts being built (fanquake)
- tortoisecoin/tortoisecoin#21116 Disable --disable-fuzz-binary for gitian/guix builds (hebasto)
- tortoisecoin/tortoisecoin#21182 remove mostly pointless `BOOST_PROCESS` macro (fanquake)
- tortoisecoin/tortoisecoin#21205 actually fail when Boost is missing (fanquake)
- tortoisecoin/tortoisecoin#21209 use newer source for libnatpmp (fanquake)
- tortoisecoin/tortoisecoin#21226 Fix fuzz binary compilation under windows (danben)
- tortoisecoin/tortoisecoin#21231 Add /opt/homebrew to path to look for boost libraries (fyquah)
- tortoisecoin/tortoisecoin#21239 guix: Add codesignature attachment support for osx+win (dongcarl)
- tortoisecoin/tortoisecoin#21250 Make `HAVE_O_CLOEXEC` available outside LevelDB (bugfix) (theStack)
- tortoisecoin/tortoisecoin#21272 guix: Passthrough `SDK_PATH` into container (dongcarl)
- tortoisecoin/tortoisecoin#21274 assumptions:  Assume C++17 (fanquake)
- tortoisecoin/tortoisecoin#21286 Bump minimum Qt version to 5.9.5 (hebasto)
- tortoisecoin/tortoisecoin#21298 guix: Bump time-machine, glibc, and linux-headers (dongcarl)
- tortoisecoin/tortoisecoin#21304 guix: Add guix-clean script + establish gc-root for container profiles (dongcarl)
- tortoisecoin/tortoisecoin#21320 fix libnatpmp macos cross compile (fanquake)
- tortoisecoin/tortoisecoin#21321 guix: Add curl to required tool list (hebasto)
- tortoisecoin/tortoisecoin#21333 set Unicode true for NSIS installer (fanquake)
- tortoisecoin/tortoisecoin#21339 Make `AM_CONDITIONAL([ENABLE_EXTERNAL_SIGNER])` unconditional (hebasto)
- tortoisecoin/tortoisecoin#21349 Fix fuzz-cuckoocache cross-compiling with DEBUG=1 (hebasto)
- tortoisecoin/tortoisecoin#21354 build, doc: Drop no longer required packages from macOS cross-compiling dependencies (hebasto)
- tortoisecoin/tortoisecoin#21363 build, qt: Improve Qt static plugins/libs check code (hebasto)
- tortoisecoin/tortoisecoin#21375 guix: Misc feedback-based fixes + hier restructuring (dongcarl)
- tortoisecoin/tortoisecoin#21376 Qt 5.12.10 (fanquake)
- tortoisecoin/tortoisecoin#21382 Clean remnants of QTBUG-34748 fix (hebasto)
- tortoisecoin/tortoisecoin#21400 Fix regression introduced in #21363 (hebasto)
- tortoisecoin/tortoisecoin#21403 set --build when configuring packages in depends (fanquake)
- tortoisecoin/tortoisecoin#21421 don't try and use -fstack-clash-protection on Windows (fanquake)
- tortoisecoin/tortoisecoin#21423 Cleanups and follow ups after bumping Qt to 5.12.10 (hebasto)
- tortoisecoin/tortoisecoin#21427 Fix `id_string` invocations (dongcarl)
- tortoisecoin/tortoisecoin#21430 Add -Werror=implicit-fallthrough compile flag (hebasto)
- tortoisecoin/tortoisecoin#21457 Split libtapi and clang out of `native_cctools` (fanquake)
- tortoisecoin/tortoisecoin#21462 guix: Add guix-{attest,verify} scripts (dongcarl)
- tortoisecoin/tortoisecoin#21495 build, qt: Fix static builds on macOS Big Sur (hebasto)
- tortoisecoin/tortoisecoin#21497 Do not opt-in unused CoreWLAN stuff in depends for macOS (hebasto)
- tortoisecoin/tortoisecoin#21543 Enable safe warnings for msvc builds (hebasto)
- tortoisecoin/tortoisecoin#21565 Make `tortoisecoin_qt.m4` more generic (fanquake)
- tortoisecoin/tortoisecoin#21610 remove -Wdeprecated-register from NOWARN flags (fanquake)
- tortoisecoin/tortoisecoin#21613 enable -Wdocumentation (fanquake)
- tortoisecoin/tortoisecoin#21629 Fix configuring when building depends with `NO_BDB=1` (fanquake)
- tortoisecoin/tortoisecoin#21654 build, qt: Make Qt rcc output always deterministic (hebasto)
- tortoisecoin/tortoisecoin#21655 build, qt: No longer need to set `QT_RCC_TEST=1` for determinism (hebasto)
- tortoisecoin/tortoisecoin#21658 fix make deploy for arm64-darwin (sgulls)
- tortoisecoin/tortoisecoin#21694 Use XLIFF file to provide more context to Transifex translators (hebasto)
- tortoisecoin/tortoisecoin#21708, tortoisecoin/tortoisecoin#21593 Drop pointless sed commands (hebasto)
- tortoisecoin/tortoisecoin#21731 Update msvc build to use Qt5.12.10 binaries (sipsorcery)
- tortoisecoin/tortoisecoin#21733 Re-add command to install vcpkg (dplusplus1024)
- tortoisecoin/tortoisecoin#21793 Use `-isysroot` over `--sysroot` on macOS (fanquake)
- tortoisecoin/tortoisecoin#21869 Add missing `-D_LIBCPP_DEBUG=1` to debug flags (MarcoFalke)
- tortoisecoin/tortoisecoin#21889 macho: check for control flow instrumentation (fanquake)
- tortoisecoin/tortoisecoin#21920 Improve macro for testing -latomic requirement (MarcoFalke)
- tortoisecoin/tortoisecoin#21991 libevent 2.1.12-stable (fanquake)
- tortoisecoin/tortoisecoin#22054 Bump Qt version to 5.12.11 (hebasto)
- tortoisecoin/tortoisecoin#22063 Use Qt archive of the same version as the compiled binaries (hebasto)
- tortoisecoin/tortoisecoin#22070 Don't use cf-protection when targeting arm-apple-darwin (fanquake)
- tortoisecoin/tortoisecoin#22071 Latest config.guess and config.sub (fanquake)
- tortoisecoin/tortoisecoin#22075 guix: Misc leftover usability improvements (dongcarl)
- tortoisecoin/tortoisecoin#22123 Fix qt.mk for mac arm64 (promag)
- tortoisecoin/tortoisecoin#22174 build, qt: Fix libraries linking order for Linux hosts (hebasto)
- tortoisecoin/tortoisecoin#22182 guix: Overhaul how guix-{attest,verify} works and hierarchy (dongcarl)
- tortoisecoin/tortoisecoin#22186 build, qt: Fix compiling qt package in depends with GCC 11 (hebasto)
- tortoisecoin/tortoisecoin#22199 macdeploy: minor fixups and simplifications (fanquake)
- tortoisecoin/tortoisecoin#22230 Fix MSVC linker /SubSystem option for tortoisecoin-qt.exe (hebasto)
- tortoisecoin/tortoisecoin#22234 Mark print-% target as phony (dgoncharov)
- tortoisecoin/tortoisecoin#22238 improve detection of eBPF support (fanquake)
- tortoisecoin/tortoisecoin#22258 Disable deprecated-copy warning only when external warnings are enabled (MarcoFalke)
- tortoisecoin/tortoisecoin#22320 set minimum required Boost to 1.64.0 (fanquake)
- tortoisecoin/tortoisecoin#22348 Fix cross build for Windows with Boost Process (hebasto)
- tortoisecoin/tortoisecoin#22365 guix: Avoid relying on newer symbols by rebasing our cross toolchains on older glibcs (dongcarl)
- tortoisecoin/tortoisecoin#22381 guix: Test security-check sanity before performing them (with macOS) (fanquake)
- tortoisecoin/tortoisecoin#22405 Remove --enable-glibc-back-compat from Guix build (fanquake)
- tortoisecoin/tortoisecoin#22406 Remove --enable-determinism configure option (fanquake)
- tortoisecoin/tortoisecoin#22410 Avoid GCC 7.1 ABI change warning in guix build (sipa)
- tortoisecoin/tortoisecoin#22436 use aarch64 Clang if cross-compiling for darwin on aarch64 (fanquake)
- tortoisecoin/tortoisecoin#22465 guix: Pin kernel-header version, time-machine to upstream 1.3.0 commit (dongcarl)
- tortoisecoin/tortoisecoin#22511 guix: Silence `getent(1)` invocation, doc fixups (dongcarl)
- tortoisecoin/tortoisecoin#22531 guix: Fixes to guix-{attest,verify} (achow101)
- tortoisecoin/tortoisecoin#22642 release: Release with separate sha256sums and sig files (dongcarl)
- tortoisecoin/tortoisecoin#22685 clientversion: No suffix `#if CLIENT_VERSION_IS_RELEASE` (dongcarl)
- tortoisecoin/tortoisecoin#22713 Fix build with Boost 1.77.0 (sizeofvoid)

### Tests and QA
- tortoisecoin/tortoisecoin#14604 Add test and refactor `feature_block.py` (sanket1729)
- tortoisecoin/tortoisecoin#17556 Change `feature_config_args.py` not to rely on strange regtest=0 behavior (ryanofsky)
- tortoisecoin/tortoisecoin#18795 wallet issue with orphaned rewards (domob1812)
- tortoisecoin/tortoisecoin#18847 compressor: Use a prevector in CompressScript serialization (jb55)
- tortoisecoin/tortoisecoin#19259 fuzz: Add fuzzing harness for LoadMempool(…) and DumpMempool(…) (practicalswift)
- tortoisecoin/tortoisecoin#19315 Allow outbound & block-relay-only connections in functional tests. (amitiuttarwar)
- tortoisecoin/tortoisecoin#19698 Apply strict verification flags for transaction tests and assert backwards compatibility (glozow)
- tortoisecoin/tortoisecoin#19801 Check for all possible `OP_CLTV` fail reasons in `feature_cltv.py` (BIP 65) (theStack)
- tortoisecoin/tortoisecoin#19893 Remove or explain syncwithvalidationinterfacequeue (MarcoFalke)
- tortoisecoin/tortoisecoin#19972 fuzz: Add fuzzing harness for node eviction logic (practicalswift)
- tortoisecoin/tortoisecoin#19982 Fix inconsistent lock order in `wallet_tests/CreateWallet` (hebasto)
- tortoisecoin/tortoisecoin#20000 Fix creation of "std::string"s with \0s (vasild)
- tortoisecoin/tortoisecoin#20047 Use `wait_for_{block,header}` helpers in `p2p_fingerprint.py` (theStack)
- tortoisecoin/tortoisecoin#20171 Add functional test `test_txid_inv_delay` (ariard)
- tortoisecoin/tortoisecoin#20189 Switch to BIP341's suggested scheme for outputs without script (sipa)
- tortoisecoin/tortoisecoin#20248 Fix length of R check in `key_signature_tests` (dgpv)
- tortoisecoin/tortoisecoin#20276, tortoisecoin/tortoisecoin#20385, tortoisecoin/tortoisecoin#20688, tortoisecoin/tortoisecoin#20692 Run various mempool tests even with wallet disabled (mjdietzx)
- tortoisecoin/tortoisecoin#20323 Create or use existing properly initialized NodeContexts (dongcarl)
- tortoisecoin/tortoisecoin#20354 Add `feature_taproot.py --previous_release` (MarcoFalke)
- tortoisecoin/tortoisecoin#20370 fuzz: Version handshake (MarcoFalke)
- tortoisecoin/tortoisecoin#20377 fuzz: Fill various small fuzzing gaps (practicalswift)
- tortoisecoin/tortoisecoin#20425 fuzz: Make CAddrMan fuzzing harness deterministic (practicalswift)
- tortoisecoin/tortoisecoin#20430 Sanitizers: Add suppression for unsigned-integer-overflow in libstdc++ (jonasschnelli)
- tortoisecoin/tortoisecoin#20437 fuzz: Avoid time-based "non-determinism" in fuzzing harnesses by using mocked GetTime() (practicalswift)
- tortoisecoin/tortoisecoin#20458 Add `is_bdb_compiled` helper (Sjors)
- tortoisecoin/tortoisecoin#20466 Fix intermittent `p2p_fingerprint` issue (MarcoFalke)
- tortoisecoin/tortoisecoin#20472 Add testing of ParseInt/ParseUInt edge cases with leading +/-/0:s (practicalswift)
- tortoisecoin/tortoisecoin#20507 sync: print proper lock order location when double lock is detected (vasild)
- tortoisecoin/tortoisecoin#20522 Fix sync issue in `disconnect_p2ps` (amitiuttarwar)
- tortoisecoin/tortoisecoin#20524 Move `MIN_VERSION_SUPPORTED` to p2p.py (jnewbery)
- tortoisecoin/tortoisecoin#20540 Fix `wallet_multiwallet` issue on windows (MarcoFalke)
- tortoisecoin/tortoisecoin#20560 fuzz: Link all targets once (MarcoFalke)
- tortoisecoin/tortoisecoin#20567 Add option to git-subtree-check to do full check, add help (laanwj)
- tortoisecoin/tortoisecoin#20569 Fix intermittent `wallet_multiwallet` issue with `got_loading_error` (MarcoFalke)
- tortoisecoin/tortoisecoin#20613 Use Popen.wait instead of RPC in `assert_start_raises_init_error` (MarcoFalke)
- tortoisecoin/tortoisecoin#20663 fuzz: Hide `script_assets_test_minimizer` (MarcoFalke)
- tortoisecoin/tortoisecoin#20674 fuzz: Call SendMessages after ProcessMessage to increase coverage (MarcoFalke)
- tortoisecoin/tortoisecoin#20683 Fix restart node race (MarcoFalke)
- tortoisecoin/tortoisecoin#20686 fuzz: replace CNode code with fuzz/util.h::ConsumeNode() (jonatack)
- tortoisecoin/tortoisecoin#20733 Inline non-member functions with body in fuzzing headers (pstratem)
- tortoisecoin/tortoisecoin#20737 Add missing assignment in `mempool_resurrect.py` (MarcoFalke)
- tortoisecoin/tortoisecoin#20745 Correct `epoll_ctl` data race suppression (hebasto)
- tortoisecoin/tortoisecoin#20748 Add race:SendZmqMessage tsan suppression (MarcoFalke)
- tortoisecoin/tortoisecoin#20760 Set correct nValue for multi-op-return policy check (MarcoFalke)
- tortoisecoin/tortoisecoin#20761 fuzz: Check that `NULL_DATA` is unspendable (MarcoFalke)
- tortoisecoin/tortoisecoin#20765 fuzz: Check that certain script TxoutType are nonstandard (mjdietzx)
- tortoisecoin/tortoisecoin#20772 fuzz: Bolster ExtractDestination(s) checks (mjdietzx)
- tortoisecoin/tortoisecoin#20789 fuzz: Rework strong and weak net enum fuzzing (MarcoFalke)
- tortoisecoin/tortoisecoin#20828 fuzz: Introduce CallOneOf helper to replace switch-case (MarcoFalke)
- tortoisecoin/tortoisecoin#20839 fuzz: Avoid extraneous copy of input data, using Span<> (MarcoFalke)
- tortoisecoin/tortoisecoin#20844 Add sanitizer suppressions for AMD EPYC CPUs (MarcoFalke)
- tortoisecoin/tortoisecoin#20857 Update documentation in `feature_csv_activation.py` (PiRK)
- tortoisecoin/tortoisecoin#20876 Replace getmempoolentry with testmempoolaccept in MiniWallet (MarcoFalke)
- tortoisecoin/tortoisecoin#20881 fuzz: net permission flags in net processing (MarcoFalke)
- tortoisecoin/tortoisecoin#20882 fuzz: Add missing muhash registration (MarcoFalke)
- tortoisecoin/tortoisecoin#20908 fuzz: Use mocktime in `process_message*` fuzz targets (MarcoFalke)
- tortoisecoin/tortoisecoin#20915 fuzz: Fail if message type is not fuzzed (MarcoFalke)
- tortoisecoin/tortoisecoin#20946 fuzz: Consolidate fuzzing TestingSetup initialization (dongcarl)
- tortoisecoin/tortoisecoin#20954 Declare `nodes` type `in test_framework.py` (kiminuo)
- tortoisecoin/tortoisecoin#20955 Fix `get_previous_releases.py` for aarch64 (MarcoFalke)
- tortoisecoin/tortoisecoin#20969 check that getblockfilter RPC fails without block filter index (theStack)
- tortoisecoin/tortoisecoin#20971 Work around libFuzzer deadlock (MarcoFalke)
- tortoisecoin/tortoisecoin#20993 Store subversion (user agent) as string in `msg_version` (theStack)
- tortoisecoin/tortoisecoin#20995 fuzz: Avoid initializing version to less than `MIN_PEER_PROTO_VERSION` (MarcoFalke)
- tortoisecoin/tortoisecoin#20998 Fix BlockToJsonVerbose benchmark (martinus)
- tortoisecoin/tortoisecoin#21003 Move MakeNoLogFileContext to `libtest_util`, and use it in bench (MarcoFalke)
- tortoisecoin/tortoisecoin#21008 Fix zmq test flakiness, improve speed (theStack)
- tortoisecoin/tortoisecoin#21023 fuzz: Disable shuffle when merge=1 (MarcoFalke)
- tortoisecoin/tortoisecoin#21037 fuzz: Avoid designated initialization (C++20) in fuzz tests (practicalswift)
- tortoisecoin/tortoisecoin#21042 doc, test: Improve `setup_clean_chain` documentation (fjahr)
- tortoisecoin/tortoisecoin#21080 fuzz: Configure check for main function (take 2) (MarcoFalke)
- tortoisecoin/tortoisecoin#21084 Fix timeout decrease in `feature_assumevalid` (brunoerg)
- tortoisecoin/tortoisecoin#21096 Re-add dead code detection (flack)
- tortoisecoin/tortoisecoin#21100 Remove unused function `xor_bytes` (theStack)
- tortoisecoin/tortoisecoin#21115 Fix Windows cross build (hebasto)
- tortoisecoin/tortoisecoin#21117 Remove `assert_blockchain_height` (MarcoFalke)
- tortoisecoin/tortoisecoin#21121 Small unit test improvements, including helper to make mempool transaction (amitiuttarwar)
- tortoisecoin/tortoisecoin#21124 Remove unnecessary assignment in bdb (brunoerg)
- tortoisecoin/tortoisecoin#21125 Change `BOOST_CHECK` to `BOOST_CHECK_EQUAL` for paths (kiminuo)
- tortoisecoin/tortoisecoin#21142, tortoisecoin/tortoisecoin#21512 fuzz: Add `tx_pool` fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#21165 Use mocktime in `test_seed_peers` (dhruv)
- tortoisecoin/tortoisecoin#21169 fuzz: Add RPC interface fuzzing. Increase fuzzing coverage from 65% to 70% (practicalswift)
- tortoisecoin/tortoisecoin#21170 bench: Add benchmark to write json into a string (martinus)
- tortoisecoin/tortoisecoin#21178 Run `mempool_reorg.py` even with wallet disabled (DariusParvin)
- tortoisecoin/tortoisecoin#21185 fuzz: Remove expensive and redundant muhash from crypto fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#21200 Speed up `rpc_blockchain.py` by removing miniwallet.generate() (MarcoFalke)
- tortoisecoin/tortoisecoin#21211 Move `P2WSH_OP_TRUE` to shared test library (MarcoFalke)
- tortoisecoin/tortoisecoin#21228 Avoid comparision of integers with different signs (jonasschnelli)
- tortoisecoin/tortoisecoin#21230 Fix `NODE_NETWORK_LIMITED_MIN_BLOCKS` disconnection (MarcoFalke)
- tortoisecoin/tortoisecoin#21252 Add missing wait for sync to `feature_blockfilterindex_prune` (MarcoFalke)
- tortoisecoin/tortoisecoin#21254 Avoid connecting to real network when running tests (MarcoFalke)
- tortoisecoin/tortoisecoin#21264 fuzz: Two scripted diff renames (MarcoFalke)
- tortoisecoin/tortoisecoin#21280 Bug fix in `transaction_tests` (glozow)
- tortoisecoin/tortoisecoin#21293 Replace accidentally placed bit-OR with logical-OR (hebasto)
- tortoisecoin/tortoisecoin#21297 `feature_blockfilterindex_prune.py` improvements (jonatack)
- tortoisecoin/tortoisecoin#21310 zmq test: fix sync-up by matching notification to generated block (theStack)
- tortoisecoin/tortoisecoin#21334 Additional BIP9 tests (Sjors)
- tortoisecoin/tortoisecoin#21338 Add functional test for anchors.dat (brunoerg)
- tortoisecoin/tortoisecoin#21345 Bring `p2p_leak.py` up to date (mzumsande)
- tortoisecoin/tortoisecoin#21357 Unconditionally check for fRelay field in test framework (jarolrod)
- tortoisecoin/tortoisecoin#21358 fuzz: Add missing include (`test/util/setup_common.h`) (MarcoFalke)
- tortoisecoin/tortoisecoin#21371 fuzz: fix gcc Woverloaded-virtual build warnings (jonatack)
- tortoisecoin/tortoisecoin#21373 Generate fewer blocks in `feature_nulldummy` to fix timeouts, speed up (jonatack)
- tortoisecoin/tortoisecoin#21390 Test improvements for UTXO set hash tests (fjahr)
- tortoisecoin/tortoisecoin#21410 increase `rpc_timeout` for fundrawtx `test_transaction_too_large` (jonatack)
- tortoisecoin/tortoisecoin#21411 add logging, reduce blocks, move `sync_all` in `wallet_` groups (jonatack)
- tortoisecoin/tortoisecoin#21438 Add ParseUInt8() test coverage (jonatack)
- tortoisecoin/tortoisecoin#21443 fuzz: Implement `fuzzed_dns_lookup_function` as a lambda (practicalswift)
- tortoisecoin/tortoisecoin#21445 cirrus: Use SSD cluster for speedup (MarcoFalke)
- tortoisecoin/tortoisecoin#21477 Add test for CNetAddr::ToString IPv6 address formatting (RFC 5952) (practicalswift)
- tortoisecoin/tortoisecoin#21487 fuzz: Use ConsumeWeakEnum in addrman for service flags (MarcoFalke)
- tortoisecoin/tortoisecoin#21488 Add ParseUInt16() unit test and fuzz coverage (jonatack)
- tortoisecoin/tortoisecoin#21491 test: remove duplicate assertions in util_tests (jonatack)
- tortoisecoin/tortoisecoin#21522 fuzz: Use PickValue where possible (MarcoFalke)
- tortoisecoin/tortoisecoin#21531 remove qt byteswap compattests (fanquake)
- tortoisecoin/tortoisecoin#21557 small cleanup in RPCNestedTests tests (fanquake)
- tortoisecoin/tortoisecoin#21586 Add missing suppression for signed-integer-overflow:txmempool.cpp (MarcoFalke)
- tortoisecoin/tortoisecoin#21592 Remove option to make TestChain100Setup non-deterministic (MarcoFalke)
- tortoisecoin/tortoisecoin#21597 Document `race:validation_chainstatemanager_tests` suppression (MarcoFalke)
- tortoisecoin/tortoisecoin#21599 Replace file level integer overflow suppression with function level suppression (practicalswift)
- tortoisecoin/tortoisecoin#21604 Document why no symbol names can be used for suppressions (MarcoFalke)
- tortoisecoin/tortoisecoin#21606 fuzz: Extend psbt fuzz target a bit (MarcoFalke)
- tortoisecoin/tortoisecoin#21617 fuzz: Fix uninitialized read in i2p test (MarcoFalke)
- tortoisecoin/tortoisecoin#21630 fuzz: split FuzzedSock interface and implementation (vasild)
- tortoisecoin/tortoisecoin#21634 Skip SQLite fsyncs while testing (achow101)
- tortoisecoin/tortoisecoin#21669 Remove spurious double lock tsan suppressions by bumping to clang-12 (MarcoFalke)
- tortoisecoin/tortoisecoin#21676 Use mocktime to avoid intermittent failure in `rpc_tests` (MarcoFalke)
- tortoisecoin/tortoisecoin#21677 fuzz: Avoid use of low file descriptor ids (which may be in use) in FuzzedSock (practicalswift)
- tortoisecoin/tortoisecoin#21678 Fix TestPotentialDeadLockDetected suppression (hebasto)
- tortoisecoin/tortoisecoin#21689 Remove intermittently failing and not very meaningful `BOOST_CHECK` in `cnetaddr_basic` (practicalswift)
- tortoisecoin/tortoisecoin#21691 Check that no versionbits are re-used (MarcoFalke)
- tortoisecoin/tortoisecoin#21707 Extend functional tests for addr relay (mzumsande)
- tortoisecoin/tortoisecoin#21712 Test default `include_mempool` value of gettxout (promag)
- tortoisecoin/tortoisecoin#21738 Use clang-12 for ASAN, Add missing suppression (MarcoFalke)
- tortoisecoin/tortoisecoin#21740 add new python linter to check file names and permissions (windsok)
- tortoisecoin/tortoisecoin#21749 Bump shellcheck version (hebasto)
- tortoisecoin/tortoisecoin#21754 Run `feature_cltv` with MiniWallet (MarcoFalke)
- tortoisecoin/tortoisecoin#21762 Speed up `mempool_spend_coinbase.py` (MarcoFalke)
- tortoisecoin/tortoisecoin#21773 fuzz: Ensure prevout is consensus-valid (MarcoFalke)
- tortoisecoin/tortoisecoin#21777 Fix `feature_notifications.py` intermittent issue (MarcoFalke)
- tortoisecoin/tortoisecoin#21785 Fix intermittent issue in `p2p_addr_relay.py` (MarcoFalke)
- tortoisecoin/tortoisecoin#21787 Fix off-by-ones in `rpc_fundrawtransaction` assertions (jonatack)
- tortoisecoin/tortoisecoin#21792 Fix intermittent issue in `p2p_segwit.py` (MarcoFalke)
- tortoisecoin/tortoisecoin#21795 fuzz: Terminate immediately if a fuzzing harness tries to perform a DNS lookup (belt and suspenders) (practicalswift)
- tortoisecoin/tortoisecoin#21798 fuzz: Create a block template in `tx_pool` targets (MarcoFalke)
- tortoisecoin/tortoisecoin#21804 Speed up `p2p_segwit.py` (jnewbery)
- tortoisecoin/tortoisecoin#21810 fuzz: Various RPC fuzzer follow-ups (practicalswift)
- tortoisecoin/tortoisecoin#21814 Fix `feature_config_args.py` intermittent issue (MarcoFalke)
- tortoisecoin/tortoisecoin#21821 Add missing test for empty P2WSH redeem (MarcoFalke)
- tortoisecoin/tortoisecoin#21822 Resolve bug in `interface_tortoisecoin_cli.py` (klementtan)
- tortoisecoin/tortoisecoin#21846 fuzz: Add `-fsanitize=integer` suppression needed for RPC fuzzer (`generateblock`) (practicalswift)
- tortoisecoin/tortoisecoin#21849 fuzz: Limit toxic test globals to their respective scope (MarcoFalke)
- tortoisecoin/tortoisecoin#21867 use MiniWallet for `p2p_blocksonly.py` (theStack)
- tortoisecoin/tortoisecoin#21873 minor fixes & improvements for files linter test (windsok)
- tortoisecoin/tortoisecoin#21874 fuzz: Add `WRITE_ALL_FUZZ_TARGETS_AND_ABORT` (MarcoFalke)
- tortoisecoin/tortoisecoin#21884 fuzz: Remove unused --enable-danger-fuzz-link-all option (MarcoFalke)
- tortoisecoin/tortoisecoin#21890 fuzz: Limit ParseISO8601DateTime fuzzing to 32-bit (MarcoFalke)
- tortoisecoin/tortoisecoin#21891 fuzz: Remove strprintf test cases that are known to fail (MarcoFalke)
- tortoisecoin/tortoisecoin#21892 fuzz: Avoid excessively large min fee rate in `tx_pool` (MarcoFalke)
- tortoisecoin/tortoisecoin#21895 Add TSA annotations to the WorkQueue class members (hebasto)
- tortoisecoin/tortoisecoin#21900 use MiniWallet for `feature_csv_activation.py` (theStack)
- tortoisecoin/tortoisecoin#21909 fuzz: Limit max insertions in timedata fuzz test (MarcoFalke)
- tortoisecoin/tortoisecoin#21922 fuzz: Avoid timeout in EncodeBase58 (MarcoFalke)
- tortoisecoin/tortoisecoin#21927 fuzz: Run const CScript member functions only once (MarcoFalke)
- tortoisecoin/tortoisecoin#21929 fuzz: Remove incorrect float round-trip serialization test (MarcoFalke)
- tortoisecoin/tortoisecoin#21936 fuzz: Terminate immediately if a fuzzing harness tries to create a TCP socket (belt and suspenders) (practicalswift)
- tortoisecoin/tortoisecoin#21941 fuzz: Call const member functions in addrman fuzz test only once (MarcoFalke)
- tortoisecoin/tortoisecoin#21945 add P2PK support to MiniWallet (theStack)
- tortoisecoin/tortoisecoin#21948 Fix off-by-one in mockscheduler test RPC (MarcoFalke)
- tortoisecoin/tortoisecoin#21953 fuzz: Add `utxo_snapshot` target (MarcoFalke)
- tortoisecoin/tortoisecoin#21970 fuzz: Add missing CheckTransaction before CheckTxInputs (MarcoFalke)
- tortoisecoin/tortoisecoin#21989 Use `COINBASE_MATURITY` in functional tests (kiminuo)
- tortoisecoin/tortoisecoin#22003 Add thread safety annotations (ajtowns)
- tortoisecoin/tortoisecoin#22004 fuzz: Speed up transaction fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#22005 fuzz: Speed up banman fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#22029 [fuzz] Improve transport deserialization fuzz test coverage (dhruv)
- tortoisecoin/tortoisecoin#22048 MiniWallet: introduce enum type for output mode (theStack)
- tortoisecoin/tortoisecoin#22057 use MiniWallet (P2PK mode) for `feature_dersig.py` (theStack)
- tortoisecoin/tortoisecoin#22065 Mark `CheckTxInputs` `[[nodiscard]]`. Avoid UUM in fuzzing harness `coins_view` (practicalswift)
- tortoisecoin/tortoisecoin#22069 fuzz: don't try and use fopencookie() when building for Android (fanquake)
- tortoisecoin/tortoisecoin#22082 update nanobench from release 4.0.0 to 4.3.4 (martinus)
- tortoisecoin/tortoisecoin#22086 remove BasicTestingSetup from unit tests that don't need it (fanquake)
- tortoisecoin/tortoisecoin#22089 MiniWallet: fix fee calculation for P2PK and check tx vsize (theStack)
- tortoisecoin/tortoisecoin#21107, tortoisecoin/tortoisecoin#22092 Convert documentation into type annotations (fanquake)
- tortoisecoin/tortoisecoin#22095 Additional BIP32 test vector for hardened derivation with leading zeros (kristapsk)
- tortoisecoin/tortoisecoin#22103 Fix IPv6 check on BSD systems (n-thumann)
- tortoisecoin/tortoisecoin#22118 check anchors.dat when node starts for the first time (brunoerg)
- tortoisecoin/tortoisecoin#22120 `p2p_invalid_block`: Check that a block rejected due to too-new tim… (willcl-ark)
- tortoisecoin/tortoisecoin#22153 Fix `p2p_leak.py` intermittent failure (mzumsande)
- tortoisecoin/tortoisecoin#22169 p2p, rpc, fuzz: various tiny follow-ups (jonatack)
- tortoisecoin/tortoisecoin#22176 Correct outstanding -Werror=sign-compare errors (Empact)
- tortoisecoin/tortoisecoin#22180 fuzz: Increase branch coverage of the float fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#22187 Add `sync_blocks` in `wallet_orphanedreward.py` (domob1812)
- tortoisecoin/tortoisecoin#22201 Fix TestShell to allow running in Jupyter Notebook (josibake)
- tortoisecoin/tortoisecoin#22202 Add temporary coinstats suppressions (MarcoFalke)
- tortoisecoin/tortoisecoin#22203 Use ConnmanTestMsg from test lib in `denialofservice_tests` (MarcoFalke)
- tortoisecoin/tortoisecoin#22210 Use MiniWallet in `test_no_inherited_signaling` RBF test (MarcoFalke)
- tortoisecoin/tortoisecoin#22224 Update msvc and appveyor builds to use Qt5.12.11 binaries (sipsorcery)
- tortoisecoin/tortoisecoin#22249 Kill process group to avoid dangling processes when using `--failfast` (S3RK)
- tortoisecoin/tortoisecoin#22267 fuzz: Speed up crypto fuzz target (MarcoFalke)
- tortoisecoin/tortoisecoin#22270 Add tortoisecoin-util tests (+refactors) (MarcoFalke)
- tortoisecoin/tortoisecoin#22271 fuzz: Assert roundtrip equality for `CPubKey` (theStack)
- tortoisecoin/tortoisecoin#22279 fuzz: add missing ECCVerifyHandle to `base_encode_decode` (apoelstra)
- tortoisecoin/tortoisecoin#22292 bench, doc: benchmarking updates and fixups (jonatack)
- tortoisecoin/tortoisecoin#22306 Improvements to `p2p_addr_relay.py` (amitiuttarwar)
- tortoisecoin/tortoisecoin#22310 Add functional test for replacement relay fee check (ariard)
- tortoisecoin/tortoisecoin#22311 Add missing syncwithvalidationinterfacequeue in `p2p_blockfilters` (MarcoFalke)
- tortoisecoin/tortoisecoin#22313 Add missing `sync_all` to `feature_coinstatsindex` (MarcoFalke)
- tortoisecoin/tortoisecoin#22322 fuzz: Check banman roundtrip (MarcoFalke)
- tortoisecoin/tortoisecoin#22363 Use `script_util` helpers for creating P2{PKH,SH,WPKH,WSH} scripts (theStack)
- tortoisecoin/tortoisecoin#22399 fuzz: Rework CTxDestination fuzzing (MarcoFalke)
- tortoisecoin/tortoisecoin#22408 add tests for `bad-txns-prevout-null` reject reason (theStack)
- tortoisecoin/tortoisecoin#22445 fuzz: Move implementations of non-template fuzz helpers from util.h to util.cpp (sriramdvt)
- tortoisecoin/tortoisecoin#22446 Fix `wallet_listdescriptors.py` if bdb is not compiled (hebasto)
- tortoisecoin/tortoisecoin#22447 Whitelist `rpc_rawtransaction` peers to speed up tests (jonatack)
- tortoisecoin/tortoisecoin#22742 Use proper target in `do_fund_send` (S3RK)

### Miscellaneous
- tortoisecoin/tortoisecoin#19337 sync: Detect double lock from the same thread (vasild)
- tortoisecoin/tortoisecoin#19809 log: Prefix log messages with function name and source code location if -logsourcelocations is set (practicalswift)
- tortoisecoin/tortoisecoin#19866 eBPF Linux tracepoints (jb55)
- tortoisecoin/tortoisecoin#20024 init: Fix incorrect warning "Reducing -maxconnections from N to N-1, because of system limitations" (practicalswift)
- tortoisecoin/tortoisecoin#20145 contrib: Add getcoins.py script to get coins from (signet) faucet (kallewoof)
- tortoisecoin/tortoisecoin#20255 util: Add assume() identity function (MarcoFalke)
- tortoisecoin/tortoisecoin#20288 script, doc: Contrib/seeds updates (jonatack)
- tortoisecoin/tortoisecoin#20358 src/randomenv.cpp: Fix build on uclibc (ffontaine)
- tortoisecoin/tortoisecoin#20406 util: Avoid invalid integer negation in formatmoney and valuefromamount (practicalswift)
- tortoisecoin/tortoisecoin#20434 contrib: Parse elf directly for symbol and security checks (laanwj)
- tortoisecoin/tortoisecoin#20451 lint: Run mypy over contrib/devtools (fanquake)
- tortoisecoin/tortoisecoin#20476 contrib: Add test for elf symbol-check (laanwj)
- tortoisecoin/tortoisecoin#20530 lint: Update cppcheck linter to c++17 and improve explicit usage (fjahr)
- tortoisecoin/tortoisecoin#20589 log: Clarify that failure to read/write `fee_estimates.dat` is non-fatal (MarcoFalke)
- tortoisecoin/tortoisecoin#20602 util: Allow use of c++14 chrono literals (MarcoFalke)
- tortoisecoin/tortoisecoin#20605 init: Signal-safe instant shutdown (laanwj)
- tortoisecoin/tortoisecoin#20608 contrib: Add symbol check test for PE binaries (fanquake)
- tortoisecoin/tortoisecoin#20689 contrib: Replace binary verification script verify.sh with python rewrite (theStack)
- tortoisecoin/tortoisecoin#20715 util: Add argsmanager::getcommand() and use it in tortoisecoin-wallet (MarcoFalke)
- tortoisecoin/tortoisecoin#20735 script: Remove outdated extract-osx-sdk.sh (hebasto)
- tortoisecoin/tortoisecoin#20817 lint: Update list of spelling linter false positives, bump to codespell 2.0.0 (theStack)
- tortoisecoin/tortoisecoin#20884 script: Improve robustness of tortoisecoind.service on startup (hebasto)
- tortoisecoin/tortoisecoin#20906 contrib: Embed c++11 patch in `install_db4.sh` (gruve-p)
- tortoisecoin/tortoisecoin#21004 contrib: Fix docker args conditional in gitian-build (setpill)
- tortoisecoin/tortoisecoin#21007 tortoisecoind: Add -daemonwait option to wait for initialization (laanwj)
- tortoisecoin/tortoisecoin#21041 log: Move "Pre-allocating up to position 0x[…] in […].dat" log message to debug category (practicalswift)
- tortoisecoin/tortoisecoin#21059 Drop boost/preprocessor dependencies (hebasto)
- tortoisecoin/tortoisecoin#21087 guix: Passthrough `BASE_CACHE` into container (dongcarl)
- tortoisecoin/tortoisecoin#21088 guix: Jump forwards in time-machine and adapt (dongcarl)
- tortoisecoin/tortoisecoin#21089 guix: Add support for powerpc64{,le} (dongcarl)
- tortoisecoin/tortoisecoin#21110 util: Remove boost `posix_time` usage from `gettime*` (fanquake)
- tortoisecoin/tortoisecoin#21111 Improve OpenRC initscript (parazyd)
- tortoisecoin/tortoisecoin#21123 code style: Add EditorConfig file (kiminuo)
- tortoisecoin/tortoisecoin#21173 util: Faster hexstr => 13% faster blocktojson (martinus)
- tortoisecoin/tortoisecoin#21221 tools: Allow argument/parameter bin packing in clang-format (jnewbery)
- tortoisecoin/tortoisecoin#21244 Move GetDataDir to ArgsManager (kiminuo)
- tortoisecoin/tortoisecoin#21255 contrib: Run test-symbol-check for risc-v (fanquake)
- tortoisecoin/tortoisecoin#21271 guix: Explicitly set umask in build container (dongcarl)
- tortoisecoin/tortoisecoin#21300 script: Add explanatory comment to tc.sh (dscotese)
- tortoisecoin/tortoisecoin#21317 util: Make assume() usable as unary expression (MarcoFalke)
- tortoisecoin/tortoisecoin#21336 Make .gitignore ignore src/test/fuzz/fuzz.exe (hebasto)
- tortoisecoin/tortoisecoin#21337 guix: Update darwin native packages dependencies (hebasto)
- tortoisecoin/tortoisecoin#21405 compat: remove memcpy -> memmove backwards compatibility alias (fanquake)
- tortoisecoin/tortoisecoin#21418 contrib: Make systemd invoke dependencies only when ready (laanwj)
- tortoisecoin/tortoisecoin#21447 Always add -daemonwait to known command line arguments (hebasto)
- tortoisecoin/tortoisecoin#21471 bugfix: Fix `bech32_encode` calls in `gen_key_io_test_vectors.py` (sipa)
- tortoisecoin/tortoisecoin#21615 script: Add trusted key for hebasto (hebasto)
- tortoisecoin/tortoisecoin#21664 contrib: Use lief for macos and windows symbol & security checks (fanquake)
- tortoisecoin/tortoisecoin#21695 contrib: Remove no longer used contrib/tortoisecoin-qt.pro (hebasto)
- tortoisecoin/tortoisecoin#21711 guix: Add full installation and usage documentation (dongcarl)
- tortoisecoin/tortoisecoin#21799 guix: Use `gcc-8` across the board (dongcarl)
- tortoisecoin/tortoisecoin#21802 Avoid UB in util/asmap (advance a dereferenceable iterator outside its valid range) (MarcoFalke)
- tortoisecoin/tortoisecoin#21823 script: Update reviewers (jonatack)
- tortoisecoin/tortoisecoin#21850 Remove `GetDataDir(net_specific)` function (kiminuo)
- tortoisecoin/tortoisecoin#21871 scripts: Add checks for minimum required os versions (fanquake)
- tortoisecoin/tortoisecoin#21966 Remove double serialization; use software encoder for fee estimation (sipa)
- tortoisecoin/tortoisecoin#22060 contrib: Add torv3 seed nodes for testnet, drop v2 ones (laanwj)
- tortoisecoin/tortoisecoin#22244 devtools: Correctly extract symbol versions in symbol-check (laanwj)
- tortoisecoin/tortoisecoin#22533 guix/build: Remove vestigial SKIPATTEST.TAG (dongcarl)
- tortoisecoin/tortoisecoin#22643 guix-verify: Non-zero exit code when anything fails (dongcarl)
- tortoisecoin/tortoisecoin#22654 guix: Don't include directory name in SHA256SUMS (achow101)

### Documentation
- tortoisecoin/tortoisecoin#15451 clarify getdata limit after #14897 (HashUnlimited)
- tortoisecoin/tortoisecoin#15545 Explain why CheckBlock() is called before AcceptBlock (Sjors)
- tortoisecoin/tortoisecoin#17350 Add developer documentation to isminetype (HAOYUatHZ)
- tortoisecoin/tortoisecoin#17934 Use `CONFIG_SITE` variable instead of --prefix option (hebasto)
- tortoisecoin/tortoisecoin#18030 Coin::IsSpent() can also mean never existed (Sjors)
- tortoisecoin/tortoisecoin#18096 IsFinalTx comment about nSequence & `OP_CLTV` (nothingmuch)
- tortoisecoin/tortoisecoin#18568 Clarify developer notes about constant naming (ryanofsky)
- tortoisecoin/tortoisecoin#19961 doc: tor.md updates (jonatack)
- tortoisecoin/tortoisecoin#19968 Clarify CRollingBloomFilter size estimate (robot-dreams)
- tortoisecoin/tortoisecoin#20200 Rename CODEOWNERS to REVIEWERS (adamjonas)
- tortoisecoin/tortoisecoin#20329 docs/descriptors.md: Remove hardened marker in the path after xpub (dgpv)
- tortoisecoin/tortoisecoin#20380 Add instructions on how to fuzz the P2P layer using Honggfuzz NetDriver (practicalswift)
- tortoisecoin/tortoisecoin#20414 Remove generated manual pages from master branch (laanwj)
- tortoisecoin/tortoisecoin#20473 Document current boost dependency as 1.71.0 (laanwj)
- tortoisecoin/tortoisecoin#20512 Add bash as an OpenBSD dependency (emilengler)
- tortoisecoin/tortoisecoin#20568 Use FeeModes doc helper in estimatesmartfee (MarcoFalke)
- tortoisecoin/tortoisecoin#20577 libconsensus: add missing error code description, fix NTortoisecoin link (theStack)
- tortoisecoin/tortoisecoin#20587 Tidy up Tor doc (more stringent) (wodry)
- tortoisecoin/tortoisecoin#20592 Update wtxidrelay documentation per BIP339 (jonatack)
- tortoisecoin/tortoisecoin#20601 Update for FreeBSD 12.2, add GUI Build Instructions (jarolrod)
- tortoisecoin/tortoisecoin#20635 fix misleading comment about call to non-existing function (pox)
- tortoisecoin/tortoisecoin#20646 Refer to BIPs 339/155 in feature negotiation (jonatack)
- tortoisecoin/tortoisecoin#20653 Move addr relay comment in net to correct place (MarcoFalke)
- tortoisecoin/tortoisecoin#20677 Remove shouty enums in `net_processing` comments (sdaftuar)
- tortoisecoin/tortoisecoin#20741 Update 'Secure string handling' (prayank23)
- tortoisecoin/tortoisecoin#20757 tor.md and -onlynet help updates (jonatack)
- tortoisecoin/tortoisecoin#20829 Add -netinfo help (jonatack)
- tortoisecoin/tortoisecoin#20830 Update developer notes with signet (jonatack)
- tortoisecoin/tortoisecoin#20890 Add explicit macdeployqtplus dependencies install step (hebasto)
- tortoisecoin/tortoisecoin#20913 Add manual page generation for tortoisecoin-util (laanwj)
- tortoisecoin/tortoisecoin#20985 Add xorriso to macOS depends packages (fanquake)
- tortoisecoin/tortoisecoin#20986 Update developer notes to discourage very long lines (jnewbery)
- tortoisecoin/tortoisecoin#20987 Add instructions for generating RPC docs (ben-kaufman)
- tortoisecoin/tortoisecoin#21026 Document use of make-tag script to make tags (laanwj)
- tortoisecoin/tortoisecoin#21028 doc/bips: Add BIPs 43, 44, 49, and 84 (luke-jr)
- tortoisecoin/tortoisecoin#21049 Add release notes for listdescriptors RPC (S3RK)
- tortoisecoin/tortoisecoin#21060 More precise -debug and -debugexclude doc (wodry)
- tortoisecoin/tortoisecoin#21077 Clarify -timeout and -peertimeout config options (glozow)
- tortoisecoin/tortoisecoin#21105 Correctly identify script type (niftynei)
- tortoisecoin/tortoisecoin#21163 Guix is shipped in Debian and Ubuntu (MarcoFalke)
- tortoisecoin/tortoisecoin#21210 Rework internal and external links (MarcoFalke)
- tortoisecoin/tortoisecoin#21246 Correction for VerifyTaprootCommitment comments (roconnor-blockstream)
- tortoisecoin/tortoisecoin#21263 Clarify that squashing should happen before review (MarcoFalke)
- tortoisecoin/tortoisecoin#21323 guix, doc: Update default HOSTS value (hebasto)
- tortoisecoin/tortoisecoin#21324 Update build instructions for Fedora (hebasto)
- tortoisecoin/tortoisecoin#21343 Revamp macOS build doc (jarolrod)
- tortoisecoin/tortoisecoin#21346 install qt5 when building on macOS (fanquake)
- tortoisecoin/tortoisecoin#21384 doc: add signet to tortoisecoin.conf documentation (jonatack)
- tortoisecoin/tortoisecoin#21394 Improve comment about protected peers (amitiuttarwar)
- tortoisecoin/tortoisecoin#21398 Update fuzzing docs for afl-clang-lto (MarcoFalke)
- tortoisecoin/tortoisecoin#21444 net, doc: Doxygen updates and fixes in netbase.{h,cpp} (jonatack)
- tortoisecoin/tortoisecoin#21481 Tell howto install clang-format on Debian/Ubuntu (wodry)
- tortoisecoin/tortoisecoin#21567 Fix various misleading comments (glozow)
- tortoisecoin/tortoisecoin#21661 Fix name of script guix-build (Emzy)
- tortoisecoin/tortoisecoin#21672 Remove boostrap info from `GUIX_COMMON_FLAGS` doc (fanquake)
- tortoisecoin/tortoisecoin#21688 Note on SDK for macOS depends cross-compile (jarolrod)
- tortoisecoin/tortoisecoin#21709 Update reduce-memory.md and tortoisecoin.conf -maxconnections info (jonatack)
- tortoisecoin/tortoisecoin#21710 update helps for addnode rpc and -addnode/-maxconnections config options (jonatack)
- tortoisecoin/tortoisecoin#21752 Clarify that feerates are per virtual size (MarcoFalke)
- tortoisecoin/tortoisecoin#21811 Remove Visual Studio 2017 reference from readme (sipsorcery)
- tortoisecoin/tortoisecoin#21818 Fixup -coinstatsindex help, update tortoisecoin.conf and files.md (jonatack)
- tortoisecoin/tortoisecoin#21856 add OSS-Fuzz section to fuzzing.md doc (adamjonas)
- tortoisecoin/tortoisecoin#21912 Remove mention of priority estimation (MarcoFalke)
- tortoisecoin/tortoisecoin#21925 Update bips.md for 0.21.1 (MarcoFalke)
- tortoisecoin/tortoisecoin#21942 improve make with parallel jobs description (klementtan)
- tortoisecoin/tortoisecoin#21947 Fix OSS-Fuzz links (MarcoFalke)
- tortoisecoin/tortoisecoin#21988 note that brew installed qt is not supported (jarolrod)
- tortoisecoin/tortoisecoin#22056 describe in fuzzing.md how to reproduce a CI crash (jonatack)
- tortoisecoin/tortoisecoin#22080 add maxuploadtarget to tortoisecoin.conf example (jarolrod)
- tortoisecoin/tortoisecoin#22088 Improve note on choosing posix mingw32 (jarolrod)
- tortoisecoin/tortoisecoin#22109 Fix external links (IRC, …) (MarcoFalke)
- tortoisecoin/tortoisecoin#22121 Various validation doc fixups (MarcoFalke)
- tortoisecoin/tortoisecoin#22172 Update tor.md, release notes with removal of tor v2 support (jonatack)
- tortoisecoin/tortoisecoin#22204 Remove obsolete `okSafeMode` RPC guideline from developer notes (theStack)
- tortoisecoin/tortoisecoin#22208 Update `REVIEWERS` (practicalswift)
- tortoisecoin/tortoisecoin#22250 add basic I2P documentation (vasild)
- tortoisecoin/tortoisecoin#22296 Final merge of release notes snippets, mv to wiki (MarcoFalke)
- tortoisecoin/tortoisecoin#22335 recommend `--disable-external-signer` in OpenBSD build guide (theStack)
- tortoisecoin/tortoisecoin#22339 Document minimum required libc++ version (hebasto)
- tortoisecoin/tortoisecoin#22349 Repository IRC updates (jonatack)
- tortoisecoin/tortoisecoin#22360 Remove unused section from release process (MarcoFalke)
- tortoisecoin/tortoisecoin#22369 Add steps for Transifex to release process (jonatack)
- tortoisecoin/tortoisecoin#22393 Added info to tortoisecoin.conf doc (bliotti)
- tortoisecoin/tortoisecoin#22402 Install Rosetta on M1-macOS for qt in depends (hebasto)
- tortoisecoin/tortoisecoin#22432 Fix incorrect `testmempoolaccept` doc (glozow)
- tortoisecoin/tortoisecoin#22648 doc, test: improve i2p/tor docs and i2p reachable unit tests (jonatack)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Adam Jonas
- amadeuszpawlik
- Amiti Uttarwar
- Andrew Chow
- Andrew Poelstra
- Anthony Towns
- Antoine Poinsot
- Antoine Riard
- apawlik
- apitko
- Ben Carman
- Ben Woosley
- benk10
- Bezdrighin
- Block Mechanic
- Brian Liotti
- Bruno Garcia
- Carl Dong
- Christian Decker
- coinforensics
- Cory Fields
- Dan Benjamin
- Daniel Kraft
- Darius Parvin
- Dhruv Mehta
- Dmitry Goncharov
- Dmitry Petukhov
- dplusplus1024
- dscotese
- Duncan Dean
- Elle Mouton
- Elliott Jin
- Emil Engler
- Ethan Heilman
- eugene
- Evan Klitzke
- Fabian Jahr
- Fabrice Fontaine
- fanquake
- fdov
- flack
- Fotis Koutoupas
- Fu Yong Quah
- fyquah
- glozow
- Gregory Sanders
- Guido Vranken
- Gunar C. Gessner
- h
- HAOYUatHZ
- Hennadii Stepanov
- Igor Cota
- Ikko Ashimine
- Ivan Metlushko
- jackielove4u
- James O'Beirne
- Jarol Rodriguez
- Joel Klabo
- John Newbery
- Jon Atack
- Jonas Schnelli
- João Barbosa
- Josiah Baker
- Karl-Johan Alm
- Kiminuo
- Klement Tan
- Kristaps Kaupe
- Larry Ruane
- lisa neigut
- Lucas Ontivero
- Luke Dashjr
- Maayan Keshet
- MarcoFalke
- Martin Ankerl
- Martin Zumsande
- Michael Dietz
- Michael Polzer
- Michael Tidwell
- Niklas Gögge
- nthumann
- Oliver Gugger
- parazyd
- Patrick Strateman
- Pavol Rusnak
- Peter Bushnell
- Pierre K
- Pieter Wuille
- PiRK
- pox
- practicalswift
- Prayank
- R E Broadley
- Rafael Sadowski
- randymcmillan
- Raul Siles
- Riccardo Spagni
- Russell O'Connor
- Russell Yanofsky
- S3RK
- saibato
- Samuel Dobson
- sanket1729
- Sawyer Billings
- Sebastian Falbesoner
- setpill
- sgulls
- sinetek
- Sjors Provoost
- Sriram
- Stephan Oeste
- Suhas Daftuar
- Sylvain Goumy
- t-bast
- Troy Giorshev
- Tushar Singla
- Tyler Chambers
- Uplab
- Vasil Dimov
- W. J. van der Laan
- willcl-ark
- William Bright
- William Casarin
- windsok
- wodry
- Yerzhan Mazhkenov
- Yuval Kogman
- Zero

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/tortoisecoin/tortoisecoin/).
