// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINSELECTION_H
#define BITCOIN_WALLET_COINSELECTION_H

#include <amount.h>
#include <mw/models/wallet/Coin.h>
#include <policy/feerate.h>
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

        m_index = COutPoint(tx->GetHash(), i);
        m_output = tx->vout[i];
        effective_value = tx->vout[i].nValue;
    }

    CInputCoin(const CTransactionRef& tx, unsigned int i, int input_bytes) : CInputCoin(tx, i)
    {
        m_input_bytes = input_bytes;
    }

    CInputCoin(const uint256& tx_hash, uint32_t i, const CAmount& nValue, const CScript& scriptPubKey)
        : m_output(CTxOut(nValue, scriptPubKey)), m_index(COutPoint(tx_hash, i)) {}

    explicit CInputCoin(const mw::Coin& coin)
        : m_input_bytes(0), m_output(coin), m_index(coin.output_id) {}

    bool IsMWEB() const noexcept { return m_output.type() == typeid(mw::Coin); }
    CAmount GetAmount() const noexcept { return IsMWEB() ? boost::get<mw::Coin>(m_output).amount : boost::get<CTxOut>(m_output).nValue; }
    const OutputIndex& GetIndex() const noexcept { return m_index; }

    const COutPoint& GetOutpoint() const noexcept
    {
        assert(!IsMWEB());
        return boost::get<COutPoint>(m_index);
    }

    mw::Coin GetMWEBCoin() const noexcept
    {
        assert(IsMWEB());
        return boost::get<mw::Coin>(m_output);
    }

    CAmount CalculateFee(const CFeeRate& feerate) const noexcept
    {
        if (IsMWEB()) {
            return 0;
        }

        return m_input_bytes < 0 ? 0 : feerate.GetFee(m_input_bytes);
    }
	
    CAmount effective_value;
    CAmount m_fee{0};
    CAmount m_long_term_fee{0};

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int m_input_bytes{-1};

    bool operator<(const CInputCoin& rhs) const {
        return m_index < rhs.m_index;
    }

    bool operator!=(const CInputCoin& rhs) const {
        return m_index != rhs.m_index;
    }

    bool operator==(const CInputCoin& rhs) const {
        return m_index == rhs.m_index;
    }

private:
    boost::variant<CTxOut, mw::Coin> m_output;

    OutputIndex m_index;
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

enum class InputPreference {
    // Use LTC and MWEB inputs (MIXED)
    ANY,
    // Only use MWEB inputs (used when explicitly pegging-out)
    MWEB_ONLY,
    // Only use canonical inputs (used when explicitly pegging-in)
    LTC_ONLY
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

    OutputGroup() {}
    OutputGroup(std::vector<CInputCoin>&& outputs, bool from_me, CAmount value, int depth, size_t ancestors, size_t descendants)
    : m_outputs(std::move(outputs))
    , m_from_me(from_me)
    , m_value(value)
    , m_depth(depth)
    , m_ancestors(ancestors)
    , m_descendants(descendants)
    {}
    OutputGroup(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants) : OutputGroup() {
        Insert(output, depth, from_me, ancestors, descendants);
    }
    void Insert(const CInputCoin& output, int depth, bool from_me, size_t ancestors, size_t descendants);
    std::vector<CInputCoin>::iterator Discard(const CInputCoin& output);
    bool EligibleForSpending(const CoinEligibilityFilter& eligibility_filter, const InputPreference& input_preference) const;
    bool IsMWEB() const noexcept
    {
        return !m_outputs.empty() && m_outputs.front().IsMWEB();
    }

    //! Update the OutputGroup's fee, long_term_fee, and effective_value based on the given feerates
    void SetFees(const CFeeRate effective_feerate, const CFeeRate long_term_feerate);
    OutputGroup GetPositiveOnlyGroup();
};

bool SelectCoinsBnB(std::vector<OutputGroup>& utxo_pool, const CAmount& target_value, const CAmount& cost_of_change, std::set<CInputCoin>& out_set, CAmount& value_ret, CAmount not_input_fees);

// Original coin selection algorithm as a fallback
bool KnapsackSolver(const CAmount& nTargetValue, std::vector<OutputGroup>& groups, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet);

#endif // BITCOIN_WALLET_COINSELECTION_H
