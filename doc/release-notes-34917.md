Wallet RPC and Startup Option
------------
Signalling for RBF is not necessary for replacing transactions due
to nodes being fullrbf and the wallet doesn't need to opt into it
by default. Therefore, RBF related RPC and startup options can be
marked deprecated now and removed in the next release.

In PR #34917, the `bip125-replaceable` key in the wallet transaction
RPCs such as `listtransactions`, `listsinceblock`, and `gettransaction`
is marked as deprecated. Users still have the option to retrieve this
key by passing the `-deprecatedrpc=bip125` startup option. Also,
the `-walletrbf` startup option has been marked as deprecated and
will be fully removed in the next release. Using this option emits
a warning in the logs.

Additionally, the `replaceable` argument in several wallet RPC
requests has been marked deprecated in PR #35433 and for now
users have the option to use this argument via the
`-deprecatedrpc=bip125` startup option. Affected RPCs are
`createrawtransaction`, `fundrawtransaction`, `createpsbt`,
`walletcreatefundedpsbt`, `send`, `sendtoaddress`, `sendmany`,
`sendall`, `bumpfee`, and `psbtbumpfee`.

Moreover, the default value of -walletrbf startup option has been
switched to 0 in PR #35405.
