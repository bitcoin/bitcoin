// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include "primitives/transaction.h"

class CCoinControlEntry
{
public:
    COutPoint op;
    int nType;
    int nDepth;
    CAmount nValue;
    CScript scriptPubKey;
    int64_t nTxTime;
};

/** Coin Control Features. */
class CCoinControl
{
public:
    CScript scriptChange;
    CTxDestination destChange;
    //! If false, allows unselected inputs, but requires all selected inputs be used
    bool fAllowOtherInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE criteria
    bool fAllowWatchOnly;
    //! Minimum absolute fee (not per kilobyte)
    CAmount nMinimumTotalFee;
    //! Override estimated feerate
    bool fOverrideFeeRate;
    //! Feerate to use if overrideFeeRate is true
    CFeeRate nFeeRate;
    //! Override the default confirmation target, 0 = use default
    int nConfirmTarget;
    
    int nCoinType;

    CCoinControl()
    {
        SetNull();
    }

    void SetNull()
    {
        destChange = CNoDestination();
        fAllowOtherInputs = false;
        fAllowWatchOnly = false;
        setSelected.clear();
        nMinimumTotalFee = 0;
        nFeeRate = CFeeRate(0);
        fOverrideFeeRate = false;
        nConfirmTarget = 0;
        
        nCoinType = OUTPUT_STANDARD;
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
    
    size_t NumSelected()
    {
        return setSelected.size();
    }

//private:
    std::set<COutPoint> setSelected;
};

#endif // BITCOIN_WALLET_COINCONTROL_H
