// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_QUORUMS_SIGNING_SHARES_H
#define DASH_QUORUMS_SIGNING_SHARES_H

#include "bls/bls.h"
#include "chainparams.h"
#include "net.h"
#include "random.h"
#include "serialize.h"
#include "sync.h"
#include "tinyformat.h"
#include "uint256.h"

#include "llmq/quorums.h"

#include <thread>
#include <mutex>

class CEvoDB;
class CScheduler;

namespace llmq
{

// <signHash, quorumMember>
typedef std::pair<uint256, uint16_t> SigShareKey;

// this one does not get transmitted over the wire as it is batched inside CBatchedSigShares
class CSigShare
{
public:
    uint8_t llmqType;
    uint256 quorumHash;
    uint16_t quorumMember;
    uint256 id;
    uint256 msgHash;
    CBLSLazySignature sigShare;

    SigShareKey key;

public:
    void UpdateKey();
    const SigShareKey& GetKey() const
    {
        return key;
    }
    const uint256& GetSignHash() const
    {
        assert(!key.first.IsNull());
        return key.first;
    }
};

class CSigSharesInv
{
public:
    uint8_t llmqType;
    uint256 signHash;
    std::vector<bool> inv;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);

        auto& consensus = Params().GetConsensus();
        auto it = consensus.llmqs.find((Consensus::LLMQType)llmqType);
        if (it == consensus.llmqs.end()) {
            throw std::ios_base::failure("invalid llmqType");
        }
        const auto& params = it->second;

        READWRITE(signHash);
        READWRITE(AUTOBITSET(inv, (size_t)params.size));
    }

    void Init(Consensus::LLMQType _llmqType, const uint256& _signHash);
    bool IsSet(uint16_t quorumMember) const;
    void Set(uint16_t quorumMember, bool v);
    void Merge(const CSigSharesInv& inv2);

    size_t CountSet() const;
    std::string ToString() const;
};

// sent through the message QBSIGSHARES as a vector of multiple batches
class CBatchedSigShares
{
public:
    uint8_t llmqType;
    uint256 quorumHash;
    uint256 id;
    uint256 msgHash;
    std::vector<std::pair<uint16_t, CBLSLazySignature>> sigShares;

public:
    ADD_SERIALIZE_METHODS;

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(id);
        READWRITE(msgHash);
        READWRITE(sigShares);
    }

    CSigShare RebuildSigShare(size_t idx) const
    {
        assert(idx < sigShares.size());
        auto& s = sigShares[idx];
        CSigShare sigShare;
        sigShare.llmqType = llmqType;
        sigShare.quorumHash = quorumHash;
        sigShare.quorumMember = s.first;
        sigShare.id = id;
        sigShare.msgHash = msgHash;
        sigShare.sigShare = s.second;
        sigShare.UpdateKey();
        return sigShare;
    }

    CSigSharesInv ToInv() const;
};

class CSigSharesNodeState
{
public:
    struct Session {
        CSigSharesInv announced;
        CSigSharesInv requested;
        CSigSharesInv knows;
    };
    // TODO limit number of sessions per node
    std::map<uint256, Session> sessions;

    std::map<SigShareKey, CSigShare> pendingIncomingSigShares;
    std::map<SigShareKey, int64_t> requestedSigShares;

    // elements are added whenever we receive a valid sig share from this node
    // this triggers us to send inventory items to him as he seems to be interested in these
    std::set<std::pair<Consensus::LLMQType, uint256>> interestedIn;

    bool banned{false};

    Session& GetOrCreateSession(Consensus::LLMQType llmqType, const uint256& signHash);

    void MarkAnnounced(const uint256& signHash, const CSigSharesInv& inv);
    void MarkRequested(const uint256& signHash, const CSigSharesInv& inv);
    void MarkKnows(const uint256& signHash, const CSigSharesInv& inv);

    void MarkAnnounced(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember);
    void MarkRequested(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember);
    void MarkKnows(Consensus::LLMQType llmqType, const uint256& signHash, uint16_t quorumMember);

    void RemoveSession(const uint256& signHash);
};

class CSigSharesManager
{
    static const int64_t SIGNING_SESSION_TIMEOUT = 60 * 1000;
    static const int64_t SIG_SHARE_REQUEST_TIMEOUT = 5 * 1000;

private:
    CCriticalSection cs;

    std::thread workThread;
    CThreadInterrupt workInterrupt;

    std::map<SigShareKey, CSigShare> sigShares;
    std::map<uint256, int64_t> firstSeenForSessions;

    std::map<NodeId, CSigSharesNodeState> nodeStates;
    std::map<SigShareKey, std::pair<NodeId, int64_t>> sigSharesRequested;
    std::set<SigShareKey> sigSharesToAnnounce;

    std::vector<std::tuple<const CQuorumCPtr, uint256, uint256>> pendingSigns;

    // must be protected by cs
    FastRandomContext rnd;

    int64_t lastCleanupTime{0};

public:
    CSigSharesManager();
    ~CSigSharesManager();

    void InterruptWorkerThread();

public:
    void ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    void AsyncSign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash);
    void Sign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash);

private:
    void StartWorkerThread();
    void StopWorkerThread();

    void ProcessMessageSigSharesInv(CNode* pfrom, const CSigSharesInv& inv, CConnman& connman);
    void ProcessMessageGetSigShares(CNode* pfrom, const CSigSharesInv& inv, CConnman& connman);
    void ProcessMessageBatchedSigShares(CNode* pfrom, const CBatchedSigShares& batchedSigShares, CConnman& connman);

    bool VerifySigSharesInv(NodeId from, const CSigSharesInv& inv);
    bool PreVerifyBatchedSigShares(NodeId nodeId, const CBatchedSigShares& batchedSigShares, bool& retBan);

    void CollectPendingSigSharesToVerify(size_t maxUniqueSessions, std::map<NodeId, std::vector<CSigShare>>& retSigShares, std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr>& retQuorums);
    bool ProcessPendingSigShares(CConnman& connman);

    void ProcessPendingSigSharesFromNode(NodeId nodeId, const std::vector<CSigShare>& sigShares, const std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr>& quorums, CConnman& connman);

    void ProcessSigShare(NodeId nodeId, const CSigShare& sigShare, CConnman& connman, const CQuorumCPtr& quorum);
    void TryRecoverSig(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash, CConnman& connman);

private:
    void Cleanup();
    void RemoveSigSharesForSession(const uint256& signHash);
    void RemoveBannedNodeStates();

    void BanNode(NodeId nodeId);

    bool SendMessages();
    void CollectSigSharesToRequest(std::map<NodeId, std::map<uint256, CSigSharesInv>>& sigSharesToRequest);
    void CollectSigSharesToSend(std::map<NodeId, std::map<uint256, CBatchedSigShares>>& sigSharesToSend);
    void CollectSigSharesToAnnounce(std::map<NodeId, std::map<uint256, CSigSharesInv>>& sigSharesToAnnounce);
    bool SignPendingSigShares();
    void WorkThreadMain();
};

extern CSigSharesManager* quorumSigSharesManager;

}

#endif //DASH_QUORUMS_SIGNING_SHARES_H
