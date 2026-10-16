# Assumeutxo Usage

Existing snapshots can still be resumed. New snapshot imports are no longer supported.

For notes on the design of Assumeutxo, please refer to [the design doc](/doc/design/assumeutxo.md).

## Existing snapshots

After loading, monitor snapshot sync with `getchainstates`. Earlier blocks are
not downloaded or validated in the background. To discard the snapshot and
validate from genesis, restart with `-reindex`. Disable pruning for archival
storage. Wallet operations requiring missing blocks need this history too.

### Pruning

The minimum `-prune` setting is 550 MiB, but this functionality ignores that
minimum and uses at least 1100 MiB.

### Indexes

Index startup requires the historical blocks
and undo data needed to catch up. Use `-reindex` to restore missing history.

## Generating a snapshot

The RPC command `dumptxoutset` can be used to generate a snapshot for the current
tip (using type "latest") or a recent height (using type "rollback").

Using the type parameter "rollback", `dumptxoutset` can also be used to verify the
hardcoded snapshot hash in the source code by regenerating the snapshot and
comparing the hash.

Example usage:

```
$ bitcoin-cli -rpcclienttimeout=0 dumptxoutset /path/to/output rollback
```

For most of the duration of `dumptxoutset` running the node is in a temporary
state that does not actually reflect reality, i.e. blocks are marked invalid
although we know they are not invalid. Because of this it is discouraged to
interact with the node in any other way during this time to avoid inconsistent
results and race conditions, particularly RPCs that interact with blockstorage.
This inconsistent state is also why network activity is temporarily disabled,
causing us to disconnect from all peers.

`dumptxoutset` takes some time to complete, independent of hardware and
what parameter is chosen. Because of that it is recommended to increase the RPC
client timeout value (use `-rpcclienttimeout=0` for no timeout).
