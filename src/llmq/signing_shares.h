// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNING_SHARES_H
#define BITCOIN_LLMQ_SIGNING_SHARES_H

#include <llmq/signing.h>

#include <bls/bls.h>
#include <random.h>
#include <saltedhasher.h>
#include <serialize.h>
#include <sync.h>
#include <util/threadinterrupt.h>
#include <uint256.h>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

class CNode;
class CConnman;
class CDeterministicMN;
class CSporkManager;
class PeerManager;

using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

namespace llmq
{
class CSigningManager;

// <signHash, quorumMember>
using SigShareKey = std::pair<uint256, uint16_t>;

constexpr uint32_t UNINITIALIZED_SESSION_ID{std::numeric_limits<uint32_t>::max()};

class CSigShare : virtual public CSigBase
{
protected:
    uint16_t quorumMember{std::numeric_limits<uint16_t>::max()};
public:
    CBLSLazySignature sigShare;

    SigShareKey key;

    [[nodiscard]] auto getQuorumMember() const {
        return quorumMember;
    }

    CSigShare(Consensus::LLMQType _llmqType, const uint256& _quorumHash, const uint256& _id, const uint256& _msgHash,
              uint16_t _quorumMember, const CBLSLazySignature& _sigShare) :
                    CSigBase(_llmqType, _quorumHash, _id, _msgHash),
                    quorumMember(_quorumMember),
                    sigShare(_sigShare) {};

    // This should only be used for serialization
    CSigShare() = default;


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

    SERIALIZE_METHODS(CSigShare, obj)
    {
        READWRITE(obj.llmqType, obj.quorumHash, obj.quorumMember, obj.id, obj.msgHash, obj.sigShare);
        SER_READ(obj, obj.UpdateKey());
    }
};

// Nodes will first announce a signing session with a sessionId to be used in all future P2P messages related to that
// session. We locally keep track of the mapping for each node. We also assign new sessionIds for outgoing sessions
// and send QSIGSESANN messages appropriately. All values except the max value for uint32_t are valid as sessionId
class CSigSesAnn : virtual public CSigBase
{
private:
    uint32_t sessionId{UNINITIALIZED_SESSION_ID};

public:
    CSigSesAnn(uint32_t _sessionId, Consensus::LLMQType _llmqType, const uint256& _quorumHash, const uint256& _id,
               const uint256& _msgHash) : CSigBase(_llmqType, _quorumHash, _id, _msgHash), sessionId(_sessionId) {};
    // ONLY FOR SERIALIZATION
    CSigSesAnn() = default;



    [[nodiscard]] auto getSessionId() const {
        return sessionId;
    }

    SERIALIZE_METHODS(CSigSesAnn, obj)
    {
        READWRITE(VARINT(obj.sessionId), obj.llmqType, obj.quorumHash, obj.id, obj.msgHash);
    }

    [[nodiscard]] std::string ToString() const;
};

class CSigSharesInv
{
public:
    uint32_t sessionId{UNINITIALIZED_SESSION_ID};
    std::vector<bool> inv;

public:
    SERIALIZE_METHODS(CSigSharesInv, obj)
    {
        uint64_t invSize = obj.inv.size();
        READWRITE(VARINT(obj.sessionId), COMPACTSIZE(invSize));
        autobitset_t bitset = std::make_pair(obj.inv, (size_t)invSize);
        READWRITE(AUTOBITSET(bitset));
        SER_READ(obj, obj.inv = bitset.first);
    }

    void Init(size_t size);
    void Set(uint16_t quorumMember, bool v);
    void SetAll(bool v);
    void Merge(const CSigSharesInv& inv2);

    [[nodiscard]] size_t CountSet() const;
    [[nodiscard]] std::string ToString() const;
};

// sent through the message QBSIGSHARES as a vector of multiple batches
class CBatchedSigShares
{
public:
    uint32_t sessionId{UNINITIALIZED_SESSION_ID};
    std::vector<std::pair<uint16_t, CBLSLazySignature>> sigShares;

public:
    SERIALIZE_METHODS(CBatchedSigShares, obj)
    {
        READWRITE(VARINT(obj.sessionId), obj.sigShares);
    }

    [[nodiscard]] std::string ToInvString() const;
};

template<typename T>
class SigShareMap
{
private:
    std::unordered_map<uint256, std::unordered_map<uint16_t, T>, StaticSaltedHasher> internalMap;

public:
    bool Add(const SigShareKey& k, const T& v)
    {
        auto& m = internalMap[k.first];
        return m.emplace(k.second, v).second;
    }

    void Erase(const SigShareKey& k)
    {
        auto it = internalMap.find(k.first);
        if (it == internalMap.end()) {
            return;
        }
        it->second.erase(k.second);
        if (it->second.empty()) {
            internalMap.erase(it);
        }
    }

    void Clear()
    {
        internalMap.clear();
    }

    [[nodiscard]] bool Has(const SigShareKey& k) const
    {
        auto it = internalMap.find(k.first);
        if (it == internalMap.end()) {
            return false;
        }
        return it->second.count(k.second) != 0;
    }

    T* Get(const SigShareKey& k)
    {
        auto it = internalMap.find(k.first);
        if (it == internalMap.end()) {
            return nullptr;
        }

        auto jt = it->second.find(k.second);
        if (jt == it->second.end()) {
            return nullptr;
        }

        return &jt->second;
    }

    T& GetOrAdd(const SigShareKey& k)
    {
        T* v = Get(k);
        if (!v) {
            Add(k, T());
            v = Get(k);
        }
        return *v;
    }

    const T* GetFirst() const
    {
        if (internalMap.empty()) {
            return nullptr;
        }
        return &internalMap.begin()->second.begin()->second;
    }

    [[nodiscard]] size_t Size() const
    {
        size_t s = 0;
        for (auto& p : internalMap) {
            s += p.second.size();
        }
        return s;
    }

    [[nodiscard]] size_t CountForSignHash(const uint256& signHash) const
    {
        auto it = internalMap.find(signHash);
        if (it == internalMap.end()) {
            return 0;
        }
        return it->second.size();
    }

    [[nodiscard]] bool Empty() const
    {
        return internalMap.empty();
    }

    const std::unordered_map<uint16_t, T>* GetAllForSignHash(const uint256& signHash) const
    {
        auto it = internalMap.find(signHash);
        if (it == internalMap.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void EraseAllForSignHash(const uint256& signHash)
    {
        internalMap.erase(signHash);
    }

    template<typename F>
    void EraseIf(F&& f)
    {
        for (auto it = internalMap.begin(); it != internalMap.end(); ) {
            SigShareKey k;
            k.first = it->first;
            for (auto jt = it->second.begin(); jt != it->second.end(); ) {
                k.second = jt->first;
                if (f(k, jt->second)) {
                    jt = it->second.erase(jt);
                } else {
                    ++jt;
                }
            }
            if (it->second.empty()) {
                it = internalMap.erase(it);
            } else {
                ++it;
            }
        }
    }

    template<typename F>
    void ForEach(F&& f)
    {
        for (auto& p : internalMap) {
            SigShareKey k;
            k.first = p.first;
            for (auto& p2 : p.second) {
                k.second = p2.first;
                f(k, p2.second);
            }
        }
    }
};

class CSigSharesNodeState
{
public:
    // Used to avoid holding locks too long
    struct SessionInfo
    {
        Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
        uint256 quorumHash;
        uint256 id;
        uint256 msgHash;
        uint256 signHash;

        CQuorumCPtr quorum;
    };

    struct Session {
        uint32_t recvSessionId{UNINITIALIZED_SESSION_ID};
        uint32_t sendSessionId{UNINITIALIZED_SESSION_ID};

        Consensus::LLMQType llmqType;
        uint256 quorumHash;
        uint256 id;
        uint256 msgHash;
        uint256 signHash;

        CQuorumCPtr quorum;

        CSigSharesInv announced;
        CSigSharesInv requested;
        CSigSharesInv knows;
    };
    // TODO limit number of sessions per node
    std::unordered_map<uint256, Session, StaticSaltedHasher> sessions;

    std::unordered_map<uint32_t, Session*> sessionByRecvId;

    SigShareMap<CSigShare> pendingIncomingSigShares;
    SigShareMap<int64_t> requestedSigShares;

    bool banned{false};

    Session& GetOrCreateSessionFromShare(const CSigShare& sigShare);
    Session& GetOrCreateSessionFromAnn(const CSigSesAnn& ann);
    Session* GetSessionBySignHash(const uint256& signHash);
    Session* GetSessionByRecvId(uint32_t sessionId);
    bool GetSessionInfoByRecvId(uint32_t sessionId, SessionInfo& retInfo);

    void RemoveSession(const uint256& signHash);
};

class CSignedSession
{
public:
    CSigShare sigShare;
    CQuorumCPtr quorum;

    int64_t nextAttemptTime{0};
    int attempt{0};
};

class CSigSharesManager : public CRecoveredSigsListener
{
private:
    static constexpr int64_t SESSION_NEW_SHARES_TIMEOUT{60};
    static constexpr int64_t SIG_SHARE_REQUEST_TIMEOUT{5};

    // we try to keep total message size below 10k
    static constexpr size_t MAX_MSGS_CNT_QSIGSESANN{100};
    static constexpr size_t MAX_MSGS_CNT_QGETSIGSHARES{200};
    static constexpr size_t MAX_MSGS_CNT_QSIGSHARESINV{200};
    // 400 is the maximum quorum size, so this is also the maximum number of sigs we need to support
    static constexpr size_t MAX_MSGS_TOTAL_BATCHED_SIGS{400};

    static constexpr int64_t EXP_SEND_FOR_RECOVERY_TIMEOUT{2000};
    static constexpr int64_t MAX_SEND_FOR_RECOVERY_TIMEOUT{10000};
    static constexpr size_t MAX_MSGS_SIG_SHARES{32};

    RecursiveMutex cs;

    std::thread workThread;
    CThreadInterrupt workInterrupt;

    SigShareMap<CSigShare> sigShares GUARDED_BY(cs);
    std::unordered_map<uint256, CSignedSession, StaticSaltedHasher> signedSessions GUARDED_BY(cs);

    // stores time of last receivedSigShare. Used to detect timeouts
    std::unordered_map<uint256, int64_t, StaticSaltedHasher> timeSeenForSessions GUARDED_BY(cs);

    std::unordered_map<NodeId, CSigSharesNodeState> nodeStates GUARDED_BY(cs);
    SigShareMap<std::pair<NodeId, int64_t>> sigSharesRequested GUARDED_BY(cs);
    SigShareMap<bool> sigSharesQueuedToAnnounce GUARDED_BY(cs);

    struct PendingSignatureData {
        const CQuorumCPtr quorum;
        const uint256 id;
        const uint256 msgHash;

        PendingSignatureData(CQuorumCPtr quorum, const uint256& id, const uint256& msgHash) : quorum(std::move(quorum)), id(id), msgHash(msgHash){}
    };

    Mutex cs_pendingSigns;
    std::vector<PendingSignatureData> pendingSigns GUARDED_BY(cs_pendingSigns);

    FastRandomContext rnd GUARDED_BY(cs);

    CSigningManager& sigman;
    const CActiveMasternodeManager* const m_mn_activeman;
    const CQuorumManager& qman;
    const CSporkManager& m_sporkman;

    int64_t lastCleanupTime{0};
    std::atomic<uint32_t> recoveredSigsCounter{0};

public:
    explicit CSigSharesManager(CSigningManager& _sigman, const CActiveMasternodeManager* const mn_activeman,
                               const CQuorumManager& _qman, const CSporkManager& sporkman) :
        sigman(_sigman),
        m_mn_activeman(mn_activeman),
        qman(_qman),
        m_sporkman(sporkman)
    {
        workInterrupt.reset();
    };
    CSigSharesManager() = delete;
    ~CSigSharesManager() override = default;

    void StartWorkerThread(CConnman& connman, PeerManager& peerman);
    void StopWorkerThread();
    void RegisterAsRecoveredSigsListener();
    void UnregisterAsRecoveredSigsListener();
    void InterruptWorkerThread();

    void ProcessMessage(const CNode& pnode, PeerManager& peerman, const CSporkManager& sporkman,
                        const std::string& msg_type, CDataStream& vRecv);

    void AsyncSign(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash);
    std::optional<CSigShare> CreateSigShare(const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash) const;
    void ForceReAnnouncement(const CQuorumCPtr& quorum, Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);

    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override;

    static CDeterministicMNCPtr SelectMemberForRecovery(const CQuorumCPtr& quorum, const uint256& id, int attempt);

private:
    // all of these return false when the currently processed message should be aborted (as each message actually contains multiple messages)
    bool ProcessMessageSigSesAnn(const CNode& pfrom, const CSigSesAnn& ann);
    bool ProcessMessageSigSharesInv(const CNode& pfrom, const CSigSharesInv& inv);
    bool ProcessMessageGetSigShares(const CNode& pfrom, const CSigSharesInv& inv);
    bool ProcessMessageBatchedSigShares(const CNode& pfrom, const CBatchedSigShares& batchedSigShares);
    void ProcessMessageSigShare(NodeId fromId, PeerManager& peerman, const CSigShare& sigShare);

    static bool VerifySigSharesInv(Consensus::LLMQType llmqType, const CSigSharesInv& inv);
    static bool PreVerifyBatchedSigShares(const CActiveMasternodeManager& mn_activeman, const CQuorumManager& quorum_manager,
                                          const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, bool& retBan);

    bool CollectPendingSigSharesToVerify(
        size_t maxUniqueSessions, std::unordered_map<NodeId, std::vector<CSigShare>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums);
    bool ProcessPendingSigShares(PeerManager& peerman, const CConnman& connman);

    void ProcessPendingSigShares(
        const std::vector<CSigShare>& sigSharesToProcess,
        const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums,
        PeerManager& peerman, const CConnman& connman);

    void ProcessSigShare(PeerManager& peerman, const CSigShare& sigShare, const CConnman& connman,
                         const CQuorumCPtr& quorum);
    void TryRecoverSig(PeerManager& peerman, const CQuorumCPtr& quorum, const uint256& id, const uint256& msgHash);

    bool GetSessionInfoByRecvId(NodeId nodeId, uint32_t sessionId, CSigSharesNodeState::SessionInfo& retInfo);
    static CSigShare RebuildSigShare(const CSigSharesNodeState::SessionInfo& session, const std::pair<uint16_t, CBLSLazySignature>& in);

    void Cleanup(const CConnman& connman);
    void RemoveSigSharesForSession(const uint256& signHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void RemoveBannedNodeStates(PeerManager& peerman);

    void BanNode(NodeId nodeId, PeerManager& peerman);

    bool SendMessages(CConnman& connman);
    void CollectSigSharesToRequest(std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToRequest) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToSend(std::unordered_map<NodeId, std::unordered_map<uint256, CBatchedSigShares, StaticSaltedHasher>>& sigSharesToSend) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToSendConcentrated(std::unordered_map<NodeId, std::vector<CSigShare>>& sigSharesToSend, const std::vector<CNode*>& vNodes) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToAnnounce(
        const CConnman& connman,
        std::unordered_map<NodeId, std::unordered_map<uint256, CSigSharesInv, StaticSaltedHasher>>& sigSharesToAnnounce)
        EXCLUSIVE_LOCKS_REQUIRED(cs);
    void SignPendingSigShares(const CConnman& connman, PeerManager& peerman);
    void WorkThreadMain(CConnman& connman, PeerManager& peerman);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNING_SHARES_H
