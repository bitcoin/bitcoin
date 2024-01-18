mempool.dat compatibility
========================

The `mempool.dat` file created by -persistmempool or the savemempool RPC will
be written in a new format, which can not be read by previous software
releases. To allow for a downgrade, a temporary setting `-persistmempoolv1` has
been added to fall back to the legacy format.
