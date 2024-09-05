# Assumeutxo Usage

Assumeutxo is a feature that allows fast bootstrapping of a validating bitcoind
instance.

For notes on the design of Assumeutxo, please refer to [the design doc](/doc/design/assumeutxo.md).

## Loading a snapshot

There is currently no canonical source for snapshots, but any downloaded snapshot
will be checked against a hash that's been hardcoded in source code. If there is
no source for the snapshot you need, you can generate it yourself using
`dumptxoutset` on another node that is already synced (see
[Generating a snapshot](#generating-a-snapshot)).

Once you've obtained the snapshot, you can use the RPC command `loadtxoutset` to
load it.

```
$ bitcoin-cli -rpcclienttimeout=0 loadtxoutset /path/to/input
```

After the snapshot has loaded, the syncing process of both the snapshot chain
and the background IBD chain can be monitored with the `getchainstates` RPC.

### Pruning

A pruned node can load a snapshot. To save space, it's possible to delete the
snapshot file as soon as `loadtxoutset` finishes.

The minimum `-prune` setting is 550 MiB, but this functionality ignores that
minimum and uses at least 1100 MiB.

As the background sync continues there will be temporarily two chainstate
directories, each multiple gigabytes in size (likely growing larger than the
downloaded snapshot).

### Indexes

Indexes work but don't take advantage of this feature. They always start building
from the genesis block and can only apply blocks in order. Once the background
validation reaches the snapshot block, indexes will continue to build all the
way to the tip.


For indexes that support pruning, note that these indexes only allow blocks that
were already indexed to be pruned. Blocks that are not indexed yet will also
not be pruned.

This means that, if the snapshot is old, then a lot of blocks after the snapshot
block will need to be downloaded, and these blocks can't be pruned until they
are indexed, so they could consume a lot of disk space until indexing catches up
to the snapshot block.

## Generating a snapshot

The RPC command `dumptxoutset` can be used to generate a snapshot for the current
tip (using type "latest") or a recent height (using type "rollback"). A generated
snapshot from one node can then be loaded
on any other node. However, keep in mind that the snapshot hash needs to be
listed in the chainparams to make it usable. If there is no snapshot hash for
the height you have chosen already, you will need to change the code there and
re-compile.

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
