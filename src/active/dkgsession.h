// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ACTIVE_DKGSESSION_H
#define BITCOIN_ACTIVE_DKGSESSION_H

#include <llmq/dkgsession.h>

namespace llmq {
class ActiveDKGSession final : public llmq::CDKGSession
{
private:
    CMasternodeMetaMan& m_mn_metaman;
    const CActiveMasternodeManager& m_mn_activeman;
    const CSporkManager& m_sporkman;
    const bool m_use_legacy_bls{false};

public:
    ActiveDKGSession() = delete;
    ActiveDKGSession(const ActiveDKGSession&) = delete;
    ActiveDKGSession& operator=(const ActiveDKGSession&) = delete;
    ActiveDKGSession(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CDKGDebugManager& dkgdbgman,
                     CDKGSessionManager& qdkgsman, CMasternodeMetaMan& mn_metaman, CQuorumSnapshotManager& qsnapman,
                     const CActiveMasternodeManager& mn_activeman, const ChainstateManager& chainman,
                     const CSporkManager& sporkman, const CBlockIndex* base_block_index,
                     const Consensus::LLMQParams& params);
    ~ActiveDKGSession();

public:
    // Phase 1: contribution
    void Contribute(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override;
    void SendContributions(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override;
    void VerifyPendingContributions() override EXCLUSIVE_LOCKS_REQUIRED(cs_pending);

    // Phase 2: complaint
    void VerifyAndComplain(CConnman& connman, CDKGPendingMessages& pendingMessages, PeerManager& peerman) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending);
    void VerifyConnectionAndMinProtoVersions(CConnman& connman) const override;
    void SendComplaint(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override;

    // Phase 3: justification
    void VerifyAndJustify(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override
        EXCLUSIVE_LOCKS_REQUIRED(!invCs);
    void SendJustification(CDKGPendingMessages& pendingMessages, PeerManager& peerman,
                           const std::set<uint256>& forMembers) override;

    // Phase 4: commit
    void VerifyAndCommit(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override;
    void SendCommitment(CDKGPendingMessages& pendingMessages, PeerManager& peerman) override;

    // Phase 5: aggregate/finalize
    std::vector<CFinalCommitment> FinalizeCommitments() EXCLUSIVE_LOCKS_REQUIRED(!invCs) override;

    // All Phases 5-in-1 for single-node-quorum
    CFinalCommitment FinalizeSingleCommitment() override;

private:
    //! CDKGSession
    bool MaybeDecrypt(const CBLSIESMultiRecipientObjects<CBLSSecretKey>& obj, size_t idx, CBLSSecretKey& ret_obj,
                      int version) override;
};
} // namespace llmq

#endif // BITCOIN_ACTIVE_DKGSESSION_H
