
Bitcoin Core version 0.19.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.19.0/>
=======
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

0.19.0 change log
=================

### Consensus
- #16128 Delete error-prone CScript constructor only used with FindAndDelete (instagibbs)
- #16060 Bury bip9 deployments (jnewbery)

### Policy
- #15557 Enhance `bumpfee` to include inputs when targeting a feerate (instagibbs)
- #15846 Make sending to future native witness outputs standard (sipa)

### Block and transaction handling
- #15632 Remove ResendWalletTransactions from the Validation Interface (jnewbery)
- #14121 Index for BIP 157 block filters (jimpo)
- #15141 Rewrite DoS interface between validation and net_processing (sdaftuar)
- #15880 utils and libraries: Replace deprecated Boost Filesystem functions (hebasto)
- #15971 validation: Add compile-time checking for negative locking requirement in LimitValidationInterfaceQueue (practicalswift)
- #15999 init: Remove dead code in LoadChainTip (MarcoFalke)
- #16015 validation: Hold cs_main when reading chainActive in RewindBlockIndex (practicalswift)
- #16056 remove unused magic number from consistency check (instagibbs)
- #16171 Remove -mempoolreplacement to prevent needless block prop slowness (TheBlueMatt)
- #15894 Remove duplicated "Error: " prefix in logs (hebasto)
- #14193 validation: Add missing mempool locks (MarcoFalke)
- #15681 Allow one extra single-ancestor transaction per package (TheBlueMatt)
- #15305 [validation] Crash if disconnecting a block fails (sdaftuar)
- #16471 log correct messages when CPFP fails (jnewbery)
- #16433 txmempool: Remove unused default value MemPoolRemovalReason::UNKNOWN (MarcoFalke)
- #13868 Remove unused fScriptChecks parameter from CheckInputs (Empact)
- #16421 Conservatively accept RBF bumps bumping one tx at the package limits (TheBlueMatt)
- #16854 Prevent UpdateTip log message from being broken up (stevenroose)
- #16956 validation: Make GetWitnessCommitmentIndex public (MarcoFalke)
- #16713 Ignore old versionbit activations to avoid 'unknown softforks' warning (jnewbery)
- #17002 chainparams: Bump assumed chain params (MarcoFalke)
- #16849 Fix block index inconsistency in InvalidateBlock() (sdaftuar)

### P2P protocol and network code
- #15597 Generate log entry when blocks messages are received unexpectedly (pstratem)
- #15654 Remove unused unsanitized user agent string CNode::strSubVer (MarcoFalke)
- #15689 netaddress: Update CNetAddr for ORCHIDv2 (dongcarl)
- #15834 Fix transaction relay bugs introduced in #14897 and expire transactions from peer in-flight map (sdaftuar)
- #15651 torcontrol: Use the default/standard network port for Tor hidden services, even if the internal port is set differently (luke-jr)
- #16188 Document what happens to getdata of unknown type (MarcoFalke)
- #15649 Add ChaCha20Poly1305@Bitcoin AEAD (jonasschnelli)
- #16152 Disable bloom filtering by default (TheBlueMatt)
- #15993 Drop support of the insecure miniUPnPc versions (hebasto)
- #16197 Use mockable time for tx download (MarcoFalke)
- #16248 Make whitebind/whitelist permissions more flexible (NicolasDorier)
- #16618 [Fix] Allow connection of a noban banned peer (NicolasDorier)
- #16631 Restore default whitelistrelay to true (NicolasDorier)
- #15759 Add 2 outbound block-relay-only connections (sdaftuar)
- #15558 Don't query all DNS seeds at once (sipa)
- #16999 0.19 seeds update (laanwj)

### Wallet
- #15288 Remove wallet -> node global function calls (ryanofsky)
- #15491 Improve log output for errors during load (gwillen)
- #13541 wallet/rpc: sendrawtransaction maxfeerate (kallewoof)
- #15680 Remove resendwallettransactions RPC method (jnewbery)
- #15508 Refactor analyzepsbt for use outside RPC code (gwillen)
- #15747 Remove plethora of Get*Balance (MarcoFalke)
- #15728 Refactor relay transactions (jnewbery)
- #15639 bitcoin-wallet tool: Drop libbitcoin_server.a dependency (ryanofsky)
- #15853 Remove unused import checkpoints.h (MarcoFalke)
- #15780 add cachable amounts for caching credit/debit values (kallewoof)
- #15778 Move maxtxfee from node to wallet (jnewbery)
- #15901 log on rescan completion (andrewtoth)
- #15917 Avoid logging no_such_file_or_directory error (promag)
- #15452 Replace CScriptID and CKeyID in CTxDestination with dedicated types (instagibbs)
- #15870 Only fail rescan when blocks have actually been pruned (MarcoFalke)
- #15006 Add option to create an encrypted wallet (achow101)
- #16001 Give WalletModel::UnlockContext move semantics (sipa)
- #15741 Batch write imported stuff in importmulti (achow101)
- #16144 do not encrypt wallets with disabled private keys (mrwhythat)
- #15024 Allow specific private keys to be derived from descriptor (meshcollider)
- #13756 "avoid_reuse" wallet flag for improved privacy (kallewoof)
- #16226 Move ismine to the wallet module (achow101)
- #16239 wallet/rpc: follow-up clean-up/fixes to avoid_reuse (kallewoof)
- #16286 refactoring: wallet: Fix GCC 7.4.0 warning (hebasto)
- #16257 abort when attempting to fund a transaction above -maxtxfee (Sjors)
- #16237 Have the wallet give out destinations instead of keys (achow101)
- #16322 Fix -maxtxfee check by moving it to CWallet::CreateTransaction (promag)
- #16361 Remove redundant pre-TopUpKeypool check (instagibbs)
- #16244 Move wallet creation out of the createwallet rpc into its own function (achow101)
- #16227 Refactor CWallet's inheritance chain (achow101)
- #16208 Consume ReserveDestination on successful CreateTransaction (instagibbs)
- #16301 Use CWallet::Import* functions in all import* RPCs (achow101)
- #16402 Remove wallet settings from chainparams (MarcoFalke)
- #16415 Get rid of PendingWalletTx class (ryanofsky)
- #15588 Log the actual wallet file version and no longer publicly expose the "version" record (achow101)
- #16399 Improve wallet creation (fjahr)
- #16475 Enumerate walletdb keys (MarcoFalke)
- #15709 Do not add "setting" key as unknown (Bushstar)
- #16451 Remove CMerkleTx (jnewbery)
- #15906 Move min_depth and max_depth to coin control (amitiuttarwar)
- #16502 Drop unused OldKey (promag)
- #16394 Allow createwallet to take empty passwords to make unencrypted wallets (achow101)
- #15911 Use wallet RBF default for walletcreatefundedpsbt (Sjors)
- #16503 Remove p2pEnabled from Chain interface (ariard)
- #16557 restore coinbase and confirmed/conflicted checks in SubmitMemoryPoolAndRelay() (jnewbery)
- #14934 Descriptor expansion cache clarifications (Sjors)
- #16383 rpcwallet: default include_watchonly to true for watchonly wallets (jb55)
- #16542 Return more specific errors about invalid descriptors (achow101)
- #16572 Fix Char as Bool in Wallet (JeremyRubin)
- #16753 extract PubKey from P2PK script with Solver (theStack)
- #16716 Use wallet name instead of pointer on unload/release (promag)
- #16185 gettransaction: add an argument to decode the transaction (darosior)
- #16745 Translate all initErrors in CreateWalletFromFile (MarcoFalke)
- #16792 Assert that the HRP is lowercase in Bech32::Encode (meshcollider)
- #16624 encapsulate transactions state (ariard)
- #16830 Cleanup walletinitinterface.h (hebasto)
- #16796 Fix segfault in CreateWalletFromFile (MarcoFalke)
- #16866 Rename 'decode' argument in gettransaction method to 'verbose' (jnewbery)
- #16727 Explicit feerate for bumpfee (instagibbs)
- #16609 descriptor: fix missed m_script_arg arg renaming in #14934 (fanquake)

### RPC and other APIs
- #15492 remove deprecated generate method (Sjors)
- #15566 cli: Replace testnet with chain and return network name as per bip70 (fanquake)
- #15564 cli: Remove duplicate wallet fields from -getinfo (fanquake)
- #15642 Remove deprecated rpc warnings (jnewbery)
- #15637 Rename size to vsize in mempool related calls (fanquake)
- #15620 Uncouple non-wallet rpcs from maxTxFee global (MarcoFalke)
- #15616 Clarify decodescript RPCResult doc (MarcoFalke)
- #15669 Fix help text for signtransactionwithXXX (torkelrogstad)
- #15596 Ignore sendmany::minconf as dummy value (MarcoFalke)
- #15755 remove unused var in rawtransaction.cpp (Bushstar)
- #15746 RPCHelpMan: Always name dictionary keys (MarcoFalke)
- #15748 remove dead mining code (jnewbery)
- #15751 Speed up deriveaddresses for large ranges (sipa)
- #15770 Validate maxfeerate with AmountFromValue (promag)
- #15474 rest/rpc: Make mempoolinfo atomic (promag)
- #15463 Speedup getaddressesbylabel (promag)
- #15784 Remove dependency on interfaces::Chain in SignTransaction (ariard)
- #15323 Expose g_is_mempool_loaded via getmempoolinfo (Empact)
- #15932 Serialize in getblock without cs_main (MarcoFalke)
- #15930 Add balances RPC (MarcoFalke)
- #15730 Show scanning details in getwalletinfo (promag)
- #14802 faster getblockstats using BlockUndo data (FelixWeis)
- #14984 Speedup getrawmempool when verbose=true (promag)
- #16071 Hint for importmulti in help output of importpubkey and importaddress (kristapsk)
- #16063 Mention getwalletinfo where a rescan is triggered (promag)
- #16024 deriveaddresses: Correction of descriptor checksum in RPC example (ccapo)
- #16217 getrawtransaction: inform about blockhash argument when lookup fails (darosior)
- #15427 Add support for descriptors to utxoupdatepsbt (sipa)
- #16262 Allow shutdown while in generateblocks (pstratem)
- #15483 Adding a 'logpath' entry to getrpcinfo (darosior)
- #16325 Clarify that block count means height excl genesis (MarcoFalke)
- #16326 add new utxoupdatepsbt arguments to the CRPCCommand and CPRCCvertParam tables (jnewbery)
- #16332 Add logpath description for getrpcinfo (instagibbs)
- #16240 JSONRPCRequest-aware RPCHelpMan (kallewoof)
- #15996 Deprecate totalfee argument in `bumpfee` (instagibbs)
- #16467 sendrawtransaction help privacy note (jonatack)
- #16596 Fix getblocktemplate CLI example (emilengler)
- #15986 Add checksum to getdescriptorinfo (sipa)
- #16647 add weight to getmempoolentry output (fanquake)
- #16695 Add window final block height to getchaintxstats (leto)
- #16798 Refactor rawtransaction_util's SignTransaction to separate prevtx parsing (achow101)
- #16285 Improve scantxoutset response and help message (promag)
- #16725 Don't show addresses or P2PK in decoderawtransaction (NicolasDorier)
- #16787 Human readable network services (darosior)
- #16251 Improve signrawtransaction error reporting (ajtowns)
- #16873 fix regression in gettransaction (jonatack)
- #16512 Shuffle inputs and outputs after joining psbts (achow101)
- #16521 Use the default maxfeerate value as BTC/kB (Remagpie)
- #16817 Fix casing in getblockchaininfo to be inline with other fields (dangershony)
- #17131 fix -rpcclienttimeout 0 option (fjahr)
- #17249 Add missing deque include to fix build (jbeich)

### GUI
- #15464 Drop unused return values in WalletFrame (promag)
- #15614 Defer removeAndDeleteWallet when no modal widget is active (promag)
- #15711 Generate bech32 addresses by default (MarcoFalke)
- #15829 update request payment button text and tab description (fanquake)
- #15874 Resolve the qt/guiutil <-> qt/optionsmodel CD (251Labs)
- #15371 Uppercase bech32 addresses in qr codes (benthecarman)
- #15928 Move QRImageWidget to its own file-pair (luke-jr)
- #16113 move coin control "OK" to the right hand side of the dialog (fanquake)
- #16090 Add vertical spacer to peer detail widget (JosuGZ)
- #15886 qt, wallet: Revamp SendConfirmationDialog (hebasto)
- #16263 Use qInfo() if no error occurs (hebasto)
- #16153 Add antialiasing to traffic graph widget (JosuGZ)
- #16350 Remove unused guard (hebasto)
- #16106 Sort wallets in open wallet menu (promag)
- #16291 Stop translating PACKAGE_NAME (MarcoFalke)
- #16380 Remove unused bits from the service flags enum (MarcoFalke)
- #16379 Fix autostart filenames on Linux for testnet/regtest (hebasto)
- #16366 init: Use InitError for all errors in bitcoind/qt (MarcoFalke)
- #16436 Do not create payment server if -disablewallet option provided (hebasto)
- #16514 Remove unused RPCConsole::tabFocus (promag)
- #16497 Generate bech32 addresses by default (take 2, fixup) (MarcoFalke)
- #16349 Remove redundant WalletController::addWallet slot (hebasto)
- #16578 Do not pass in command line arguments to QApplication (achow101)
- #16612 Remove menu icons (laanwj)
- #16677 remove unused PlatformStyle::TextColorIcon (fanquake)
- #16694 Ensure transaction send error is always visible (fanquake)
- #14879 Add warning messages to the debug window (hebasto)
- #16708 Replace obsolete functions of QSslSocket (hebasto)
- #16701 Replace functions deprecated in Qt 5.13 (hebasto)
- #16706 Replace deprecated QSignalMapper by lambda expressions (hebasto)
- #16707 Remove obsolete QModelIndex::child() (hebasto)
- #16758 Replace QFontMetrics::width() with TextWidth() (hebasto)
- #16760 Change uninstall icon on Windows (GChuf)
- #16720 Replace objc_msgSend() function calls with the native Objective-C syntax (hebasto)
- #16788 Update transifex slug for 0.19 (laanwj)
- #15450 Create wallet menu option (achow101)
- #16735 Remove unused menu items for Windows and Linux (GChuf)
- #16826 Do additional character escaping for wallet names and address labels (achow101)
- #15529 Add Qt programs to msvc build (updated, no code changes) (sipsorcery)
- #16714 add prune to intro screen with smart default (Sjors)
- #16858 advise users not to switch wallets when opening a BIP70 URI (jameshilliard)
- #16822 Create wallet menu option follow-ups (jonatack)
- #16882 Re-generate translations before 0.19.0 (MarcoFalke)
- #16928 Rename address checkbox back to bech32 (MarcoFalke)
- #16837 Fix {C{,XX},LD}FLAGS pickup (dongcarl)
- #16971 Change default size of intro frame (emilengler)
- #16988 Periodic translations update (laanwj)
- #16852 When BIP70 is disabled, get PaymentRequest merchant using string search (achow101)
- #16952 make sure to update the UI when deleting a transaction (jonasschnelli)
- #17031 Prevent processing duplicate payment requests (promag)
- #17135 Make polling in ClientModel asynchronous (promag)
- #17120 Fix start timer from non QThread (promag)
- #17257 disable font antialiasing for QR image address (fanquake)

### Build system
- #14954 Require python 3.5 (MarcoFalke)
- #15580 native_protobuf: avoid system zlib (dongcarl)
- #15601 Switch to python3 (take 3) (MarcoFalke)
- #15581 Make less assumptions about build env (dongcarl)
- #14853 latest RapidCheck (fanquake)
- #15446 Improve depends debuggability (dongcarl)
- #13788 Fix --disable-asm for newer assembly checks/code (luke-jr)
- #12051 add missing debian contrib file to tarball (puchu)
- #15919 Remove unused OpenSSL includes to make it more clear where OpenSSL is used (practicalswift)
- #15978 .gitignore: Don't ignore depends patches (dongcarl)
- #15939 gitian: Remove windows 32 bit build (MarcoFalke)
- #15239 scripts and tools: Move non-linux build source tarballs to "bitcoin-binaries/version" directory (hebasto)
- #14047 Add HKDF_HMAC256_L32 and method to negate a private key (jonasschnelli)
- #16051 add patch to common dependencies (fanquake)
- #16049 switch to secure download of all dependencies (Kemu)
- #16059 configure: Fix thread_local detection (dongcarl)
- #16089 add ability to skip building zeromq (fanquake)
- #15844 Purge libtool archives (dongcarl)
- #15461 update to Boost 1.70 (Sjors)
- #16141 remove GZIP export from gitian descriptors (fanquake)
- #16235 Cleaned up and consolidated msbuild files (no code changes) (sipsorcery)
- #16246 MSVC: Fix error in debug mode (Fix #16245) (NicolasDorier)
- #16183 xtrans: Configure flags cleanup (dongcarl)
- #16258 [MSVC]: Create the config.ini as part of bitcoind build (NicolasDorier)
- #16271 remove -Wall from rapidcheck build flags (fanquake)
- #16309 [MSVC] allow user level project customization (NicolasDorier)
- #16308 [MSVC] Copy build output to src/ automatically after build (NicolasDorier)
- #15457 Check std::system for -[alert|block|wallet]notify (Sjors)
- #16344 use #if HAVE_SYSTEM instead of defined(HAVE_SYSTEM) (Sjors)
- #16352 prune dbus from depends (fanquake)
- #16270 expat 2.2.7 (fanquake)
- #16408 Prune X packages (dongcarl)
- #16386 disable unused Qt features (fanquake)
- #16424 Treat -Wswitch as error when --enable-werror (MarcoFalke)
- #16441 remove qt libjpeg check from bitcoin_qt.m4 (fanquake)
- #16434 Specify AM_CPPFLAGS for ZMQ (domob1812)
- #16534 add Qt Creator Makefile.am.user to .gitignore (Bushstar)
- #16573 disable building libsecp256k1 benchmarks (fanquake)
- #16533 disable libxcb extensions (fanquake)
- #16589 Remove unused src/obj-test folder (MarcoFalke)
- #16435 autoconf: Sane `--enable-debug` defaults (dongcarl)
- #16622 echo property tests status during build (jonatack)
- #16611 Remove src/obj directory from repository (laanwj)
- #16371 ignore macOS make deploy artefacts & add them to clean-local (fanquake)
- #16654 build: update RapidCheck Makefile (jonatack)
- #16370 cleanup package configure flags (fanquake)
- #16746 msbuild: Ignore linker warning (sipsorcery)
- #16750 msbuild: adds bench_bitcoin to auto generated project files (sipsorcery)
- #16810 guix: Remove ssp spec file hack (dongcarl)
- #16477 skip deploying plugins we dont use in macdeployqtplus (fanquake)
- #16413 Bump QT to LTS release 5.9.8 (THETCR)
- #15584 disable BIP70 support by default (fanquake)
- #16871 make building protobuf optional in depends (fanquake)
- #16879 remove redundant sed patching (fanquake)
- #16809 zlib: Move toolchain options to configure (dongcarl)
- #15146 Solve SmartOS FD_ZERO build issue (Empact)
- #16870 update boost macros to latest upstream for improved error reporting (fanquake)
- #16982 Factor out qt translations from build system (laanwj)
- #16926 Add OpenSSL termios fix for musl libc (nmarley)
- #16927 Refresh ZeroMQ 4.3.1 patch (nmarley)
- #17005 Qt version appears only if GUI is being built (ch4ot1c)
- #16468 Exclude depends/Makefile in .gitignore (promag)

### Tests and QA
- #15296 Add script checking for deterministic line coverage in unit tests (practicalswift)
- #15338 ci: Build and run tests once on freebsd (MarcoFalke)
- #15479 Add .style.yapf (MarcoFalke)
- #15534 lint-format-strings: open files sequentially (fix for OS X) (gwillen)
- #15504 fuzz: Link BasicTestingSetup (shared with unit tests) (MarcoFalke)
- #15473 bench: Benchmark mempooltojson (MarcoFalke)
- #15466 Print remaining jobs in test_runner.py (stevenroose)
- #15631 mininode: Clearer error message on invalid magic bytes (MarcoFalke)
- #15255 Remove travis_wait from lint script (gkrizek)
- #15686 make pruning test faster (jnewbery)
- #15533 .style.yapf: Set column_limit=160 (MarcoFalke)
- #15660 Overhaul p2p_compactblocks.py (sdaftuar)
- #15495 Add regtests for HTTP status codes (domob1812)
- #15772 Properly log named args in authproxy (MarcoFalke)
- #15771 Prevent concurrency issues reading .cookie file (promag)
- #15693 travis: Switch to ubuntu keyserver to avoid timeouts (MarcoFalke)
- #15629 init: Throw error when network specific config is ignored (MarcoFalke)
- #15773 Add BitcoinTestFramework::sync_* methods (MarcoFalke)
- #15797 travis: Bump second timeout to 33 minutes, add rationale (MarcoFalke)
- #15788 Unify testing setups for fuzz, bench, and unit tests (MarcoFalke)
- #15352 Reduce noise level in test_bitcoin output (practicalswift)
- #15779 Add wallet_balance benchmark (MarcoFalke)
- #15843 fix outdated include in blockfilter_index_tests (jamesob)
- #15866 Add missing syncwithvalidationinterfacequeue to wallet_import_rescan (MarcoFalke)
- #15697 Make swap_magic_bytes in p2p_invalid_messages atomic (MarcoFalke)
- #15895 Avoid re-reading config.ini unnecessarily (luke-jr)
- #15896 feature_filelock, interface_bitcoin_cli: Use PACKAGE_NAME in messages rather than hardcoding Bitcoin Core (luke-jr)
- #15897 QA/mininode: Send all headers upfront in send_blocks_and_test to avoid sending an unconnected one (luke-jr)
- #15696 test_runner: Move feature_pruning to base tests (MarcoFalke)
- #15869 Add settings merge test to prevent regresssions (ryanofsky)
- #15758 Add further tests to wallet_balance (MarcoFalke)
- #15841 combine_logs: append node stderr and stdout if it exists (MarcoFalke)
- #15949 test_runner: Move pruning back to extended (MarcoFalke)
- #15927 log thread names by default in functional tests (jnewbery)
- #15664 change default Python block serialization to witness (instagibbs)
- #15988 Add test for ArgsManager::GetChainName (ryanofsky)
- #15963 Make random seed logged and settable (jnewbery)
- #15943 Fail if RPC has been added without tests (MarcoFalke)
- #16036 travis: Run all lint scripts even if one fails (scravy)
- #13555 parameterize adjustment period in versionbits_computeblockversion (JBaczuk)
- #16079 wallet_balance.py: Prevent edge cases (stevenroose)
- #16078 replace tx hash with txid in rawtransaction test (LongShao007)
- #16042 Bump MAX_NODES to 12 (MarcoFalke)
- #16124 Limit Python linting to files in the repo (practicalswift)
- #16143 Mark unit test blockfilter_index_initial_sync as non-deterministic (practicalswift)
- #16214 travis: Fix caching issues (MarcoFalke)
- #15982 Make msg_block a witness block (MarcoFalke)
- #16225 Make coins_tests/updatecoins_simulation_test deterministic (practicalswift)
- #16236 fuzz: Log output even if fuzzer failed (MarcoFalke)
- #15520 cirrus: Run extended test feature_pruning (MarcoFalke)
- #16234 Add test for unknown args (MarcoFalke)
- #16207 stop generating lcov coverage when functional tests fail (asood123)
- #16252 Log to debug.log in all unit tests (MarcoFalke)
- #16289 Add missing ECC_Stop() in GUI rpcnestedtests.cpp (jonasschnelli)
- #16278 Remove unused includes (practicalswift)
- #16302 Add missing syncwithvalidationinterfacequeue to wallet_balance test (MarcoFalke)
- #15538 wallet_bumpfee.py: Make sure coin selection produces change (instagibbs)
- #16294 Create at most one testing setup (MarcoFalke)
- #16299 bench: Move generated data to a dedicated translation unit (promag)
- #16329 Add tests for getblockchaininfo.softforks (MarcoFalke)
- #15687 tool wallet test coverage for unexpected writes to wallet (jonatack)
- #16267 bench: Benchmark blocktojson (fanatid)
- #14505 Add linter to make sure single parameter constructors are marked explicit (practicalswift)
- #16338 Disable other targets when enable-fuzz is set (qmma70)
- #16334 rpc_users: Also test rpcauth.py with password (dongcarl)
- #15282 Replace hard-coded hex tx with class in test framework (stevenroose)
- #16390 Add --filter option to test_runner.py (promag)
- #15891 Require standard txs in regtest by default (MarcoFalke)
- #16374 Enable passing wildcard test names to test runner from root (jonatack)
- #16420 Fix race condition in wallet_encryption test (jonasschnelli)
- #16422 remove redundant setup in addrman_tests (zenosage)
- #16438 travis: Print memory and number of cpus (MarcoFalke)
- #16445 Skip flaky p2p_invalid_messages test on macOS (fjahr)
- #16459 Fix race condition in example_test.py (sdaftuar)
- #16464 Ensure we don't generate a too-big block in p2sh sigops test (sdaftuar)
- #16491 fix deprecated log.warn in feature_dbcrash test (jonatack)
- #15134 Switch one of the Travis jobs to an unsigned char environment (-funsigned-char) (practicalswift)
- #16505 Changes verbosity of msbuild from quiet to normal in the appveyor script (sipsorcery)
- #16293 Make test cases separate functions (MarcoFalke)
- #16470 Fail early on disconnect in mininode.wait_for_* (MarcoFalke)
- #16277 Suppress output in test_bitcoin for expected errors (gertjaap)
- #16493 Fix test failures (MarcoFalke)
- #16538 Add missing sync_blocks to feature_pruning (MarcoFalke)
- #16509 Adapt test framework for chains other than "regtest" (MarcoFalke)
- #16363 Add test for BIP30 duplicate tx (MarcoFalke)
- #16535 Explain why -whitelist is used in feature_fee_estimation (MarcoFalke)
- #16554 only include and use OpenSSL where it's actually needed (BIP70) (fanquake)
- #16598 Remove confusing hash256 function in util (elichai)
- #16595 travis: Use extended 90 minute timeout when available (MarcoFalke)
- #16563 Add unit test for AddTimeData (mzumsande)
- #16561 Use colors and dots in test_runner.py output only if standard output is a terminal (practicalswift)
- #16465 Test p2sh-witness and bech32 in wallet_import_rescan (MarcoFalke)
- #16582 Rework ci (Use travis only as fallback env) (MarcoFalke)
- #16633 travis: Fix test_runner.py timeouts (MarcoFalke)
- #16646 Run tests with UPnP disabled (fanquake)
- #16623 ci: Add environment files for all settings (MarcoFalke)
- #16656 fix rpc_setban.py race (jonasschnelli)
- #16570 Make descriptor tests deterministic (davereikher)
- #16404 Test ZMQ notification after chain reorg (promag)
- #16726 Avoid common Python default parameter gotcha when mutable dict/list:s are used as default parameter values (practicalswift)
- #16739 ci: Pass down $makejobs to test_runner.py, other improvements (MarcoFalke)
- #16767 Check for codespell in lint-spelling.sh (kristapsk)
- #16768 Make lint-includes.sh work from any directory (kristapsk)
- #15257 Scripts and tools: Bump flake8 to 3.7.8 (Empact)
- #16804 Remove unused try-block in assert_debug_log (MarcoFalke)
- #16850 `servicesnames` field in `getpeerinfo` and `getnetworkinfo` (darosior)
- #16551 Test that low difficulty chain fork is rejected (MarcoFalke)
- #16737 Establish only one connection between nodes in rpc_invalidateblock (MarcoFalke)
- #16845 Add notes on how to generate data/wallets/high_minversion (MarcoFalke)
- #16888 Bump timeouts in slow running tests (MarcoFalke)
- #16864 Add python bech32 impl round-trip test (instagibbs)
- #16865 add some unit tests for merkle.cpp (soroosh-sdi)
- #14696 Add explicit references to related CVE's in p2p_invalid_block test (lucash-dev)
- #16907 lint: Add DisabledOpcodeTemplates to whitelist (MarcoFalke)
- #16898 Remove connect_nodes_bi (MarcoFalke)
- #16917 Move common function assert_approx() into util.py (fridokus)
- #16921 Add information on how to add Vulture suppressions (practicalswift)
- #16920 Fix extra_args in wallet_import_rescan.py (MarcoFalke)
- #16918 Make PORT_MIN in test runner configurable (MarcoFalke)
- #16941 travis: Disable feature_block in tsan run due to oom (MarcoFalke)
- #16929 follow-up to rpc: default maxfeerate value as BTC/kB (jonatack)
- #16959 ci: Set $host before setting fallback values (MarcoFalke)
- #16961 Remove python dead code linter (laanwj)
- #16931 add unittests for CheckProofOfWork (soroosh-sdi)
- #16991 Fix service flag comparison check in rpc_net test (luke-jr) (laanwj)
- #16987 Correct docstring param name (jbampton)
- #17015 Explain QT_QPA_PLATFORM for gui tests (MarcoFalke)
- #17006 Enable UBSan for Travis fuzzing job (practicalswift)
- #17086 Fix fs_tests for unknown locales (carnhofdaki)
- #15903 appveyor: Write @PACKAGE_NAME@ to config (MarcoFalke)
- #16742 test: add executable flag for wallet_watchonly.py (theStack)
- #16740 qa: Relax so that the subscriber is ready before publishing zmq messages (#16740)

### Miscellaneous
- #15335 Fix lack of warning of unrecognized section names (AkioNak)
- #15528 contrib: Bump gitian descriptors for 0.19 (MarcoFalke)
- #15609 scripts and tools: Set 'distro' explicitly (hebasto)
- #15519 Add Poly1305 implementation (jonasschnelli)
- #15643 contrib: Gh-merge: include acks in merge commit (MarcoFalke)
- #15838 scripts and tools: Fetch missing review comments in github-merge.py (nkostoulas)
- #15920 lint: Check that all wallet args are hidden (MarcoFalke)
- #15849 Thread names in logs and deadlock debug tools (jamesob)
- #15650 Handle the result of posix_fallocate system call (lucayepa)
- #15766 scripts and tools: Upgrade gitian image before signing (hebasto)
- #15512 Add ChaCha20 encryption option (XOR) (jonasschnelli)
- #15968 Fix portability issue with pthreads (grim-trigger)
- #15970 Utils and libraries: fix static_assert for macro HAVE_THREAD_LOCAL (orientye)
- #15863 scripts and tools: Ensure repos are up-to-date in gitian-build.py (hebasto)
- #15224 Add RNG strengthening (10ms once every minute) (sipa)
- #15840 Contrib scripts: Filter IPv6 by ASN (abitfan)
- #13998 Scripts and tools: gitian-build.py improvements and corrections (hebasto)
- #15236 scripts and tools: Make --setup command independent (hebasto)
- #16114 contrib: Add curl as a required program in gitian-build.py (fanquake)
- #16046 util: Add type safe gettime (MarcoFalke)
- #15703 Update secp256k1 subtree to latest upstream (sipa)
- #16086 contrib: Use newer config.guess & config.sub in install_db4.sh (fanquake)
- #16130 Don't GPG sign intermediate commits with github-merge tool (stevenroose)
- #16162 scripts: Add key for michael ford (fanquake) to trusted keys list (fanquake)
- #16201 devtools: Always use unabbreviated commit IDs in github-merge.py (laanwj)
- #16112 util: Log early messages (MarcoFalke)
- #16223 devtools: Fetch and display ACKs at sign-off time in github-merge (laanwj)
- #16300 util: Explain why the path is cached (MarcoFalke)
- #16314 scripts and tools: Update copyright_header.py script (hebasto)
- #16158 Fix logic of memory_cleanse() on MSVC and clean up docs (real-or-random)
- #14734 fix an undefined behavior in uint::SetHex (kazcw)
- #16327 scripts and tools: Update ShellCheck linter (hebasto)
- #15277 contrib: Enable building in guix containers (dongcarl)
- #16362 Add bilingual_str type (hebasto)
- #16481 logs: add missing space (harding)
- #16581 sipsorcery gitian key (sipsorcery)
- #16566 util: Refactor upper/lowercase functions (kallewoof)
- #16620 util: Move resolveerrmsg to util/error (MarcoFalke)
- #16625 scripts: Remove github-merge.py (fanquake)
- #15864 Fix datadir handling (hebasto)
- #16670 util: Add join helper to join a list of strings (MarcoFalke)
- #16665 scripts: Move update-translations.py to maintainer-tools repo (fanquake)
- #16730 Support serialization of `std::vector<bool>` (sipa)
- #16556 Fix systemd service file configuration directory setup (setpill)
- #15615 Add log output during initial header sync (jonasschnelli)
- #16774 Avoid unnecessary "Synchronizing blockheaders" log messages (jonasschnelli)
- #16489 log: harmonize bitcoind logging (jonatack)
- #16577 util: Cbufferedfile fixes and unit test (LarryRuane)
- #16984 util: Make thread names shorter (hebasto)
- #17038 Don't rename main thread at process level (laanwj)
- #17184 util: Filter out macos process serial number (hebasto)
- #17095 util: Filter control characters out of log messages (laanwj)
- #17085 init: Change fallback locale to C.UTF-8 (laanwj)
- #16957 9% less memory: make SaltedOutpointHasher noexcept (martinus)

### Documentation
- #15514 Update Transifex links (fanquake)
- #15513 add "sections" info to example bitcoin.conf (fanquake)
- #15530 Move wallet lock annotations to header (MarcoFalke)
- #15562 remove duplicate clone step in build-windows.md (fanquake)
- #15565 remove release note fragments (fanquake)
- #15444 Additional productivity tips (Sjors)
- #15577 Enable TLS in link to chris.beams.io (JeremyRand)
- #15604 release note for disabling reject messages by default (jnewbery)
- #15611 Add Gitian key for droark (droark)
- #15626 Update ACK description in CONTRIBUTING.md (jonatack)
- #15603 Add more tips to productivity.md (gwillen)
- #15683 Comment for seemingly duplicate LIBBITCOIN_SERVER (Bushstar)
- #15685 rpc-mining: Clarify error messages (MarcoFalke)
- #15760 Clarify sendrawtransaction::maxfeerate==0 help (MarcoFalke)
- #15659 fix findFork comment (r8921039)
- #15718 Improve netaddress comments (dongcarl)
- #15833 remove out-of-date comment on pay-to-witness support (r8921039)
- #15821 Remove upgrade note in release notes from EOL versions (MarcoFalke)
- #15267 explain AcceptToMemoryPoolWorker's coins_to_uncache (jamesob)
- #15887 Align code example style with clang-format (hebasto)
- #15877 Fix -dustrelayfee= argument docs grammar (keepkeyjon)
- #15908 Align MSVC build options with Linux build ones (hebasto)
- #15941 Add historical release notes for 0.18.0 (laanwj)
- #15794 Clarify PR guidelines w/re documentation (dongcarl)
- #15607 Release process updates (jonatack)
- #14364 Clarify -blocksdir usage (sangaman)
- #15777 Add doxygen comments for keypool classes (jnewbery)
- #15820 Add productivity notes for dummy rebases (dongcarl)
- #15922 Explain how to pass in non-fundamental types into functions (MarcoFalke)
- #16080 build/doc: update bitcoin_config.h packages, release process (jonatack)
- #16047 analyzepsbt description in doc/psbt.md (jonatack)
- #16039 add release note for 14954 (fanquake)
- #16139 Add riscv64 to outputs list in release-process.md (JeremyRand)
- #16140 create security policy (narula)
- #16164 update release process for SECURITY.md (jonatack)
- #16213 Remove explicit mention of versions from SECURITY.md (MarcoFalke)
- #16186 doc/lint: Fix spelling errors identified by codespell 1.15.0 (Empact)
- #16149 Rework section on ACK in CONTRIBUTING.md (MarcoFalke)
- #16196 Add release notes for 14897 & 15834 (MarcoFalke)
- #16241 add rapidcheck to vcpkg install list (fanquake)
- #16243 Remove travis badge from readme (MarcoFalke)
- #16256 remove orphaned header in developer notes (jonatack)
- #15964 Improve build-osx document formatting (giulio92)
- #16313 Fix broken link in doc/build-osx.md (jonatack)
- #16330 Use placeholder instead of key expiration date (hebasto)
- #16339 add reduce-memory.md (fanquake)
- #16347 Include static members in Doxygen (dongcarl)
- #15824 Improve netbase comments (dongcarl)
- #16430 Update bips 35, 37 and 111 status (MarcoFalke)
- #16455 Remove downgrading warning in release notes, per 0.18 branch (MarcoFalke)
- #16484 update labels in CONTRIBUTING.md (MarcoFalke)
- #16483 update Python command in msvc readme (sipsorcery)
- #16504 Add release note for the deprecated totalFee option of bumpfee (promag)
- #16448 add note on precedence of options in bitcoin.conf (fanquake)
- #16536 Update and extend benchmarking.md (ariard)
- #16530 Fix grammar and punctuation in developer notes (Tech1k)
- #16574 Add historical release notes for 0.18.1 (laanwj)
- #16585 Update Markdown syntax for bdb packages (emilengler)
- #16586 Mention other ways to conserve memory on compilation (MarcoFalke)
- #16605 Add missing contributor to 0.18.1 release notes (meshcollider)
- #16615 Fix typos in COPYRIGHT (gapeman)
- #16626 Fix spelling error chache -> cache (nilswloewen)
- #16587 Improve versionbits.h documentation (ariard)
- #16643 Add ZMQ dependencies to the Fedora build instructions (hebasto)
- #16634 Refer in rpcbind doc to the manpage (MarcoFalke)
- #16555 mention whitelist is inbound, and applies to blocksonly (Sjors)
- #16645 initial RapidCheck property-based testing documentation (jonatack)
- #16691 improve depends prefix documentation (fanquake)
- #16629 Add documentation for the new whitelist permissions (NicolasDorier)
- #16723 Update labels in CONTRIBUTING.md (hebasto)
- #16461 Tidy up shadowing section (promag)
- #16621 add default bitcoin.conf locations (GChuf)
- #16752 Delete stale URL in test README (michaelfolkson)
- #14862 Declare BLOCK_VALID_HEADER reserved (MarcoFalke)
- #16806 Add issue templates for bug and feature request (MarcoFalke)
- #16857 Elaborate need to re-login on Debian-based after usermod for Tor group (clashicly)
- #16863 Add a missing closing parenthesis in the bitcoin-wallet's help (darosior)
- #16757 CChainState return values (MarcoFalke)
- #16847 add comments clarifying how local services are advertised (jamesob)
- #16812 Fix whitespace errs in .md files, bitcoin.conf, and Info.plist.in (ch4ot1c)
- #16885 Update tx-size-small comment with relevant CVE disclosure (instagibbs)
- #16900 Fix doxygen comment for SignTransaction in rpc/rawtransaction_util (MarcoFalke)
- #16914 Update homebrew instruction for doxygen (Sjors)
- #16912 Remove Doxygen intro from src/bitcoind.cpp (ch4ot1c)
- #16960 replace outdated OpenSSL comment in test README (fanquake)
- #16968 Remove MSVC update step from translation process (laanwj)
- #16953 Improve test READMEs (fjahr)
- #16962 Put PR template in comments (laanwj)
- #16397 Clarify includeWatching for fundrawtransaction (stevenroose)
- #15459 add how to calculate blockchain and chainstate size variables to release process (marcoagner)
- #16997 Update bips.md for 0.19 (laanwj)
- #17001 Remove mention of renamed mapBlocksUnlinked (MarcoFalke)
- #17014 Consolidate release notes before 0.19.0 (move-only) (MarcoFalke)
- #17111 update bips.md with buried BIP9 deployments (MarcoFalke)

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
