// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/coincontrol.h>

#include <util/system.h>

CCoinControl::CCoinControl(const CCoinControl &obj) {
    destChange = obj.destChange;
    m_change_type = obj.m_change_type;
    fAllowOtherInputs = obj.fAllowOtherInputs;
    fAllowWatchOnly = obj.fAllowWatchOnly;
    m_avoid_partial_spends = obj.m_avoid_partial_spends;
    m_avoid_address_reuse = obj.m_avoid_address_reuse;
    setSelected = obj.setSelected;
    m_feerate = obj.m_feerate;
    fOverrideFeeRate = obj.fOverrideFeeRate;
    m_confirm_target = obj.m_confirm_target;
    m_signal_bip125_rbf = obj.m_signal_bip125_rbf;
    m_fee_mode = obj.m_fee_mode;
    m_min_depth = obj.m_min_depth;
    m_max_depth = obj.m_max_depth;
}


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
