1.1.0 Release Notes
====================

Widecoin Core version 1.1.0 is now available from:

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

Widecoin 1.1.0 Change log
=========================

Block reward rule is adjusted due to the block is only 0.5 minutes and limitation of max supply:

Block Height (BH)

Block (B)

BH > 1 and BH <= 50,000 => Receive 50 WCN/B

BH >= 50,001  and BH <= 100,000 => Receive 20 WCN/B

BH >= 100,001  and BH <= 500,000 => Receive 10 WCN/B

BH >= 500,001  and BH <= 2,102,399 => Receive 5 WCN/B

BH = 2102400 = 5 WCN (Subsidy is cut in half every 2,102,400 blocks which will occur approximately every 2 years)

Credits
=======

Thanks to everyone who directly contributed to this release:

- David Jack 
- Justin
- Jonh
- Tony
- Cole
