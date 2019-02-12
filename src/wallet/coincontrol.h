// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_COINCONTROL_H
#define BITCOIN_WALLET_COINCONTROL_H

#include <primitives/transaction.h>
#include <wallet/wallet.h>

#include <boost/optional.hpp>

/** Coin Control Features. */
class CCoinControl
{
public:
    //! Custom change destination, if not set an address is generated
    CTxDestination destChange;
    //! Custom change type, ignored if destChange is set, defaults to g_change_type
    OutputType change_type;
    //! If false, allows unselected inputs, but requires all selected inputs be used
    bool fAllowOtherInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE criteria
    bool fAllowWatchOnly;
    //! Override the default confirmation target if set
    boost::optional<unsigned int> m_confirm_target;

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
        m_confirm_target.reset();
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
