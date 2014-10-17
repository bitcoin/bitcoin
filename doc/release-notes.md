(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

Block file backwards-compatibility warning
===========================================

Because release 0.10.0 makes use of headers-first synchronization and parallel
block download, the block files and databases are not backwards-compatible
with older versions of Bitcoin Core:

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

Transaction fee changes
=======================

This release automatically estimates how high a transaction fee (or how
high a priority) transactions require to be confirmed quickly. The default
settings will create transactions that confirm quickly; see the new
'txconfirmtarget' setting to control the tradeoff between fees and
confirmation times.

Prior releases used hard-coded fees (and priorities), and would
sometimes create transactions that took a very long time to confirm.


New Command Line Options
========================

-txconfirmtarget=n : create transactions that have enough fees (or priority)
so they are likely to confirm within n blocks (default: 1). This setting
is over-ridden by the -paytxfee option.

New RPC methods
===============

Fee/Priority estimation
-----------------------

estimatefee nblocks : Returns approximate fee-per-1,000-bytes needed for
a transaction to be confirmed within nblocks. Returns -1 if not enough
transactions have been observed to compute a good estimate.

estimatepriority nblocks : Returns approximate priority needed for
a zero-fee transaction to confirm within nblocks. Returns -1 if not
enough free transactions have been observed to compute a good
estimate.

Statistics used to estimate fees and priorities are saved in the
data directory in the 'fee_estimates.dat' file just before
program shutdown, and are read in at startup.
