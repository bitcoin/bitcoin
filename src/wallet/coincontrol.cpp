// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coincontrol.h>

#include <util/system.h>

void CCoinControl::SetNull()
{
    destChange = CNoDestination();
    m_change_type.reset();
    fAllowOtherInputs = false;
    fAllowWatchOnly = false;
    m_avoid_partial_spends = gArgs.GetBoolArg("-avoidpartialspends", DEFAULT_AVOIDPARTIALSPENDS);
    m_avoid_address_reuse = false;
    setSelected.clear();
    m_feerate.reset();
    fOverrideFeeRate = false;
    m_confirm_target.reset();
    m_signal_bip125_rbf.reset();
    m_fee_mode = FeeEstimateMode::UNSET;
    m_min_depth = DEFAULT_MIN_DEPTH;
    m_max_depth = DEFAULT_MAX_DEPTH;
}

