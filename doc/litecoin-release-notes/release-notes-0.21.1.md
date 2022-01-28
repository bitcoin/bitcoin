0.21.1 Release Notes
====================

Summary:
==============
In this update to Litecoin Core, Taproot has been activated, enhancing Litecoin’s security, privacy and scalability. Litecoin Core 0.21.1 is the most significant update in years - with performance improvements, new features (like Taproot), bug fixes and updated translations - and sets the foundations for MWEB’s imminent arrival. 

Litecoin Core 0.21.1 is available now, right here: <https://download.litecoin.org/litecoin-0.21.1/>.

As always, we welcome the expertise of the community. To report bugs please use the issue tracker at GitHub - <https://github.com/litecoin-project/litecoin/issues > - and to get security and update notifications subscribe via <https://groups.google.com/forum/#!forum/litecoin-dev>. 

Anyone can contribute to Litecoin Core. Please scroll down to ‘How to contribute to Litecoin Core’ for details.

By downloading or upgrading Litecoin Core, you’re helping to secure the network, so thank you and enjoy the upgrade!


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

From Litecoin Core 0.21.1 onwards, macOS versions earlier than 10.12 are no longer supported. Additionally, Litecoin Core doesn’t yet change appearance when macOS "dark mode" is activated.

The node's known peers are persisted to disk in a file called `peers.dat`. The format of this file has been changed in a backwards-incompatible way in order to accommodate the storage of Tor v3 and other BIP155 addresses. This means that if the file is modified by 0.21 or newer then older versions will not be able to read it. Those old versions, in the event of a downgrade, will log an error message - "incorrect keysize in addrman deserialization" and will continue normal operation as if the file was missing, creating a new empty one. (#19954, #20284)


Notable changes:
===============

P2P and network changes
-----------------------

- The mempool now tracks whether transactions submitted via the wallet or RPCs
  have been successfully broadcast. Every 10-15 minutes, the node will try to
  announce unbroadcast transactions until a peer requests it via a `getdata`
  message or the transaction is removed from the mempool for other reasons.
  The node will not track the broadcast status of transactions submitted to the
  node using P2P relay. This version reduces the initial broadcast guarantees
  for wallet transactions submitted via P2P to a node running the wallet. (#18038)

- The size of the set of transactions that peers have announced and we consider
  for requests has been reduced from 100000 to 5000 (per peer), and further
  announcements will be ignored when that limit is reached. If you need to dump
  (very) large batches of transactions, exceptions can be made for trusted
  peers using the "relay" network permission. For localhost for example it can
  be enabled using the command line option `-whitelist=relay@127.0.0.1`.
  (#19988)

- This release adds support for Tor version 3 hidden services, and rumoring them
  over the network to other peers using
  [BIP155](https://github.com/Litecoin/bips/blob/master/bip-0155.mediawiki).
  Version 2 hidden services are still fully supported by Litecoin Core, but the
  Tor network will start
  [deprecating](https://blog.torproject.org/v2-deprecation-timeline) them in the
  coming months. (#19954)

- The Tor onion service that is automatically created by setting the
  `-listenonion` configuration parameter will now be created as a Tor v3 service
  instead of Tor v2. The private key that was used for Tor v2 (if any) will be
  left untouched in the `onion_private_key` file in the data directory (see
  `-datadir`) and can be removed if not needed. Litecoin Core will no longer
  attempt to read it. The private key for the Tor v3 service will be saved in a
  file named `onion_v3_private_key`. To use the deprecated Tor v2 service (not
  recommended), the `onion_private_key` can be copied over
  `onion_v3_private_key`, e.g: `cp -f onion_private_key onion_v3_private_key`. (#19954)

- The client writes a file (`anchors.dat`) at shutdown with the network addresses
  of the node’s two outbound block-relay-only peers (so called "anchors"). The
  next time the node starts, it reads this file and attempts to reconnect to those
  same two peers. This prevents an attacker from using node restarts to trigger a
  complete change in peers, which would be something they could use as part of an
  eclipse attack. (#17428)

- This release adds support for serving
  [BIP157](https://github.com/Litecoin/bips/blob/master/bip-0157.mediawiki) compact
  filters to peers on the network when enabled using
  `-blockfilterindex=1 -peercfilters=1`. (#16442)

- This release implements
  [BIP339](https://github.com/Litecoin/bips/blob/master/bip-0339.mediawiki)
  wtxid relay. When negotiated, transactions are announced using their wtxid
  instead of their txid. (#18044).

- This release implements the proposed Taproot consensus rules
  ([BIP341](https://github.com/Litecoin/bips/blob/master/bip-0341.mediawiki) and
  [BIP342](https://github.com/Litecoin/bips/blob/master/bip-0342.mediawiki)),
  without activation on mainnet. (#19553)

Updated RPCs
------------

- The `getpeerinfo` RPC has a new `network` field that provides the type of
  network ("ipv4", "ipv6", or "onion") that the peer connected through. (#20002)

- The `getpeerinfo` RPC now has additional `last_block` and `last_transaction`
  fields that return the UNIX epoch time of the last block and the last *valid*
  transaction received from each peer. (#19731)

- `getnetworkinfo` now returns two new fields, `connections_in` and
  `connections_out`, that provide the number of inbound and outbound peer
  connections. These new fields are in addition to the existing `connections`
  field, which returns the total number of peer connections. (#19405)

- Exposed transaction version numbers are now treated as unsigned 32-bit
  integers instead of signed 32-bit integers. This matches their treatment in
  consensus logic. Versions greater than 2 continue to be non-standard
  (matching previous behavior of smaller than 1 or greater than 2 being
  non-standard). Note that this includes the `joinpsbt` command, which combines
  partially-signed transactions by selecting the highest version number.
  (#16525)

- `getmempoolinfo` now returns an additional `unbroadcastcount` field. The
  mempool tracks locally submitted transactions until their initial broadcast
  is acknowledged by a peer. This field returns the count of transactions
  waiting for acknowledgement.

- Mempool RPCs such as `getmempoolentry` and `getrawmempool` with
  `verbose=true` now return an additional `unbroadcast` field. This indicates
  whether initial broadcast of the transaction has been acknowledged by a
  peer. `getmempoolancestors` and `getmempooldescendants` are also updated.

- The `getpeerinfo` RPC no longer returns the `banscore` field unless the configuration
  option `-deprecatedrpc=banscore` is used. The `banscore` field will be fully
  removed in the next major release. (#19469)

- The `testmempoolaccept` RPC returns `vsize` and a `fees` object with the `base` fee
  if the transaction would pass validation. (#19940)

- The `getpeerinfo` RPC now returns a `connection_type` field. This indicates
  the type of connection established with the peer. It will return one of six
  options. For more information, see the `getpeerinfo` help documentation.
  (#19725)

- The `getpeerinfo` RPC no longer returns the `addnode` field by default. This
  field will be fully removed in the next major release.  It can be accessed
  with the configuration option `-deprecatedrpc=getpeerinfo_addnode`. However,
  it is recommended to instead use the `connection_type` field (it will return
  `manual` when addnode is true). (#19725)

- The `getpeerinfo` RPC no longer returns the `whitelisted` field by default. 
  This field will be fully removed in the next major release. It can be accessed 
  with the configuration option `-deprecatedrpc=getpeerinfo_whitelisted`. However, 
  it is recommended to instead use the `permissions` field to understand if specific 
  privileges have been granted to the peer. (#19770)

- The `walletcreatefundedpsbt` RPC call will now fail with
  `Insufficient funds` when inputs are manually selected but are not enough to cover
  the outputs and fee. Additional inputs can automatically be added through the
  new `add_inputs` option. (#16377)

- The `fundrawtransaction` RPC now supports `add_inputs` option that when `false`
  prevents adding more inputs if necessary and consequently the RPC fails.

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

New RPCs
--------

- The `getindexinfo` RPC returns the actively running indices of the node,
  including their current sync status and height. It also accepts an `index_name`
  to specify returning the status of that index only. (#19550)


Updated settings
----------------

- The same ZeroMQ notification (e.g. `-zmqpubhashtx=address`) can now be
  specified multiple times to publish the same notification to different ZeroMQ
  sockets. (#18309)

- The `-banscore` configuration option, which modified the default threshold for
  disconnecting and discouraging misbehaving peers, has been removed as part of
  changes in 0.20.1 and in this release to the handling of misbehaving peers.
  Refer to "Changes regarding misbehaving peers" in the 0.20.1 release notes for
  details. (#19464)

- The `-debug=db` logging category, which was deprecated in 0.20 and replaced by
  `-debug=walletdb` to distinguish it from `coindb`, has been removed. (#19202)

- A `download` permission has been extracted from the `noban` permission. For
  compatibility, `noban` implies the `download` permission, but this may change
  in future releases. Refer to the help of the affected settings `-whitebind`
  and `-whitelist` for more details. (#19191)

- Netmasks that contain 1-bits after 0-bits (the 1-bits are not contiguous on
  the left side, e.g. 255.0.255.255) are no longer accepted. They are invalid
  according to RFC 4632. Netmasks are used in the `-rpcallowip` and `-whitelist`
  configuration options and in the `setban` RPC. (#19628)

- The `-blocksonly` setting now completely disables fee estimation. (#18766)

Changes to Wallet or GUI related settings can be found in the GUI or Wallet section below.
Tools and Utilities
-------------------

- A new `Litecoin-cli -netinfo` command provides a network peer connections
  dashboard that displays data from the `getpeerinfo` and `getnetworkinfo` RPCs
  in a human-readable format. An optional integer argument from `0` to `4` may
  be passed to see increasing levels of detail. (#19643)

- A new `Litecoin-cli -generate` command, equivalent to RPC `generatenewaddress`
  followed by `generatetoaddress`, can generate blocks for command line testing
  purposes. This is a client-side version of the former `generate` RPC. See the
  help for details. (#19133)

- The `Litecoin-cli -getinfo` command now displays the wallet name and balance for
  each of the loaded wallets when more than one is loaded (e.g. in multiwallet
  mode) and a wallet is not specified with `-rpcwallet`. (#18594)

- The `connections` field of `Litecoin-cli -getinfo` is now expanded to return a JSON
  object with `in`, `out` and `total` numbers of peer connections. It previously
  returned a single integer value for the total number of peer connections. (#19405)

New settings
------------

- The `startupnotify` option is used to specify a command to
  execute when Litecoin Core has finished with its startup
  sequence. (#15367)

Wallet
------

- Backwards compatibility has been dropped for two `getaddressinfo` RPC
  deprecations, as notified in the 0.20 release notes. The deprecated `label`
  field has been removed as well as the deprecated `labels` behavior of
  returning a JSON object containing `name` and `purpose` key-value pairs. Since
  0.20, the `labels` field returns a JSON array of label names. (#19200)

- To improve wallet privacy, the frequency of wallet rebroadcast attempts is
  reduced from approximately once every 15 minutes to once every 12-36 hours.
  To maintain a similar level of guarantee for initial broadcast of wallet
  transactions, the mempool tracks these transactions as a part of the newly
  introduced unbroadcast set. See the "P2P and network changes" section for
  more information on the unbroadcast set. (#18038)

- The `sendtoaddress` and `sendmany` RPCs accept an optional `verbose=True`
  argument to also return the fee reason about the sent tx. (#19501)

- The wallet can create a transaction without change even when the keypool is
  empty. Previously it failed. (#17219)

- The `-salvagewallet` startup option has been removed. A new `salvage` command
  has been added to the `Litecoin-wallet` tool which performs the salvage
  operations that `-salvagewallet` did. (#18918)

- A new configuration flag `-maxapsfee` has been added, which sets the max
  allowed avoid partial spends (APS) fee. It defaults to 0 (i.e. fee is the
  same with and without APS). Setting it to -1 will disable APS, unless
  `-avoidpartialspends` is set. (#14582)

- The wallet will now avoid partial spends (APS) by default, if this does not
  result in a difference in fees compared to the non-APS variant. The allowed
  fee threshold can be adjusted using the new `-maxapsfee` configuration
  option. (#14582)

- The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept
  `load_on_startup` options to modify the settings list. Unless these options
  are explicitly set to true or false, the list is not modified, so the RPC
  methods remain backwards compatible. (#15937)

- A new `send` RPC with similar syntax to `walletcreatefundedpsbt`, including
  support for coin selection and a custom fee rate, is added. The `send` RPC is
  experimental and may change in subsequent releases. (#16378)

- The `estimate_mode` parameter is now case-insensitive in the `bumpfee`,
  `fundrawtransaction`, `sendmany`, `sendtoaddress`, `send` and
  `walletcreatefundedpsbt` RPCs. (#11413)

- The `bumpfee` RPC now uses `conf_target` rather than `confTarget` in the
  options. (#11413)

- `fundrawtransaction` and `walletcreatefundedpsbt` when used with the
  `lockUnspents` argument now lock manually selected coins, in addition to
  automatically selected coins. Note that locked coins are never used in
  automatic coin selection, but can still be manually selected. (#18244)

- The `-zapwallettxes` startup option has been removed and its functionality
  removed from the wallet.  This option was originally intended to allow for
  rescuing wallets which were affected by a malleability attack. More recently,
  it has been used in the fee bumping of transactions that did not signal RBF.
  This functionality has been superseded with the abandon transaction feature. (#19671)

- The error code when no wallet is loaded, but a wallet RPC is called, has been
  changed from `-32601` (method not found) to `-18` (wallet not found).
  (#20101)

### Automatic wallet creation removed

Litecoin Core will no longer automatically create new wallets on startup. It will
load existing wallets specified by `-wallet` options on the command line or in
`Litecoin.conf` or `settings.json` files. And by default it will also load a
top-level unnamed ("") wallet. However, if specified wallets don't exist,
Litecoin Core will now just log warnings instead of creating new wallets with
new keys and addresses like previous releases did.

New wallets can be created through the GUI (which has a more prominent create
wallet option), through the `Litecoin-cli createwallet` or `Litecoin-wallet
create` commands, or the `createwallet` RPC. (#15454, #20186)


### Wallet RPC changes

- The `upgradewallet` RPC replaces the `-upgradewallet` command line option.
  (#15761)

- The `settxfee` RPC will fail if the fee was set higher than the `-maxtxfee`
  command line setting. The wallet will already fail to create transactions
  with fees higher than `-maxtxfee`. (#18467)

- A new `fee_rate` parameter/option denominated in satoshis per vbyte (sat/vB)
  is introduced to the `sendtoaddress`, `sendmany`, `fundrawtransaction` and
  `walletcreatefundedpsbt` RPCs as well as to the experimental new `send`
  RPC. The legacy `feeRate` option in `fundrawtransaction` and
  `walletcreatefundedpsbt` still exists for setting a fee rate in BTC per 1,000
  vbytes (BTC/kvB), but it is expected to be deprecated soon to avoid
  confusion. For these RPCs, the fee rate error message is updated from BTC/kB
  to sat/vB and the help documentation in BTC/kB is updated to BTC/kvB. The
  `send` and `sendtoaddress` RPC examples are updated to aid users in creating
  transactions with explicit fee rates. (#20305, #11413)

- The `bumpfee` RPC `fee_rate` option is changed from BTC/kvB to sat/vB and the
  help documentation is updated. Users are warned that this is a breaking API
  change, but it should be relatively benign: the large (100,000 times)
  difference between BTC/kvB and sat/vB units means that a transaction with a
  fee rate mistakenly calculated in BTC/kvB rather than sat/vB should raise an
  error due to the fee rate being set too low. In the worst case, the
  transaction may send at 1 sat/vB, but as Replace-by-Fee (BIP125 RBF) is active
  by default when an explicit fee rate is used, the transaction fee can be
  bumped. (#20305)

GUI changes
-----------

- Wallets created or loaded in the GUI will now be automatically loaded on
  startup, so they don't need to be manually reloaded next time Litecoin Core is
  started. The list of wallets to load on startup is stored in
  `\<datadir\>/settings.json` and augments any command line or `Litecoin.conf`
  `-wallet=` settings that specify more wallets to load. Wallets that are
  unloaded in the GUI get removed from the settings list so they won't load
  again automatically next startup. (#19754)

- The GUI Peers window no longer displays a "Ban Score" field. This is part of
  changes in 0.20.1 and in this release to the handling of misbehaving
  peers. Refer to "Changes regarding misbehaving peers" in the 0.20.1 release
  notes for details. (#19512)

Low-level changes
=================

RPC
---

- To make RPC `sendtoaddress` more consistent with `sendmany` the following error
    `sendtoaddress` codes were changed from `-4` to `-6`:
  - Insufficient funds
  - Fee estimation failed
  - Transaction has too long of a mempool chain

- The `sendrawtransaction` error code for exceeding `maxfeerate` has been changed from
  `-26` to `-25`. The error string has been changed from "absurdly-high-fee" to
  "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)." The
  `testmempoolaccept` RPC returns `max-fee-exceeded` rather than `absurdly-high-fee`
  as the `reject-reason`. (#19339)

- To make wallet and rawtransaction RPCs more consistent, the error message for
  exceeding maximum feerate has been changed to "Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)." (#19339)

Tests
-----

- The `generateblock` RPC allows testers using regtest mode to
  generate blocks that consist of a custom set of transactions. (#17693)


Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers]
- Adrian Gallagher
- David Burkett


