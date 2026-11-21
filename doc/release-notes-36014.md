Updated RPCs
------------

- `addnode` and `getaddednodeinfo` now consistently treat values that refer to
  the same peer as the same added node. Two values refer to the same peer if
  they are the same string, the same numeric address (for example `[::1]` and
  `[0:0:0:0:0:0:0:1]`), or the same host and port, where a missing port means
  the default port (for example `node.example` and `node.example:8333` on
  mainnet). No DNS lookups are done for this comparison. (#36014)
  - `addnode add` now also rejects a host that was already added with or
    without the default port. It already rejected equivalent numeric
    addresses.
  - `addnode remove` removes the added node that is equivalent to the given
    value. Previously, only an exact string match was removed.
  - `getaddednodeinfo` returns the added node that is equivalent to the given
    value. The `addednode` field shows the value as it was originally added.

Updated settings
----------------

- Startup `-addnode` values that refer to the same peer as an earlier value,
  using the same rules as the `addnode` RPC, are now ignored. A warning is
  logged for each one, naming the earlier value it matched. Previously, each
  value was stored, retried and listed in `getaddednodeinfo` separately.
  (#36014)
