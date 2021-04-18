1.0.0 Release Notes
====================

Widecoin Core version 1.0.0 is now available from:

  <https://widecoin.org>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/widecoin-project/widecoin/issues>

Downgrade warning
------------------

Because release 1.0+ and later makes use of headers-first synchronization and
parallel block download (see further), the block files and databases are not
backwards-compatible with pre-1.0 versions of Widecoin Core or other software:

* Blocks will be stored on disk out of order (in the order they are
received, really), which makes it incompatible with some tools or
other programs. Reindexing using earlier versions will also not work
anymore as a result of this.

* The block index database will now hold headers for which no block is
stored on disk, which earlier versions won't support.

If you want to be able to downgrade smoothly, make a backup of your entire data
directory. Without this your node will need start syncing (or importing from
bootstrap.dat) anew afterwards. 

This does not affect wallet forward or backward compatibility.

Widecoin 1.0.0 Change log
=========================
This release is based upon Bitcoin Core v0.21.0.  Their upstream changelog applies to us and
is included in as separate release-notes.  This section describes the Widecoin-specific differences.

Protocol:

- Keep Sha256d Proof-of-Work.
- Widecoin TCP port 8553 (instead of 8333)
- RPC TCP port 8552 (instead of 8332)
- Testnet TCP port 18553 (instead of 18333)
- Testnet RPC TCP port 18552 (instead of 18332)
- Signet TCP port 38553 (instead of 38333)
- Signet RPC TCP port 38552 (instead of 38332)
- 35 Million Coin Limit  (instead of 21 million)
- Magic 0xf8bfc3dc       (instead of 0xf9beb4d9)
- Target Block Time 0.5 minutes (instead of 10 minutes)
- Target Timespan 1 hour      (instead of two weeks)
- Max Block Weight 32000000 (instead of 4000000)
- Max Block Serialized Size 8000000 (instead of 4000000)
- Coinbase Maturity 50 (instead of 100)
- nbits 0x1e0ffff0 (instead of 0x1d00ffff)
- SubsidyHalvingInterval 2years (instead of 4years)

Credits
=======

Thanks to everyone who directly contributed to this release:

- David Jack 
- Justin
- Jonh
- Tony
- Cole
