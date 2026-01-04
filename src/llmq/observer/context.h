// Copyright (c) 2025-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_OBSERVER_CONTEXT_H
#define BITCOIN_LLMQ_OBSERVER_CONTEXT_H

#include <llmq/options.h>

#include <validationinterface.h>

#include <memory>

class CBLSWorker;
class CBlockIndex;
class CConnman;
class CDeterministicMNManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CSporkManager;
namespace llmq {
class CDKGDebugManager;
class CDKGSessionManager;
class CQuorumBlockProcessor;
class CQuorumManager;
class CQuorumSnapshotManager;
class QuorumObserver;
} // namespace llmq
namespace util {
struct DbWrapperParams;
} // namespace util

namespace llmq {
struct ObserverContext final : public CValidationInterface {
private:
    llmq::CQuorumManager& m_qman;

public:
    ObserverContext() = delete;
    ObserverContext(const ObserverContext&) = delete;
    ObserverContext& operator=(const ObserverContext&) = delete;
    ObserverContext(CBLSWorker& bls_worker, CConnman& connman, CDeterministicMNManager& dmnman,
                    CMasternodeMetaMan& mn_metaman, CMasternodeSync& mn_sync, llmq::CQuorumBlockProcessor& qblockman,
                    llmq::CQuorumManager& qman, llmq::CQuorumSnapshotManager& qsnapman, const ChainstateManager& chainman,
                    const CSporkManager& sporkman, const llmq::QvvecSyncModeMap& sync_map,
                    const util::DbWrapperParams& db_params, bool quorums_recovery);
    ~ObserverContext();

    void Start();
    void Stop();

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

public:
    const std::unique_ptr<llmq::CDKGDebugManager> dkgdbgman;
    const std::unique_ptr<llmq::CDKGSessionManager> qdkgsman;

private:
    const std::unique_ptr<llmq::QuorumObserver> qman_handler;
};
} // namespace llmq

#endif // BITCOIN_LLMQ_OBSERVER_CONTEXT_H
