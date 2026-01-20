24.0.1 Release Notes
====================

Due to last-minute issues (#26616), 24.0, although tagged, was never fully
announced or released.

Bitcoin Core version 24.0.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-24.0.1/>

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
using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

Notice of new option for transaction replacement policies
=========================================================

This version of Bitcoin Core adds a new `mempoolfullrbf` configuration
option which allows users to change the policy their individual node
will use for relaying and mining unconfirmed transactions.  The option
defaults to the same policy that was used in previous releases and no
changes to node policy will occur if everyone uses the default.

Some Bitcoin services today expect that the first version of an
unconfirmed transaction that they see will be the version of the
transaction that ultimately gets confirmed---a transaction acceptance
policy sometimes called "first-seen".

The Bitcoin Protocol does not, and cannot, provide any assurance that
the first version of an unconfirmed transaction seen by a particular
node will be the version that gets confirmed.  If there are multiple
versions of the same unconfirmed transaction available, only the miner
who includes one of those transactions in a block gets to decide which
version of the transaction gets confirmed.

Despite this lack of assurance, multiple merchants and services today
still make this assumption.

There are several benefits to users from removing this *first-seen*
simplification.  One key benefit, the ability for the sender of a
transaction to replace it with an alternative version paying higher
fees, was realized in [Bitcoin Core 0.12.0][] (February 2016) with the
introduction of [BIP125][] opt-in Replace By Fee (RBF).

Since then, there has been discussion about completely removing the
first-seen simplification and allowing users to replace any of their
older unconfirmed transactions with newer transactions, a feature called
*full-RBF*.  This release includes a `mempoolfullrbf` configuration
option that allows enabling full-RBF, although it defaults to off
(allowing only opt-in RBF).

Several alternative node implementations have already enabled full-RBF by
default for years, and several contributors to Bitcoin Core are
advocating for enabling full-RBF by default in a future version of
Bitcoin Core.

As more nodes that participate in relay and mining begin enabling
full-RBF, replacement of unconfirmed transactions by ones offering higher
fees may rapidly become more reliable.

Contributors to this project strongly recommend that merchants and services
not accept unconfirmed transactions as final, and if they insist on doing so,
to take the appropriate steps to ensure they have some recourse or plan for
when their assumptions do not hold.

[Bitcoin Core 0.12.0]: https://bitcoincore.org/en/releases/0.12.0/#opt-in-replace-by-fee-transactions
[bip125]: https://github.com/bitcoin/bips/blob/master/bip-0125.mediawiki

Notable changes
===============

P2P and network changes
-----------------------

- To address a potential denial-of-service, the logic to download headers from peers
  has been reworked.  This is particularly relevant for nodes starting up for the
  first time (or for nodes which are starting up after being offline for a long time).

  Whenever headers are received from a peer that have a total chainwork that is either
  less than the node's `-minimumchainwork` value or is sufficiently below the work at
  the node's tip, a "presync" phase will begin, in which the node will download the
  peer's headers and verify the cumulative work on the peer's chain, prior to storing
  those headers permanently. Once that cumulative work is verified to be sufficiently high,
  the headers will be redownloaded from that peer and fully validated and stored.

  This may result in initial headers sync taking longer for new nodes starting up for
  the first time, both because the headers will be downloaded twice, and because the effect
  of a peer disconnecting during the presync phase (or while the node's best headers chain has less
  than `-minimumchainwork`), will result in the node needing to use the headers presync mechanism
  with the next peer as well (downloading the headers twice, again). (#25717)

- With I2P connections, a new, transient address is used for each outbound
  connection if `-i2pacceptincoming=0`. (#25355)

Updated RPCs
------------

- The `-deprecatedrpc=softforks` configuration option has been removed.  The
  RPC `getblockchaininfo` no longer returns the `softforks` field, which was
  previously deprecated in 23.0. (#23508) Information on soft fork status is
  now only available via the `getdeploymentinfo` RPC.

- The `deprecatedrpc=exclude_coinbase` configuration option has been removed.
  The `receivedby` RPCs (`listreceivedbyaddress`, `listreceivedbylabel`,
  `getreceivedbyaddress` and `getreceivedbylabel`) now always return results
  accounting for received coins from coinbase outputs, without an option to
  change that behaviour. Excluding coinbases was previously deprecated in 23.0.
  (#25171)

- The `deprecatedrpc=fees` configuration option has been removed. The top-level
  fee fields `fee`, `modifiedfee`, `ancestorfees` and `descendantfees` are no
  longer returned by RPCs `getmempoolentry`, `getrawmempool(verbose=true)`,
  `getmempoolancestors(verbose=true)` and `getmempooldescendants(verbose=true)`.
  The same fee fields can be accessed through the `fees` object in the result.
  The top-level fee fields were previously deprecated in 23.0. (#25204)

- The `getpeerinfo` RPC has been updated with a new `presynced_headers` field,
  indicating the progress on the presync phase mentioned in the
  "P2P and network changes" section above.

Changes to wallet related RPCs can be found in the Wallet section below.

New RPCs
--------

- The `sendall` RPC spends specific UTXOs to one or more recipients
  without creating change. By default, the `sendall` RPC will spend
  every UTXO in the wallet. `sendall` is useful to empty wallets or to
  create a changeless payment from select UTXOs. When creating a payment
  from a specific amount for which the recipient incurs the transaction
  fee, continue to use the `subtractfeefromamount` option via the
  `send`, `sendtoaddress`, or `sendmany` RPCs. (#24118)

- A new `gettxspendingprevout` RPC has been added, which scans the mempool to find
  transactions spending any of the given outpoints. (#24408)

- The `simulaterawtransaction` RPC iterates over the inputs and outputs of the given
  transactions, and tallies up the balance change for the given wallet. This can be
  useful e.g. when verifying that a coin join like transaction doesn't contain unexpected
  inputs that the wallet will then sign for unintentionally. (#22751)

Updated REST APIs
-----------------

- The `/headers/` and `/blockfilterheaders/` endpoints have been updated to use
  a query parameter instead of path parameter to specify the result count. The
  count parameter is now optional, and defaults to 5 for both endpoints. The old
  endpoints are still functional, and have no documented behaviour change.

  For `/headers`, use
  `GET /rest/headers/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`
  instead of
  `GET /rest/headers/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

  For `/blockfilterheaders/`, use
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<BLOCK-HASH>.<bin|hex|json>?count=<COUNT=5>`
  instead of
  `GET /rest/blockfilterheaders/<FILTERTYPE>/<COUNT>/<BLOCK-HASH>.<bin|hex|json>` (deprecated)

  (#24098)

Build System
------------

- Guix builds are now reproducible across architectures (x86_64 & aarch64). (#21194)

New settings
------------

- A new `mempoolfullrbf` option has been added, which enables the mempool to
  accept transaction replacement without enforcing BIP125 replaceability
  signaling. (#25353)

Wallet
------

- The `-walletrbf` startup option will now default to `true`. The
  wallet will now default to opt-in RBF on transactions that it creates. (#25610)

- The `replaceable` option for the `createrawtransaction` and
  `createpsbt` RPCs will now default to `true`. Transactions created
  with these RPCs will default to having opt-in RBF enabled. (#25610)

- The `wsh()` output descriptor was extended with Miniscript support. You can import Miniscript
  descriptors for P2WSH in a watchonly wallet to track coins, but you can't spend from them using
  the Bitcoin Core wallet yet.
  You can find more about Miniscript on the [reference website](https://bitcoin.sipa.be/miniscript/). (#24148)

- The `tr()` output descriptor now supports multisig scripts through the `multi_a()` and
  `sortedmulti_a()` functions. (#24043)

- To help prevent fingerprinting transactions created by the Bitcoin Core wallet, change output
  amounts are now randomized. (#24494)

- The `listtransactions`, `gettransaction`, and `listsinceblock`
  RPC methods now include a wtxid field (hash of serialized transaction,
  including witness data) for each transaction. (#24198)

- The `listsinceblock`, `listtransactions` and `gettransaction` output now contain a new
  `parent_descs` field for every "receive" entry. (#25504)

- A new optional `include_change` parameter was added to the `listsinceblock` command.

- RPC `getreceivedbylabel` now returns an error, "Label not found
  in wallet" (-4), if the label is not in the address book. (#25122)

Migrating Legacy Wallets to Descriptor Wallets
---------------------------------------------

An experimental RPC `migratewallet` has been added to migrate Legacy (non-descriptor) wallets to
Descriptor wallets. More information about the migration process is available in the
[documentation](https://github.com/bitcoin/bitcoin/blob/master/doc/managing-wallets.md#migrating-legacy-wallets-to-descriptor-wallets).

GUI changes
-----------

- A new menu item to restore a wallet from a backup file has been added (gui#471).

- Configuration changes made in the bitcoin GUI (such as the pruning setting,
proxy settings, UPNP preferences) are now saved to `<datadir>/settings.json`
file rather than to the Qt settings backend (windows registry or unix desktop
config files), so these settings will now apply to bitcoind, instead of being
ignored. (#15936, gui#602)

- Also, the interaction between GUI settings and `bitcoin.conf` settings is
simplified. Settings from `bitcoin.conf` are now displayed normally in the GUI
settings dialog, instead of in a separate warning message ("Options set in this
dialog are overridden by the configuration file: -setting=value"). And these
settings can now be edited because `settings.json` values take precedence over
`bitcoin.conf` values. (#15936)

Low-level changes
=================

RPC
---

- The `deriveaddresses`, `getdescriptorinfo`, `importdescriptors` and `scantxoutset` commands now
  accept Miniscript expression within a `wsh()` descriptor. (#24148)

- The `getaddressinfo`, `decodescript`, `listdescriptors` and `listunspent` commands may now output
  a Miniscript descriptor inside a `wsh()` where a `wsh(raw())` descriptor was previously returned. (#24148)

Credits
=======

Thanks to everyone who directly contributed to this release:

- /dev/fd0
- 0xb10c
- Adam Jonas
- akankshakashyap
- Ali Sherief
- amadeuszpawlik
- Andreas Kouloumos
- Andrew Chow
- Anthony Towns
- Antoine Poinsot
- Antoine Riard
- Aurèle Oulès
- avirgovi
- Ayush Sharma
- Baas
- Ben Woosley
- BrokenProgrammer
- brunoerg
- brydinh
- Bushstar
- Calvin Kim
- CAnon
- Carl Dong
- chinggg
- Cory Fields
- Daniel Kraft
- Daniela Brozzoni
- darosior
- Dave Scotese
- David Bakin
- dergoegge
- dhruv
- Dimitri
- dontbyte
- Duncan Dean
- eugene
- Eunoia
- Fabian Jahr
- furszy
- Gleb Naumenko
- glozow
- Greg Weber
- Gregory Sanders
- gruve-p
- Hennadii Stepanov
- hiago
- Igor Bubelov
- ishaanam
- Jacob P.
- Jadi
- James O'Beirne
- Janna
- Jarol Rodriguez
- Jeremy Rand
- Jeremy Rubin
- jessebarton
- João Barbosa
- John Newbery
- Jon Atack
- Josiah Baker
- Karl-Johan Alm
- KevinMusgrave
- Kiminuo
- klementtan
- Kolby Moroz
- kouloumos
- Kristaps Kaupe
- Larry Ruane
- Luke Dashjr
- MarcoFalke
- Marnix
- Martin Leitner-Ankerl
- Martin Zumsande
- Michael Dietz
- Michael Folkson
- Michael Ford
- Murch
- mutatrum
- muxator
- Oskar Mendel
- Pablo Greco
- pasta
- Patrick Strateman
- Pavol Rusnak
- Peter Bushnell
- phyBrackets
- Pieter Wuille
- practicalswift
- randymcmillan
- Robert Spigler
- Russell Yanofsky
- S3RK
- Samer Afach
- Sebastian Falbesoner
- Seibart Nedor
- Shashwat
- Sjors Provoost
- Smlep
- sogoagain
- Stacie
- Stéphan Vuylsteke
- Suhail Saqan
- Suhas Daftuar
- t-bast
- TakeshiMusgrave
- Vasil Dimov
- W. J. van der Laan
- w0xlt
- whiteh0rse
- willcl-ark
- William Casarin
- Yancy Ribbens

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
