Updated RPCs
------------
Signalling for RBF is not necessary for replacing transactions due
to nodes being fullrbf and the wallet doesn't need to opt into it
by default.

Therefore, the `replaceable` argument in several RPC requests has been
marked deprecated and for now the users have the option to use
this argument via the `-deprecatedrpc=bip125` startup option.
Effected RPCs are `createrawtransaction`, `fundrawtransaction`,
`createpsbt`, `walletcreatefundedpsbt`, `send`, `sendtoaddress`,
`sendmany`, `sendall`, `bumpfee`, and `psbtbumpfee`.

