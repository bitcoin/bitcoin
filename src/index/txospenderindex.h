// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSPENDERINDEX_H
#define BITCOIN_INDEX_TXOSPENDERINDEX_H

#include <crypto/siphash.h>
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

struct CDiskTxPos;
namespace txospenderindex_tests {
class TxoSpenderIndexTest;
}
struct FlatFilePos;

inline constexpr bool DEFAULT_TXOSPENDERINDEX{false};

struct TxoSpender {
    CTransactionRef tx;
    uint256 block_hash;
};

/**
 * TxoSpenderIndex is used to look up which transaction spent a given output.
 * The index is written to a LevelDB database and, for each input of each transaction in a block,
 * records a hash prefix of the outpoint that is spent and the block sequence number and offset of the spending transaction.
 */
class TxoSpenderIndex final : public BaseIndex
{
private:
    friend class txospenderindex_tests::TxoSpenderIndexTest;
    /// Whether the database contains any legacy (disk position) entries.
    const bool m_has_legacy;
    std::unique_ptr<BaseIndex::DB> m_db;
    /// Used to hash the outpoint to compute the key prefix.
    const SipHasher13UJ m_hasher;
    /// Hasher of the legacy entries. Only set if the database contains any.
    std::optional<PresaltedSipHasher> m_legacy_hasher;
    bool AllowPrune() const override { return false; }
    util::Expected<CTransactionRef, std::string> ReadTransaction(const FlatFilePos& pos) const;
    util::Expected<TxoSpender, std::string> ReadLegacyTransaction(const CDiskTxPos& pos) const;
    /// Look up a spender among the legacy entries.
    std::optional<TxoSpender> FindLegacySpender(const COutPoint& txo) const;

protected:
    bool CustomAppend(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const override;

public:
    explicit TxoSpenderIndex(std::unique_ptr<interfaces::Chain> chain, size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /**
     * Search the index for a transaction that spends the given outpoint.
     *
     * @param[in] txo  The outpoint to search for.
     *
     * @return  std::nullopt               if the outpoint has not been spent in the active chain.
     *          TxoSpender                 if the output has been spent in the active chain. Contains the spending
     *                                     transaction and the block it was confirmed in.
     *
     * Candidates whose transaction cannot be read are skipped, as in TxIndex::FindTx.
     */
    std::optional<TxoSpender> FindSpender(const COutPoint& txo) const;
};

/// The global txo spender index. May be null.
extern std::unique_ptr<TxoSpenderIndex> g_txospenderindex;


#endif // BITCOIN_INDEX_TXOSPENDERINDEX_H
