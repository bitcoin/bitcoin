// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_DEBUG_H
#define DASH_QUORUMS_DEBUG_H

#include "consensus/params.h"
#include "serialize.h"
#include "bls/bls.h"
#include "sync.h"
#include "univalue.h"
#include "net.h"

class CDataStream;
class CInv;
class CScheduler;

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
        };
        uint16_t statusBitset;
    };

    std::set<uint16_t> complaintsFromMembers;

public:
    CDKGDebugMemberStatus() : statusBitset(0) {}

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(statusBitset);
        READWRITE(complaintsFromMembers);
    }
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
        };
        uint16_t statusBitset;
    };

    std::vector<CDKGDebugMemberStatus> members;
    bool receivedFinalCommitment{false};

public:
    CDKGDebugSessionStatus() : statusBitset(0) {}

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(quorumHeight);
        READWRITE(phase);
        READWRITE(statusBitset);
        READWRITE(members);
        READWRITE(receivedFinalCommitment);
    }

    UniValue ToJson(int detailLevel) const;
};

class CDKGDebugStatus
{
public:
    uint256 proTxHash;
    int64_t nTime{0};
    uint32_t nHeight{0};

    std::map<uint8_t, CDKGDebugSessionStatus> sessions;

    CBLSSignature sig;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOpWithoutSig(Stream& s, Operation ser_action)
    {
        READWRITE(proTxHash);
        READWRITE(nTime);
        READWRITE(nHeight);
        READWRITE(sessions);
    }

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        SerializationOpWithoutSig(s, ser_action);
        READWRITE(sig);
    }

    uint256 GetSignHash() const
    {
        CHashWriter hw(SER_GETHASH, 0);
        NCONST_PTR(this)->SerializationOpWithoutSig(hw, CSerActionSerialize());
        hw << CBLSSignature();
        return hw.GetHash();
    }

    UniValue ToJson(int detailLevel) const;
};

class CDKGDebugManager
{
private:
    CCriticalSection cs;

    std::map<uint256, CDKGDebugStatus> statuses;
    std::map<uint256, uint256> statusesForMasternodes;

    CDKGDebugStatus localStatus;
    uint256 lastStatusHash;

public:
    CDKGDebugManager(CScheduler* scheduler);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);
    void ProcessDebugStatusMessage(NodeId nodeId, CDKGDebugStatus& status);

    bool AlreadyHave(const CInv& inv);
    bool GetDebugStatus(const uint256& hash, CDKGDebugStatus& ret);
    bool GetDebugStatusForMasternode(const uint256& proTxHash, CDKGDebugStatus& ret);
    void GetLocalDebugStatus(CDKGDebugStatus& ret);

    void ResetLocalSessionStatus(Consensus::LLMQType llmqType, const uint256& quorumHash, int quorumHeight);

    void UpdateLocalStatus(std::function<bool(CDKGDebugStatus& status)>&& func);
    void UpdateLocalSessionStatus(Consensus::LLMQType llmqType, std::function<bool(CDKGDebugSessionStatus& status)>&& func);
    void UpdateLocalMemberStatus(Consensus::LLMQType llmqType, size_t memberIdx, std::function<bool(CDKGDebugMemberStatus& status)>&& func);

    void SendLocalStatus();
};

extern CDKGDebugManager* quorumDKGDebugManager;

}

#endif //DASH_QUORUMS_DEBUG_H
