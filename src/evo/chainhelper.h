// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_CHAINHELPER_H
#define BITCOIN_EVO_CHAINHELPER_H

#include <memory>

class CCreditPoolManager;
class CDeterministicMNManager;
class ChainstateManager;
class CMNHFManager;
class CMNPaymentsProcessor;
class CMasternodeSync;
class CGovernanceManager;
class CSpecialTxProcessor;
class CSporkManager;

namespace Consensus { struct Params; }
namespace llmq {
class CChainLocksHandler;
class CQuorumBlockProcessor;
class CQuorumManager;
}

class CChainstateHelper
{
public:
    explicit CChainstateHelper(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman, CMNHFManager& mnhfman, CGovernanceManager& govman,
                               llmq::CQuorumBlockProcessor& qblockman, const ChainstateManager& chainman, const Consensus::Params& consensus_params,
                               const CMasternodeSync& mn_sync, const CSporkManager& sporkman, const llmq::CChainLocksHandler& clhandler,
                               const llmq::CQuorumManager& qman);
    ~CChainstateHelper();

    CChainstateHelper() = delete;
    CChainstateHelper(const CChainstateHelper&) = delete;

public:
    const std::unique_ptr<CMNPaymentsProcessor> mn_payments;
    const std::unique_ptr<CSpecialTxProcessor> special_tx;
};

#endif // BITCOIN_EVO_CHAINHELPER_H
