// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSPENDERINDEX_H
#define BITCOIN_INDEX_TXOSPENDERINDEX_H

#include <index/base.h>
#include <index/disktxpos.h>

static constexpr bool DEFAULT_TXOSPENDERINDEX{false};

/**
 * TxoSpenderIndex is used to look up which transaction spent a given output.
 * The index is written to a LevelDB database and, for each input of each transaction in a block,
 * records the outpoint that is spent and the hash of the spending transaction.
 */
class TxoSpenderIndex final : public BaseIndex
{
private:
    std::unique_ptr<BaseIndex::DB> m_db;
    std::pair<uint64_t, uint64_t> m_siphash_key;
    uint64_t CreateKey(const COutPoint& vout) const;
    bool AllowPrune() const override { return true; }
    bool WriteSpenderInfos(const std::vector<std::pair<COutPoint, CDiskTxPos>>& items);
    bool EraseSpenderInfos(const std::vector<COutPoint>& items);
    bool ReadTransaction(const CDiskTxPos& pos, CTransactionRef& tx) const;

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRewind(const interfaces::BlockRef& current_tip, const interfaces::BlockRef& new_tip) override;

    BaseIndex::DB& GetDB() const override;

public:
    explicit TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Destroys unique_ptr to an incomplete type.
    virtual ~TxoSpenderIndex() override;

    CTransactionRef FindSpender(const COutPoint& txo) const;
};

/// The global txo spender index. May be null.
extern std::unique_ptr<TxoSpenderIndex> g_txospenderindex;


#endif // BITCOIN_INDEX_TXOSPENDERINDEX_H
