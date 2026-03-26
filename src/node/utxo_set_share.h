// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SET_SHARE_H
#define BITCOIN_NODE_UTXO_SET_SHARE_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <serialize.h>
#include <uint256.h>
#include <util/fs.h>

class CChainParams;

namespace node {

static constexpr uint64_t UTXO_SET_CHUNK_SIZE = 3'900'000;

/** Verify a Merkle proof for a chunk against a known root */
bool VerifyChunkMerkleProof(const uint256& leaf_hash,
                            const std::vector<uint256>& proof,
                            uint32_t chunk_index,
                            uint32_t total_chunks,
                            const uint256& merkle_root);

/** Entry in a utxosetinfo message, describing an available UTXO set. */
struct UTXOSetInfoEntry {
    uint32_t height{0};
    uint256 block_hash;
    uint256 serialized_hash;
    uint64_t data_length{0};
    uint256 merkle_root;

    SERIALIZE_METHODS(UTXOSetInfoEntry, obj)
    {
        READWRITE(obj.height);
        READWRITE(obj.block_hash);
        READWRITE(obj.serialized_hash);
        READWRITE(obj.data_length);
        READWRITE(obj.merkle_root);
    }
};

/** getutxoset message: request a chunk of UTXO set data. */
struct MsgGetUTXOSet {
    uint32_t height{0};
    uint256 block_hash;
    uint32_t chunk_index{0};

    SERIALIZE_METHODS(MsgGetUTXOSet, obj)
    {
        READWRITE(obj.height);
        READWRITE(obj.block_hash);
        READWRITE(obj.chunk_index);
    }
};

/** utxoset message: one chunk of UTXO set data with the Merkle proof. */
struct MsgUTXOSet {
    uint32_t height{0};
    uint256 block_hash;
    uint32_t chunk_index{0};
    std::vector<uint256> proof_hashes;
    std::vector<unsigned char> data;

    SERIALIZE_METHODS(MsgUTXOSet, obj)
    {
        READWRITE(obj.height);
        READWRITE(obj.block_hash);
        READWRITE(obj.chunk_index);
        READWRITE(obj.proof_hashes);
        READWRITE(obj.data);
    }
};

/** Sidecar file with leaf hashes and Merkle root */
struct ChunkMerkleCache {
    uint64_t snapshot_file_size{0};
    std::vector<uint256> leaf_hashes;
    uint256 merkle_root;

    SERIALIZE_METHODS(ChunkMerkleCache, obj)
    {
        READWRITE(obj.snapshot_file_size);
        READWRITE(obj.leaf_hashes);
        READWRITE(obj.merkle_root);
    }
};

/** Serves UTXO set chunks and Merkle proofs for snapshot files in the share dir */
class UTXOSetShareProvider
{
public:
    /** Scan share dir for valid snapshot files, load or compute sidecar
     *  caches. Returns the number of snapshots ready to serve. */
    size_t Initialize(const fs::path& share_dir, const CChainParams& params);

    /** Get info entries for all available snapshots */
    std::vector<UTXOSetInfoEntry> GetInfoEntries() const;

    /** Read a chunk from the snapshot identified by (height, block_hash).
     *  Returns {chunk_data, merkle_proof}, or nullopt if not found. */
    std::optional<std::pair<std::vector<unsigned char>, std::vector<uint256>>> GetChunk(uint32_t height, const uint256& block_hash, uint32_t chunk_index) const;

private:
    struct SnapshotData {
        fs::path path;
        UTXOSetInfoEntry info;
        std::vector<uint256> leaf_hashes;
    };

    std::vector<SnapshotData> m_snapshots;
};

} // namespace node

#endif // BITCOIN_NODE_UTXO_SET_SHARE_H
