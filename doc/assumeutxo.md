# Assumeutxo Usage

Assumeutxo is a feature that allows fast bootstrapping of a validating bitcoind
instance.

For notes on the design of Assumeutxo, please refer to [the design doc](/doc/assumeutxo.md).

## Loading a snapshot

There is currently no canonical source for snapshots, but any downloaded snapshot
will be checked against a hash that's been hardcoded in source code. If there is
no source for the snapshot you need, you can generate it yourself using
`dumptxoutset` (see [Generating a snapshot](#generating-a-snapshot)).

Once you've obtained the snapshot, you can use the RPC command `loadtxoutset` to
load it.

```
$ bitcoin-cli loadtxoutset /path/to/input
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
from the genesis block. Once the background validation reaches the snapshot block,
indexes will continue to build all the way to the tip.

For indexes that support pruning, note that no pruning will take place between
the snapshot and the tip, until the background sync has completed - after which
everything is pruned. Depending on how old the snapshot is, this may temporarily
use a significant amount of disk space.

## Generating a snapshot

The RPC command `dumptxoutset` can be used to generate a snapshot for the current
tip or a recent height. A generated snapshot from one node can then be loaded
on any other node. However, keep in mind that the snapshot hash needs to be
listed in the chainparams to make it usable. If there is no snapshot hash for
the height you have chosen already, you will need to change the code there and
re-compile.

Using the height parameter, `dumptxoutset` can also be used to verify the
hardcoded snapshot hash in the source code by regenerating the snapshot and
comparing the hash.

Example usage (with testnet height):

```
$ bitcoin-cli dumptxoutset /path/to/output 2500000
```
