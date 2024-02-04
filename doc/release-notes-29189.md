libbitcoinconsensus
========================

This library is deprecated and will be removed for v28.

It has existed for nearly 10 years with very little known uptake or impact. It
has become a maintenance burden.

The underlying functionality does not change between versions, so any users of
the library can continue to use the final release indefinitely, with the
understanding that Taproot is its final consensus update.

In the future, libbitcoinkernel will provide a much more useful API that is
aware of the UTXO set, and therefore be able to fully validate transactions and
blocks.
