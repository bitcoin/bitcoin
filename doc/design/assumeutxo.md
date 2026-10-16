# Assumeutxo Design

For notes on the usage of Assumeutxo, please refer to [the usage doc](/doc/assumeutxo.md).

## General background

- [assumeutxo proposal](https://github.com/jamesob/assumeutxo-docs/tree/2019-04-proposal/proposal)
- [Github issue](https://github.com/bitcoin/bitcoin/issues/15605)
- [draft PR](https://github.com/bitcoin/bitcoin/pull/15606)

## Design notes

- The concept of UTXO snapshots is treated as an implementation detail that lives
  behind the ChainstateManager interface. The external presentation of the changes
  required to facilitate the use of UTXO snapshots is the understanding that there are
  now certain regions of the chain that can be temporarily assumed to be valid.
  In certain cases, e.g. wallet rescanning, this is very similar to dealing with
  a pruned chain.

  Logic outside ChainstateManager should try not to know about snapshots, instead
  preferring to work in terms of more general states like assumed-valid.


## Chainstate phases

### "Normal" operation via initial block download

`ChainstateManager` manages a single Chainstate object, for which
`m_from_snapshot_blockhash` is `std::nullopt`. This chainstate is (maybe obviously)
considered active. This is the "traditional" mode of operation for bitcoind.

|    |    |
| ---------- | ----------- |
| number of chainstates | 1 |
| active chainstate | ibd |

### Existing snapshot chainstates

Previously imported snapshots are stored in `chainstate_snapshot`.

When this directory is present in the datadir, the snapshot chainstate will be detected
and loaded as active on node startup (via `LoadAssumeutxoChainstate()`).

A special file is created within that directory, `base_blockhash`, which contains the
serialized `uint256` of the base block of the snapshot. This is used to reinitialize
the snapshot chainstate on subsequent inits. Otherwise, the directory is a normal
leveldb database.

|    |    |
| ---------- | ----------- |
| number of chainstates | 2 |
| active chainstate | snapshot |

The snapshot syncs to tip while the original chainstate remains stopped.
Use `-reindex` to discard the snapshot and reconstruct the UTXO set from genesis.
