// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include <policy/feerate.h>
#include <policy/fees.h>
#include <primitives/transaction.h>
#include <wallet/wallet.h>

#include <boost/optional.hpp>


/** Coin pick policy */
enum class CoinPickPolicy {
    IncludeIfSet = 0,   //! Pick all coin if "destPick" set. Default
    ExcludeIfSet,       //! Pick all coin but exclude if "destPick" set
    MoveAllTo,          //! Pick all but exclude "destPick". Only for move to
};

/** Coin Control Features. */
class CCoinControl
{
public:
    //! Custom change destination, if not set will output to primary destination
    CTxDestination destChange;
    //! Custom change type, ignored if destChange is set, defaults to g_change_type
    OutputType change_type;
    //! If false, allows unselected inputs, but requires all selected inputs be used
    bool fAllowOtherInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE criteria
    bool fAllowWatchOnly;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate;
    //! Override the default payTxFee if set
    boost::optional<CFeeRate> m_feerate;
    //! Override the default confirmation target if set
    boost::optional<unsigned int> m_confirm_target;
    //! Signal BIP-125 replace by fee.
    bool signalRbf;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode;
    //! Use fixed fee on m_fee_mode=FeeEstimateMode::FIXED
    CAmount fixedFee;
    //! Coin pick policy
    CoinPickPolicy coinPickPolicy;
    //! Custom pick destination, if not set unlimit select
    CTxDestination destPick;

    CCoinControl()
    {
        SetNull();
    }

    void SetNull()
    {
        destChange = CNoDestination();
        change_type = g_change_type;
        fAllowOtherInputs = false;
        fAllowWatchOnly = false;
        setSelected.clear();
        m_feerate.reset();
        fOverrideFeeRate = false;
        m_confirm_target.reset();
        signalRbf = fWalletRbf;
        m_fee_mode = FeeEstimateMode::UNSET;
        fixedFee = 0;
        coinPickPolicy = CoinPickPolicy::IncludeIfSet;
        destPick = CNoDestination();
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

private:
    std::set<COutPoint> setSelected;
};

#endif // BITCOIN_WALLET_COINCONTROL_H
