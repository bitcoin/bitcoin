Updated RPCs
------------

The no-op `includeWatching`, `include_watching`, and `include_watchonly`
named options of `fundrawtransaction`, `walletcreatefundedpsbt`,
`send`, `sendall`, and `simulaterawtransaction` are deprecated. They can only
be passed when the RPC server is started with `-deprecatedrpc=watchonly` and
will be removed in the next release. (#36855)
