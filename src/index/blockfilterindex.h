// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_BLOCKFILTERINDEX_H
#define BITCOIN_INDEX_BLOCKFILTERINDEX_H

#include <attributes.h>
#include <blockfilter.h>
#include <chain.h>
#include <flatfile.h>
#include <index/base.h>
#include <util/hasher.h>

#include <unordered_map>

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
    std::unordered_map<uint256, uint256, FilterHeaderHasher> m_headers_cache GUARDED_BY(m_cs_headers_cache);

    // Last computed header to avoid disk reads on every new block.
    uint256 m_last_header{};

    bool AllowPrune() const override { return true; }

    bool Write(const BlockFilter& filter, uint32_t block_height, const uint256& filter_header);

    std::optional<uint256> ReadFilterHeader(int height, const uint256& expected_block_hash);

protected:
    bool CustomInit(const std::optional<interfaces::BlockKey>& block) override;

    bool CustomCommit(CDBBatch& batch) override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override;

    BaseIndex::DB& GetDB() const LIFETIMEBOUND override { return *m_db; }

public:
    /** Constructs the index, which becomes available to be queried. */
    explicit BlockFilterIndex(std::unique_ptr<interfaces::Chain> chain, BlockFilterType filter_type,
                              size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    BlockFilterType GetFilterType() const { return m_filter_type; }

    /** Get a single filter by block. */
    bool LookupFilter(const CBlockIndex* block_index, BlockFilter& filter_out) const;

    /** Get a single filter header by block. */
    bool LookupFilterHeader(const CBlockIndex* block_index, uint256& header_out) EXCLUSIVE_LOCKS_REQUIRED(!m_cs_headers_cache);

    /** Get a range of filters between two heights on a chain. */
    bool LookupFilterRange(int start_height, const CBlockIndex* stop_index,
                           std::vector<BlockFilter>& filters_out) const;

    /** Get a range of filter hashes between two heights on a chain. */
    bool LookupFilterHashRange(int start_height, const CBlockIndex* stop_index,
                               std::vector<uint256>& hashes_out) const;
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
