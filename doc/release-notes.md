Bitcoin Core version 0.9.0rc1 is now available from:

  http://sourceforge.net/projects/bitcoin/files/Bitcoin/bitcoin-0.9.0rc1/

This is a release candidate for a new major version. A major version brings
both new features and bug fixes.

Please report bugs using the issue tracker at github:

  https://github.com/bitcoin/bitcoin/issues

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), uninstall all
earlier versions of Bitcoin, then run the installer (on Windows) or just copy
over /Applications/Bitcoin-Qt (on Mac) or bitcoind/bitcoin-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you run
0.9.0 your blockchain files will be re-indexed, which will take anywhere from
30 minutes to several hours, depending on the speed of your machine.

On Windows, do not forget to uninstall all earlier versions of the Bitcoin
client first, especially if you are switching to the 64-bit version.

Windows 64-bit installer
-------------------------

New in 0.9.0 is the Windows 64-bit version of the client. There have been
frequent reports of users running out of virtual memory on 32-bit systems
during the initial sync. Because of this it is recommended to install the
64-bit version if your system supports it.

Rebranding to Bitcoin Core
---------------------------

To reduce confusion between Bitcoin-the-network and Bitcoin-the-software we
have renamed the reference client to Bitcoin Core.

Autotools build system
-----------------------

For 0.9.0 we switched to an autotools-based build system instead of individual
(q)makefiles.

Using the standard “./autogen.sh; ./configure; make” to build Bitcoin-Qt and
bitcoind makes it easier for experienced open source developers to contribute
to the project.

Be sure to check doc/build-*.md for your platform before building from source.

Bitcoin-cli
-------------

Another change in the 0.9 release is moving away from the bitcoind executable
functioning both as a server and as a RPC client. The RPC client functionality
(“tell the running bitcoin daemon to do THIS”) was split into a separate
executable, 'bitcoin-cli'. The RPC client code will eventually be removed from
bitcoind, but will be kept for backwards compatibility for a release or two.

`walletpassphrase` RPC
-----------------------

The behavior of the `walletpassphrase` RPC when the wallet is already unlocked
has changed between 0.8 and 0.9.

The 0.8 behavior of `walletpassphrase` is to fail when the wallet is already unlocked:

    > walletpassphrase 1000
    walletunlocktime = now + 1000
    > walletpassphrase 10
    Error: Wallet is already unlocked (old unlock time stays)

The new behavior of `walletpassphrase` is to set a new unlock time overriding
the old one:

    > walletpassphrase 1000
    walletunlocktime = now + 1000
    > walletpassphrase 10
    walletunlocktime = now + 10 (overriding the old unlock time)

0.9.0rc1 Release notes
=======================

RPC:

- 'listreceivedbyaddress' now provides tx ids
- Updated help and tests for 'getreceivedby(account|address)'
- In 'getblock', accept 2nd 'verbose' parameter, similar to getrawtransaction,
  but defaulting to 1 for backward compatibility
- Add 'verifychain', to verify chain database at runtime
- Add 'dumpwallet' and 'importwallet' RPCs
- 'keypoolrefill' gains optional size parameter
- Add 'getbestblockhash', to return tip of best chain
- Add 'chainwork' (the total work done by all blocks since the genesis block)
  to 'getblock' output
- Make RPC password resistant to timing attacks
- Clarify help messages and add examples
- Add 'getrawchangeaddress' call for raw transaction change destinations
- Reject insanely high fees by default in 'sendrawtransaction'
- Add RPC call 'decodescript' to decode a hex-encoded transaction script
- Make 'validateaddress' provide redeemScript
- Add 'getnetworkhashps' to get the calculated network hashrate
- New RPC 'ping' command to request ping, new 'pingtime' and 'pingwait' fields
  in 'getpeerinfo' output
- Adding new 'addrlocal' field to 'getpeerinfo' output
- Add verbose boolean to 'getrawmempool'
- Add rpc command 'getunconfirmedbalance' to obtain total unconfirmed balance
- Explicitly ensure that wallet is unlocked in `importprivkey`
- Add check for valid keys in `importprivkey`

Command-line options:

- Rename option '-tor' to '-onion' to better reflect what it does
- Add '-disablewallet' mode to let bitcoind run entirely without wallet (when
  built with wallet)
- Update default '-rpcsslciphers' to include TLSv1.2
- make '-logtimestamps' default on and rework help-message
- RPC client option: '-rpcwait', to wait for server start
- Remove '-logtodebugger'
- Allow `-noserver` with bitcoind

Block-chain handling and storage:

- Update leveldb to 1.15
- Check for correct genesis (prevent cases where a datadir from the wrong
  network is accidentally loaded)
- Allow txindex to be removed and add a reindex dialog
- Log aborted block database rebuilds
- Store orphan blocks in serialized form, to save memory
- Limit the number of orphan blocks in memory to 750
- Fix non-standard disconnected transactions causing mempool orphans
- Add a new checkpoint at block 279,000

Wallet:

- Store key creation time. Calculate whole-wallet birthday.
- Optimize rescan to skip blocks prior to birthday
- Let user select wallet file with -wallet=foo.dat
- Consider generated coins mature at 101 instead of 120 blocks
- Improve wallet load time
- Don't count txins for priority to encourage sweeping
- Don't create empty transactions when reading a corrupted wallet
- Fix rescan to start from beginning after importprivkey
- Only create signatures with low S values.

Mining:

- Increase default -blockmaxsize/prioritysize to 750K/50K
- 'getblocktemplate' does not require a key to create a block template

Protocol and network:

- Send tx relay flag with version
- New 'reject' P2P message (BIP 0061, see https://gist.github.com/gavinandresen/7079034 for draft)
- Dump addresses every 15 minutes instead of 10 seconds
- Relay OP_RETURN data TxOut as standard transaction type
- Remove CENT-output free transaction rule when relaying
- Lower maximum size for free transaction creation
- Send multiple inv messages if mempool.size > MAX_INV_SZ
- Split MIN_PROTO_VERSION into INIT_PROTO_VERSION and MIN_PEER_PROTO_VERSION
- Do not treat fFromMe transaction differently when broadcasting
- Process received messages one at a time without sleeping between messages
- Improve logging of failed connections
- Bump protocol version to 70002
- Add some additional logging to give extra network insight
- Added new DNS seed from bitcoinstats.com

Validation:

- Log reason for non-standard transaction rejection
- Prune provably-unspendable outputs, and adapt consistency check for it.
- Detect any sufficiently long fork and add a warning
- Call the -alertnotify script when we see a long or invalid fork
- Fix multi-block reorg transaction resurrection
- Reject non-canonically-encoded serialization sizes
- Reject dust amounts during validation
- Accept nLockTime transactions that finalize in the next block

Build system:

- Switch to autotools-based build system
- Build without wallet by passing `--disable-wallet` to configure, this removes
  the BerkeleyDB dependency
- Upgrade gitian dependencies (libpng, libz, libupnpc, boost, openssl) to more
  recent versions
- Windows 64-bit build support
- Solaris compatibility fixes
- Check integrity of gitian input source tarballs
- Enable full GCC Stack-smashing protection for all OSes

GUI:

- Switch to Qt 5.2.0 for Windows build
- Add payment request (BIP 0070) support
- Improve options dialog
- Show transaction fee in new send confirmation dialog
- Add total balance in overview page
- Allow user to choose data directory on first start, when data directory is
  missing, or when the -choosedatadir option is passed
- Save and restore window positions
- Add vout index to transaction id in transactions details dialog
- Add network traffic graph in debug window
- Add open URI dialog
- Add Coin Control Features
- Improve receive coins workflow: make the 'Receive' tab into a form to request
  payments, and move historical address list functionality to File menu.
- Rebrand to `Bitcoin Core`
- Move initialization/shutdown to a thread. This prevents “Not responding”
  messages during startup. Also show a window during shutdown.
- Don't regenerate autostart link on every client startup
- Show and store message of normal bitcoin:URI
- Fix richtext detection hang issue on very old Qt versions
- osx: Make use of the 10.8+ user notification center to display growl like
       notifications
- osx: Added NSHighResolutionCapable flag to Info.plist for better font
       rendering on Retina displays.
- osx: Fix bitcoin-qt startup crash when clicking dock icon
- linux: Fix Gnome bitcoin: URI handler

Miscellaneous:

- Add Linux script (contrib/qos/tc.sh) to limit outgoing bandwidth
- Add '-regtest' mode, similar to testnet but private with instant block
  generation with 'setgenerate' RPC.
- Add 'linearize.py' script to contrib, for creating bootstrap.dat
- Add separate bitcoin-cli client

Credits
--------

Thanks to everyone who contributed to this release:

- Andrey
- bitsofproof
- Brandon Dahler
- Christian Decker
- Christopher Latham
- Chuck
- coblee
- constantined
- Cory Fields
- Cozz Lovan
- Daniel Larimer
- David Hill
- Dmitry Smirnov
- Eric Lombrozo
- fanquake
- fcicq
- Florin
- Gavin Andresen
- Gregory Maxwell
- Guillermo Céspedes Tabárez
- HaltingState
- Han Lin Yap
- harry
- Jeff Garzik
- Johnathan Corgan
- Jonas Schnelli
- Josh Lehan
- Josh Triplett
- Julian Langschaedel
- Kangmo
- Lake Denman
- Luke Dashjr
- Mark Friedenbach
- Matt Corallo
- Michael Bauer
- Michael Ford
- Michagogo
- Mike Hearn
- Nils Schneider
- Olivier Langlois
- patrick s
- Patrick Strateman
- Peter Todd
- phantomcircuit
- phelixbtc
- Philip Kaufmann
- Pieter Wuille
- Rav3nPL
- regergregregerrge
- Robert Backhaus
- Roman Mindalev
- Rune K. Svendsen
- Ryan Niebur
- Scott Ellis
- Scott Willeke
- Sergey Kazenyuk
- Shawn Wilkinson
- Sined
- sje
- super3
- Tamas Blummer
- theuni
- Thomas Holenstein
- Timon Rapp
- Timothy Stranex
- Vaclav Vobornik
- vhf / victor felder
- Vinnie Falco
- Warren Togami
- Wladimir J. van der Laan
