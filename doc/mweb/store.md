# Data Storage

In addition to extending the existing `CBlock` and `CTransaction` objects already used in Litecoin, the following data stores are created or modified for MWEB.

### CBlockUndo

After a new block is connected, a `rev<index>.dat` file is created that describes how to remove or "undo" the block from the chain state.
When performing a reorg, the undo file can be processed, which will revert the UTXO set back to the way it was just before the block.

To support undo-ing MWEB blocks, we''ve added a new `mw::BlockUndo` object to `CBlockUndo`, which gets serialized at the end.
This contains the following fields:

* `prev_header` - the MWEB header in the previous block
* `utxos_spent` - vector of `UTXO`s that were spent in the block
* `utxos_added` - vector of coin IDs (hashes) that were added in the block

To revert the block from the chain state, the node adds back the `UTXO`s in `utxos_spent`, removes the matching `UTXO`s in `utxos_added`, and sets `prev_header` as the MWEB chain tip.

NOTE: For backward compatibility, when deserializing a `CBlockUndo`, we first must look up the size of the `CBlockUndo` object.
After deserializing vtxundo (the vector of `CTxUndo`s), if more data remains, assume it\'s the `mw::BlockUndo` object.
If no more data remains, assume the `CBlockUndo` does not have MWEB data.
An `UnserializeBlockUndo` function was added to handle this.

### UTXOs
##### CoinDB (leveldb)

Litecoin's leveldb instance is used to maintain a UTXO table (prefix: 'U') with `UTXO` objects, consisting of the following data fields:

* output_hash (key) - The hash of the output.
* block_height - The block height the UTXO was included.
* leaf_index - The index of the leaf in the output PMMR.
* output - The full `Output` object, including the rangeproof and owner data.

### PMMRs
##### MMR Info (leveldb)
Litecoin's leveldb instance is used to maintain an MMR Info table (prefix: "M") with `MMRInfo` objects consisting of the following data fields:

* version - A version byte that allows for future schema upgrades.
* index (key) - File number of the PMMR files.
* pruned_hash - Hash of latest header this PMMR represents.
* compact_index - File number of the PruneList bitset.
* compacted_hash - Hash of the header this MMR was compacted for. You cannot rewind beyond this point.

Each time the PMMRs are flushed to disk, a new MMRInfo object is written to the DB and marked as the latest.

##### Leaves (leveldb)
Litecoin's leveldb instance is used to maintain MMR leaf tables (prefix: 'O' for outputs) to store uncompacted PMMR leaves consisting of the following data fields:

* leaf_index (key) - The zero-based leaf position.
* leaf - The raw leaf data committed to by the PMMR.

Leaves spent before the horizon will be removed during compaction.

##### MMR Hashes (file)

Stored in file `<prefix><index>.dat` where `<prefix>` refers to 'O' for the output PMMR, and `<index>` is a 6-digit number that matches the `index` value of the latest `MMRInfo` object.
Example: If the latest `MMRInfo` object has an `index` of 123, the matching output PMMR hash file will be named `O000123.dat`.

The hash file consists of un-compacted leaf hashes and their parent hashes.

##### Leafset (file)

Stored in file `leaf<index>.dat`.

The leafset file consists of a bitset indicating which leaf indices of the output PMMR are unspent.
Example: If the PMMR contains 5 leaves where leaf indices 0, 1, and 2 are spent, but 3 and 4 are unspent, the file will contain a single byte of 00011000 = 0x18.

##### PruneList (file)

Stored in file `prun<index>.dat`.

The prunelist file consists of a bitset indicating which nodes of the output PMMR are not included in the output PMMR hash file.
Example: If nodes 0, 1, 3, and 4 are compacted (not included in PMMR), then the first byte of the prune list bitset will be 11011000 = 0xD8.