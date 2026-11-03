// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_SPENTINDEX_H
#define BITCOIN_INDEX_SPENTINDEX_H

#include <index/base.h>
#include <index/spentindex_types.h>

static constexpr bool DEFAULT_SPENTINDEX{false};

/**
 * SpentIndex tracks which transactions spend specific outputs.
 * For each spent output, it records the spending transaction details,
 * including height, amount, and address information.
 *
 * The index reads undo data to extract spent output information (amount, address),
 * which requires that undo files are available. Therefore, this index is NOT
 * compatible with pruned nodes.
 *
 * The index maintains a separate LevelDB database at <datadir>/indexes/spentindex/
 */
class SpentIndex final : public BaseIndex
{
protected:
    class DB;

private:
    const std::unique_ptr<DB> m_db;

protected:
    class DB : public BaseIndex::DB
    {
    public:
        explicit DB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

        /// Write a batch of spent index entries
        bool WriteBatch(const std::vector<CSpentIndexEntry>& entries);

        /// Read spent information for a specific output
        bool ReadSpentIndex(const CSpentIndexKey& key, CSpentIndexValue& value);

        /// Erase spent index entries
        void EraseSpentIndex(CDBBatch& batch, const std::vector<CSpentIndexKey>& keys);
    };

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRewind(const interfaces::BlockKey& current_tip, const interfaces::BlockKey& new_tip) override;

    BaseIndex::DB& GetDB() const override;

    /// SpentIndex cannot work with pruned nodes as it requires UTXO data
    bool AllowPrune() const override { return false; }

public:
    /// Constructs a new SpentIndex.
    explicit SpentIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /// Destructor
    virtual ~SpentIndex() override;

    /// Retrieve spent information for a specific output
    bool GetSpentInfo(const CSpentIndexKey& key, CSpentIndexValue& value) const;
};

#endif // BITCOIN_INDEX_SPENTINDEX_H
