// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/observer/context.h>

#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/observer/quorums.h>
#include <llmq/quorumsman.h>

namespace llmq {
ObserverContext::ObserverContext(CBLSWorker& bls_worker, CConnman& connman, CDeterministicMNManager& dmnman,
                                 CMasternodeMetaMan& mn_metaman, CMasternodeSync& mn_sync,
                                 llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumManager& qman,
                                 llmq::CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                                 const CSporkManager& sporkman, const llmq::QvvecSyncModeMap& sync_map,
                                 const util::DbWrapperParams& db_params, bool quorums_recovery) :
    m_qman{qman},
    dkgdbgman{std::make_unique<llmq::CDKGDebugManager>()},
    qdkgsman{std::make_unique<llmq::CDKGSessionManager>(bls_worker, dmnman, *dkgdbgman, mn_metaman, qblockman, qsnapman,
                                                        /*mn_activeman=*/nullptr, chainman, sporkman, db_params,
                                                        /*quorums_watch=*/true)},
    qman_handler{std::make_unique<llmq::QuorumObserver>(connman, dmnman, qman, qsnapman, chainman, mn_sync, sporkman,
                                                        sync_map, quorums_recovery)}
{
    m_qman.ConnectManagers(qman_handler.get(), qdkgsman.get());
}

ObserverContext::~ObserverContext()
{
    m_qman.DisconnectManagers();
}

void ObserverContext::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (fInitialDownload || pindexNew == pindexFork) // In IBD or blocks were disconnected without any new ones
        return;

    qdkgsman->UpdatedBlockTip(pindexNew, fInitialDownload);
    qman_handler->UpdatedBlockTip(pindexNew, fInitialDownload);
}
} // namespace llmq
