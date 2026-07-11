// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNING_SHARES_H
#define BITCOIN_LLMQ_SIGNING_SHARES_H

#include <bls/bls.h>
#include <ctpl_stl.h>
#include <evo/types.h>
#include <llmq/signhash.h>
#include <llmq/signing.h>
#include <msg_result.h>

#include <random.h>
#include <saltedhasher.h>
#include <serialize.h>
#include <sync.h>
#include <uint256.h>
#include <util/threadinterrupt.h>
#include <util/time.h>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

class CActiveMasternodeManager;
class CNode;
class CConnman;
class CDeterministicMN;
class CSporkManager;
class PeerManager;

namespace llmq
{
class CSigningManager;

// <signHash, quorumMember>
using SigShareKey = std::pair<uint256, uint16_t>;

constexpr uint32_t UNINITIALIZED_SESSION_ID{std::numeric_limits<uint32_t>::max()};

// 400 is the maximum quorum size, so this is also the maximum number of sigs we need to support
constexpr size_t MAX_MSGS_TOTAL_BATCHED_SIGS{400};

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
        if (invSize > MAX_MSGS_TOTAL_BATCHED_SIGS) {
            throw std::ios_base::failure("CSigSharesInv::inv size too large");
        }
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

/**
 * Two-level (signHash -> quorumMember) map with a running entry count, so Size() is O(1)
 * instead of a fold over all sign hash buckets. All structural mutations go through the
 * counted methods; Buckets() is for lookups and in-place value updates only.
 */
template<typename T>
class CountedBucketMap
{
public:
    using BucketMap = Uint256HashMap<std::unordered_map<uint16_t, T>>;

private:
    BucketMap m_data;
    size_t m_num_entries{0};

public:
    BucketMap& Buckets() { return m_data; }
    const BucketMap& Buckets() const { return m_data; }
    [[nodiscard]] size_t Size() const { return m_num_entries; }

    bool Emplace(const SigShareKey& k, const T& v)
    {
        if (!m_data[k.first].emplace(k.second, v).second) {
            return false;
        }
        ++m_num_entries;
        return true;
    }

    void Erase(const SigShareKey& k)
    {
        auto it = m_data.find(k.first);
        if (it == m_data.end()) {
            return;
        }
        m_num_entries -= it->second.erase(k.second);
        if (it->second.empty()) {
            m_data.erase(it);
        }
    }

    void EraseBucket(const uint256& signHash)
    {
        auto it = m_data.find(signHash);
        if (it == m_data.end()) {
            return;
        }
        m_num_entries -= it->second.size();
        m_data.erase(it);
    }

    template<typename F>
    void EraseIf(F&& f)
    {
        for (auto it = m_data.begin(); it != m_data.end(); ) {
            SigShareKey k;
            k.first = it->first;
            for (auto jt = it->second.begin(); jt != it->second.end(); ) {
                k.second = jt->first;
                if (f(k, jt->second)) {
                    jt = it->second.erase(jt);
                    --m_num_entries;
                } else {
                    ++jt;
                }
            }
            if (it->second.empty()) {
                it = m_data.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Clear()
    {
        m_data.clear();
        m_num_entries = 0;
    }
};

template<typename T>
class SigShareMap
{
private:
    CountedBucketMap<T> internalMap;

public:
    bool Add(const SigShareKey& k, const T& v)
    {
        return internalMap.Emplace(k, v);
    }

    void Erase(const SigShareKey& k)
    {
        internalMap.Erase(k);
    }

    void Clear()
    {
        internalMap.Clear();
    }

    [[nodiscard]] bool Has(const SigShareKey& k) const
    {
        auto& m = internalMap.Buckets();
        auto it = m.find(k.first);
        if (it == m.end()) {
            return false;
        }
        return it->second.count(k.second) != 0;
    }

    T* Get(const SigShareKey& k)
    {
        auto& m = internalMap.Buckets();
        auto it = m.find(k.first);
        if (it == m.end()) {
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
        auto& m = internalMap.Buckets();
        if (m.empty()) {
            return nullptr;
        }
        return &m.begin()->second.begin()->second;
    }

    [[nodiscard]] size_t Size() const
    {
        return internalMap.Size();
    }

    [[nodiscard]] size_t CountForSignHash(const uint256& signHash) const
    {
        auto& m = internalMap.Buckets();
        auto it = m.find(signHash);
        if (it == m.end()) {
            return 0;
        }
        return it->second.size();
    }

    [[nodiscard]] bool Empty() const
    {
        return internalMap.Buckets().empty();
    }

    const std::unordered_map<uint16_t, T>* GetAllForSignHash(const uint256& signHash) const
    {
        auto& m = internalMap.Buckets();
        auto it = m.find(signHash);
        if (it == m.end()) {
            return nullptr;
        }
        return &it->second;
    }

    void EraseAllForSignHash(const uint256& signHash)
    {
        internalMap.EraseBucket(signHash);
    }

    template<typename F>
    void EraseIf(F&& f)
    {
        internalMap.EraseIf(f);
    }

    template<typename F>
    void ForEach(F&& f)
    {
        for (auto& p : internalMap.Buckets()) {
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
        llmq::SignHash signHash;

        CQuorumCPtr quorum;
    };

    struct Session {
        uint32_t recvSessionId{UNINITIALIZED_SESSION_ID};
        uint32_t sendSessionId{UNINITIALIZED_SESSION_ID};

        Consensus::LLMQType llmqType;
        uint256 quorumHash;
        uint256 id;
        uint256 msgHash;
        llmq::SignHash signHash;

        CQuorumCPtr quorum;

        CSigSharesInv announced;
        CSigSharesInv requested;
        CSigSharesInv knows;

        bool receivedAnnouncement{false};
    };
    Uint256HashMap<Session> sessions;

    std::unordered_map<uint32_t, Session*> sessionByRecvId;

    SigShareMap<CSigShare> pendingIncomingSigShares;
    SigShareMap<int64_t> requestedSigShares;

    bool banned{false};

    Session& GetOrCreateSessionFromShare(const CSigShare& sigShare);
    Session& GetOrCreateSessionFromAnn(const CSigSesAnn& ann);
    [[nodiscard]] bool CanCreateSessionFromAnn(const CSigSesAnn& ann, size_t maxSessions) const;
    [[nodiscard]] size_t GetSessionCount() const;
    [[nodiscard]] size_t GetSessionCount(Consensus::LLMQType llmqType) const;
    [[nodiscard]] size_t GetAnnouncementSessionCount(Consensus::LLMQType llmqType) const;
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

class CSigSharesManager : public llmq::CRecoveredSigsListener
{
private:
    static constexpr int64_t SESSION_NEW_SHARES_TIMEOUT{60};
    static constexpr int64_t SIG_SHARE_REQUEST_TIMEOUT{5};

    // we try to keep total message size below 10k
    static constexpr size_t MAX_MSGS_CNT_QSIGSESANN{100};
    static constexpr size_t MAX_MSGS_CNT_QGETSIGSHARES{200};
    static constexpr size_t MAX_MSGS_CNT_QSIGSHARESINV{200};

    static constexpr int64_t EXP_SEND_FOR_RECOVERY_TIMEOUT{2000};
    static constexpr int64_t MAX_SEND_FOR_RECOVERY_TIMEOUT{10000};
    static constexpr size_t MAX_MSGS_SIG_SHARES{32};

    Mutex cs;

    mutable ctpl::thread_pool workerPool;
    std::thread housekeepingThread;
    std::thread dispatcherThread;
    CThreadInterrupt workInterrupt;

    SigShareMap<CSigShare> sigShares GUARDED_BY(cs);
    Uint256HashMap<CSignedSession> signedSessions GUARDED_BY(cs);

    // stores time of last receivedSigShare. Used to detect timeouts
    Uint256HashMap<int64_t> timeSeenForSessions GUARDED_BY(cs);

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

    CConnman& m_connman;
    CChainState& m_chainstate;
    CSigningManager& sigman;
    PeerManager& m_peerman;
    const CActiveMasternodeManager& m_mn_activeman;
    const CQuorumManager& qman;
    const CSporkManager& m_sporkman;

    CleanupThrottler<NodeClock> cleanupThrottler;
    std::atomic<uint32_t> recoveredSigsCounter{0};

public:
    CSigSharesManager() = delete;
    CSigSharesManager(const CSigSharesManager&) = delete;
    CSigSharesManager& operator=(const CSigSharesManager&) = delete;
    explicit CSigSharesManager(CConnman& connman, CChainState& chainstate, CSigningManager& _sigman,
                               PeerManager& peerman, const CActiveMasternodeManager& mn_activeman,
                               const CQuorumManager& _qman, const CSporkManager& sporkman);
    ~CSigSharesManager() override;

    void Start() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void Stop() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void RegisterRecoveryInterface() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void UnregisterRecoveryInterface() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void InterruptWorkerThread() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ProcessMessage(const CNode& pnode, const std::string& msg_type, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void AsyncSign(CQuorumCPtr quorum, const uint256& id, const uint256& msgHash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingSigns, !cs);
    std::optional<CSigShare> CreateSigShare(const CQuorum& quorum, const uint256& id, const uint256& msgHash) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void ForceReAnnouncement(const CQuorum& quorum, Consensus::LLMQType llmqType, const uint256& id,
                             const uint256& msgHash) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    static CDeterministicMNCPtr SelectMemberForRecovery(const CQuorum& quorum, const uint256& id, int attempt);

    bool AsyncSignIfMember(Consensus::LLMQType llmqType, CSigningManager& sigman, const uint256& id,
                           const uint256& msgHash, const uint256& quorumHash = uint256(), bool allowReSign = false,
                           bool allowDiffMsgHashSigning = false) EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingSigns, !cs);

    void NotifyRecoveredSig(const std::shared_ptr<const CRecoveredSig>& sig, bool proactive_relay) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    std::optional<CSigShare> CreateSigShareForSingleMember(const CQuorum& quorum, const uint256& id, const uint256& msgHash) const;

    // all of these return false when the currently processed message should be aborted (as each message actually contains multiple messages)
    bool ProcessMessageSigSesAnn(const CNode& pfrom, const CSigSesAnn& ann) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ProcessMessageSigSharesInv(const CNode& pfrom, const CSigSharesInv& inv) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ProcessMessageGetSigShares(const CNode& pfrom, const CSigSharesInv& inv) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ProcessMessageBatchedSigShares(const CNode& pfrom, const CBatchedSigShares& batchedSigShares)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void ProcessMessageSigShare(NodeId fromId, const CSigShare& sigShare) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    static bool VerifySigSharesInv(Consensus::LLMQType llmqType, const CSigSharesInv& inv);
    static bool PreVerifyBatchedSigShares(const CActiveMasternodeManager& mn_activeman, const CQuorumManager& quorum_manager,
                                          const CSigSharesNodeState::SessionInfo& session, const CBatchedSigShares& batchedSigShares, bool& retBan);

    // CollectPendingSigSharesToVerify returns true if there's more work to do.
    // The returned batch contains at most maxShares actual sig shares, drawn one
    // at a time in randomized round-robin order across peers so that no single
    // peer can dominate a batch. Bounding by shares (not by unique sessions)
    // caps the amount of BLS work that can be in-flight in the worker pool.
    bool CollectPendingSigSharesToVerify(
        size_t maxShares, std::unordered_map<NodeId, std::vector<CSigShare>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    bool ProcessPendingSigShares() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ProcessPendingSigShares(
        const std::vector<CSigShare>& sigSharesToProcess,
        const std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& quorums)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void ProcessSigShare(const CSigShare& sigShare, const CQuorumCPtr& quorum) EXCLUSIVE_LOCKS_REQUIRED(!cs);
    std::shared_ptr<CRecoveredSig> TryRecoverSig(const CQuorum& quorum, const uint256& id, const uint256& msgHash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool GetSessionInfoByRecvId(NodeId nodeId, uint32_t sessionId, CSigSharesNodeState::SessionInfo& retInfo)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    static CSigShare RebuildSigShare(const CSigSharesNodeState::SessionInfo& session, const std::pair<uint16_t, CBLSLazySignature>& in);
    bool TryAddPendingIncomingSigShare(NodeId nodeId, CSigSharesNodeState& nodeState, const CSigShare& sigShare)
        EXCLUSIVE_LOCKS_REQUIRED(cs);

    void Cleanup() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void RemoveSigSharesForSession(const uint256& signHash) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void RemoveBannedNodeStates() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void BanNode(NodeId nodeId) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    bool SendMessages() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void CollectSigSharesToRequest(std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>>& sigSharesToRequest)
        EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToSend(std::unordered_map<NodeId, Uint256HashMap<CBatchedSigShares>>& sigSharesToSend)
        EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToSendConcentrated(std::unordered_map<NodeId, std::vector<CSigShare>>& sigSharesToSend, const std::vector<CNode*>& vNodes) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void CollectSigSharesToAnnounce(std::unordered_map<NodeId, Uint256HashMap<CSigSharesInv>>& sigSharesToAnnounce)
        EXCLUSIVE_LOCKS_REQUIRED(cs);

    // Thread main functions
    void HousekeepingThreadMain() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void WorkDispatcherThreadMain() EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingSigns, !cs);

    // Dispatcher functions
    void DispatchPendingSigns() EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingSigns);
    void DispatchPendingProcessing() EXCLUSIVE_LOCKS_REQUIRED(!cs);

    // Worker pool task functions
    void ProcessPendingSigSharesLoop() EXCLUSIVE_LOCKS_REQUIRED(!cs);
    void SignAndProcessSingleShare(PendingSignatureData work) EXCLUSIVE_LOCKS_REQUIRED(!cs_pendingSigns, !cs);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNING_SHARES_H
