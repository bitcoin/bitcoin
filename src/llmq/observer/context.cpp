// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/observer/context.h>

#include <llmq/dkgsessionmgr.h>
#include <llmq/quorums.h>

namespace llmq {
ObserverContext::ObserverContext(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman,
                                 CMasternodeMetaMan& mn_metaman, llmq::CDKGDebugManager& dkg_debugman,
                                 llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman,
                                 llmq::CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                 const CSporkManager& sporkman, const util::DbWrapperParams& db_params) :
    m_qman{qman},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(bls_worker, dmnman, dkg_debugman, mn_metaman, qblockman,
                                                        qsnapman, /*mn_activeman=*/nullptr, chainman, sporkman,
                                                        db_params, /*quorums_watch=*/true)}
{
    m_qman.ConnectManager(qdkgsman.get());
}

ObserverContext::~ObserverContext()
{
    m_qman.DisconnectManager();
}

void ObserverContext::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
}
} // namespace llmq
