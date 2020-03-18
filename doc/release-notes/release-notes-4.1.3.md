Syscoin Core version 4.1.3 is now available from:

  https://github.com/syscoin/syscoin/releases/tag/v4.1.3

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/syscoin/syscoin/issues>


Upgrade Instructions: https://syscoin.readme.io/v4.1.3/docs/syscoin-41-upgrade-guide
Basic upgrade instructions below:

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Syscoin-Qt` (on Mac)
or `syscoind`/`syscoin-qt` (on Linux).

If you are upgrading from a version older than 4.1.0, PLEASE READ: https://syscoin.readme.io/v4.1.2/docs/syscoin-41-upgrade-guide

Upgrading directly from a version of Syscoin Core that has reached its EOL is
possible, but might take some time if the datadir needs to be migrated.  Old
wallet versions of Syscoin Core are generally supported.

Compatibility
==============

Syscoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.12+, and Windows 7 and newer. It is not recommended
to use Syscoin Core on unsupported systems.

A target block height of 448000 has been set for upgrade deadline.
Nodes not upgraded to Syscoin Core 4.1.3 may have issue syncing up to the network past block 448000
After the block height, 2 new changes will be activated on the network
1. The ability to bridge all ERC-20 Standard Token to Syscoin blockchain
2. The asset guid generated from assetnew will be deterministic

The previous in-compatibility between 4.0.x and 4.1.x is the interim .node files 
(specifically `scrypt.node`) that may exist in the binary path or /usr/local/bin
 on linux that may impede the functioning of the [relayer](https://github.com/Syscoin/relayer).
The node version used to build the [relayer](https://github.com/Syscoin/relayer) 
was incremented and thus .node requirement was removed, however if the .node files 
exist, it will not run the [relayer](https://github.com/Syscoin/relayer) and as a
result Syscoin will not get the Ethereum block headers needed for consensus 
validation of Syscoin Mint transactions. 
See: https://syscoin.readme.io/v4.1.3/docs/syscoin-41-upgrade-guide

Syscoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.  Geth is an Ethereum client that runs inside of Syscoin.  The standard 64-bit Unix distribution is packaged up when installing Syscoin.  If you have a distribution that does not work with the supplied binaries, you can download the binaries from: 
https://geth.ethereum.org/downloads/ and place it in the src/bin/linux directory. Read more:
https://github.com/Syscoin/Syscoin/blob/master/src/bin/linux/README.md.

From Syscoin Core 4.1.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Syscoin Core does not yet change appearance
when macOS "dark mode" is activated.

Notable changes
===============

Syscoin Core Changes
--------------------

### ZDAG Fixes and Optimizations

- RBF (replace-by-fee) has been added and updated to work properly with assetallocation transactions.
  This includes correcting the behavior of handling double spend for ZDAG transactions
- Doubled the relay fee of ZDAG transaction to align with the bandwidth usage
- Amended ZDAG rules for optimization
   - Sender balance has to be within the PoW balance within a block.  This allow us to avoid doing ordering algorithms in mempool and speed up the process
- Refactored ZDAG code for optimization
- Fixed [#390](https://github.com/syscoin/syscoin/issues/390)

### Asset Protocol Fixes

- Enabled detereminstic asset guid so someone cannot frontrun assets unknowingly to the user.  This change will kick in automatically after blockheight 448000
- Fixed issue with re-org when disconnecting blocks that contain mint asset transactions
remove resync on miner 
- Fixed inconsistent TXIDs due to uninitialized CAmount in OP_RETURN
- Fixed issue that allowed assetnew to have a supply balance of 0 

### Masternode fixes
- Fixed masternodes counting against outbound resulting in extra count

### Bridge Fixes
- Added support for all ERC-20. Previously we were only bridging to openzeppelin standard ERC20's as a precautionary mechanism. This feature unlocks the bridge to all standard ERC-20 implementations.  This will be enabled automatically after blockheight 448000

### Auxpow Fixes
- Syned up auxpow code to the Bitcoin change where generate() is replaced by generatetoaddress()

### New Syscoind Argument
- `zmqpubrawmempooltx` has been added for publishing raw transactions in address when entering mempool only
- `zmqpubrawmempooltxhwm` has been added for setting publish raw mempool transaction outbound message high water mark

### Ethereum Client Version Upgrade
- The Geth client bundled has been upgraded from v1.9.9 to v1.9.12


P2P and network changes
-----------------------

#### Removal of reject network messages from Bitcoin Core (BIP61)

The command line option to enable BIP61 (`-enablebip61`) has been removed.

This feature has been disabled by default since Syscoin Core version 4.1.0.
Nodes on the network can not generally be trusted to send valid ("reject")
messages, so this should only ever be used when connected to a trusted node.
Please use the recommended alternatives if you rely on this deprecated feature:

* Testing or debugging of implementations of the Bitcoin P2P network protocol
  should be done by inspecting the log messages that are produced by a recent
  version of Bitcoin Core. Bitcoin Core logs debug messages
  (`-debug=<category>`) to a stream (`-printtoconsole`) or to a file
  (`-debuglogfile=<debug.log>`).

* Testing the validity of a block can be achieved by specific RPCs:
  - `submitblock`
  - `getblocktemplate` with `'mode'` set to `'proposal'` for blocks with
    potentially invalid POW

* Testing the validity of a transaction can be achieved by specific RPCs:
  - `sendrawtransaction`
  - `testmempoolaccept`

* Wallets should not use the absence of "reject" messages to indicate a
  transaction has propagated the network, nor should wallets use "reject"
  messages to set transaction fees. Wallets should rather use fee estimation
  to determine transaction fees and set replace-by-fee if desired. Thus, they
  could wait until the transaction has confirmed (taking into account the fee
  target they set (compare the RPC `estimatesmartfee`)) or listen for the
  transaction announcement by other network peers to check for propagation.

The removal of BIP61 REJECT message support also has the following minor RPC
and logging implications:

* `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. They still return the
  verbal reject reason.

* Log messages that previously reported the REJECT code when a transaction was
  not accepted to the mempool now no longer report the REJECT code. The reason
  for rejection is still reported.

Updated RPCs
------------

- `testmempoolaccept` and `sendrawtransaction` no longer return the P2P REJECT
  code when a transaction is not accepted to the mempool. See the Section
  _Removal of reject network messages from Bitcoin Core (BIP61)_ for details on
  the removal of BIP61 REJECT message support.

- A new descriptor type `sortedmulti(...)` has been added to support multisig scripts where the public keys are sorted lexicographically in the resulting script.

- `walletprocesspsbt` and `walletcreatefundedpsbt` now include BIP 32 derivation paths by default for public keys if we know them. This can be disabled by setting `bip32derivs` to `false`.

Build System
------------

- OpenSSL is no longer used by Syscoin Core. The last usage of the library
was removed in #17265.

- glibc 2.17 or greater is now required to run the release binaries. This
retains compatibility with RHEL 7, CentOS 7, Debian 8 and Ubuntu 14.04 LTS.
Further details can be found in #17538.

New RPCs
--------

New settings
------------

- RPC Whitelist system. It can give certain RPC users permissions to only some RPC calls.
It can be set with two command line arguments (`rpcwhitelist` and `rpcwhitelistdefault`). (#12763)

Updated settings
----------------

Importing blocks upon startup via the `bootstrap.dat` file no longer occurs by default. The file must now be specified with `-loadblock=<file>`.

-  The `-debug=db` logging category has been renamed to `-debug=walletdb`, to distinguish it from `coindb`.
   `-debug=db` has been deprecated and will be removed in the next major release.

GUI changes
-----------

- The "Start Syscoin Core on system login" option has been removed on macOS.

Wallet
------

=======
- The wallet now by default uses bech32 addresses when using RPC, and creates native segwit change outputs.
- The way that output trust was computed has been fixed in #16766, which impacts confirmed/unconfirmed balance status and coin selection.

- The RPC gettransaction, listtransactions and listsinceblock responses now also
includes the height of the block that contains the wallet transaction, if any.

- RPC `getaddressinfo` changes:

  - the `label` field has been deprecated in favor of the `labels` field and
    will be removed in an upcoming release. It can be re-enabled in the interim by launching
    with `-deprecatedrpc=label`.

  - the `labels` behavior of returning an array of JSON objects containing name
    and purpose key/value pairs has been deprecated in favor of an array of
    label names and will be removed in 0.21. The previous behavior can be
    re-enabled in the interim by launching with `-deprecatedrpc=labelspurpose`.

Low-level changes
=================

Command line
------------

Command line options prefixed with main/test/regtest network names like
`-main.port=8333` `-test.server=1` previously were allowed but ignored. Now
they trigger "Invalid parameter" errors on startup.

Tests
-----

- It is now an error to use an unqualified `walletdir=path` setting in the config file if running on testnet or regtest
  networks. The setting now needs to be qualified as `chain.walletdir=path` or placed in the appropriate `[chain]`
  section. (#17447)

- `-fallbackfee` was 0 (disabled) by default for the main chain, but 0.0002 by default for the test chains. Now it is 0
  by default for all chains. Testnet and regtest users will have to add `fallbackfee=0.0002` to their configuration if
  they weren't setting it and they want it to keep working like before. (#16524)

4.1.3 Change Log
================

4d55397500:
- Update merkle.cpp

0xb10c:
- add: test that transactions expire from mempool

Aaron Clauson:
- Ignore msvc linker warning and update to msvc build instructions.
- Updated appveyor job to checkout a specific vcpkg commit ID.

Amiti Uttarwar:
- [util] allow scheduler to be mocked
- [test] unit test for new MockForward scheduler method
- [test] add chainparams property to indicate chain allows time mocking
- [lib] add scheduler to node context
- [rpc] expose ability to mock scheduler via the rpc

Andrew Toth:
- Add missing supported rpcs to doc/descriptors.md

Anthony Towns:
- psbt_wallet_tests: use unique_ptr for GetSigningProvider
- scheduler: don't rely on boost interrupt on shutdown
- sync.h: add REVERSE_LOCK
- scheduler: switch from boost to std
- Drop unused reverselock.h
- scheduler_tests: re-enable mockforward test
- lint-cppcheck: Remove -DHAVE_WORKING_BOOST_SLEEP_FOR

Ben Woosley:
- Fix improper Doxygen inline comments
- doc: Correct spelling errors in comments
- refactor: Convert ping time from double to int64_t
- refactor: Convert min ping time from double to int64_t
- refactor: Convert ping wait time from double to int64_t
- refactor: Cast ping values to double before output

Carl Dong:
- guix: Pin Guix using `guix time-machine`
- guix: Update documentation for time-machine

Dan Gershony:
- Add missing step in win deployment instructions

Elichai Turkel:
- Replace coroutine with async def in p2p_invalid_messages.py
- Fix benchmarks filters

Fabian Jahr:
- doc: Improve fuzzing docs for macOS users

Gloria Zhao:
- [rpc] changed MineBlocksOnDemand to IsMockableChain

Gregory Sanders:
- IsUsedDestination shouldn't use key id as script id for ScriptHash
- Don't allow implementers to think ScriptHash(Witness*()) results in nesting computation
- Add some test logging to wallet_bumpfee.py
- bumpfee test: exit loop at proper time with new fee value being compared

Harris:
- gui: hide HD & encryption icons when no wallet loaded

Hennadii Stepanov:
- refactor: Remove never used default parameter
- refactor: Simplify connection syntax
- Revert "refactor: Simplify connection syntax"
- Revert "refactor: Remove never used default parameter"
- qt: Fix deprecated QCharRef usage
- Specify ignored bitcoin-qt file precisely
- Ignore only auto-generated .vcxproj files
- gui: Throttle GUI update pace when -reindex

Jon Atack:
- net: reference instead of copy in BlockConnected range loop
- test: add feature_asmap functional tests
- config: use default value in -asmap config
- config: enable passing -asmap an absolute file path
- config: separate the asmap finding and parsing checks
- test: add functional test for an empty, unparsable asmap
- logging: asmap logging and #include fixups
- net: extract conditional to bool CNetAddr::IsHeNet
- rpc: fix getpeerinfo RPCResult `mapped_as` type
- init: move asmap code earlier in init process
- test: add logging to wallet_listsinceblock.py

Jonas Schnelli:
- Merge #17998: gui: Shortcut to close ModalOverlay
- Merge #17096: gui: rename debug window
- ZDAG fixes + miner code related to input conflicts
- Merge #17453: gui: Fix intro dialog labels when the prune button is toggled
- Merge #17937: gui: Remove WalletView and BitcoinGUI circular dependency
- Qt: pass clientmodel changes from walletframe to walletviews

John Newbery:
- tests: simplify next_block() function in feature_block

João Barbosa:
- gui: Remove warning "unused variable 'wallet_model'"
- wallet: Improve CWallet:MarkDestinationsDirty
- gui: Add transactionClicked and coinsSent signals to WalletView
- gui: Remove WalletView and BitcoinGUI circular dependency
- gui: Drop BanTableModel dependency to ClientModel
- gui: Drop ShutdownWindow dependency to BitcoinGUI
- gui: Drop PeerTableModel dependency to ClientModel
- gui: Fix unintialized WalletView::progressDialog
- refactor: rpc: Remove vector copy from listtransactions
- qa: Add getdescriptorinfo functional test
- Fix missing header in sync.h

Karl-Johan Alm:
- test: add missing #include to fix compiler errors
- rpc/wallet: initialize nFeeRequired to avoid using garbage value on failure

Larry Ruane:
- on startup, write config options to debug.log

Luke Dashjr:
- GUI: Use PACKAGE_NAME in modal overlay
- bitcoin-wallet: Use PACKAGE_NAME in usage help
- Bugfix: GUI: Hide the HD/encrypt icons earlier so they get re-shown if another wallet is open

MarcoFalke:
- Merge #18371: test: use fs namespace in dbwrapper unicodepath test
- Merge #18155: tests: Add harness which fuzzes EvalScript and VerifyScript using a fuzzed signature checker
- Merge #18300: fuzz: Add option to merge input dir to test runner
- Merge #18350: test: Fix mining to an invalid target + ensure that a new block has the correct hash internally
- Merge #18353: tests: Add fuzzing harnesses for classes CBlockHeader, CFeeRate and various functions
- Merge #17319: Tests: remove bignum module
- Merge #15283: log: Fix UB with bench on genesis block
- Merge #18289: refactor: Make scheduler methods type safe
- scripted-diff: Replace CCriticalSection with RecursiveMutex
- scripted-diff: Bump copyright of files changed in 2020
- Merge #17819: doc: developer notes guideline on RPCExamples addresses
- Merge #17541: test: add functional test for non-standard bare multisig txs
- Merge #17900: ci: Combine 32-bit build with CentOS 7 build
- Merge #17691: doc: Add missed copyright headers
- ensure balance is 0 for xfer
- build: Fix appveyor test_bitcoin build of *.raw
- Merge #16681: Tests: Use self.chain instead of 'regtest' in all current tests
- Merge #18032: rpc: Output a descriptor in createmultisig and addmultisigaddress
- scripted-diff: Add missing spaces in RPCResult, Fix type names
- Squashed 'src/univalue/' changes from 5a58a46671..98261b1e7b
- Update univalue subtree
- depends: Remove reference to win32
- build: Skip i686 build by default in guix and gitian
- Merge #18037: Util: Allow scheduler to be mocked
- Merge #18166: ci: Run fuzz testing test cases (bitcoin-core/qa-assets) under valgrind to catch memory errors
- test: Set catch_system_errors=no on boost unit tests
- Merge #18183: test: Set catch_system_errors=no on boost unit tests
- Merge #18181: test: Remove incorrect assumptions in validation_flush_tests
- Merge #18193: scripted-diff: Wallet: Rename incorrectly named *UsedDestination
- remove message sign/verify in messagesigner, not used
- Merge #17771: tests: Add fuzzing harness for V1TransportDeserializer (P2P transport)
- Merge #17461: test: check custom descendant limit in mempool_packages.py
- Merge #18195: test: Add cost_of_change parameter assertions to bnb_search_test
- Merge #16562: Refactor message transport packaging
- Merge #17399: validation: Templatize ValidationState instead of subclassing
- Merge #17809: rpc: Auto-format RPCResult
- doc: Merge release notes for 0.20.0 release
- Merge #18109: tests: Avoid hitting some known minor tinyformat issues when fuzzing strprintf(...)
- Merge #17917: tests: Add amount compression/decompression fuzzing to existing fuzzing harness
- Merge #17996: tests: Add fuzzing harness for serialization/deserialization of floating-points and integrals
- use utxo as input for deterministic asset guid
- Merge #18292: fuzz: Add assert(script == decompressed_script)
- Merge #18047: tests: Add basic fuzzing harness for CNetAddr/CService/CSubNet related functions (netaddress.h)
- Merge #18176: tests: Add fuzzing harness for CScript and CScriptNum operations
- test: Explain why test logging should be used
- ci: Enable all functional tests in valgrind
- doc: Explain rebase/squash policy in CONTRIBUTING.md
- Merge #18310: doc: asmap release note
- Merge #17159: doc: Add a note about backporting
- Merge #17833: doc: Added running functional tests in valgrind
- Merge #18170: doc: Minor grammatical changes and flow improvements
- Merge #18219: doc: Add warning against wallet.dat re-use
- Merge #18208: rpc: Change RPCExamples to bech32
- test: Bump walletpassphrase timeouts in wallet_createwallet to avoid valgrind timeouts
- Merge #18268: rpc: Remove redundant types from descriptions
- test: Bump rpc timeout in feature_assumevalid to avoid valgrind timeouts
- fuzz: Add missing ECC_Start to key_io test
- Merge #13693: [test] Add coverage to estimaterawfee and estimatesmartfee
- Merge #18213: test: Fix race in p2p_segwit
- Merge #18228: test: Add missing syncwithvalidationinterfacequeue
- test: Bump timeouts to avoid valgrind failures
- Merge #17997: refactor: Remove mempool global from net

Micky Yun Chan:
- bump test timeouts so that functional tests run in valgrind

Peter Bushnell:
- depends: Consistent use of package variable

Pieter Wuille:
- Add custom vector-element formatter
- Make std::vector and prevector reuse the VectorFormatter logic
- Convert undo.h to new serialization framework
- Get rid of VARINT default argument

Russell Yanofsky:
- gui: Set CConnman byte counters earlier to avoid uninitialized reads
- gui: Fix race in WalletModel::pollBalanceChanged

Samuel Dobson:
- Merge #17843: wallet: Reset reused transactions cache
- Merge #17719: Document better -keypool as a look-ahead safety mechanism
- Merge #17261: Make ScriptPubKeyMan an actual interface and the wallet to have multiple
- Merge #17585: rpc: deprecate getaddressinfo label
- Merge #18067: wallet: Improve LegacyScriptPubKeyMan::CanProvide script recognition
- Merge #18034: Get the OutputType for a descriptor
- Merge #17577: refactor: deduplicate the message sign/verify code
- Merge #17264: rpc: set default bip32derivs to true for psbt methods
- Merge #18224: Make AnalyzePSBT next role calculation simple, correct
- Merge #18115: wallet: Pass in transactions and messages for signing instead of exporting the private keys

Sebastian Falbesoner:
- test: rename test suite name "tx_validationcache_tests" to match filename
- test: replace 'regtest' leftovers by self.chain
- test: test OP_CSV empty stack fail in feature_csv_activation.py
- test: check for OP_CSV empty stack fail reject reason in feature_csv_activation.py
- test: eliminiated magic numbers in feature_csv_activation.py
- test: check specific reject reasons in feature_csv_activation.py
- refactor: test/bench: dedup SetupDummyInputs()

Sjors Provoost:
- [scripts] build earlier releases
- [tests] check v0.17.1 and v0.18.1 backwards compatibility
- [scripts] support release candidates of earlier releases
- [tests] add wallet backwards compatility tests
- [test] add v0.17.1 wallet upgrade test
- [test] add 0.19 backwards compatibility tests
- build: add Wreturn-type to Werror flags
- ci: use --enable-werror on more hosts

Suhas Daftuar:
- Use rolling bloom filter of recent block tx's for AlreadyHave() check

Willy Ko:
- Updated Geth Binaries to 1.9.10
- Updated Geth Binaries to 1.9.11

Wladimir J. van der Laan:
- Merge #18364: random: remove getentropy() fallback for macOS < 10.12
- Merge #16688: log: Add validation interface logging
- Merge #16945: refactor: introduce CChainState::GetCoinsCacheSizeState
- Merge #17823: scripts: Read suspicious hosts from a file instead of hardcoding
- Merge #17945: doc: Fix doxygen errors
- Merge #17777: tests: Add fuzzing harness for DecodeHexTx(…)
- Merge #17916: windows: Enable heap terminate-on-corruption
- Merge #17887: bug-fix macos: give free bytes to F_PREALLOCATE
- Merge #17754: net: Don't allow resolving of std::string with embedded NUL characters. Add tests.
- Merge #17863: scripts: Add MACHO dylib checks to symbol-check.py
- Merge #17767: ci: Fix qemu issues
- Merge #17738: build: remove linking librt for backwards compatibility
- Merge #16702: p2p: supplying and using asmap to improve IP bucketing in addrman
- Merge #17957: Serialization improvements step 3 (compression.h)
- Merge #17984: test: Add p2p test for forcerelay permission
- Merge #17925: Improve UpdateTransactionsFromBlock with Epochs
- Merge #16974: Walk pindexBestHeader back to ChainActive().Tip() if it is invalid
- Merge #18029: tests: Add fuzzing harness for AS-mapping (asmap)
- Merge #18023: Fix some asmap issues
- Merge #17660: build: remove deprecated key from macOS Info.plist
- Merge #18052: Remove false positive GCC warning
- Merge #17804: doc: Misc RPC help fixes
- Merge #17482: util: Disallow network-qualified command line options
- Merge #17398: build: Update leveldb to 1.22+
- test: Disable s390 build on travis
- Merge #18021: Serialization improvements step 4 (undo.h)
- Merge #17947: test: add unit test for non-standard txs with too large tx size
- Merge #18051: build: Fix behavior when ALLOW_HOST_PACKAGES unset
- Merge #17708: prevector: avoid misaligned member accesses
- Merge #13339: wallet: Replace %w by wallet name in -walletnotify script
- Merge #17985: net: Remove forcerelay of rejected txs
- Merge #18167: Fix a violation of C++ standard rules where unions are used for type-punning
- Merge #18135: build: add --enable-determinism configure flag
- Merge #17800: random: don't special case clock usage on macOS
- Merge #18168: httpserver: use own HTTP status codes
- Merge #18056: ci: Check for submodules
- Merge #18112: Serialization improvements step 5 (blockencodings)
- doc: Add historical release notes for 0.19.1
- Merge #18285: test: Check that wait_until returns if time point is in the past
- Merge #18290: build: Set minimum Automake version to 1.13
- Merge #18255: test: Add bad-txns-*-toolarge test cases to invalid_txs
- Merge #18002: Abstract out script execution out of VerifyWitnessProgram()
- Merge #18204: descriptors: improve descriptor cache and cache xpubs
- Merge #16902: O(1) OP_IF/NOTIF/ELSE/ENDIF script implementation
- qt: Periodical translations update
- tx: Bump transifex slug to 020x
- Merge #18341: doc: Replace remaining literal BTC with CURRENCY_UNIT.
- Merge #18344: doc: Fix nit in getblockchaininfo
- Merge #18346: rpc: Document an RPCResult for all calls; Enforce at compile time

darosior:
- src/init: correct a typo

fanquake:
- Merge #17899: msvc: Ignore msvc linker warning and update to msvc build instructions.
- Merge #17893: qa: Fix double-negative arg test
- build: remove double LIBBITCOIN_SERVER linking
- Merge #17873: doc: Add to Doxygen documentation guidelines
- fix wallet governance related commands
- Merge #17896: Serialization improvements (step 2)
- Merge #17492: QT: bump fee returns PSBT on clipboard for watchonly-only wallets
- Merge #17897: init: Stop indexes on shutdown after ChainStateFlushed callback.
- build: remove configure checks for win libraries we don't link against
- Merge #17740: build: remove configure checks for win libraries we don't link against
- test: only declare a main() when fuzzing with AFL
- Merge #17156: psbt: check that various indexes and amounts are within bounds
- Merge #17971: refactor: Remove redundant conditional
- tests: reset fIsBareMultisigStd after bare-multisig tests
- depends: clang 6.0.1
- depends: native_cctools 921, ld64 409.12, libtapi 1000.10.8
- build: use macOS 10.14 SDK
- build: add additional attributes to Win installer
- Merge #17336: scripts: search for first block file for linearize-data with some block files pruned
- Merge #18003: build: remove --large-address-aware linker flag
- refactor our bMiner replace with bSanityCheck
- build: don't embed a build-id when building libdmg-hfsplus
- logging: enable thread_local usage on macOS
- test: set a name for CI Docker containers
- doc: remove PPA note from release-process.md
- build: pass -fno-ident in Windows gitian descriptor
- Merge #18122: rpc: update validateaddress RPCExamples to bech32
- Merge #18070: doc: add note about `brew doctor`
- Merge #18162: util: Avoid potential uninitialized read in FormatISO8601DateTime(int64_t) by checking gmtime_s/gmtime_r return value
- Merge #18211: test: Disable mockforward scheduler unit test for now
- Merge #18209: test: Reduce unneeded whitelist permissions in tests
- Merge #18225: util: Fail to parse empty string in ParseMoney
- Merge #18229: random: drop unused MACH time headers
- Merge #18249: test: Bump timeouts to accomodate really slow disks
- Merge #16117: util: Replace boost sleep with std sleep
- Merge #18241: wallet/refactor: refer to CWallet immutably when possible
- Merge #18286: build: Add locale fuzzer to FUZZERS_MISSING_CORPORA (and unbreak Travis! :))
- Merge #18264: build: Remove Boost Chrono
- Merge #18320: guix: Remove now-unnecessary gcc make flag
- Merge #18316: util: HelpExampleRpc formatting

practicalswift:
- tests: Update FuzzedDataProvider.h from upstream (LLVM)
- tests: Add fuzzer strprintf to FUZZERS_MISSING_CORPORA (temporarily)
- tests: Add fuzzing harness for strprintf(...)
- tests: Add --valgrind option to test/fuzz/test_runner.py for running fuzzing test cases under valgrind
- tests: Remove -detect_leaks=0 from test/fuzz/test_runner.py - no longer needed
- tests: Add support for excluding fuzz targets using -x/--exclude
- tests: Add --exclude integer,parse_iso8601 (temporarily) to make Travis pass until uninitialized read issue in FormatISO8601DateTime is fixed
- tests: Improve test runner output in case of target errors
- tests: Add fuzzing harness for bloom filter class CBloomFilter
- tests: Add fuzzing harness for rolling bloom filter class CRollingBloomFilter
- compressor: Make the domain of CompressAmount(...) explicit
- tests: Add serialization/deserialization fuzzing for integral types
- tests: Add float to FUZZERS_MISSING_CORPORA (temporarily)
- tests: Add fuzzing harness for CKey related functions
- tests: Add fuzzing harness for locale independence testing
- build: Add locale fuzzer to FUZZERS_MISSING_CORPORA
- tests: Add key_io fuzzing harness
- tests: Fuzz additional functions in the hex fuzzing harness
- tests: Fuzz additional functions in the integer fuzzing harness
- tests: Fuzz additional functions in the transaction fuzzing harness
- tests: Fuzz additional functions in the script fuzzing harness
- tests: Reset FUZZERS_MISSING_CORPORA to enable regression fuzzing for more harnesses
- tests: Remove FUZZERS_MISSING_CORPORA
- tests: Add fuzzing harness for CScriptNum operations
- Make lifetime correctness easier to see (avoid reference lifetime extension)
- tests: Add fuzzing harness for ProcessMessage(...)
- tests: Add one specialized ProcessMessage(...) fuzzing binary per message type for optimal results when using coverage-guided fuzzing
- tests: Add deserialization fuzzing of SnapshotMetadata (utxo_snapshot), uint160 and uint256
- tests: Remove unit test from fuzzing harness
- tests: Re-arrange test cases in parse_univalue to increase coverage
- tests: Fuzz currently uncovered code path in TxToUniv(...)
- tests: Increase fuzzing coverage of DecompressScript(...)
- tests: Fuzz operator!= of CService
- tests: Fuzz RecursiveDynamicUsage(const std::shared_ptr<X>& p)
- tests: Fuzz DecodeHexBlk(...)
- tests: Simplify code by removing unwarranted use of unique_ptr:s
- tests: Fuzz DecodeBase64PSBT(...)
- tests: Fuzz HasAllDesirableServiceFlags(...) and MayHaveUsefulAddressDB(...)
- tests: Add fuzzing of CSubNet, CNetAddr and CService related functions

sidhujag:
- masternodes shouldn't count against extra outbound count
- uninited camount shows up on opreturn giving in consistent txids
- add raw tx zmq message for mempool inclusion specifically
- add hwm for memool tx
- create seperate notifier for mempool tx
- mempool tx should be false not true
- sync up auxpow by removing generate() replacing with generatetoaddress()
- enforce tx in and out sizes for zdag transactions
- fixes #390
- rmv enforcement
- add existsConflict logic
- order by time
- remove graph ordering by time logic
- check for log counts properly to account for different erc20 standards
- remove resync on miner
- work on zdag fixes
- refactor and clean up zdag
- fix node merge
- save checksyscoininputs state seperately
- create iszdagtx type and use it to store zdag structures
- dbl costs and set rbf opt out for zdag txs only
- remove bridge start block logic that doesn't have any affect now
- fix mempool emplace only when zdag tx
- sanity checks on allocation types
- ensure no RBF for neighbouring txs as well
- fix disconnectmintasset
- fix precision test
- add assetindex back temporarily for spark
- ubunt 16 fix build
- update checkpoints and other settings
- spelling fixes for CI
- ci auxpow
- remove spellcheck for getwork wrapper
- include check.h
- remove fuzz CI's for now
- cleanup validation removal from sys code
- chain active check around syscoin consensus
- deterministic asset guids
- add height check around asset guid feature
- receipts log check around the start block for compatibility
- remove static empty asset/allocation use tmp var not static for consistency and avoid intermittent trampling
- Revert "remove static empty asset/allocation use tmp var not static for consistency and avoid intermittent trampling"
- remove moves because of asset index using these variables
- add tests and remove chainactive check for sys consensus
- support rbf + allocation dbl spend
- allow RBF to work properly with asset allocation txs
- fix setconflicts check
- update slug
- rpc: Document an RPCResult for all calls;

Vasil Dimov:
- net: fix use-after-free in tests

wincss:
- fix masternode list-conf bug


Credits
=======

Thanks to everyone who directly contributed to this release:

