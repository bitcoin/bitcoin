// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ACTIVE_QUORUMS_H
#define BITCOIN_ACTIVE_QUORUMS_H

#include <llmq/observer/quorums.h>
#include <llmq/types.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <span.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>

#include <map>

class CActiveMasternodeManager;
class CBlockIndex;
class CBLSWorker;
class CConnman;
class CDeterministicMNManager;
class CDKGSessionManager;
class CNode;
class CSporkManager;
struct MessageProcessingResult;
namespace llmq {
class CQuorum;
class CQuorumDataRequest;
class CQuorumSnapshotManager;
enum class QvvecSyncMode : int8_t;
} // namespace llmq

namespace llmq {
class QuorumParticipant final : public QuorumObserver
{
private:
    CBLSWorker& m_bls_worker;
    const CActiveMasternodeManager& m_mn_activeman;
    const bool m_quorums_watch{false};

public:
    QuorumParticipant() = delete;
    QuorumParticipant(const QuorumParticipant&) = delete;
    QuorumParticipant& operator=(const QuorumParticipant&) = delete;
    explicit QuorumParticipant(CBLSWorker& bls_worker, CConnman& connman, CDeterministicMNManager& dmnman,
                               QuorumObserverParent& qman, CQuorumSnapshotManager& qsnapman,
                               const CActiveMasternodeManager& mn_activeman, const ChainstateManager& chainman,
                               const CMasternodeSync& mn_sync, const CSporkManager& sporkman,
                               const llmq::QvvecSyncModeMap& sync_map, bool quorums_recovery, bool quorums_watch);
    ~QuorumParticipant();

public:
    // QuorumObserver
    bool IsMasternode() const override;
    bool IsWatching() const override;
    bool SetQuorumSecretKeyShare(CQuorum& quorum, Span<CBLSSecretKey> skContributions) const override;
    [[nodiscard]] MessageProcessingResult ProcessContribQGETDATA(bool request_limit_exceeded, CDataStream& vStream,
                                                                 const CQuorum& quorum, CQuorumDataRequest& request,
                                                                 gsl::not_null<const CBlockIndex*> block_index) override;
    [[nodiscard]] MessageProcessingResult ProcessContribQDATA(CNode& pfrom, CDataStream& vStream, CQuorum& quorum,
                                                              CQuorumDataRequest& request) override;

protected:
    // QuorumObserver
    void CheckQuorumConnections(const Consensus::LLMQParams& llmqParams,
                                gsl::not_null<const CBlockIndex*> pindexNew) const override;
    void TriggerQuorumDataRecoveryThreads(gsl::not_null<const CBlockIndex*> block_index) const override;

private:
    /// Returns the start offset for the masternode with the given proTxHash. This offset is applied when picking data
    /// recovery members of a quorum's memberlist and is calculated based on a list of all member of all active quorums
    /// for the given llmqType in a way that each member should receive the same number of request if all active
    /// llmqType members requests data from one llmqType quorum.
    size_t GetQuorumRecoveryStartOffset(const CQuorum& quorum, gsl::not_null<const CBlockIndex*> pIndex) const;

    void StartDataRecoveryThread(gsl::not_null<const CBlockIndex*> pIndex, CQuorumCPtr pQuorum, uint16_t nDataMaskIn) const;
};
} // namespace llmq

#endif // BITCOIN_ACTIVE_QUORUMS_H
