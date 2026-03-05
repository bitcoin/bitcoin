// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSPENDERINDEX_H
#define BITCOIN_INDEX_TXOSPENDERINDEX_H

#include <index/base.h>
#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/expected.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct CDiskTxPos;

static constexpr bool DEFAULT_TXOSPENDERINDEX{false};

struct TxoSpender {
    CTransactionRef tx;
    uint256 block_hash;
};

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
    bool AllowPrune() const override { return false; }
    void WriteSpenderInfos(const std::vector<std::pair<COutPoint, CDiskTxPos>>& items);
    void EraseSpenderInfos(const std::vector<std::pair<COutPoint, CDiskTxPos>>& items);
    util::Expected<TxoSpender, std::string> ReadTransaction(const CDiskTxPos& pos) const;

protected:
    interfaces::Chain::NotifyOptions CustomOptions() override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRemove(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const override;

public:
    explicit TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /**
     * Search the index for a transaction that spends the given outpoint.
     *
     * @param[in] txo  The outpoint to search for.
     *
     * @return  std::nullopt               if the outpoint has not been spent on-chain.
     *          std::optional{TxoSpender}  if the output has been spent on-chain. Contains the spending transaction
     *                                     and the block it was confirmed in.
     *          util::Unexpected{error}    if something unexpected happened (i.e. disk or deserialization error).
     */
    util::Expected<std::optional<TxoSpender>, std::string> FindSpender(const COutPoint& txo) const;
};

/// The global txo spender index. May be null.
extern std::unique_ptr<TxoSpenderIndex> g_txospenderindex;


#endif // BITCOIN_INDEX_TXOSPENDERINDEX_H
