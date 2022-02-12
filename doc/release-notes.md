*After branching off for a major version release of Bitcoin Core, use this
template to create the initial release notes draft.*

*The release notes draft is a temporary file that can be added to by anyone. See
[/doc/developer-notes.md#release-notes](/doc/developer-notes.md#release-notes)
for the process.*

*Create the draft, named* "*version* Release Notes Draft"
*(e.g. "22.0 Release Notes Draft"), as a collaborative wiki in:*

https://github.com/bitcoin-core/bitcoin-devwiki/wiki/

*Before the final release, move the notes back to this git repository.*

*version* Release Notes Draft
===============================

Bitcoin Core version *version* is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-*version*/>

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
using the Linux kernel, macOS 10.15+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

P2P and network changes
-----------------------

- A bitcoind node will no longer rumour addresses to inbound peers by default.
  They will become eligible for address gossip after sending an ADDR, ADDRV2,
  or GETADDR message. (#21528)

Fee estimation changes
----------------------

- Fee estimation now takes the feerate of replacement (RBF) transactions into
  account. (#22539)

Rescan startup parameter removed
--------------------------------

The `-rescan` startup parameter has been removed. Wallets which require
rescanning due to corruption will still be rescanned on startup.
Otherwise, please use the `rescanblockchain` RPC to trigger a rescan. (#23123)

Updated RPCs
------------

- The `validateaddress` RPC now returns an `error_locations` array for invalid
  addresses, with the indices of invalid character locations in the address (if
  known). For example, this will attempt to locate up to two Bech32 errors, and
  return their locations if successful. Success and correctness are only guaranteed
  if fewer than two substitution errors have been made.
  The error message returned in the `error` field now also returns more specific
  errors when decoding fails. (#16807)

- The `-deprecatedrpc=addresses` configuration option has been removed.  RPCs
  `gettxout`, `getrawtransaction`, `decoderawtransaction`, `decodescript`,
  `gettransaction verbose=true` and REST endpoints `/rest/tx`, `/rest/getutxos`,
  `/rest/block` no longer return the `addresses` and `reqSigs` fields, which
  were previously deprecated in 22.0. (#22650)
- The `getblock` RPC command now supports verbosity level 3 containing transaction inputs'
  `prevout` information.  The existing `/rest/block/` REST endpoint is modified to contain
  this information too. Every `vin` field will contain an additional `prevout` subfield
  describing the spent output. `prevout` contains the following keys:
  - `generated` - true if the spent coins was a coinbase.
  - `height`
  - `value`
  - `scriptPubKey`

- The top-level fee fields `fee`, `modifiedfee`, `ancestorfees` and `descendantfees`
  returned by RPCs `getmempoolentry`,`getrawmempool(verbose=true)`,
  `getmempoolancestors(verbose=true)` and `getmempooldescendants(verbose=true)`
  are deprecated and will be removed in the next major version (use
  `-deprecated=fees` if needed in this version). The same fee fields can be accessed
  through the `fees` object in the result. WARNING: deprecated
  fields `ancestorfees` and `descendantfees` are denominated in sats, whereas all
  fields in the `fees` object are denominated in BTC. (#22689)

- Both `createmultisig` and `addmultisigaddress` now include a `warnings`
  field, which will show a warning if a non-legacy address type is requested
  when using uncompressed public keys. (#23113)

New RPCs
--------

Build System
------------

Files
-----

* On startup, the list of banned hosts and networks (via `setban` RPC) in
  `banlist.dat` is ignored and only `banlist.json` is considered. Bitcoin Core
  version 22.x is the only version that can read `banlist.dat` and also write
  it to `banlist.json`. If `banlist.json` already exists, version 22.x will not
  try to translate the `banlist.dat` into json. After an upgrade, `listbanned`
  can be used to double check the parsed entries. (#22570)

New settings
------------

Updated settings
----------------

- In previous releases, the meaning of the command line option
  `-persistmempool` (without a value provided) incorrectly disabled mempool
  persistence.  `-persistmempool` is now treated like other boolean options to
  mean `-persistmempool=1`. Passing `-persistmempool=0`, `-persistmempool=1`
  and `-nopersistmempool` is unaffected. (#23061)

- `-maxuploadtarget` now allows human readable byte units [k|K|m|M|g|G|t|T].
  E.g. `-maxuploadtarget=500g`. No whitespace, +- or fractions allowed.
  Default is `M` if no suffix provided. (#23249)

Tools and Utilities
-------------------

- Update `-getinfo` to return data in a user-friendly format that also reduces vertical space. (#21832)

- CLI `-addrinfo` now returns a single field for the number of `onion` addresses
  known to the node instead of separate `torv2` and `torv3` fields, as support
  for Tor V2 addresses was removed from Bitcoin Core in 22.0. (#22544)

Wallet
------

- `upgradewallet` will now automatically flush the keypool if upgrading
  from a non-HD wallet to an HD wallet, to immediately start using the
  newly-generated HD keys. (#23093)

- a new RPC `newkeypool` has been added, which will flush (entirely
  clear and refill) the keypool. (#23093)

- `listunspent` now includes `ancestorcount`, `ancestorsize`, and
  `ancestorfees` for each transaction output that is still in the mempool.
  (#12677)

- `lockunspent` now optionally takes a third parameter, `persistent`, which
  causes the lock to be written persistently to the wallet database. This
  allows UTXOs to remain locked even after node restarts or crashes. (#23065)

- `receivedby` RPCs now include coinbase transactions. Previously, the
  following wallet RPCs excluded coinbase transactions: `getreceivedbyaddress`,
  `getreceivedbylabel`, `listreceivedbyaddress`, `listreceivedbylabel`. This
  release changes this behaviour and returns results accounting for received
  coins from coinbase outputs. The previous behaviour can be restored using the
  configuration `-deprecatedrpc=exclude_coinbase`, but may be removed in a
  future release. (#14707)

- A new option in the same `receivedby` RPCs, `include_immature_coinbase`
  (default=`false`), determines whether to account for immature coinbase
  transactions. Immature coinbase transactions are coinbase transactions that
  have 100 or fewer confirmations, and are not spendable. (#14707)

GUI changes
-----------

- UTXOs which are locked via the GUI are now stored persistently in the
  wallet database, so are not lost on node shutdown or crash. (#23065)

- The Bech32 checkbox has been replaced with a dropdown for all address types, including the new Bech32m (BIP-350) standard for Taproot enabled wallets.

Low-level changes
=================

RPC
---

- `getblockchaininfo` now returns a new `time` field, that provides the chain tip time. (#22407)

Tests
-----

- For the `regtest` network the activation heights of several softforks were
  set to block height 1. They can be changed by the runtime setting
  `-testactivationheight=name@height`. (#22818)

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
