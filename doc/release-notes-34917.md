RPC and Startup Option
------------
The `bip125-replaceable` key in the wallet transaction RPCs such
as `listtransactions`, `listsinceblock`, and `gettransaction` is
marked as deprecated. Users still have the option to retrieve this
key by passing the `-deprecatedrpc=bip125` startup option. Also,
the `-walletrbf` startup option has been marked as deprecated and
will be fully removed in the next release. Using this option emits
a warning in the logs.
