// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include <key.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <script/standard.h>

#include <boost/optional.hpp>

enum class CoinType
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

/** Coin Control Features. */
class CCoinControl
{
public:
    CTxDestination destChange;
    //! If false, allows unselected inputs, but requires all selected inputs be used if fAllowOtherInputs is true (default)
    bool fAllowOtherInputs;
    //! If false, only include as many inputs as necessary to fulfill a coin selection request. Only usable together with fAllowOtherInputs
    bool fRequireAllInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE criteria
    bool fAllowWatchOnly;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate;
    //! Override the wallet's m_pay_tx_fee if set
    boost::optional<CFeeRate> m_feerate;
    //! Override the discard feerate estimation with m_discard_feerate in CreateTransaction if set
    boost::optional<CFeeRate> m_discard_feerate;
    //! Override the default confirmation target if set
    boost::optional<unsigned int> m_confirm_target;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode;
    //! Controls which types of coins are allowed to be used (default: ALL_COINS)
    CoinType nCoinType;

    CCoinControl()
    {
        SetNull();
    }

    void SetNull(bool fResetCoinType = true)
    {
        destChange = CNoDestination();
        fAllowOtherInputs = false;
        fRequireAllInputs = true;
        fAllowWatchOnly = false;
        setSelected.clear();
        m_feerate.reset();
        m_discard_feerate.reset();
        fOverrideFeeRate = false;
        m_confirm_target.reset();
        m_fee_mode = FeeEstimateMode::UNSET;
        if (fResetCoinType) {
            nCoinType = CoinType::ALL_COINS;
        }
    }

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

#endif // BITCOIN_WALLET_COINCONTROL_H
