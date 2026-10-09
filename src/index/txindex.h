// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_H
#define BITCOIN_INDEX_TXINDEX_H

#include <index/base.h>
#include <index/tx_lookup_result.h>
#include <primitives/transaction.h>

#include <cstddef>
#include <memory>

namespace interfaces {
class Chain;
}
namespace txindex_tests {
class TxIndexTest;
}

inline constexpr bool DEFAULT_TXINDEX{false};

/**
 * TxIndex is used to look up transactions included in the blockchain by hash.
 * The index is written to a LevelDB database and records the block sequence
 * number and serialized block offset of each transaction by transaction hash.
 */
class TxIndex final : public BaseIndex
{
protected:
    class DB;

private:
    friend class txindex_tests::TxIndexTest;
    const std::unique_ptr<DB> m_db;

    /// Look up a transaction among the legacy (full-txid) entries.
    TxLookupResult FindLegacyTx(const Txid& tx_hash) const;

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const override;

public:
    /// Constructs the index, which becomes available to be queried.
    explicit TxIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Destructor is declared because this class contains a unique_ptr to an incomplete type.
    virtual ~TxIndex() override;

    /// Pruning is allowed unless the database still contains legacy entries.
    bool AllowPrune() const override;

    /// Look up a transaction by hash.
    ///
    /// @param[in]   tx_hash  The hash of the transaction to be returned.
    /// @return  The transaction and containing block hash if found.
    ///          If not found, the hashes of blocks that may contain the tx but were pruned are returned instead.
    TxLookupResult FindTx(const Txid& tx_hash) const;
};

/// The global transaction index, used in GetTransaction. May be null.
extern std::unique_ptr<TxIndex> g_txindex;

#endif // BITCOIN_INDEX_TXINDEX_H
