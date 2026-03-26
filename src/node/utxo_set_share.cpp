// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/utxo_set_share.h>

#include <consensus/merkle.h>
#include <hash.h>
#include <kernel/chainparams.h>
#include <logging.h>
#include <node/utxo_snapshot.h>
#include <streams.h>
#include <uint256.h>
#include <util/fs.h>

#include <cstdint>
#include <vector>

namespace node {

bool VerifyChunkMerkleProof(const uint256& leaf_hash,
                            const std::vector<uint256>& proof,
                            uint32_t chunk_index,
                            uint32_t total_chunks,
                            const uint256& merkle_root)
{
    if (total_chunks == 0) return false;

    // Verify proof depth matches the expected tree height
    uint32_t expected_depth{0};
    for (uint32_t n{total_chunks - 1}; n > 0; n >>= 1) ++expected_depth;
    if (proof.size() != expected_depth) return false;

    uint256 computed{leaf_hash};
    uint32_t index{chunk_index};

    for (const uint256& sibling : proof) {
        bool is_right_child{(index % 2) != 0};
        if (is_right_child) {
            computed = Hash(sibling, computed);
        } else {
            computed = Hash(computed, sibling);
        }
        index /= 2;
    }

    return computed == merkle_root;
}

/** Compute leaf hashes by from snapshot file */
static std::vector<uint256> ComputeLeafHashes(const fs::path& path, uint64_t file_size)
{
    AutoFile file{fsbridge::fopen(path, "rb")};
    if (file.IsNull()) return {};

    std::vector<uint256> leaf_hashes;
    uint64_t remaining{file_size};

    while (remaining > 0) {
        uint64_t chunk_size{std::min(remaining, UTXO_SET_CHUNK_SIZE)};
        std::vector<unsigned char> buffer(chunk_size);
        file.read(MakeWritableByteSpan(buffer));
        leaf_hashes.push_back(Hash(buffer));
        remaining -= chunk_size;
    }

    return leaf_hashes;
}

/** Try to load a sidecar file. Returns nullopt if not found. */
static std::optional<ChunkMerkleCache> LoadSidecar(const fs::path& sidecar_path, uint64_t expected_size)
{
    AutoFile file{fsbridge::fopen(sidecar_path, "rb")};
    if (file.IsNull()) return std::nullopt;

    try {
        ChunkMerkleCache cache;
        file >> cache;
        if (cache.snapshot_file_size != expected_size) return std::nullopt;
        // Validate leaf count matches expected number of chunks (ceiling division)
        if (cache.leaf_hashes.size() != (expected_size - 1) / UTXO_SET_CHUNK_SIZE + 1) return std::nullopt;
        return cache;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

/** Write a sidecar file */
static bool WriteSidecar(const fs::path& sidecar_path, const ChunkMerkleCache& cache)
{
    AutoFile file{fsbridge::fopen(sidecar_path, "wb")};
    if (file.IsNull()) return false;

    try {
        file << cache;
        if (!file.Commit()) return false;
        if (file.fclose() != 0) return false;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

size_t UTXOSetShareProvider::Initialize(const fs::path& share_dir, const CChainParams& params)
{
    m_snapshots.clear();

    std::error_code error;
    if (!fs::is_directory(share_dir, error)) return 0;

    for (const auto& entry : fs::directory_iterator(share_dir, error)) {
        if (!entry.is_regular_file(error)) continue;
        if (entry.path().extension() == ".merkle") continue;

        // Try to read snapshot metadata
        AutoFile file{fsbridge::fopen(entry.path(), "rb")};
        if (file.IsNull()) continue;

        SnapshotMetadata metadata(params.MessageStart());
        try {
            file >> metadata;
        } catch (const std::exception& e) {
            LogDebug(BCLog::UTXOSETSHARE, "Unable to read %s: %s", fs::PathToString(entry.path()), e.what());
            continue;
        }

        // Check with assumeutxo params
        auto au_data{params.AssumeutxoForBlockhash(metadata.m_base_blockhash)};
        if (!au_data) {
            LogDebug(BCLog::UTXOSETSHARE, "Skipping %s: no assumeutxo params for block %s",
                     fs::PathToString(entry.path()), metadata.m_base_blockhash.ToString());
            continue;
        }

        uint64_t file_size{fs::file_size(entry.path(), error)};
        if (error) continue;

        // Try loading the sidecar file or compute it
        fs::path sidecar_path{entry.path()};
        sidecar_path += ".merkle";

        std::vector<uint256> leaf_hashes;
        uint256 merkle_root;

        auto cached{LoadSidecar(sidecar_path, file_size)};
        if (cached) {
            leaf_hashes = std::move(cached->leaf_hashes);
            merkle_root = cached->merkle_root;
            LogDebug(BCLog::UTXOSETSHARE, "Loaded sidecar for %s", fs::PathToString(entry.path()));
        } else {
            LogInfo("Computing chunk hashes for %s", fs::PathToString(entry.path()));
            leaf_hashes = ComputeLeafHashes(entry.path(), file_size);
            if (leaf_hashes.empty()) continue;

            merkle_root = ComputeMerkleRoot(leaf_hashes);

            ChunkMerkleCache cache;
            cache.snapshot_file_size = file_size;
            cache.leaf_hashes = leaf_hashes;
            cache.merkle_root = merkle_root;

            if (WriteSidecar(sidecar_path, cache)) {
                LogDebug(BCLog::UTXOSETSHARE, "Wrote sidecar for %s", fs::PathToString(entry.path()));
            } else {
                LogWarning("Failed to write sidecar for %s", fs::PathToString(entry.path()));
            }
        }

        SnapshotData sd;
        sd.path = entry.path();
        sd.info.height = au_data->height;
        sd.info.block_hash = au_data->blockhash;
        sd.info.serialized_hash = uint256{std::span<const unsigned char>{au_data->hash_serialized.data(), 32}};
        sd.info.data_length = file_size;
        sd.info.merkle_root = merkle_root;
        sd.leaf_hashes = std::move(leaf_hashes);

        LogInfo("Serving UTXO set at height %d", sd.info.height);
        m_snapshots.push_back(std::move(sd));
    }

    return m_snapshots.size();
}

std::vector<UTXOSetInfoEntry> UTXOSetShareProvider::GetInfoEntries() const
{
    std::vector<UTXOSetInfoEntry> entries;
    entries.reserve(m_snapshots.size());
    for (const auto& sd : m_snapshots) {
        entries.push_back(sd.info);
    }
    return entries;
}

std::optional<std::pair<std::vector<unsigned char>, std::vector<uint256>>> UTXOSetShareProvider::GetChunk(uint32_t height, const uint256& block_hash, uint32_t chunk_index) const
{
    const SnapshotData* snapshot{nullptr};
    for (const auto& sd : m_snapshots) {
        if (sd.info.height == height && sd.info.block_hash == block_hash) {
            snapshot = &sd;
            break;
        }
    }
    if (!snapshot) return std::nullopt;

    std::vector<uint256> leaf_hashes = snapshot->leaf_hashes;
    uint32_t total_chunks = leaf_hashes.size();
    if (chunk_index >= total_chunks) return std::nullopt;

    AutoFile file{fsbridge::fopen(snapshot->path, "rb")};
    if (file.IsNull()) return std::nullopt;

    uint64_t offset{chunk_index * UTXO_SET_CHUNK_SIZE};
    file.seek(offset, SEEK_SET);
    uint64_t chunk_size{UTXO_SET_CHUNK_SIZE};
    if (chunk_index == total_chunks - 1) {
        chunk_size = snapshot->info.data_length - offset;
    }

    std::vector<unsigned char> data(chunk_size);
    file.read(MakeWritableByteSpan(data));

    std::vector<uint256> proof{ComputeMerklePath(leaf_hashes, chunk_index)};

    return std::make_pair(std::move(data), std::move(proof));
}

} // namespace node
