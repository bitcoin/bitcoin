// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include <key.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/standard.h>

#include <optional>

namespace wallet {
enum class CoinType : uint8_t
{
    ALL_COINS,
    ONLY_FULLY_MIXED,
    ONLY_READY_TO_MIX,
    ONLY_NONDENOMINATED,
    ONLY_MASTERNODE_COLLATERAL, // find masternode outputs including locked ones (use with caution)
    ONLY_COINJOIN_COLLATERAL,
    // Attributes
    MIN_COIN_TYPE = ALL_COINS,
    MAX_COIN_TYPE = ONLY_COINJOIN_COLLATERAL,
};

//! Default for -avoidpartialspends
static constexpr bool DEFAULT_AVOIDPARTIALSPENDS = false;

const int DEFAULT_MIN_DEPTH = 0;
const int DEFAULT_MAX_DEPTH = 9999999;

/** Coin Control Features. */
class CCoinControl
{
public:
    //! Custom change destination, if not set an address is generated
    CTxDestination destChange = CNoDestination();
    //! If false, only selected inputs are used
    bool m_add_inputs = true;
    //! If false, only safe inputs will be used
    bool m_include_unsafe_inputs = false;
    //! If false, allows unselected inputs, but requires all selected inputs be used if fAllowOtherInputs is true (default)
    bool fAllowOtherInputs = false;
    //! If false, only include as many inputs as necessary to fulfill a coin selection request. Only usable together with fAllowOtherInputs
    bool fRequireAllInputs = true;
    //! Includes watch only addresses which are solvable
    bool fAllowWatchOnly = false;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate = false;
    //! Override the wallet's m_pay_tx_fee if set
    std::optional<CFeeRate> m_feerate;
    //! Override the discard feerate estimation with m_discard_feerate in CreateTransaction if set
    std::optional<CFeeRate> m_discard_feerate;
    //! Override the default confirmation target if set
    std::optional<unsigned int> m_confirm_target;
    //! Avoid partial use of funds sent to a given address
    bool m_avoid_partial_spends = DEFAULT_AVOIDPARTIALSPENDS;
    //! Forbids inclusion of dirty (previously used) addresses
    bool m_avoid_address_reuse = false;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode = FeeEstimateMode::UNSET;
    //! Minimum chain depth value for coin availability
    int m_min_depth = DEFAULT_MIN_DEPTH;
    //! Maximum chain depth value for coin availability
    int m_max_depth = DEFAULT_MAX_DEPTH;
    //! Controls which types of coins are allowed to be used (default: ALL_COINS)
    CoinType nCoinType = CoinType::ALL_COINS;

    CCoinControl(CoinType coin_type = CoinType::ALL_COINS);

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    bool IsSelected(const COutPoint& output) const
    {
        return (setSelected.count(output) > 0);
    }

    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }

    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
    }

    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

    // Dash-specific helpers

    void UseCoinJoin(bool fUseCoinJoin)
    {
        nCoinType = fUseCoinJoin ? CoinType::ONLY_FULLY_MIXED : CoinType::ALL_COINS;
    }

    bool IsUsingCoinJoin() const
    {
        return nCoinType == CoinType::ONLY_FULLY_MIXED;
    }

private:
    std::set<COutPoint> setSelected;
};
} // namespace wallet

#endif // BITCOIN_WALLET_COINCONTROL_H
