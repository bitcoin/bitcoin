// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DEBUG_H
#define BITCOIN_LLMQ_DEBUG_H

#include <consensus/params.h>
#include <sync.h>
#include <univalue.h>

#include <functional>
#include <set>

class CDataStream;
class CDeterministicMNManager;
class ChainstateManager;
class CInv;
class CScheduler;

namespace llmq
{
class CQuorumSnapshotManager;

enum class QuorumPhase;

class CDKGDebugMemberStatus
{
public:
    union {
        struct
        {
            // is it locally considered as bad (and thus removed from the validMembers set)
            bool bad : 1;
            // did we complain about this member
            bool weComplain : 1;

            // received message for DKG phases
            bool receivedContribution : 1;
            bool receivedComplaint : 1;
            bool receivedJustification : 1;
            bool receivedPrematureCommitment : 1;
        } statusBits;
        uint8_t statusBitset;
    };

    std::set<uint16_t> complaintsFromMembers;

public:
    CDKGDebugMemberStatus() : statusBitset(0) {}
};

class CDKGDebugSessionStatus
{
public:
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint32_t quorumHeight{0};
    QuorumPhase phase{0};

    union {
        struct
        {
            // sent messages for DKG phases
            bool sentContributions : 1;
            bool sentComplaint : 1;
            bool sentJustification : 1;
            bool sentPrematureCommitment : 1;

            bool aborted : 1;
        } statusBits;
        uint8_t statusBitset;
    };

    std::vector<CDKGDebugMemberStatus> members;

public:
    CDKGDebugSessionStatus() : statusBitset(0) {}

    UniValue ToJson(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                    const ChainstateManager& chainman, int quorumIndex, int detailLevel) const;
};

class CDKGDebugStatus
{
public:
    int64_t nTime{0};

    std::map<std::pair<Consensus::LLMQType, int>, CDKGDebugSessionStatus> sessions;
    //std::map<Consensus::LLMQType, CDKGDebugSessionStatus> sessions;

public:
    UniValue ToJson(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                    const ChainstateManager& chainman, int detailLevel) const;
};

class CDKGDebugManager
{
private:
    mutable Mutex cs_lockStatus;
    CDKGDebugStatus localStatus GUARDED_BY(cs_lockStatus);

public:
    CDKGDebugManager();

    void GetLocalDebugStatus(CDKGDebugStatus& ret) const EXCLUSIVE_LOCKS_REQUIRED(!cs_lockStatus);

    void ResetLocalSessionStatus(Consensus::LLMQType llmqType, int quorumIndex) EXCLUSIVE_LOCKS_REQUIRED(!cs_lockStatus);
    void InitLocalSessionStatus(const Consensus::LLMQParams& llmqParams, int quorumIndex, const uint256& quorumHash,
                                int quorumHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_lockStatus);

    void UpdateLocalSessionStatus(Consensus::LLMQType llmqType, int quorumIndex,
                                  std::function<bool(CDKGDebugSessionStatus& status)>&& func)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_lockStatus);
    void UpdateLocalMemberStatus(Consensus::LLMQType llmqType, int quorumIndex, size_t memberIdx,
                                 std::function<bool(CDKGDebugMemberStatus& status)>&& func)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_lockStatus);
};

} // namespace llmq

#endif // BITCOIN_LLMQ_DEBUG_H
