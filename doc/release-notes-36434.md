Updated settings
----------------

- `-rpcbind` now accepts a unix domain socket on platforms that support unix
  sockets (not Windows), using the same syntax as `-ipcbind`: `-rpcbind=unix`
  listens on the default path `<datadir>/http.sock`, and
  `-rpcbind=unix:<path>` listens on a custom path. Relative paths are
  interpreted relative to the network data directory. The JSON-RPC and REST
  interfaces are then served over that socket. When every `-rpcbind` value is
  a unix socket, the default TCP localhost listeners are not bound and
  `-rpcallowip` is not required. (#36434)

  Connections over a unix socket bypass `-rpcallowip` entirely, so access to the
  RPC server is controlled by filesystem permissions on the socket file
  (restricted to the owner after binding) and the permissions of the directory
  it is placed in, in addition to the usual RPC authentication. The maximum
  path length, after resolving a relative path against the data directory, is
  platform dependent, around 100 bytes. Missing parent directories are created
  (with default permissions, so place the socket in a suitably restricted directory)
  and a stale socket file left by a previous run is removed at startup. Startup
  fails if the path exists and is not a socket.
  The socket file is not removed on shutdown.

Tools and Utilities
-------------------

- `bitcoin-cli` can connect to a node over a unix domain socket with
  `-rpcconnect=unix` for the default path `<datadir>/http.sock`, or
  `-rpcconnect=unix:<path>` for a custom path. Relative paths are interpreted
  relative to the network data directory, matching `-rpcbind`. `-rpcport` is
  ignored in that case. (#36434)
