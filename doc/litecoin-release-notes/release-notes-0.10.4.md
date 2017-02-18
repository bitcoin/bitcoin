Litecoin Core version 0.10.4.0 is now available from:

  <https://download.litecoin.org/litecoin-0.10.4.0/>

This is a new minor version release, bringing bug fixes, the BIP65
(CLTV) consensus change, and relay policy preparation for BIP113. It is
recommended to upgrade to this version as soon as possible.

Please report bugs using the issue tracker at github:

  <https://github.com/litecoin-project/litecoin/issues>

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Litecoin-Qt (on Mac) or
litecoind/litecoin-qt (on Linux).

Downgrade warning
------------------

Because release 0.10+ and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-0.10 versions of Litecoin Core or other software:

* Blocks will be stored on disk out of order (in the order they are
received, really), which makes it incompatible with some tools or
other programs. Reindexing using earlier versions will also not work
anymore as a result of this.

* The block index database will now hold headers for which no block is
stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data
directory. Without this your node will need start syncing (or importing from
bootstrap.dat) anew afterwards. It is possible that the data from a completely
synchronised 0.10 node may be usable in older versions as-is, but this is not
supported and may break as soon as the older version attempts to reindex.

This does not affect wallet forward or backward compatibility.

Notable changes since 0.10.3
============================

BIP65 soft fork to enforce OP_CHECKLOCKTIMEVERIFY opcode
--------------------------------------------------------

This release includes several changes related to the [BIP65][] soft fork
which redefines the existing OP_NOP2 opcode as OP_CHECKLOCKTIMEVERIFY
(CLTV) so that a transaction output can be made unspendable until a
specified point in the future.

1. This release will only relay and mine transactions spending a CLTV
   output if they comply with the BIP65 rules as provided in code.

2. This release will produce version 4 blocks by default. Please see the
   *notice to miners* below.

3. Once 951 out of a sequence of 1,001 blocks on the local node's best block
   chain contain version 4 (or higher) blocks, this release will no
   longer accept new version 3 blocks and it will only accept version 4
   blocks if they comply with the BIP65 rules for CLTV.

**Notice to miners:** Litecoin Core’s block templates are now for
version 4 blocks only, and any mining software relying on its
getblocktemplate must be updated in parallel to use libblkmaker either
version v0.4.3 or any version from v0.5.2 onward.

- If you are solo mining, this will affect you the moment you upgrade
  Litecoin Core, which must be done prior to BIP65 achieving its 951/1001
  status.

- If you are mining with the stratum mining protocol: this does not
  affect you.

- If you are mining with the getblocktemplate protocol to a pool: this
  will affect you at the pool operator’s discretion, which must be no
  later than BIP65 achieving its 951/1001 status.

[BIP65]: https://github.com/bitcoin/bips/blob/master/bip-0065.mediawiki


Windows bug fix for corrupted UTXO database on unclean shutdowns
----------------------------------------------------------------

Several Windows users reported that they often need to reindex the
entire blockchain after an unclean shutdown of Litecoin Core on Windows
(or an unclean shutdown of Windows itself). Although unclean shutdowns
remain unsafe, this release no longer relies on memory-mapped files for
the UTXO database, which significantly reduced the frequency of unclean
shutdowns leading to required reindexes during testing.

For more information, see: <https://github.com/bitcoin/bitcoin/pull/6917>

Other fixes for database corruption on Windows are expected in the
next major release.

0.10.4 Change log
=================

This release is based upon Bitcoin Core v0.10.4.  Their upstream changelog applies to us and
is included in as separate release-notes, see: [Release Notes](release-notes.md).  
This section describes the Litecoin-specific differences.

- Added BIP65 CHECKLOCKTIMEVERIFY softfork.
- Increased OP_RETURN relay size to 80 bytes.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Charles Lee
- pooler
- Adrian Gallagher
- Anton Yemelyanov
- Warren Togami
- BtcDrak
