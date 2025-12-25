Bitcoin Core version 27.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-27.0/>

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
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux Kernel 3.17+, macOS 11.0+, and Windows 7 and newer. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

libbitcoinconsensus
-------------------

- libbitcoinconsensus is deprecated and will be removed for v28. This library has
  existed for nearly 10 years with very little known uptake or impact. It has
  become a maintenance burden.

  The underlying functionality does not change between versions, so any users of
  the library can continue to use the final release indefinitely, with the
  understanding that Taproot is its final consensus update.

  In the future, libbitcoinkernel will provide a much more useful API that is
  aware of the UTXO set, and therefore be able to fully validate transactions and
  blocks. (#29189)

mempool.dat compatibility
-------------------------

- The `mempool.dat` file created by -persistmempool or the savemempool RPC will
  be written in a new format. This new format includes the XOR'ing of transaction
  contents to mitigate issues where external programs (such as anti-virus) attempt
  to interpret and potentially modify the file.

  This new format can not be read by previous software releases. To allow for a
  downgrade, a temporary setting `-persistmempoolv1` has been added to fall back
  to the legacy format. (#28207)

P2P and network changes
-----------------------

- BIP324 v2 transport is now enabled by default. It remains possible to disable v2
  by running with `-v2transport=0`. (#29347)
- Manual connection options (`-connect`, `-addnode` and `-seednode`) will
  now follow `-v2transport` to connect with v2 by default. They will retry with
  v1 on failure. (#29058)

- Network-adjusted time has been removed from consensus code. It is replaced
  with (unadjusted) system time. The warning for a large median time offset
  (70 minutes or more) is kept. This removes the implicit security assumption of
  requiring an honest majority of outbound peers, and increases the importance
  of the node operator ensuring their system time is (and stays) correct to not
  fall out of consensus with the network. (#28956)

Mempool Policy Changes
----------------------

- Opt-in Topologically Restricted Until Confirmation (TRUC) Transactions policy
  (aka v3 transaction policy) is available for use on test networks when
  `-acceptnonstdtxn=1` is set. By setting the transaction version number to 3, TRUC transactions
  request the application of limits on spending of their unconfirmed outputs. These
  restrictions simplify the assessment of incentive compatibility of accepting or
  replacing TRUC transactions, thus ensuring any replacements are more profitable for
  the node and making fee-bumping more reliable. TRUC transactions are currently
  nonstandard and can only be used on test networks where the standardness rules are
  relaxed or disabled (e.g. with `-acceptnonstdtxn=1`). (#28948)

External Signing
----------------

- Support for external signing on Windows has been disabled. It will be re-enabled
  once the underlying dependency (Boost Process), has been replaced with a different
  library. (#28967)

Updated RPCs
------------

- The addnode RPC now follows the `-v2transport` option (now on by default, see above) for making connections.
  It remains possible to specify the transport type manually with the v2transport argument of addnode. (#29239)

Build System
------------

- A C++20 capable compiler is now required to build Bitcoin Core. (#28349)
- MacOS releases are configured to use the hardened runtime libraries (#29127)

Wallet
------

- The CoinGrinder coin selection algorithm has been introduced to mitigate unnecessary
  large input sets and lower transaction costs at high feerates. CoinGrinder
  searches for the input set with minimal weight. Solutions found by
  CoinGrinder will produce a change output. CoinGrinder is only active at
  elevated feerates (default: 30+ sat/vB, based on `-consolidatefeerate`×3). (#27877)
- The Branch And Bound coin selection algorithm will be disabled when the subtract fee
  from outputs feature is used. (#28994)
- If the birth time of a descriptor is detected to be later than the first transaction
  involving that descriptor, the birth time will be reset to the earlier time. (#28920)

Low-level changes
=================

Pruning
-------

- When pruning during initial block download, more blocks will be pruned at each
  flush in order to speed up the syncing of such nodes. (#20827)

Init
----

- Various fixes to prevent issues where subsequent instances of Bitcoin Core would
  result in deletion of files in use by an existing instance. (#28784, #28946)
- Improved handling of empty `settings.json` files. (#29144)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 22388o⚡️
- Aaron Clauson
- Amiti Uttarwar
- Andrew Toth
- Anthony Towns
- Antoine Poinsot
- Ava Chow
- Brandon Odiwuor
- brunoerg
- Chris Stewart
- Cory Fields
- dergoegge
- djschnei21
- Fabian Jahr
- fanquake
- furszy
- Gloria Zhao
- Greg Sanders
- Hennadii Stepanov
- Hernan Marino
- iamcarlos94
- ismaelsadeeq
- Jameson Lopp
- Jesse Barton
- John Moffett
- Jon Atack
- josibake
- jrakibi
- Justin Dhillon
- Kashif Smith
- kevkevin
- Kristaps Kaupe
- L0la L33tz
- Luke Dashjr
- Lőrinc
- marco
- MarcoFalke
- Mark Friedenbach
- Marnix
- Martin Leitner-Ankerl
- Martin Zumsande
- Max Edwards
- Murch
- muxator
- naiyoma
- Nikodemas Tuckus
- ns-xvrn
- pablomartin4btc
- Peter Todd
- Pieter Wuille
- Richard Myers
- Roman Zeyde
- Russell Yanofsky
- Ryan Ofsky
- Sebastian Falbesoner
- Sergi Delgado Segura
- Sjors Provoost
- stickies-v
- stratospher
- Supachai Kheawjuy
- TheCharlatan
- UdjinM6
- Vasil Dimov
- w0xlt
- willcl-ark


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
