// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_COINSELECTION_H
#define SYSCOIN_WALLET_COINSELECTION_H

#include <amount.h>
#include <primitives/transaction.h>
#include <random.h>

class CFeeRate;

//! target minimum change amount
static constexpr CAmount MIN_CHANGE{COIN / 100};
//! final minimum change amount after paying for fees
static const CAmount MIN_FINAL_CHANGE = MIN_CHANGE/2;

class CInputCoin {
public:
    CInputCoin(const CTransactionRef& tx, unsigned int i)
    {
        if (!tx)
            throw std::invalid_argument("tx should not be null");
        if (i >= tx->vout.size())
            throw std::out_of_range("The output index is out of range");

        outpoint = COutPoint(tx->GetHash(), i);
        txout = tx->vout[i];
        effective_value = txout.nValue;
        // SYSCOIN
        effective_value_asset = txout.assetInfo;
    }

    CInputCoin(const CTransactionRef& tx, unsigned int i, int input_bytes) : CInputCoin(tx, i)
    {
        m_input_bytes = input_bytes;
    }

    COutPoint outpoint;
    CTxOut txout;
    CAmount effective_value;
    // SYSCOIN
    CAssetCoinInfo effective_value_asset;
    CAmount m_fee{0};
    CAmount m_long_term_fee{0};

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int m_input_bytes{-1};

    bool operator<(const CInputCoin& rhs) const {
        return outpoint < rhs.outpoint;
    }

    bool operator!=(const CInputCoin& rhs) const {
        return outpoint != rhs.outpoint;
    }

    bool operator==(const CInputCoin& rhs) const {
        return outpoint == rhs.outpoint;
    }
};

struct CoinEligibilityFilter
{
    const int conf_mine;
    const int conf_theirs;
    const uint64_t max_ancestors;
    const uint64_t max_descendants;

    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
};

struct OutputGroup
{
    std::vector<CInputCoin> m_outputs;
    bool m_from_me{true};
    CAmount m_value{0};
    int m_depth{999};
    size_t m_ancestors{0};
    size_t m_descendants{0};
    CAmount effective_value{0};
    CAmount fee{0};
    CAmount long_term_fee{0};
    // SYSCOIN
    CAmount m_value_asset{0};
    CAssetCoinInfo effective_value_asset;
    OutputGroup() {}
    // SYSCOIN
    OutputGroup(std::vector<CInputCoin>&& outputs, bool from_me, CAmount value, CAmount value_asset, int depth, size_t ancestors, size_t descendants)
    : m_outputs(std::move(outputs))
    , m_from_me(from_me)
    , m_value(value)
    , m_depth(depth)
    , m_ancestors(ancestors)
    , m_descendants(descendants)
    , m_value_asset(value_asset)
    {}
    // SYSCOIN
    OutputGroup(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants, const CAssetCoinInfo& nTargetValueAsset=CAssetCoinInfo()) : OutputGroup() {
        Insert(output, depth, from_me, ancestors, descendants, nTargetValueAsset);
    }
    // SYSCOIN
    void Insert(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants, const CAssetCoinInfo& nTargetValueAsset=CAssetCoinInfo());
    std::vector<CInputCoin>::iterator Discard(const CInputCoin& output);
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const;

    //! Update the OutputGroup's fee, long_term_fee, and effective_value based on the given feerates
    void SetFees(const CFeeRate effective_feerate, const CFeeRate long_term_feerate);
    OutputGroup GetPositiveOnlyGroup();
};

bool SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);
// SYSCOIN
bool KnapsackSolver(const CAssetCoinInfo& nTargetValueAsset, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);

#endif // SYSCOIN_WALLET_COINSELECTION_H
