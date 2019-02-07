Descriptor import support
---------------------

The `importmulti` RPC now supports importing of addresses from descriptors. A "desc" parameter can be provided instead of the "scriptPubKey" in a request, as well as an optional range for ranged descriptors to specify the start and end of the range to import. More information about
descriptors can be found [here](https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md).
