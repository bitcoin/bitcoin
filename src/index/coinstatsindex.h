// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_COINSTATSINDEX_H
#define BITCOIN_INDEX_COINSTATSINDEX_H

#include <chain.h>
#include <crypto/muhash.h>
#include <flatfile.h>
#include <index/base.h>
#include <node/coinstats.h>

/**
 * CoinStatsIndex maintains statistics on the UTXO set.
 */
class CoinStatsIndex final : public BaseIndex
{
private:
    std::string m_name;
    std::unique_ptr<BaseIndex::DB> m_db;

    MuHash3072 m_muhash;
    uint64_t m_transaction_output_count{0};
    uint64_t m_bogo_size{0};
    CAmount m_total_amount{0};
    CAmount m_total_subsidy{0};
    CAmount m_total_unspendable_amount{0};
    CAmount m_total_prevout_spent_amount{0};
    CAmount m_total_new_outputs_ex_coinbase_amount{0};
    CAmount m_total_coinbase_amount{0};
    CAmount m_total_unspendables_genesis_block{0};
    CAmount m_total_unspendables_bip30{0};
    CAmount m_total_unspendables_scripts{0};
    CAmount m_total_unspendables_unclaimed_rewards{0};

    bool ReverseBlock(const CBlock& block, const CBlockIndex* pindex);

protected:
    bool Init() override;

    bool CommitInternal(CDBBatch& batch) override;

    bool WriteBlock(const CBlock& block, const CBlockIndex* pindex) override;

    bool Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip) override;

    BaseIndex::DB& GetDB() const override { return *m_db; }

    const char* GetName() const override { return "coinstatsindex"; }

public:
    // Constructs the index, which becomes available to be queried.
    explicit CoinStatsIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    // Look up stats for a specific block using CBlockIndex
    bool LookUpStats(const CBlockIndex* block_index, CCoinsStats& coins_stats) const;
};

/// The global UTXO set hash object.
extern std::unique_ptr<CoinStatsIndex> g_coin_stats_index;

#endif // BITCOIN_INDEX_COINSTATSINDEX_H
