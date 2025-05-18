Tortoisecoin Core version 0.10.4 is now available from:

  <https://tortoisecoin.org/bin/tortoisecoin-core-0.10.4/>

This is a new minor version release, bringing bug fixes, the BIP65
(CLTV) consensus change, and relay policy preparation for BIP113. It is
recommended to upgrade to this version as soon as possible.

Please report bugs using the issue tracker at github:

  <https://github.com/tortoisecoin/tortoisecoin/issues>

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Tortoisecoin-Qt (on Mac) or
tortoisecoind/tortoisecoin-qt (on Linux).

Downgrade warning
------------------

Because release 0.10.0 and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-0.10 versions of Tortoisecoin Core or other software:

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

This does not affect wallet forward or backward compatibility. There are no
known problems when downgrading from 0.11.x to 0.10.x.

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

For more information about the soft-forking change, please see
<https://github.com/tortoisecoin/tortoisecoin/pull/6351>

Graphs showing the progress towards block version 4 adoption may be
found at the URLs below:

- Block versions over the last 50,000 blocks as progress towards BIP65
  consensus enforcement: <http://tortoisecoin.sipa.be/ver-50k.png>

- Block versions over the last 2,000 blocks showing the days to the
  earliest possible BIP65 consensus-enforced block: <http://tortoisecoin.sipa.be/ver-2k.png>

**Notice to miners:** Tortoisecoin Core’s block templates are now for
version 4 blocks only, and any mining software relying on its
getblocktemplate must be updated in parallel to use libblkmaker either
version FIXME or any version from FIXME onward.

- If you are solo mining, this will affect you the moment you upgrade
  Tortoisecoin Core, which must be done prior to BIP65 achieving its 951/1001
  status.

- If you are mining with the stratum mining protocol: this does not
  affect you.

- If you are mining with the getblocktemplate protocol to a pool: this
  will affect you at the pool operator’s discretion, which must be no
  later than BIP65 achieving its 951/1001 status.

[BIP65]: https://github.com/tortoisecoin/bips/blob/master/bip-0065.mediawiki

Windows bug fix for corrupted UTXO database on unclean shutdowns
----------------------------------------------------------------

Several Windows users reported that they often need to reindex the
entire blockchain after an unclean shutdown of Tortoisecoin Core on Windows
(or an unclean shutdown of Windows itself). Although unclean shutdowns
remain unsafe, this release no longer relies on memory-mapped files for
the UTXO database, which significantly reduced the frequency of unclean
shutdowns leading to required reindexes during testing.

For more information, see: <https://github.com/tortoisecoin/tortoisecoin/pull/6917>

Other fixes for database corruption on Windows are expected in the
next major release.

0.10.4 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

- #6953 `8b3311f` alias -h for --help
- #6953 `97546fc` Change URLs to https in debian/control
- #6953 `38671bf` Update debian/changelog and slight tweak to debian/control
- #6953 `256321e` Correct spelling mistakes in doc folder
- #6953 `eae0350` Clarification of unit test build instructions
- #6953 `90897ab` Update bluematt-key, the old one is long-since revoked
- #6953 `a2f2fb6` build: disable -Wself-assign
- #6953 `cf67d8b` Bugfix: Allow mining on top of old tip blocks for testnet (fixes testnet-in-a-box use case)
- #6953 `b3964e3` Drop "with minimal dependencies" from description
- #6953 `43c2789` Split tortoisecoin-tx into its own package
- #6953 `dfe0d4d` Include tortoisecoin-tx binary on Debian/Ubuntu
- #6953 `612efe8` [Qt] Raise debug window when requested
- #6953 `3ad96bd` Fix locking in GetTransaction
- #6953 `9c81005` Fix spelling of Qt
- #6946 `94b67e5` Update LevelDB
- #6706 `5dc72f8` CLTV: Add more tests to improve coverage
- #6706 `6a1343b` Add RPC tests for the CHECKLOCKTIMEVERIFY (BIP65) soft-fork
- #6706 `4137248` Add CHECKLOCKTIMEVERIFY (BIP65) soft-fork logic
- #6706 `0e01d0f` Enable CHECKLOCKTIMEVERIFY as a standard script verify flag
- #6706 `6d01325` Replace NOP2 with CHECKLOCKTIMEVERIFY (BIP65)
- #6706 `750d54f` Move LOCKTIME_THRESHOLD to src/script/script.h
- #6706 `6897468` Make CScriptNum() take nMaxNumSize as an argument
- #6867 `5297194` Set TCP_NODELAY on P2P sockets
- #6836 `fb818b6` Bring historical release notes up to date
- #6852 `0b3fd07` build: make sure OpenSSL heeds noexecstack

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alex Morcos
- Daniel Cousens
- Diego Viola
- Eric Lombrozo
- Esteban Ordano
- Gregory Maxwell
- Luke Dashjr
- MarcoFalke
- Matt Corallo
- Micha
- Mitchell Cash
- Peter Todd
- Pieter Wuille
- Wladimir J. van der Laan
- Zak Wilcox

And those who contributed additional code review and/or security research.

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/tortoisecoin/).
