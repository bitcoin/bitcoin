// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXOSCRIPTINDEX_H
#define BITCOIN_INDEX_TXOSCRIPTINDEX_H

#include <crypto/sha256.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <util/expected.h>
#include <util/not_null.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace txoscriptindex {
inline uint256 Sha256ScriptPubKey(const CScript& script)
{
    uint256 hash;
    CSHA256().Write(script.data(), script.size()).Finalize(hash.data());
    return hash;
}
} // namespace txoscriptindex

namespace txoscriptindex_tests {
class TxoScriptIndexTest;
}

inline constexpr bool DEFAULT_TXOSCRIPTINDEX{false};

//! Result of a txoscriptindex prefix lookup.
//! `tx` is null when a matching key was found but the block has been pruned.
struct ScriptHistoryMatch {
    CTransactionRef tx;
    uint256 block_hash;
    int32_t height;
    //! Output indexes in tx whose scriptPubKey hashes to the queried script hash.
    //! Empty when `tx` is null.
    std::vector<uint32_t> outputs;
    //! Transaction byte offset within the block; zero for pruned-block placeholders.
    uint32_t tx_offset{0};
};

/**
 * TxoScriptIndex is used to look up transactions creating outputs with a given script.
 * The index is written to a LevelDB database and, for each distinct script of each transaction in a block,
 * records the SHA256(script), along with the block height and position of the transaction in the block.
 */
class TxoScriptIndex final : public BaseIndex
{
    friend class txoscriptindex_tests::TxoScriptIndexTest;

protected:
    class DB;

private:
    const util::NotNullUniquePtr<DB> m_db;

    bool AllowPrune() const override { return true; }

protected:
    interfaces::Chain::NotifyOptions CustomOptions() override;

    bool CustomAppend(const interfaces::BlockInfo& block) override;

    bool CustomRemove(const interfaces::BlockInfo& block) override;

    BaseIndex::DB& GetDB() const override;

public:
    explicit TxoScriptIndex(std::unique_ptr<interfaces::Chain> chain, size_t cache_size, bool memory = false, bool wipe = false);

    ~TxoScriptIndex() override;

    /**
     * Search the index for confirmed transactions with outputs matching the given script hash.
     *
     * @param[in]     script_hash      SHA256 of the scriptPubKey to search for.
     * @param[in]     start_height     First block height to search (inclusive).
     * @param[in]     end_height       Last block height to search (inclusive).
     * @param[in,out] remaining_bytes  Remaining lookup budget, decremented for serialized visited keys and
     *                                candidate transactions, including repeated reads, false positives,
     *                                and keys for pruned blocks. Transaction bytes are charged after reading,
     *                                before checking script hashes.
     *
     * @return  Matches in block height and transaction position order (empty if none).
     *          util::Unexpected{error} if an index key or transaction could not be read, or the budget is exceeded.
     */
    util::Expected<std::vector<ScriptHistoryMatch>, std::string> Find(const uint256& script_hash, int32_t start_height, int32_t end_height, uint64_t& remaining_bytes) const;
};

extern std::unique_ptr<TxoScriptIndex> g_txoscriptindex;

#endif // BITCOIN_INDEX_TXOSCRIPTINDEX_H
