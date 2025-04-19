Updated settings
----------------

- Ports specified in `-port` and `-rpcport` options are now validated at startup. Values that previously worked and were considered valid can now result in errors.

P2P
---

- UNIX domain sockets can now be used for proxy connections. Set `-onion` or `-proxy` to the local socket path with the prefix `unix:` (e.g.
  `-onion=unix:/home/me/torsocket`).
