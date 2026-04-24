// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_UTXO_SET_SHARE_H
#define BITCOIN_NODE_UTXO_SET_SHARE_H

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <optional>
#include <utility>
#include <vector>

#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/time.h>

class CChainParams;
struct AssumeutxoData;

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

/** Manages downloading a UTXO set snapshot from peers */
class UTXOSetDownloadManager
{
public:
    enum class State { IDLE, DISCOVERING, DOWNLOADING, COMPLETE, FAILED };

    /** Begin a download targeting the given assumeutxo snapshot.
     *  Output is written to output_path. */
    bool StartDownload(const uint256& target_blockhash,
                       const AssumeutxoData& au_data,
                       const fs::path& output_path) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Process a utxosetinfo response from a peer. Selects the matching
     *  entry and transitions to DOWNLOADING if valid. */
    void ProcessUTXOSetInfo(int64_t peer_id,
                            const std::vector<UTXOSetInfoEntry>& entries) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Process a received chunk. Verifies the Merkle proof and writes to
     *  the output file. Returns false if the proof is invalid. */
    bool ProcessUTXOSetChunk(int64_t peer_id, const MsgUTXOSet& chunk) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Get the next chunk request to send to a peer, or nullopt if none
     *  available. Marks the chunk as in-flight for this peer. */
    std::optional<MsgGetUTXOSet> GetNextChunkRequest(int64_t peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Re-queue any in-flight chunks assigned to this peer */
    void RequeueChunksForPeer(int64_t peer_id) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Re-assign chunks that have been in-flight for too long */
    void HandleTimeouts(SteadySeconds now) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Whether peer has any chunks currently assigned to it */
    bool PeerHasInflightChunks(int64_t peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Count chunks currently in-flight for a given peer */
    int CountInFlightForPeer(int64_t peer_id) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    State GetState() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
    fs::path GetOutputPath() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    enum class ChunkState { NEEDED, IN_FLIGHT, DONE };

    struct ChunkInfo {
        ChunkState state{ChunkState::NEEDED};
        int64_t assigned_peer{-1};
        SteadySeconds request_time{};
    };

    mutable Mutex m_mutex;

    State m_state GUARDED_BY(m_mutex){State::IDLE};
    fs::path m_output_path GUARDED_BY(m_mutex);

    // Target snapshot info
    uint256 m_target_blockhash GUARDED_BY(m_mutex);
    uint256 m_expected_chunk_merkle_root GUARDED_BY(m_mutex);
    uint32_t m_target_height GUARDED_BY(m_mutex){0};
    uint64_t m_data_length GUARDED_BY(m_mutex){0};
    uint256 m_merkle_root GUARDED_BY(m_mutex);
    uint32_t m_total_chunks GUARDED_BY(m_mutex){0};

    std::vector<ChunkInfo> m_chunks GUARDED_BY(m_mutex);
    uint32_t m_chunks_done GUARDED_BY(m_mutex){0};
    static constexpr std::chrono::seconds CHUNK_TIMEOUT{60};
};

} // namespace node

#endif // BITCOIN_NODE_UTXO_SET_SHARE_H
