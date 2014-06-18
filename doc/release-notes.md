(note: this is a temporary file, to be added-to by anybody, and moved to
release-notes at release time)

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
