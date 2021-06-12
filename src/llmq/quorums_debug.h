// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_DEBUG_H
#define SYSCOIN_LLMQ_QUORUMS_DEBUG_H

#include <consensus/params.h>
#include <sync.h>
#include <univalue.h>

#include <functional>
#include <set>

class CDataStream;
class CInv;
class CScheduler;
class ChainstateManager;
namespace llmq
{

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
    uint8_t llmqType{Consensus::LLMQ_NONE};
    uint256 quorumHash;
    uint32_t quorumHeight{0};
    uint8_t phase{0};
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


    UniValue ToJson(ChainstateManager &chainman,int detailLevel) const;
};

class CDKGDebugStatus
{
public:
    int64_t nTime{0};

    std::map<uint8_t, CDKGDebugSessionStatus> sessions;

public:
    UniValue ToJson(ChainstateManager &chainman, int detailLevel) const;
};

class CDKGDebugManager
{
private:
    mutable RecursiveMutex cs;
    CDKGDebugStatus localStatus GUARDED_BY(cs);
public:
    CDKGDebugManager();

    void GetLocalDebugStatus(CDKGDebugStatus& ret);

    void ResetLocalSessionStatus(uint8_t llmqType);
    void InitLocalSessionStatus(uint8_t llmqType, const uint256& quorumHash, uint32_t quorumHeight);

    void UpdateLocalSessionStatus(uint8_t llmqType, std::function<bool(CDKGDebugSessionStatus& status)>&& func);
    void UpdateLocalMemberStatus(uint8_t llmqType, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func);
};

extern CDKGDebugManager* quorumDKGDebugManager;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_DEBUG_H
