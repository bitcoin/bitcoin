HTTP: RPC / REST
----------------

Clients attempting to connect from addresses not allowed by the `-rpcallowip`
option (or its default, `localhost`) will now be immediately disconnected
instead of receiving a `403 Forbidden`.
