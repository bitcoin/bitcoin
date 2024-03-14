// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/chainhelper.h>

#include <consensus/params.h>
#include <masternode/payments.h>

CChainstateHelper::CChainstateHelper(CGovernanceManager& govman, const Consensus::Params& consensus_params, const CMasternodeSync& mn_sync,
                                     const CSporkManager& sporkman)
    : mn_payments{std::make_unique<CMNPaymentsProcessor>(govman, consensus_params, mn_sync, sporkman)}
{}

CChainstateHelper::~CChainstateHelper() = default;
