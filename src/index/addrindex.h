// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_ADDRINDEX_H
#define BITCOIN_INDEX_ADDRINDEX_H

#include <chain.h>
#include <index/base.h>
#include <vector>
#include <txdb.h>
#include <uint256.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <script/script.h>
#include <undo.h>

/**
 * AddrIndex is used to look up transactions included in the blockchain by script.
 * The index is written to a LevelDB database and records the filesystem
 * location of transactions by script.
 */
class AddrIndex final : public BaseIndex
{
protected:
    class DB;

private:
    const std::unique_ptr<DB> m_db;

    // m_hash_seed is used by GetAddrID in its calls to MurmurHash3.
    // It is stored in the index, and restored from there on construction
    // to maintain consistency.
    unsigned int m_hash_seed;

    unsigned int GetAddrId(const CScript& script);

protected:
    bool Init() override;

    bool WriteBlock(const CBlock& block, const CBlockIndex* pindex) override;

    BaseIndex::DB& GetDB() const override;

    const char* GetName() const override { return "addr_index"; }

public:
    /// Constructs the index, which becomes available to be queried.
    explicit AddrIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Destructor is declared because this class contains a unique_ptr to an incomplete type.
    ~AddrIndex() override;

    bool FindTxsByScript(const int max_count,
                         const int skip,
                         const CScript& dest,
                         std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> &spends_result,
                         std::vector<std::pair<COutPoint, std::pair<CTransactionRef, uint256>>> &creations_result);
};

/// The global address index, used in FindTxsByScript. May be null.
extern std::unique_ptr<AddrIndex> g_addr_index;

#endif // BITCOIN_INDEX_ADDRINDEX_H
