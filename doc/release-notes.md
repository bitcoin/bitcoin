0.20.0 Release Notes
====================

Bitcoin Core version 0.20.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.20.0/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

Known Bugs
==========

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in
this release:

- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `BITCOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============

P2P and network changes
-----------------------

#### Removal of BIP61 reject network messages from Bitcoin Core

The `-enablebip61` command line option to enable BIP61 has been removed.
(#17004)

This feature has been disabled by default since Bitcoin Core version 0.18.0.
Nodes on the network can not generally be trusted to send valid messages
(including reject messages), so this should only ever be used when
connected to a trusted node.  Please use the alternatives recommended
below if you rely on this removed feature:

- Testing or debugging of implementations of the Bitcoin P2P network protocol
  should be done by inspecting the log messages that are produced by a recent
  version of Bitcoin Core. Bitcoin Core logs debug messages
  (`-debug=<category>`) to a stream (`-printtoconsole`) or to a file
  (`-debuglogfile=<debug.log>`).

- Testing the validity of a block can be achieved by specific RPCs:

  - `submitblock`

  - `getblocktemplate` with `'mode'` set to `'proposal'` for blocks with
    potentially invalid POW

- Testing the validity of a transaction can be achieved by specific RPCs:

  - `sendrawtransaction`

  - `testmempoolaccept`

- Wallets should not assume a transaction has propagated to the network
  just because there are no reject messages.  Instead, listen for the
  transaction to be announced by other peers on the network.  Wallets
  should not assume a lack of reject messages means a transaction pays
  an appropriate fee.  Instead, set fees using fee estimation and use
  replace-by-fee to increase a transaction's fee if it hasn't confirmed
  within the desired amount of time.

The removal of BIP61 reject message support also has the following minor RPC
and logging implications:

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P reject
  code when a transaction is not accepted to the mempool. They still return the
  verbal reject reason.

- Log messages that previously reported the reject code when a transaction was
  not accepted to the mempool now no longer report the reject code. The reason
  for rejection is still reported.

Updated RPCs
------------

- The RPCs which accept descriptors now accept the new `sortedmulti(...)` descriptor
  type which supports multisig scripts where the public keys are sorted
  lexicographically in the resulting script.  (#17056)

- The `walletprocesspsbt` and `walletcreatefundedpsbt` RPCs now include
  BIP32 derivation paths by default for public keys if we know them.
  This can be disabled by setting the `bip32derivs` parameter to
  `false`.  (#17264)

- The `bumpfee` RPC's parameter `totalFee`, which was deprecated in
  0.19, has been removed.  (#18312)

- The `bumpfee` RPC will return a PSBT when used with wallets that have
  private keys disabled.  (#16373)

- The `getpeerinfo` RPC now includes a `mapped_as` field to indicate the
  mapped Autonomous System used for diversifying peer selection. See the
  `-asmap` configuration option described below in _New Settings_.  (#16702)

- The `createmultisig` and `addmultisigaddress` RPCs now return an
  output script descriptor for the newly created address.  (#18032)

Build System
------------

- OpenSSL is no longer used by Bitcoin Core.  (#17265)

- BIP70 support has been fully removed from Bitcoin Core. The
  `--enable-bip70` option remains, but it will throw an error during configure.
  (#17165)

- glibc 2.17 or greater is now required to run the release binaries. This
  retains compatibility with RHEL 7, CentOS 7, Debian 8 and Ubuntu 14.04 LTS. (#17538)

- The source code archives that are provided with gitian builds no longer contain
  any autotools artifacts. Therefore, to build from such source, a user
  should run the `./autogen.sh` script from the root of the unpacked archive.
  This implies that `autotools` and other required packages are installed on the
  user's system. (#18331)

New settings
------------

- New `rpcwhitelist` and `rpcwhitelistdefault` configuration parameters
  allow giving certain RPC users permissions to only some RPC calls.
  (#12763)

- A new `-asmap` configuration option has been added to diversify the
  node's network connections by mapping IP addresses Autonomous System
  Numbers (ASNs) and then limiting the number of connections made to any
  single ASN.  See [issue #16599](https://github.com/bitcoin/bitcoin/issues/16599),
  [PR #16702](https://github.com/bitcoin/bitcoin/pull/16702), and the
  `bitcoind help` for more information.  This option is experimental and
  subject to removal or breaking changes in future releases, so the
  legacy /16 prefix mapping of IP addresses remains the default.  (#16702)

Updated settings
----------------

- All custom settings configured when Bitcoin Core starts are now
  written to the `debug.log` file to assist troubleshooting.  (#16115)

- Importing blocks upon startup via the `bootstrap.dat` file no longer
  occurs by default. The file must now be specified with
  `-loadblock=<file>`.  (#17044)

- The `-debug=db` logging category has been renamed to
  `-debug=walletdb` to distinguish it from `coindb`.  The `-debug=db`
  option has been deprecated and will be removed in the next major
  release.  (#17410)

- The `-walletnotify` configuration parameter will now replace any `%w`
  in its argument with the name of the wallet generating the
  notification.  This is not supported on Windows. (#13339)

Removed settings
----------------

- The `-whitelistforcerelay` configuration parameter has been removed after
  it was discovered that it was rendered ineffective in version 0.13 and
  hasn't actually been supported for almost four years.  (#17985)

GUI changes
-----------

- The "Start Bitcoin Core on system login" option has been removed on macOS.
  (#17567)

- In the Peers window, the details for a peer now displays a `Mapped AS`
  field to indicate the mapped Autonomous System used for diversifying
  peer selection. See the `-asmap` configuration option in _New
  Settings_, above.  (#18402)

- A "known bug" [announced](https://bitcoincore.org/en/releases/0.18.0/#wallet-gui)
  in the release notes of version 0.18 has been fixed.  The issue
  affected anyone who simultaneously used multiple Bitcoin Core wallets
  and the GUI coin control feature. (#18894)

- For watch-only wallets, creating a new transaction in the Send screen
  or fee bumping an existing transaction in the Transactions screen will
  automatically copy a Partially-Signed Bitcoin Transaction (PSBT) to
  the system clipboard.  This can then be pasted into an external
  program such as [HWI](https://github.com/bitcoin-core/HWI) for
  signing.  Future versions of Bitcoin Core should support a GUI option
  for finalizing and broadcasting PSBTs, but for now the debug console
  may be used with the `finalizepsbt` and `sendrawtransaction` RPCs.
  (#16944, #17492)

Wallet
------

- The wallet now by default uses bech32 addresses when using RPC, and
  creates native segwit change outputs.  (#16884)

- The way that output trust was computed has been fixed, which affects
  confirmed/unconfirmed balance status and coin selection.  (#16766)

- The `gettransaction`, `listtransactions` and `listsinceblock` RPC
  responses now also include the height of the block that contains the
  wallet transaction, if any.  (#17437)

- The `getaddressinfo` RPC has had its `label` field deprecated
  (re-enable for this release using the configuration parameter
  `-deprecatedrpc=label`).  The `labels` field is altered from returning
  JSON objects to returning a JSON array of label names (re-enable
  previous behavior for this release using the configuration parameter
  `-deprecatedrpc=labelspurpose`).  Backwards compatibility using the
  deprecated configuration parameters is expected to be dropped in the
  0.21 release.  (#17585, #17578)

Documentation changes
---------------------

- Bitcoin Core's automatically-generated source code documentation is
  now available at https://doxygen.bitcoincore.org.  (#17596)

Low-level changes
=================

Utilities
---------

- The `bitcoin-cli` utility used with the `-getinfo` parameter now
  returns a `headers` field with the number of downloaded block headers
  on the best headers chain (similar to the `blocks` field that is also
  returned) and a `verificationprogress` field that estimates how much
  of the best block chain has been synced by the local node.  The
  information returned no longer includes the `protocolversion`,
  `walletversion`, and `keypoololdest` fields.  (#17302, #17650)

- The `bitcoin-cli` utility now accepts a `-stdinwalletpassphrase`
  parameter that can be used when calling the `walletpassphrase` and
  `walletpassphrasechange` RPCs to read the passphrase from standard
  input without echoing it to the terminal, improving security against
  anyone who can look at your screen.  The existing `-stdinrpcpass`
  parameter is also updated to not echo the passphrase. (#13716)

Command line
------------

- Command line options prefixed with main/test/regtest network names like
  `-main.port=8333` `-test.server=1` previously were allowed but ignored. Now
  they trigger "Invalid parameter" errors on startup. (#17482)

New RPCs
--------

- The `dumptxoutset` RPC outputs a serialized snapshot of the current
  UTXO set.  A script is provided in the `contrib/devtools` directory
  for generating a snapshot of the UTXO set at a particular block
  height.  (#16899)

- The `generatetodescriptor` RPC allows testers using regtest mode to
  generate blocks that pay an arbitrary output script descriptor.
  (#16943)

Updated RPCs
------------

- The `verifychain` RPC default values are now static instead of
  depending on the command line options or configuration file
  (`-checklevel`, and `-checkblocks`). Users can pass in the RPC
  arguments explicitly when they don't want to rely on the default
  values. (#18541)

- The `getblockchaininfo` RPC's `verificationprogress` field will no
  longer report values higher than 1.  Previously it would occasionally
  report the chain was more than 100% verified.  (#17328)

Tests
-----

- It is now an error to use an unqualified `walletdir=path` setting in
  the config file if running on testnet or regtest networks. The setting
  now needs to be qualified as `chain.walletdir=path` or placed in the
  appropriate `[chain]` section. (#17447)

- `-fallbackfee` was 0 (disabled) by default for the main chain, but
  0.0002 by default for the test chains. Now it is 0 by default for all
  chains. Testnet and regtest users will have to add
  `fallbackfee=0.0002` to their configuration if they weren't setting it
  and they want it to keep working like before. (#16524)

Build system
------------

- Support is provided for building with the Android Native Development
  Kit (NDK).  (#16110)

0.20.0 change log
=================

### Mining
- #18742 miner: Avoid stack-use-after-return in validationinterface (MarcoFalke)

### Block and transaction handling
- #15283 log: Fix UB with bench on genesis block (instagibbs)
- #16507 feefilter: Compute the absolute fee rather than stored rate (instagibbs)
- #16688 log: Add validation interface logging (jkczyz)
- #16805 log: Add timing information to FlushStateToDisk() (jamesob)
- #16902 O(1) `OP_IF/NOTIF/ELSE/ENDIF` script implementation (sipa)
- #16945 introduce CChainState::GetCoinsCacheSizeState (jamesob)
- #16974 Walk pindexBestHeader back to ChainActive().Tip() if it is invalid (TheBlueMatt)
- #17004 Remove REJECT code from CValidationState (jnewbery)
- #17080 Explain why `fCheckDuplicateInputs` can not be skipped and remove it (MarcoFalke)
- #17328 GuessVerificationProgress: cap the ratio to 1 (darosior)
- #17399 Templatize ValidationState instead of subclassing (jkczyz)
- #17407 node: Add reference to mempool in NodeContext (MarcoFalke)
- #17708 prevector: Avoid misaligned member accesses (ajtowns)
- #17850,#17896,#17957,#18021,#18021,#18112 Serialization improvements (sipa)
- #17925 Improve UpdateTransactionsFromBlock with Epochs (JeremyRubin)
- #18002 Abstract out script execution out of `VerifyWitnessProgram()` (sipa)
- #18388 Make VerifyWitnessProgram use a Span stack (sipa)
- #18433 serialization: prevent int overflow for big Coin::nHeight (pierreN)
- #18500 chainparams: Bump assumed valid hash (MarcoFalke)
- #18551 Do not clear validationinterface entries being executed (sipa)

### P2P protocol and network code
- #15437 Remove BIP61 reject messages (MarcoFalke)
- #16702 Supply and use asmap to improve IP bucketing in addrman (naumenkogs)
- #16851 Continue relaying transactions after they expire from mapRelay (ajtowns)
- #17164 Avoid allocating memory for addrKnown where we don't need it (naumenkogs)
- #17243 tools: add PoissonNextSend method that returns mockable time (amitiuttarwar)
- #17251 SocketHandler logs peer id for close and disconnect (Sjors)
- #17573 Seed RNG with precision timestamps on receipt of net messages (TheBlueMatt)
- #17624 Fix an uninitialized read in ProcessMessage(…, "tx", …) when receiving a transaction we already have (practicalswift)
- #17754 Don't allow resolving of std::string with embedded NUL characters. Add tests (practicalswift)
- #17758 Fix CNetAddr::IsRFC2544 comment + tests (tynes)
- #17812 config, net, test: Asmap feature refinements and functional tests (jonatack)
- #17951 Use rolling bloom filter of recent block txs for AlreadyHave() check (sdaftuar)
- #17985 Remove forcerelay of rejected txs (MarcoFalke)
- #18023 Fix some asmap issues (sipa)
- #18054 Reference instead of copy in BlockConnected range loop (jonatack)
- #18376 Fix use-after-free in tests (vasild)
- #18454 Make addr relay mockable, add test (MarcoFalke)
- #18458 Add missing `cs_vNodes` lock (MarcoFalke)
- #18506 Hardcoded seeds update for 0.20 (laanwj)
- #18808 Drop unknown types in getdata (jnewbery)
- #18962 Only send a getheaders for one block in an INV (jnewbery)

### Wallet
- #13339 Replace %w by wallet name in -walletnotify script (promag)
- #15931 Remove GetDepthInMainChain dependency on locked chain interface (ariard)
- #16373 bumpfee: Return PSBT when wallet has privkeys disabled (instagibbs)
- #16524 Disable -fallbackfee by default (jtimon)
- #16766 Make IsTrusted scan parents recursively (JeremyRubin)
- #16884 Change default address type to bech32 (instagibbs)
- #16911 Only check the hash of transactions loaded from disk (achow101)
- #16923 Handle duplicate fileid exception (promag)
- #17056 descriptors: Introduce sortedmulti descriptor (achow101)
- #17070 Avoid showing GUI popups on RPC errors (MarcoFalke)
- #17138 Remove wallet access to some node arguments (jnewbery)
- #17237 LearnRelatedScripts only if KeepDestination (promag)
- #17260 Split some CWallet functions into new LegacyScriptPubKeyMan (achow101)
- #17261 Make ScriptPubKeyMan an actual interface and the wallet to have multiple (achow101)
- #17290 Enable BnB coin selection for preset inputs and subtract fee from outputs (achow101)
- #17373 Various fixes and cleanup to keypool handling in LegacyScriptPubKeyMan and CWallet (achow101)
- #17410 Rename `db` log category to `walletdb` (like `coindb`) (laanwj)
- #17444 Avoid showing GUI popups on RPC errors (take 2) (MarcoFalke)
- #17447 Make -walletdir network only (promag)
- #17537 Cleanup and move opportunistic and superfluous TopUp()s (achow101)
- #17553 Remove out of date comments for CalculateMaximumSignedTxSize (instagibbs)
- #17568 Fix when sufficient preset inputs and subtractFeeFromOutputs (achow101)
- #17677 Activate watchonly wallet behavior for LegacySPKM only (instagibbs)
- #17719 Document better -keypool as a look-ahead safety mechanism (ariard)
- #17843 Reset reused transactions cache (fjahr)
- #17889 Improve CWallet:MarkDestinationsDirty (promag)
- #18034 Get the OutputType for a descriptor (achow101)
- #18067 Improve LegacyScriptPubKeyMan::CanProvide script recognition (ryanofsky)
- #18115 Pass in transactions and messages for signing instead of exporting the private keys (achow101)
- #18192,#18546 Bugfix: Wallet: Safely deal with change in the address book (luke-jr)
- #18204 descriptors: Improve descriptor cache and cache xpubs (achow101)
- #18274 rpc/wallet: Initialize nFeeRequired to avoid using garbage value on failure (kallewoof)
- #18312 Remove deprecated fee bumping by totalFee (jonatack)
- #18338 Fix wallet unload race condition (promag)

### RPC and other APIs
- #12763 Add RPC Whitelist Feature from #12248 (JeremyRubin)
- #13716 cli: `-stdinwalletpassphrase` and non-echo stdin passwords (kallewoof)
- #16689 Add missing fields to wallet rpc help output (ariard)
- #16821 Fix bug where duplicate PSBT keys are accepted (erasmospunk)
- #16899 UTXO snapshot creation (dumptxoutset)
- #17156 psbt: Check that various indexes and amounts are within bounds (achow101)
- #17264 Set default bip32derivs to true for psbt methods (Sjors)
- #17283 improve getaddressinfo test coverage, help, code docs (jonatack)
- #17302 cli: Add "headers" and "verificationprogress" to -getinfo (laanwj)
- #17318 replace asserts in RPC code with `CHECK_NONFATAL` and add linter (adamjonas)
- #17437 Expose block height of wallet transactions (promag)
- #17519 Remove unused `COINBASE_FLAGS` (narula)
- #17578 Simplify getaddressinfo labels, deprecate previous behavior (jonatack)
- #17585 deprecate getaddressinfo label (jonatack)
- #17746 Remove vector copy from listtransactions (promag)
- #17809 Auto-format RPCResult (MarcoFalke)
- #18032 Output a descriptor in createmultisig and addmultisigaddress (achow101)
- #18122 Update validateaddress RPCExamples to bech32 (theStack)
- #18208 Change RPCExamples to bech32 (yusufsahinhamza)
- #18268 Remove redundant types from descriptions (docallag)
- #18346 Document an RPCResult for all calls; Enforce at compile time (MarcoFalke)
- #18396 Add missing HelpExampleRpc for getblockfilter (theStack)
- #18398 Fix broken RPCExamples for waitforblock(height) (theStack)
- #18444 Remove final comma for last entry of fixed-size arrays/objects in RPCResult (luke-jr)
- #18459 Remove unused getbalances() code (jonatack)
- #18484 Correctly compute redeemScript from witnessScript for signrawtransaction (achow101)
- #18487 Fix rpcRunLater race in walletpassphrase (promag)
- #18499 Make rpc documentation not depend on call-time rpc args (MarcoFalke)
- #18532 Avoid initialization-order-fiasco on static CRPCCommand tables (MarcoFalke)
- #18541 Make verifychain default values static, not depend on global args (MarcoFalke)
- #18809 Do not advertise dumptxoutset as a way to flush the chainstate (MarcoFalke)
- #18814 Relock wallet only if most recent callback (promag)

### GUI
- #15023 Restore RPC Console to non-wallet tray icon menu (luke-jr)
- #15084 Don't disable the sync overlay when wallet is disabled (benthecarman)
- #15098 Show addresses for "SendToSelf" transactions (hebasto)
- #15756 Add shortcuts for tab tools (promag)
- #16944 create PSBT with watch-only wallet (Sjors)
- #16964 Change sendcoins dialogue Yes to Send (instagibbs)
- #17068 Always generate `bitcoinstrings.cpp` on `make translate` (D4nte)
- #17096 Rename debug window (Zero-1729)
- #17105 Make RPCConsole::TabTypes an enum class (promag)
- #17125 Add toolTip and placeholderText to sign message fields (dannmat)
- #17165 Remove BIP70 support (fanquake)
- #17180 Improved tooltip for send amount field (JeremyCrookshank)
- #17186 Add placeholder text to the sign message field (Danny-Scott)
- #17195 Send amount placeholder value (JeremyCrookshank)
- #17226 Fix payAmount tooltip in SendCoinsEntry (promag)
- #17360 Cleaning up hide button tool tip (Danny-Scott)
- #17446 Changed tooltip for 'Label' & 'Message' text fields to be more clear (dannmat)
- #17453 Fix intro dialog labels when the prune button is toggled (hebasto)
- #17474 Bugfix: GUI: Recognise `NETWORK_LIMITED` in formatServicesStr (luke-jr)
- #17492 Bump fee returns PSBT on clipboard for watchonly-only wallets (instagibbs)
- #17567 Remove macOS start on login code (fanquake)
- #17587 Show watch-only balance in send screen (Sjors)
- #17694 Disable 3rd-party tx-urls when wallet disabled (brakmic)
- #17696 Force set nPruneSize in QSettings after the intro dialog (hebasto)
- #17702 Move static placeholder texts to forms (laanwj)
- #17826 Log Qt related info (hebasto)
- #17886 Restore English translation option (achow101)
- #17906 Set CConnman byte counters earlier to avoid uninitialized reads (ryanofsky)
- #17935 Hide HD & encryption icons when no wallet loaded (brakmic)
- #17998 Shortcut to close ModalOverlay (emilengler)
- #18007 Bugfix: GUI: Hide the HD/encrypt icons earlier so they get re-shown if another wallet is open (luke-jr)
- #18060 Drop PeerTableModel dependency to ClientModel (promag)
- #18062 Fix unintialized WalletView::progressDialog (promag)
- #18091 Pass clientmodel changes from walletframe to walletviews (jonasschnelli)
- #18101 Fix deprecated QCharRef usage (hebasto)
- #18121 Throttle GUI update pace when -reindex (hebasto)
- #18123 Fix race in WalletModel::pollBalanceChanged (ryanofsky)
- #18160 Avoid Wallet::GetBalance in WalletModel::pollBalanceChanged (promag)
- #18360 Bump transifex slug and update English translations for 0.20 (laanwj)
- #18402 Display mapped AS in peers info window (jonatack)
- #18492 Translations update pre-branch (laanwj)
- #18549 Fix Window -> Minimize menu item (hebasto)
- #18578 Fix leak in CoinControlDialog::updateView (promag)
- #18894 Fix manual coin control with multiple wallets loaded (promag)

### Build system
- #16667 Remove mingw linker workaround from win gitian descriptor (fanquake)
- #16669 Use new fork of osslsigncode for windows gitian signing (fanquake)
- #16949 Only pass --disable-dependency-tracking to packages that understand it (fanquake)
- #17008 Bump libevent to 2.1.11 in depends (stefanwouldgo)
- #17029 gitian: Various improvements for windows descriptor (dongcarl)
- #17033 Disable _FORTIFY_SOURCE when enable-debug (achow101)
- #17057 Switch to upstream libdmg-hfsplus (fanquake)
- #17066 Remove workaround for ancient libtool (hebasto)
- #17074 Added double quotes (mztriz)
- #17087 Add variable printing target to Makefiles (dongcarl)
- #17118 depends macOS: point --sysroot to SDK (Sjors)
- #17231 Fix boost mac cross build with clang 9+ (theuni)
- #17265 Remove OpenSSL (fanquake)
- #17284 Update retry to current version (RandyMcMillan)
- #17308 nsis: Write to correct filename in first place (dongcarl)
- #17324,#18099 Update univalue subtree (MarcoFalke)
- #17398 Update leveldb to 1.22+ (laanwj)
- #17409 Avoid hardcoded libfaketime dir in gitian (MarcoFalke)
- #17466 Fix C{,XX} pickup (dongcarl)
- #17483 Set gitian arch back to amd64 (MarcoFalke)
- #17486 Make Travis catch unused variables (Sjors)
- #17538 Bump minimum libc to 2.17 for release binaries (fanquake)
- #17542 Create test utility library from src/test/util/ (brakmic)
- #17545 Remove libanl.so.1 from ALLOWED_LIBRARIES (fanquake)
- #17547 Fix configure report about qr (hebasto)
- #17569 Allow export of environ symbols and work around rv64 toolchain issue (laanwj)
- #17647 lcov: filter depends from coverage reports (nijynot)
- #17658 Add ability to skip building qrencode (fanquake)
- #17678 Support for S390X and POWER targets (MarcoFalke)
- #17682 util: Update tinyformat to upstream (laanwj)
- #17698 Don't configure `xcb_proto` (fanquake)
- #17730 Remove Qt networking features (fanquake)
- #17738 Remove linking librt for backwards compatibility (fanquake)
- #17740 Remove configure checks for win libraries we don't link against (fanquake)
- #17741 Included `test_bitcoin-qt` in msvc build (sipsorcery)
- #17756 Remove `WINDOWS_BITS` from build system (fanquake)
- #17769 Set `AC_PREREQ` to 2.69 (fanquake)
- #17880 Add -Wdate-time to Werror flags (fanquake)
- #17910 Remove double `LIBBITCOIN_SERVER` linking (fanquake)
- #17928 Consistent use of package variable (Bushstar)
- #17933 guix: Pin Guix using `guix time-machine` (dongcarl)
- #17948 pass -fno-ident in Windows gitian descriptor (fanquake)
- #18003 Remove --large-address-aware linker flag (fanquake)
- #18004 Don't embed a build-id when building libdmg-hfsplus (fanquake)
- #18051 Fix behavior when `ALLOW_HOST_PACKAGES` unset (hebasto)
- #18059 Add missing attributes to Win installer (fanquake)
- #18104 Skip i686 build by default in guix and gitian (MarcoFalke)
- #18107 Add `cov_fuzz` target (MarcoFalke)
- #18135 Add --enable-determinism configure flag (fanquake)
- #18145 Add Wreturn-type to Werror flags, check on more Travis machines (Sjors)
- #18264 Remove Boost Chrono (fanquake)
- #18290 Set minimum Automake version to 1.13 (hebasto)
- #18320 guix: Remove now-unnecessary gcc make flag (dongcarl)
- #18331 Use git archive as source tarball (hebasto)
- #18397 Fix libevent linking for `bench_bitcoin` binary (hebasto)
- #18426 scripts: `Previous_release`: improve behaviour on failed download (theStack)
- #18429 Remove double `LIBBITCOIN_SERVER` from bench-Makefile (brakmic)
- #18528 Create `test_fuzz` library from src/test/fuzz/fuzz.cpp (brakmic)
- #18558 Fix boost detection for arch armv7l (hebasto)
- #18598 gitian: Add missing automake package to gitian-win-signer.yml (achow101)
- #18676 Check libevent minimum version in configure script (hebasto)
- #18945 Ensure source tarball has leading directory name (laanwj)

### Platform support
- #16110 Add Android NDK support (icota)
- #16392 macOS toolchain update (fanquake)
- #16569 Increase init file stop timeout (setpill)
- #17151 Remove OpenSSL PRNG seeding (Windows, Qt only) (fanquake)
- #17365 Update README.md with working Android targets and API levels (icota)
- #17521 Only use D-Bus with Qt on linux (fanquake)
- #17550 Set minimum supported macOS to 10.12 (fanquake)
- #17592 Appveyor install libevent[thread] vcpkg (sipsorcery)
- #17660 Remove deprecated key from macOS Info.plist (fanquake)
- #17663 Pass `-dead_strip_dylibs` to ld on macOS (fanquake)
- #17676 Don't use OpenGL in Qt on macOS (fanquake)
- #17686 Add `-bind_at_load` to macOS hardened LDFLAGS (fanquake)
- #17787 scripts: Add macho pie check to security-check.py (fanquake)
- #17800 random: don't special case clock usage on macOS (fanquake)
- #17863 scripts: Add macho dylib checks to symbol-check.py (fanquake)
- #17899 msvc: Ignore msvc linker warning and update to msvc build instructions (sipsorcery)
- #17916 windows: Enable heap terminate-on-corruption (fanquake)
- #18082 logging: Enable `thread_local` usage on macos (fanquake)
- #18108 Fix `.gitignore` policy in `build_msvc` directory (hebasto)
- #18295 scripts: Add macho lazy bindings check to security-check.py (fanquake)
- #18358 util: Fix compilation with mingw-w64 7.0.0 (fanquake)
- #18359 Fix sysctl() detection on macOS (fanquake)
- #18364 random: remove getentropy() fallback for macOS < 10.12 (fanquake)
- #18395 scripts: Add pe dylib checking to symbol-check.py (fanquake)
- #18415 scripts: Add macho tests to test-security-check.py (fanquake)
- #18425 releases: Update with new Windows code signing certificate (achow101)
- #18702 Fix ASLR for bitcoin-cli on Windows (fanquake)

### Tests and QA
- #12134 Build previous releases and run functional tests (Sjors)
- #13693 Add coverage to estimaterawfee and estimatesmartfee (Empact)
- #13728 lint: Run the ci lint stage on mac (Empact)
- #15443 Add getdescriptorinfo functional test (promag)
- #15888 Add `wallet_implicitsegwit` to test the ability to transform keys between address types (luke-jr)
- #16540 Add `ASSERT_DEBUG_LOG` to unit test framework (MarcoFalke)
- #16597 travis: Run full test suite on native macos (Sjors)
- #16681 Use self.chain instead of 'regtest' in all current tests (jtimon)
- #16786 add unit test for wallet watch-only methods involving PubKeys (theStack)
- #16943 Add generatetodescriptor RPC (MarcoFalke)
- #16973 Fix `combine_logs.py` for AppVeyor build (mzumsande)
- #16975 Show debug log on unit test failure (MarcoFalke)
- #16978 Seed test RNG context for each test case, print seed (MarcoFalke)
- #17009, #17018, #17050, #17051, #17071, #17076, #17083, #17093, #17109, #17113, #17136, #17229, #17291, #17357, #17771, #17777, #17917, #17926, #17972, #17989, #17996, #18009, #18029, #18047, #18126, #18176, #18206, #18353, #18363, #18407, #18417, #18423, #18445, #18455, #18565 Add fuzzing harnesses (practicalswift)
- #17011 ci: Use busybox utils for one build (MarcoFalke)
- #17030 Fix Python Docstring to include all Args (jbampton)
- #17041 ci: Run tests on arm (MarcoFalke)
- #17069 Pass fuzzing inputs as constant references (practicalswift)
- #17091 Add test for loadblock option and linearize scripts (fjahr)
- #17108 fix "tx-size-small" errors after default address change (theStack)
- #17121 Speed up `wallet_backup` by whitelisting peers (immediate tx relay) (theStack)
- #17124 Speed up `wallet_address_types` by whitelisting peers (immediate tx relay) (theStack)
- #17140 Fix bug in `blockfilter_index_tests` (jimpo)
- #17199 use default address type (bech32) for `wallet_bumpfee` tests (theStack)
- #17205 ci: Enable address sanitizer (asan) stack-use-after-return checking (practicalswift)
- #17206 Add testcase to simulate bitcoin schema in leveldb (adamjonas)
- #17209 Remove no longer needed UBSan suppressions (issues fixed). Add documentation (practicalswift)
- #17220 Add unit testing for the CompressScript function (adamjonas)
- #17225 Test serialisation as part of deserialisation fuzzing. Test round-trip equality where possible (practicalswift)
- #17228 Add RegTestingSetup to `setup_common` (MarcoFalke)
- #17233 travis: Run unit and functional tests on native arm (MarcoFalke)
- #17235 Skip unnecessary fuzzer initialisation. Hold ECCVerifyHandle only when needed (practicalswift)
- #17240 ci: Disable functional tests on mac host (MarcoFalke)
- #17254 Fix `script_p2sh_tests` `OP_PUSHBACK2/4` missing (adamjonas)
- #17267 bench: Fix negative values and zero for -evals flag (nijynot)
- #17275 pubkey: Assert CPubKey's ECCVerifyHandle precondition (practicalswift)
- #17288 Added TestWrapper class for interactive Python environments (jachiang)
- #17292 Add new mempool benchmarks for a complex pool (JeremyRubin)
- #17299 add reason checks for non-standard txs in `test_IsStandard` (theStack)
- #17322 Fix input size assertion in `wallet_bumpfee.py` (instagibbs)
- #17327 Add `rpc_fundrawtransaction` logging (jonatack)
- #17330 Add `shrinkdebugfile=0` to regtest bitcoin.conf (sdaftuar)
- #17340 Speed up fundrawtransaction test (jnewbery)
- #17345 Do not instantiate CAddrDB for static call CAddrDB::Read() (hebasto)
- #17362 Speed up `wallet_avoidreuse`, add logging (jonatack)
- #17363 add "diamond" unit test to MempoolAncestryTests (theStack)
- #17366 Reset global args between test suites (MarcoFalke)
- #17367 ci: Run non-cross-compile builds natively (MarcoFalke)
- #17378 TestShell: Fix typos & implement cleanups (jachiang)
- #17384 Create new test library (MarcoFalke)
- #17387 `wallet_importmulti`: use addresses of the same type as being imported (achow101)
- #17388 Add missing newline in `util_ChainMerge` test (ryanofsky)
- #17390 Add `util_ArgParsing` test (ryanofsky)
- #17420 travis: Rework `cache_err_msg` (MarcoFalke)
- #17423 ci: Make ci system read-only on the git work tree (MarcoFalke)
- #17435 check custom ancestor limit in `mempool_packages.py` (theStack)
- #17455 Update valgrind suppressions (practicalswift)
- #17461 Check custom descendant limit in `mempool_packages.py` (theStack)
- #17469 Remove fragile `assert_memory_usage_stable` (MarcoFalke)
- #17470 ci: Use clang-8 for fuzzing to run on aarch64 ci systems (MarcoFalke)
- #17480 Add unit test for non-standard txs with too large scriptSig (theStack)
- #17497 Skip tests when utils haven't been compiled (fanquake)
- #17502 Add unit test for non-standard bare multisig txs (theStack)
- #17511 Add bounds checks before base58 decoding (sipa)
- #17517 ci: Bump to clang-8 for asan build to avoid segfaults on ppc64le (MarcoFalke)
- #17522 Wait until mempool is loaded in `wallet_abandonconflict` (MarcoFalke)
- #17532 Add functional test for non-standard txs with too large scriptSig (theStack)
- #17541 Add functional test for non-standard bare multisig txs (theStack)
- #17555 Add unit test for non-standard txs with wrong nVersion (dspicher)
- #17571 Add `libtest_util` library to msvc build configuration (sipsorcery)
- #17591 ci: Add big endian platform - s390x (elichai)
- #17593 Move more utility functions into test utility library (mzumsande)
- #17633 Add option --valgrind to run the functional tests under Valgrind (practicalswift)
- #17635 ci: Add centos 7 build (hebasto)
- #17641 Add unit test for leveldb creation with unicode path (sipsorcery)
- #17674 Add initialization order fiasco detection in Travis (practicalswift)
- #17675 Enable tests which are incorrectly skipped when running `test_runner.py --usecli` (practicalswift)
- #17685 Fix bug in the descriptor parsing fuzzing harness (`descriptor_parse`) (practicalswift)
- #17705 re-enable CLI test support by using EncodeDecimal in json.dumps() (fanquake)
- #17720 add unit test for non-standard "scriptsig-not-pushonly" txs (theStack)
- #17767 ci: Fix qemu issues (MarcoFalke)
- #17793 ci: Update github actions ci vcpkg cache on msbuild update (hebasto)
- #17806 Change filemode of `rpc_whitelist.py` (emilengler)
- #17849 ci: Fix brew python link (hebasto)
- #17851 Add `std::to_string` to list of locale dependent functions (practicalswift)
- #17893 Fix double-negative arg test (hebasto)
- #17900 ci: Combine 32-bit build with centos 7 build (theStack)
- #17921 Test `OP_CSV` empty stack fail in `feature_csv_activation.py` (theStack)
- #17931 Fix `p2p_invalid_messages` failing in Python 3.8 because of warning (elichai)
- #17947 add unit test for non-standard txs with too large tx size (theStack)
- #17959 Check specific reject reasons in `feature_csv_activation.py` (theStack)
- #17984 Add p2p test for forcerelay permission (MarcoFalke)
- #18001 Updated appveyor job to checkout a specific vcpkg commit ID (sipsorcery)
- #18008 fix fuzzing using libFuzzer on macOS (fanquake)
- #18013 bench: Fix benchmarks filters (elichai)
- #18018 reset fIsBareMultisigStd after bare-multisig tests (fanquake)
- #18022 Fix appveyor `test_bitcoin` build of `*.raw` (MarcoFalke)
- #18037 util: Allow scheduler to be mocked (amitiuttarwar)
- #18056 ci: Check for submodules (emilengler)
- #18069 Replace 'regtest' leftovers by self.chain (theStack)
- #18081 Set a name for CI Docker containers (fanquake)
- #18109 Avoid hitting some known minor tinyformat issues when fuzzing strprintf(…) (practicalswift)
- #18155 Add harness which fuzzes EvalScript and VerifyScript using a fuzzed signature checker (practicalswift)
- #18159 Add --valgrind option to `test/fuzz/test_runner.py` for running fuzzing test cases under valgrind (practicalswift)
- #18166 ci: Run fuzz testing test cases (bitcoin-core/qa-assets) under valgrind to catch memory errors (practicalswift)
- #18172 Transaction expiry from mempool (0xB10C)
- #18181 Remove incorrect assumptions in `validation_flush_tests` (MarcoFalke)
- #18183 Set `catch_system_errors=no` on boost unit tests (MarcoFalke)
- #18195 Add `cost_of_change` parameter assertions to `bnb_search_test` (yancyribbens)
- #18209 Reduce unneeded whitelist permissions in tests (MarcoFalke)
- #18211 Disable mockforward scheduler unit test for now (MarcoFalke)
- #18213 Fix race in `p2p_segwit` (MarcoFalke)
- #18224 Make AnalyzePSBT next role calculation simple, correct (instagibbs)
- #18228 Add missing syncwithvalidationinterfacequeue (MarcoFalke)
- #18247 Wait for both veracks in `add_p2p_connection` (MarcoFalke)
- #18249 Bump timeouts to accomodate really slow disks (MarcoFalke)
- #18255 Add `bad-txns-*-toolarge` test cases to `invalid_txs` (MarcoFalke)
- #18263 rpc: change setmocktime check to use IsMockableChain (gzhao408)
- #18285 Check that `wait_until` returns if time point is in the past (MarcoFalke)
- #18286 Add locale fuzzer to `FUZZERS_MISSING_CORPORA` (practicalswift)
- #18292 fuzz: Add `assert(script == decompressed_script)` (MarcoFalke)
- #18299 Update `FUZZERS_MISSING_CORPORA` to enable regression fuzzing for all harnesses in master (practicalswift)
- #18300 fuzz: Add option to merge input dir to test runner (MarcoFalke)
- #18305 Explain why test logging should be used (MarcoFalke)
- #18306 Add logging to `wallet_listsinceblock.py` (jonatack)
- #18311 Bumpfee test fix (instagibbs)
- #18314 Add deserialization fuzzing of SnapshotMetadata (`utxo_snapshot`) (practicalswift)
- #18319 fuzz: Add missing `ECC_Start` to `key_io` test (MarcoFalke)
- #18334 Add basic test for BIP 37 (MarcoFalke)
- #18350 Fix mining to an invalid target + ensure that a new block has the correct hash internally (TheQuantumPhysicist)
- #18378 Bugfix & simplify bn2vch using `int.to_bytes` (sipa)
- #18393 Don't assume presence of `__builtin_mul_overflow(…)` in `MultiplicationOverflow(…)` fuzzing harness (practicalswift)
- #18406 add executable flag for `rpc_estimatefee.py` (theStack)
- #18420 listsinceblock block height checks (jonatack)
- #18430 ci: Only clone bitcoin-core/qa-assets when fuzzing (MarcoFalke)
- #18438 ci: Use homebrew addon on native macos (hebasto)
- #18447 Add coverage for script parse error in ParseScript (pierreN)
- #18472 Remove unsafe `BOOST_TEST_MESSAGE` (MarcoFalke)
- #18474 check that peer is connected when calling sync_* (MarcoFalke)
- #18477 ci: Use focal for fuzzers (MarcoFalke)
- #18481 add BIP37 'filterclear' test to p2p_filter.py (theStack)
- #18496 Remove redundant `sync_with_ping` after `add_p2p_connection` (jonatack)
- #18509 fuzz: Avoid running over all inputs after merging them (MarcoFalke)
- #18510 fuzz: Add CScriptNum::getint coverage (MarcoFalke)
- #18514 remove rapidcheck integration and tests (fanquake)
- #18515 Add BIP37 remote crash bug [CVE-2013-5700] test to `p2p_filter.py` (theStack)
- #18516 relax bumpfee `dust_to_fee` txsize an extra vbyte (jonatack)
- #18518 fuzz: Extend descriptor fuzz test (MarcoFalke)
- #18519 fuzz: Extend script fuzz test (MarcoFalke)
- #18521 fuzz: Add `process_messages` harness (MarcoFalke)
- #18529 Add fuzzer version of randomized prevector test (sipa)
- #18534 skip backwards compat tests if not compiled with wallet (fanquake)
- #18540 `wallet_bumpfee` assertion fixup (jonatack)
- #18543 Use one node to avoid a race due to missing sync in `rpc_signrawtransaction` (MarcoFalke)
- #18561 Properly raise FailedToStartError when rpc shutdown before warmup finished (MarcoFalke)
- #18562 ci: Run unit tests sequential once (MarcoFalke)
- #18563 Fix `unregister_all_during_call` cleanup (ryanofsky)
- #18566 Set `-use_value_profile=1` when merging fuzz inputs (MarcoFalke)
- #18757 Remove enumeration of expected deserialization exceptions in ProcessMessage(…) fuzzer (practicalswift)
- #18878 Add test for conflicted wallet tx notifications (ryanofsky)
- #18975 Remove const to work around compiler error on xenial (laanwj)

### Documentation
- #16947 Doxygen-friendly script/descriptor.h comments (ch4ot1c)
- #16983 Add detailed info about Bitcoin Core files (hebasto)
- #16986 Doxygen-friendly CuckooCache comments (ch4ot1c)
- #17022 move-only: Steps for "before major release branch-off" (MarcoFalke)
- #17026 Update bips.md for default bech32 addresses in 0.20.0 (MarcoFalke)
- #17081 Fix Makefile target in benchmarking.md (theStack)
- #17102 Add missing indexes/blockfilter/basic to doc/files.md (MarcoFalke)
- #17119 Fix broken bitcoin-cli examples (andrewtoth)
- #17134 Add switch on enum example to developer notes (hebasto)
- #17142 Update macdeploy README to include all files produced by `make deploy` (za-kk)
- #17146 github: Add warning for bug reports (laanwj)
- #17157 Added instructions for how to add an upsteam to forked repo (dannmat)
- #17159 Add a note about backporting (carnhofdaki)
- #17169 Correct function name in ReportHardwareRand() (fanquake)
- #17177 Describe log files + consistent paths in test READMEs (fjahr)
- #17239 Changed miniupnp links to https (sandakersmann)
- #17281 Add developer note on `c_str()` (laanwj)
- #17285 Bip70 removal follow-up (fjahr)
- #17286 Fix help-debug -checkpoints (ariard)
- #17309 update MSVC instructions to remove Qt OpenSSL linking (fanquake)
- #17339 Add template for good first issues (michaelfolkson)
- #17351 Fix some misspellings (RandyMcMillan)
- #17353 Add ShellCheck to lint tests dependencies (hebasto)
- #17370 Update doc/bips.md with recent changes in master (MarcoFalke)
- #17393 Added regtest config for linearize script (gr0kchain)
- #17411 Add some better examples for scripted diff (laanwj)
- #17503 Remove bitness from bitcoin-qt help message and manpage (laanwj)
- #17539 Update and improve Developer Notes (hebasto)
- #17561 Changed MiniUPnPc link to https in dependencies.md (sandakersmann)
- #17596 Change doxygen URL to doxygen.bitcoincore.org (laanwj)
- #17598 Update release process with latest changes (MarcoFalke)
- #17617 Unify unix epoch time descriptions (jonatack)
- #17637 script: Add keyserver to verify-commits readme (emilengler)
- #17648 Rename wallet-tool references to bitcoin-wallet (hel-o)
- #17688 Add "ci" prefix to CONTRIBUTING.md (hebasto)
- #17751 Use recommended shebang approach in documentation code block (hackerrdave)
- #17752 Fix directory path for secp256k1 subtree in developer-notes (hackerrdave)
- #17772 Mention PR Club in CONTRIBUTING.md (emilengler)
- #17804 Misc RPC help fixes (MarcoFalke)
- #17819 Developer notes guideline on RPCExamples addresses (jonatack)
- #17825 Update dependencies.md (hebasto)
- #17873 Add to Doxygen documentation guidelines (jonatack)
- #17907 Fix improper Doxygen inline comments (Empact)
- #17942 Improve fuzzing docs for macOS users (fjahr)
- #17945 Fix doxygen errors (Empact)
- #18025 Add missing supported rpcs to doc/descriptors.md (andrewtoth)
- #18070 Add note about `brew doctor` (givanse)
- #18125 Remove PPA note from release-process.md (fanquake)
- #18170 Minor grammatical changes and flow improvements (travinkeith)
- #18212 Add missing step in win deployment instructions (dangershony)
- #18219 Add warning against wallet.dat re-use (corollari)
- #18253 Correct spelling errors in comments (Empact)
- #18278 interfaces: Describe and follow some code conventions (ryanofsky)
- #18283 Explain rebase policy in CONTRIBUTING.md (MarcoFalke)
- #18340 Mention MAKE=gmake workaround when building on a BSD (fanquake)
- #18341 Replace remaining literal BTC with `CURRENCY_UNIT` (domob1812)
- #18342 Add fuzzing quickstart guides for libFuzzer and afl-fuzz (practicalswift)
- #18344 Fix nit in getblockchaininfo (stevenroose)
- #18379 Comment fix merkle.cpp (4d55397500)
- #18382 note the costs of fetching all pull requests (vasild)
- #18391 Update init and reduce-traffic docs for -blocksonly (glowang)
- #18464 Block-relay-only vs blocksonly (MarcoFalke)
- #18486 Explain new test logging (MarcoFalke)
- #18505 Update webchat URLs in README.md (SuriyaaKudoIsc)
- #18513 Fix git add argument (HashUnlimited)
- #18577 Correct scripted-diff example link (yahiheb)
- #18589 Fix naming of macOS SDK and clarify version (achow101)

### Miscellaneous
- #15600 lockedpool: When possible, use madvise to avoid including sensitive information in core dumps (luke-jr)
- #15934 Merge settings one place instead of five places (ryanofsky)
- #16115 On bitcoind startup, write config args to debug.log (LarryRuane)
- #16117 util: Replace boost sleep with std sleep (MarcoFalke)
- #16161 util: Fix compilation errors in support/lockedpool.cpp (jkczyz)
- #16802 scripts: In linearize, search for next position of magic bytes rather than fail (takinbo)
- #16889 Add some general std::vector utility functions (sipa)
- #17049 contrib: Bump gitian descriptors for 0.20 (MarcoFalke)
- #17052 scripts: Update `copyright_header` script to include additional files (GChuf)
- #17059 util: Simplify path argument for cblocktreedb ctor (hebasto)
- #17191 random: Remove call to `RAND_screen()` (Windows only) (fanquake)
- #17192 util: Add `check_nonfatal` and use it in src/rpc (MarcoFalke)
- #17218 Replace the LogPrint function with a macro (jkczyz)
- #17266 util: Rename decodedumptime to parseiso8601datetime (elichai)
- #17270 Feed environment data into RNG initializers (sipa)
- #17282 contrib: Remove accounts from bash completion (fanquake)
- #17293 Add assertion to randrange that input is not 0 (JeremyRubin)
- #17325 log: Fix log message for -par=1 (hebasto)
- #17329 linter: Strip trailing / in path for git-subtree-check (jnewbery)
- #17336 scripts: Search for first block file for linearize-data with some block files pruned (Rjected)
- #17361 scripts: Lint gitian descriptors with shellcheck (hebasto)
- #17482 util: Disallow network-qualified command line options (ryanofsky)
- #17507 random: mark RandAddPeriodic and SeedPeriodic as noexcept (fanquake)
- #17527 Fix CPUID subleaf iteration (sipa)
- #17604 util: Make schedulebatchpriority advisory only (fanquake)
- #17650 util: Remove unwanted fields from bitcoin-cli -getinfo (malevolent)
- #17671 script: Fixed wget call in gitian-build.py (willyko)
- #17699 Make env data logging optional (sipa)
- #17721 util: Don't allow base58 decoding of non-base58 strings. add base58 tests (practicalswift)
- #17750 util: Change getwarnings parameter to bool (jnewbery)
- #17753 util: Don't allow base32/64-decoding or parsemoney(…) on strings with embedded nul characters. add tests (practicalswift)
- #17823 scripts: Read suspicious hosts from a file instead of hardcoding (sanjaykdragon)
- #18162 util: Avoid potential uninitialized read in `formatiso8601datetime(int64_t)` by checking `gmtime_s`/`gmtime_r` return value (practicalswift)
- #18167 Fix a violation of C++ standard rules where unions are used for type-punning (TheQuantumPhysicist)
- #18225 util: Fail to parse empty string in parsemoney (MarcoFalke)
- #18270 util: Fail to parse whitespace-only strings in parsemoney(…) (instead of parsing as zero) (practicalswift)
- #18316 util: Helpexamplerpc formatting (jonatack)
- #18357 Fix missing header in sync.h (promag)
- #18412 script: Fix `script_err_sig_pushonly` error string (theStack)
- #18416 util: Limit decimal range of numbers parsescript accepts (pierreN)
- #18503 init: Replace `URL_WEBSITE` with `PACKAGE_URL` (MarcoFalke)
- #18526 Remove PID file at the very end (hebasto)
- #18553 Avoid non-trivial global constants in SHA-NI code (sipa)
- #18665 Do not expose and consider `-logthreadnames` when it does not work (hebasto)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- 251
- 4d55397500
- Aaron Clauson
- Adam Jonas
- Albert
- Amiti Uttarwar
- Andrew Chow
- Andrew Toth
- Anthony Towns
- Antoine Riard
- Ava Barron
- Ben Carman
- Ben Woosley
- Block Mechanic
- Brian Solon
- Bushstar
- Carl Dong
- Carnhof Daki
- Cory Fields
- Daki Carnhof
- Dan Gershony
- Daniel Kraft
- dannmat
- Danny-Scott
- darosior
- David O'Callaghan
- Dominik Spicher
- Elichai Turkel
- Emil Engler
- emu
- Fabian Jahr
- fanquake
- Filip Gospodinov
- Franck Royer
- Gastón I. Silva
- gchuf
- Gleb Naumenko
- Gloria Zhao
- glowang
- Gr0kchain
- Gregory Sanders
- hackerrdave
- Harris
- hel0
- Hennadii Stepanov
- ianliu
- Igor Cota
- James Chiang
- James O'Beirne
- Jan Beich
- Jan Sarenik
- Jeffrey Czyz
- Jeremy Rubin
- JeremyCrookshank
- Jim Posen
- John Bampton
- John L. Jegutanis
- John Newbery
- Jon Atack
- Jon Layton
- Jonas Schnelli
- João Barbosa
- Jorge Timón
- Karl-Johan Alm
- kodslav
- Larry Ruane
- Luke Dashjr
- malevolent
- MapleLaker
- marcaiaf
- MarcoFalke
- Marius Kjærstad
- Mark Erhardt
- Mark Tyneway
- Martin Erlandsson
- Martin Zumsande
- Matt Corallo
- Matt Ward
- Michael Folkson
- Michael Polzer
- Micky Yun Chan
- Neha Narula
- nijynot
- naumenkogs
- NullFunctor
- Peter Bushnell
- pierrenn
- Pieter Wuille
- practicalswift
- randymcmillan
- Rjected
- Russell Yanofsky
- Samer Afach
- Samuel Dobson
- Sanjay K
- Sebastian Falbesoner
- setpill
- Sjors Provoost
- Stefan Richter
- stefanwouldgo
- Steven Roose
- Suhas Daftuar
- Suriyaa Sundararuban
- TheCharlatan
- Tim Akinbo
- Travin Keith
- tryphe
- Vasil Dimov
- Willy Ko
- Wilson Ccasihue S
- Wladimir J. van der Laan
- Yahia Chiheb
- Yancy Ribbens
- Yusuf Sahin HAMZA
- Zakk
- Zero

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
