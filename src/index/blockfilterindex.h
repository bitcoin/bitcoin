// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_BLOCKFILTERINDEX_H
#define BITCOIN_INDEX_BLOCKFILTERINDEX_H

#include <attributes.h>
#include <flatfile.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <sync.h>
#include <uint256.h>
#include <util/hasher.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class BlockFilter;
class CBlockIndex;
enum class BlockFilterType : uint8_t;

static const char* const DEFAULT_BLOCKFILTERINDEX = "0";

/** Interval between compact filter checkpoints. See BIP 157. */
static constexpr int CFCHECKPT_INTERVAL = 1000;

/**
 * BlockFilterIndex is used to store and retrieve block filters, hashes, and headers for a range of
 * blocks by height. An index is constructed for each supported filter type with its own database
 * (ie. filter data for different types are stored in separate databases).
 *
 * This index is used to serve BIP 157 net requests.
 */
class BlockFilterIndex final : public BaseIndex
{
private:
    BlockFilterType m_filter_type;
    std::unique_ptr<BaseIndex::DB> m_db;

    FlatFilePos m_next_filter_pos;
    std::unique_ptr<FlatFileSeq> m_filter_fileseq;

    bool ReadFilterFromDisk(const FlatFilePos& pos, const uint256& hash, BlockFilter& filter) const;
    size_t WriteFilterToDisk(FlatFilePos& pos, const BlockFilter& filter);

    Mutex m_cs_headers_cache;
    /** cache of block hash to filter header, to avoid disk access when responding to getcfcheckpt. */
    std::unordered_map<uint256, uint256, BlockHasher> m_headers_cache GUARDED_BY(m_cs_headers_cache);

    /** Protects on-the-fly writes in LookupFilter from racing with CustomAppend. */
    Mutex m_cs_write;

    // Last computed header to avoid disk reads on every new block.
    uint256 m_last_header{};

    bool AllowPrune() const override { return true; }

    bool Write(const BlockFilter& filter, uint32_t block_height, const uint256& filter_header);

    std::optional<uint256> ReadFilterHeader(int height, const uint256& expected_block_hash);

    /** Compute a filter for block_index from raw block/undo data and best-effort persist it.
     *  Used to mask the race between block connection and CustomAppend writing the filter.
     *  Returns true if filter_out is populated; persistence is best-effort and not required
     *  for success. */
    bool ComputeAndPersistFilter(const CBlockIndex* block_index, BlockFilter& filter_out);

    /** Ensure every block in [start_height, stop_index] has a filter entry in the DB,
     *  computing missing entries on the fly when within a small window of the chain tip.
     *  Returns false if any missing entry lies more than ONTHEFLY_TIP_WINDOW blocks
     *  below the tip — bounds DoS for callers serving peer requests. */
    bool EnsureRangeIndexed(int start_height, const CBlockIndex* stop_index);

protected:
    bool CustomInit(const std::optional<interfaces::BlockRef>& block) override;

    bool CustomCommit(CDBBatch& batch) override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRemove(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const LIFETIMEBOUND override { return *m_db; }

public:
    /** Constructs the index, which becomes available to be queried. */
    explicit BlockFilterIndex(std::unique_ptr<interfaces::Chain> chain, BlockFilterType filter_type,
                              size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    interfaces::Chain::NotifyOptions CustomOptions() override;

    BlockFilterType GetFilterType() const { return m_filter_type; }

    /** Get a single filter by block.
     *  If the filter is not yet indexed, computes it from raw block data (when available)
     *  and writes it to the database so subsequent requests succeed immediately. */
    bool LookupFilter(const CBlockIndex* block_index, BlockFilter& filter_out);

    /** Get a single filter header by block. */
    bool LookupFilterHeader(const CBlockIndex* block_index, uint256& header_out) EXCLUSIVE_LOCKS_REQUIRED(!m_cs_headers_cache);

    /** Get a range of filters between two heights on a chain.
     *  If any filter in the range is not yet indexed but the block is within a small window of
     *  the active chain tip, computes it from raw block data and persists it. Returns false if
     *  any missing entry is outside that window (DoS protection against unindexed-block fetches). */
    bool LookupFilterRange(int start_height, const CBlockIndex* stop_index,
                           std::vector<BlockFilter>& filters_out);

    /** Get a range of filter hashes between two heights on a chain.
     *  If any hash in the range is not yet indexed but the block is within a small window of
     *  the active chain tip, computes the filter from raw block data and persists it. Returns
     *  false if any missing entry is outside that window (DoS protection). */
    bool LookupFilterHashRange(int start_height, const CBlockIndex* stop_index,
                               std::vector<uint256>& hashes_out);
};

/**
 * Get a block filter index by type. Returns nullptr if index has not been initialized or was
 * already destroyed.
 */
BlockFilterIndex* GetBlockFilterIndex(BlockFilterType filter_type);

/** Iterate over all running block filter indexes, invoking fn on each. */
void ForEachBlockFilterIndex(std::function<void (BlockFilterIndex&)> fn);

/**
 * Initialize a block filter index for the given type if one does not already exist. Returns true if
 * a new index is created and false if one has already been initialized.
 */
bool InitBlockFilterIndex(std::function<std::unique_ptr<interfaces::Chain>()> make_chain, BlockFilterType filter_type,
                          size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

/**
 * Destroy the block filter index with the given type. Returns false if no such index exists. This
 * just releases the allocated memory and closes the database connection, it does not delete the
 * index data.
 */
bool DestroyBlockFilterIndex(BlockFilterType filter_type);

/** Destroy all open block filter indexes. */
void DestroyAllBlockFilterIndexes();

#endif // BITCOIN_INDEX_BLOCKFILTERINDEX_H
