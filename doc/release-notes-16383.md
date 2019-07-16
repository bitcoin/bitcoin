RPC changes
-----------

RPCs which have an `include_watchonly` argument or `includeWatching`
option now default to `true` for watch-only wallets. Affected RPCs
are: `getbalance`, `listreceivedbyaddress`, `listreceivedbylabel`,
`listtransactions`, `listsinceblock`, `gettransaction`,
`walletcreatefundedpsbt`, and `fundrawtransaction`.
