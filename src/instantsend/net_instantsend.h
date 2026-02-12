// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSTANTSEND_NET_INSTANTSEND_H
#define BITCOIN_INSTANTSEND_NET_INSTANTSEND_H

#include <net_processing.h>
#include <saltedhasher.h>

#include <util/threadinterrupt.h>

#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace Consensus {
struct LLMQParams;
} // namespace Consensus
namespace instantsend {
struct InstantSendLock;
struct PendingISLockEntry;
} // namespace instantsend
namespace llmq {
class CInstantSendManager;
class CQuorumManager;
} // namespace llmq

class NetInstantSend final : public NetHandler
{
public:
    NetInstantSend(PeerManagerInternal* peer_manager, llmq::CInstantSendManager& is_manager, llmq::CQuorumManager& qman,
                   CChainState& chainstate) :
        NetHandler(peer_manager),
        m_is_manager{is_manager},
        m_qman(qman),
        m_chainstate{chainstate}
    {
        workInterrupt.reset();
    }
    void ProcessMessage(CNode& pfrom, const std::string& msg_type, CDataStream& vRecv) override;

    void Start() override;
    void Stop() override;
    void Interrupt() override { workInterrupt(); };

    void WorkThreadMain();

private:
    struct BatchVerificationData;

    bool ValidateIncomingISLock(const instantsend::InstantSendLock& islock, NodeId node_id);
    std::optional<int> ResolveCycleHeight(const uint256& cycle_hash);
    bool ValidateDeterministicCycleHeight(int cycle_height, const Consensus::LLMQParams& llmq_params, NodeId node_id);

    std::unique_ptr<BatchVerificationData> BuildVerificationBatch(
        const Consensus::LLMQParams& llmq_params, int signOffset,
        const std::vector<instantsend::PendingISLockEntry>& pend);
    Uint256HashSet ApplyVerificationResults(
        const Consensus::LLMQParams& llmq_params, bool ban,
        BatchVerificationData& data,
        const std::vector<instantsend::PendingISLockEntry>& pend);

    void ProcessPendingISLocks(std::vector<instantsend::PendingISLockEntry>&& locks_to_process);

    Uint256HashSet ProcessPendingInstantSendLocks(
        const Consensus::LLMQParams& llmq_params, int signOffset, bool ban,
        const std::vector<instantsend::PendingISLockEntry>& pend);
    llmq::CInstantSendManager& m_is_manager;
    llmq::CQuorumManager& m_qman;
    const CChainState& m_chainstate;

    std::thread workThread;
    CThreadInterrupt workInterrupt;
};

#endif // BITCOIN_INSTANTSEND_NET_INSTANTSEND_H
