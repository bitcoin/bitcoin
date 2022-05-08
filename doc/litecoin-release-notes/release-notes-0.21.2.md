Litecoin Core version 0.21.2 is now available from:

 <https://download.litecoin.org/litecoin-0.21.2/>.

This is the largest update ever, providing full node, wallet, and mining support for MWEB.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/litecoin-project/litecoin/issues>

To receive security and update notifications, please subscribe to:

  <https://groups.google.com/forum/#!forum/litecoin-dev>


How to upgrade: 
==============

Firstly, thank you for running Litecoin Core and helping secure the network!

As you’re running an older version of Litecoin Core, shut it down. Wait until it’s completely shut down  - which might take a few minutes for older versions - then follow these simple steps:
For Windows: simply run the installer 
For Mac: copy over to `/Applications/Litecoin-Qt` 
For Linux: copy cover `litecoind`/`litecoin-qt`.

NB: upgrading directly from an ‘end of life’ version of Litecoin Core is possible, but it might take a while if the data directory needs to be migrated. Old wallet versions of Litecoin Core are generally supported.
 

Compatibility:
==============

Litecoin Core is supported and extensively tested on operating systems using the Linux kernel, macOS 10.10+,  Windows 7 and newer. It’s not recommended to use Litecoin Core on unsupported systems.

Litecoin Core should also work on most other Unix-like systems, but is not as frequently tested on them.

MWEB fields added to BlockIndex, and block serialization format has changed. Downgrading to older versions is unsafe.

If upgrading to 0.21.2 *after* MWEB has activated, you must resync to download MWEB blocks.

Notable changes
===============

Consensus changes
-----------------

- This release implements the proposed MWEB consensus rules
  ([LIP002](https://github.com/litecoin-project/lips/blob/master/lip-0002.mediawiki),
  [LIP003](https://github.com/litecoin-project/lips/blob/master/lip-0003.mediawiki), and
  [LIP004](https://github.com/litecoin-project/lips/blob/master/lip-0004.mediawiki))

P2P and network changes
-----------------------

- A new service flag, NODE_MWEB (1 << 24), was added to signal to peers that the node supports MWEB.
  When connected peers both advertise this capability, they are expected to provide all MWEB data when
  sharing transactions, blocks, and compact blocks with each other.

- Nodes now announce compact block version 3 support, informing peers that they can provide MWEB data
  in compact blocks.


Updated RPCs
------------

- `getblockheader` now returns an additional `mweb_header` field containing
  all of the MWEB header data, and an `mweb_amount` field containing the total
  number of coins pegged-in to the MWEB after applying the block.

- `getblock` now returns an additional `mweb` field containing MWEB header info,
  and all of the inputs, outputs, and kernels in the MWEB block.

- Added `mwebweight`, `descendantmwebweight`, `ancestormwebweight`, and `mweb`
  fields to `getrawmempool`, `getmempoolancestors`, `getmempooldescendants`,
  and `getmempoolentry`.

- Added new fields to describe MWEB transaction inputs, outputs, and kernels
  for `getrawtransaction`.

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

New settings
------------

- Added "fMWEBFeatures" option for enabling the new "Advanced MWEB Features"
  control.

Wallet Database
---------------

- Added "mweb_coin" type which stores MWEB coins and their derived keys.

- Added CHDChain version 4 which includes an MWEB key index counter and
  the stealth address scan key.

- Added CKeyMetadata version 14 which includes the MWEB key index.

- Added FEATURE_MWEB = 210000 minimum database version.

Wallet RPC changes
------------------

- Added 'listwallettransactions' which matches the transaction list display values.

GUI changes
-----------

- Added an "Advanced MWEB Features" control for testing. It’s only available
  when the "-debug" argument is supplied, and the option is turned on in the
  settings dialog.


Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers](https://github.com/bitcoin/bitcoin/tree/master/doc/release-notes)
- DavidBurkett
- hectorchu
- losh11