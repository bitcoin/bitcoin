(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

0.11.0 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors or string updates.

### RPC and REST
- #5461 `5f7279a` signrawtransaction: validate private key
- #5444 `103f66b` Add /rest/headers/<count>/<hash>.<ext>
- #4964 `95ecc0a` Add scriptPubKey field to validateaddress RPC call
- #5476 `c986972` Add time offset into getpeerinfo output
- #5540 `84eba47` Add unconfirmed and immature balances to getwalletinfo
- #5599 `40e96a3` Get rid of the internal miner's hashmeter
- #5711 `87ecfb0` Push down RPC locks
- #5754 `1c4e3f9` fix getblocktemplate lock issue
- #5756 `5d901d8` Fix getblocktemplate_proposals test by mining one block
- #5548 `d48ce48` Add /rest/chaininfos
- #5992 `4c4f1b4` Push down RPC reqWallet flag
- #6036 `585b5db` Show zero value txouts in listunspent
- #5199 `6364408` Add RPC call `gettxoutproof` to generate and verify merkle blocks
- #5418 `16341cc` Report missing inputs in sendrawtransaction
- #5937 `40f5e8d` show script verification errors in signrawtransaction result
- #5420 `1fd2d39` [REST] getutxos REST command (based on Bip64)

### Configuration and command-line options
- #5636 `a353ad4` Add option `-allowselfsignedrootcertificate` to allow self signed root certs (for testing payment requests)
- #5900 `3e8a1f2` Add a consistency check `-checkblockindex` for the block chain data structures
- #5951 `7efc9cf` Make it possible to disable wallet transaction broadcast (using `-walletbroadcast=0`)
- #5911 `b6ea3bc` privacy: Stream isolation for Tor (on by default, use `-proxyrandomize=0` to disable)
- #5863 `c271304` Add autoprune functionality (`-prune=<size>`)

### Block and transaction handling
- #5367 `dcc1304` Do all block index writes in a batch
- #5253 `203632d` Check against MANDATORY flags prior to accepting to mempool
- #5459 `4406c3e` Reject headers that build on an invalid parent
- #5481 `055f3ae` Apply AreSane() checks to the fees from the network
- #5580 `40d65eb` Preemptively catch a few potential bugs
- #5349 `f55c5e9` Implement test for merkle tree malleability in CPartialMerkleTree
- #5564 `a89b837` clarify obscure uses of EvalScript()
- #5521 `8e4578a` Reject non-final txs even in testnet/regtest
- #5707 `6af674e` Change hardcoded character constants to descriptive named constants for db keys
- #5286 `fcf646c` Change the default maximum OP_RETURN size to 80 bytes
- #5710 `175d86e` Add more information to errors in ReadBlockFromDisk
- #5948 `b36f1ce` Use GetAncestor to compute new target
- #5959 `a0bfc69` Add additional block index consistency checks
- #6058 `7e0e7f8` autoprune minor post-merge improvements
- #5159 `2cc1372` New fee estimation code
- #6102 `6fb90d8` Implement accurate UTXO cache size accounting
- #6129 `2a82298` Bug fix for clearing fCheckForPruning
- #5947 `e9af4e6` Alert if it is very likely we are getting a bad chain

### P2P protocol and network code
- #5507 `844ace9` Prevent DOS attacks on in-flight data structures
- #5770 `32a8b6a` Sanitize command strings before logging them
- #5859 `dd4ffce` Add correct bool combiner for net signals
- #5876 `8e4fd0c` Add a NODE_GETUTXO service bit and document NODE_NETWORK.
- #6028 `b9311fb` Move nLastTry from CAddress to CAddrInfo
- #5662 `5048465` Change download logic to allow calling getdata on inbound peers
- #5971 `18d2832` replace absolute sleep with conditional wait
- #5918 `7bf5d5e` Use equivalent PoW for non-main-chain requests
- #6059 `f026ab6` chainparams: use SeedSpec6's rather than CAddress's for fixed seeds
- #6080 `31c0bf1` Add jonasschnellis dns seeder
- #5976 `9f7809f` Reduce download timeouts as blocks arrive

### Validation
- #5143 `48e1765` Implement BIP62 rule 6
- #5713 `41e6e4c` Implement BIP66

### Build system
- #5501 `c76c9d2` Add mips, mipsel and aarch64 to depends platforms
- #5334 `cf87536` libbitcoinconsensus: Add pkg-config support
- #5514 `ed11d53` Fix 'make distcheck'
- #5505 `a99ef7d` Build winshutdownmonitor.cpp on Windows only
- #5582 `e8a6639` Osx toolchain update
- #5684 `ab64022` osx: bump build sdk to 10.9
- #5695 `23ef5b7` depends: latest config.guess and config.sub
- #5509 `31dedb4` Fixes when compiling in c++11 mode
- #5819 `f8e68f7` release: use static libstdc++ and disable reduced exports by default
- #5510 `7c3fbc3` Big endian support
- #5149 `c7abfa5` Add script to verify all merge commits are signed
- #6082 `7abbb7e` qt: disable qt tests when one of the checks for the gui fails

### Wallet
- #2340 `811c71d` Discourage fee sniping with nLockTime
- #5485 `d01bcc4` Enforce minRelayTxFee on wallet created tx and add a maxtxfee option.
- #5508 `9a5cabf` Add RandAddSeedPerfmon to MakeNewKey
- #4805 `8204e19` Do not flush the wallet in AddToWalletIfInvolvingMe(..)
- #5319 `93b7544` Clean up wallet encryption code
- #5831 `df5c246` Subtract fee from amount
- #6076 `6c97fd1` wallet: fix boost::get usage with boost 1.58
- #5511 `23c998d` Sort pending wallet transactions before reaccepting
- #6126 `26e08a1` Change default nTxConfirmTarget to 2

### GUI
- #5219 `f3af0c8` New icons
- #5228 `bb3c75b` HiDPI (retina) support for splash screen
- #5258 `73cbf0a` The RPC Console should be a QWidget to make window more independent
- #5488 `851dfc7` Light blue icon color for regtest
- #5547 `a39aa74` New icon for the debug window
- #5493 `e515309` Adopt style colour for button icons
- #5557 `70477a0` On close of splashscreen interrupt verifyDB
- #5559 `83be8fd` Make the command-line-args dialog better
- #5144 `c5380a9` Elaborate on signverify message dialog warning
- #5489 `d1aa3c6` Optimize PNG files
- #5649 `e0cd2f5` Use text-color icons for system tray Send/Receive menu entries
- #5651 `848f55d` Coin Control: Use U+2248 "ALMOST EQUAL TO" rather than a simple tilde
- #5626 `ab0d798` Fix icon sizes and column width
- #5683 `c7b22aa` add new osx dmg background picture
- #5620 `7823598` Payment request expiration bug fix
- #5729 `9c4a5a5` Allow unit changes for read-only BitcoinAmountField
- #5753 `0f44672` Add bitcoin logo to about screen
- #5629 `a956586` Prevent amount overflow problem with payment requests
- #5830 `215475a` Don't save geometry for options and about/help window
- #5793 `d26f0b2` Honor current network when creating autostart link
- #5847 `f238add` Startup script for centos, with documentation
- #5915 `5bd3a92` Fix a static qt5 crash when using certain versions of libxcb
- #5898 `bb56781` Fix rpc console font size to flexible metrics
- #5467 `bc8535b` Payment request / server work - part 2
- #6161 `180c164` Remove movable option for toolbar
- #6160 `0d862c2` Overviewpage: make sure warning icons gets colored

### Tests
- #5453 `2f2d337` Add ability to run single test manually to RPC tests
- #5421 `886eb57` Test unexecuted OP_CODESEPARATOR
- #5530 `565b300` Additional rpc tests
- #5611 `37b185c` Fix spurious windows test failures after 012598880c
- #5613 `2eda47b` Fix smartfees test for change to relay policy
- #5612 `e3f5727` Fix zapwallettxes test
- #5642 `30a5b5f` Prepare paymentservertests for new unit tests
- #5784 `e3a3cd7` Fix usage of NegateSignatureS in script_tests
- #5813 `ee9f2bf` Add unit tests for next difficulty calculations
- #5855 `d7989c0` Travis: run unit tests in different orders
- #5852 `cdae53e` Reinitialize state in between individual unit tests.
- #5883 `164d7b6` tests: add a BasicTestingSetup and apply to all tests
- #5940 `446bb70` Regression test for ResendWalletTransactions
- #6052 `cf7adad` fix and enable bip32 unit test
- #6039 `734f80a` tests: Error when setgenerate is used on regtest
- #6074 `948beaf` Correct the PUSHDATA4 minimal encoding test in script_invalid.json
- #6032 `e08886d` Stop nodes after RPC tests, even with --nocleanup
- #6075 `df1609f` Add additional script edge condition tests
- #5981 `da38dc6` Python P2P testing 
- #5958 `9ef00c3` Add multisig rpc tests
- #6112 `fec5c0e` Add more script edge condition tests

### Miscellaneous
- #5457, #5506, #5952, #6047 Update libsecp256k1
- #5437 `84857e8` Add missing CAutoFile::IsNull() check in main
- #5490 `ec20fd7` Replace uint256/uint160 with opaque blobs where possible
- #5654, #5764 Adding jonasschnelli's GPG key
- #5477 `5f04d1d` OS X 10.10: LSSharedFileListItemResolve() is deprecated
- #5679 `beff11a` Get rid of DetectShutdownThread
- #5787 `9bd8c9b` Add fanquake PGP key
- #5366 `47a79bb` No longer check osx compatibility in RenameThread
- #5689 `07f4386` openssl: abstract out OPENSSL_cleanse
- #5708 `8b298ca` Add list of implemented BIPs
- #5809 `46bfbe7` Add bitcoin-cli man page
- #5839 `86eb461` keys: remove libsecp256k1 verification until it's actually supported
- #5749 `d734d87` Help messages correctly formatted (79 chars)
- #5884 `7077fe6` BUGFIX: Stack around the variable 'rv' was corrupted
- #5849 `41259ca` contrib/init/bitcoind.openrc: Compatibility with previous OpenRC init script variables
- #5950 `41113e3` Fix locale fallback and guard tests against invalid locale settings
- #5965 `7c6bfb1` Add git-subtree-check.sh script
- #6033 `1623f6e` FreeBSD, OpenBSD thread renaming
- #6064 `b46e7c2` Several changes to mruset
- #6104 `3e2559c` Show an init message while activating best chain
- #6125 `351f73e` Clean up parsing of bool command line args
- #5964 `b4c219b` Lightweight task scheduler
- #6116 `30dc3c1` [OSX] rename Bitcoin-Qt.app to Bitcoin-Core.app
- #6168 `b3024f0` contrib/linearize: Support linearization of testnet blocks
- #6098 `7708fcd` Update Windows resource files (and add one for bitcoin-tx)
- #6159 `e1412d3` Catch errors on datadir lock and pidfile delete

[up to date until #5976]
