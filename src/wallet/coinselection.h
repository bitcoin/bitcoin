// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_COINSELECTION_H
#define SYSCOIN_WALLET_COINSELECTION_H

#include <amount.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <random.h>

//! target minimum change amount
static constexpr CAmount MIN_CHANGE{COIN / 100};
//! final minimum change amount after paying for fees
static const CAmount MIN_FINAL_CHANGE = MIN_CHANGE/2;

/** A UTXO under consideration for use in funding a new transaction. */
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

/** Parameters for one iteration of Coin Selection. */
struct CoinSelectionParams
{
    /** Toggles use of Branch and Bound instead of Knapsack solver. */
    bool use_bnb = true;
    /** Size of a change output in bytes, determined by the output type. */
    size_t change_output_size = 0;
    /** Size of the input to spend a change output in virtual bytes. */
    size_t change_spend_size = 0;
    /** The targeted feerate of the transaction being built. */
    CFeeRate m_effective_feerate;
    /** The feerate estimate used to estimate an upper bound on what should be sufficient to spend
     * the change output sometime in the future. */
    CFeeRate m_long_term_feerate;
    /** If the cost to spend a change output at the discard feerate exceeds its value, drop it to fees. */
    CFeeRate m_discard_feerate;
    /** Size of the transaction before coin selection, consisting of the header and recipient
     * output(s), excluding the inputs and change output(s). */
    size_t tx_noinputs_size = 0;
    /** Indicate that we are subtracting the fee from outputs */
    bool m_subtract_fee_outputs = false;
    /** When true, always spend all (up to OUTPUT_GROUP_MAX_ENTRIES) or none of the outputs
     * associated with the same address. This helps reduce privacy leaks resulting from address
     * reuse. Dust outputs are not eligible to be added to output groups and thus not considered. */
    bool m_avoid_partial_spends = false;

    CoinSelectionParams(bool use_bnb, size_t change_output_size, size_t change_spend_size, CFeeRate effective_feerate,
                        CFeeRate long_term_feerate, CFeeRate discard_feerate, size_t tx_noinputs_size, bool avoid_partial) :
        use_bnb(use_bnb),
        change_output_size(change_output_size),
        change_spend_size(change_spend_size),
        m_effective_feerate(effective_feerate),
        m_long_term_feerate(long_term_feerate),
        m_discard_feerate(discard_feerate),
        tx_noinputs_size(tx_noinputs_size),
        m_avoid_partial_spends(avoid_partial)
    {}
    CoinSelectionParams() {}
};

/** Parameters for filtering which OutputGroups we may use in coin selection.
 * We start by being very selective and requiring multiple confirmations and
 * then get more permissive if we cannot fund the transaction. */
struct CoinEligibilityFilter
{
    /** Minimum number of confirmations for outputs that we sent to ourselves.
     * We may use unconfirmed UTXOs sent from ourselves, e.g. change outputs. */
    const int conf_mine;
    /** Minimum number of confirmations for outputs received from a different wallet. */
    const int conf_theirs;
    /** Maximum number of unconfirmed ancestors aggregated across all UTXOs in an OutputGroup. */
    const uint64_t max_ancestors;
    /** Maximum number of descendants that a single UTXO in the OutputGroup may have. */
    const uint64_t max_descendants;
    /** When avoid_reuse=true and there are full groups (OUTPUT_GROUP_MAX_ENTRIES), whether or not to use any partial groups.*/
    const bool m_include_partial_groups{false};

    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_ancestors) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants) {}
    CoinEligibilityFilter(int conf_mine, int conf_theirs, uint64_t max_ancestors, uint64_t max_descendants, bool include_partial) : conf_mine(conf_mine), conf_theirs(conf_theirs), max_ancestors(max_ancestors), max_descendants(max_descendants), m_include_partial_groups(include_partial) {}
};

/** A group of UTXOs paid to the same output script. */
struct OutputGroup
{
    /** The list of UTXOs contained in this output group. */
    std::vector<CInputCoin> m_outputs;
    /** Whether the UTXOs were sent by the wallet to itself. This is relevant because we may want at
     * least a certain number of confirmations on UTXOs received from outside wallets while trusting
     * our own UTXOs more. */
    bool m_from_me{true};
    /** The total value of the UTXOs in sum. */
    CAmount m_value{0};
    /** The minimum number of confirmations the UTXOs in the group have. Unconfirmed is 0. */
    int m_depth{999};
    /** The aggregated count of unconfirmed ancestors of all UTXOs in this
     * group. Not deduplicated and may overestimate when ancestors are shared. */
    size_t m_ancestors{0};
    /** The maximum count of descendants of a single UTXO in this output group. */
    size_t m_descendants{0};
    /** The value of the UTXOs after deducting the cost of spending them at the effective feerate. */
    CAmount effective_value{0};
    /** The fee to spend these UTXOs at the effective feerate. */
    CAmount fee{0};
    /** The target feerate of the transaction we're trying to build. */
    CFeeRate m_effective_feerate{0};
    /** The fee to spend these UTXOs at the long term feerate. */
    CAmount long_term_fee{0};
    /** The feerate for spending a created change output eventually (i.e. not urgently, and thus at
     * a lower feerate). Calculated using long term fee estimate. This is used to decide whether
     * it could be economical to create a change output. */
    CFeeRate m_long_term_feerate{0};
    // SYSCOIN
    CAssetCoinInfo effective_value_asset;
    OutputGroup() {}
    OutputGroup(const CFeeRate& effective_feerate, const CFeeRate& long_term_feerate) :
        m_effective_feerate(effective_feerate),
        m_long_term_feerate(long_term_feerate)
    {}
    // SYSCOIN
    std::vector<CInputCoin>::iterator Discard(const CInputCoin& output);
    void Insert(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants, bool positive_only, const CAssetCoinInfo& nTargetValueAsset=CAssetCoinInfo());
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter) const;
};

bool SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);
// SYSCOIN
bool KnapsackSolver(const CAssetCoinInfo& nTargetValueAsset, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);

#endif // SYSCOIN_WALLET_COINSELECTION_H
