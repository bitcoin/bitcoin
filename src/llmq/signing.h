// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNING_H
#define BITCOIN_LLMQ_SIGNING_H

#include <bls/bls.h>
#include <llmq/params.h>
#include <llmq/signhash.h>
#include <llmq/types.h>
#include <unordered_lru_cache.h>

#include <net_types.h>
#include <protocol.h>
#include <random.h>
#include <saltedhasher.h>
#include <sync.h>
#include <util/threadinterrupt.h>

#include <gsl/pointers.h>

#include <string_view>
#include <unordered_map>

class CActiveMasternodeManager;
class CChainState;
class CDataStream;
class CDBBatch;
class CDBWrapper;
class CInv;
class PeerManager;
struct RPCResult;

class UniValue;

namespace llmq {
class CQuorumManager;
class CSigSharesManager;

// Keep recovered signatures for a week. This is a "-maxrecsigsage" option default.
static constexpr int64_t DEFAULT_MAX_RECOVERED_SIGS_AGE{60 * 60 * 24 * 7};

class CSigBase
{
protected:
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint256 id;
    uint256 msgHash;

    CSigBase(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash)
            : llmqType(llmqType), quorumHash(quorumHash), id(id), msgHash(msgHash) {};
    CSigBase() = default;

public:
    [[nodiscard]] constexpr auto getLlmqType() const {
        return llmqType;
    }

    [[nodiscard]] constexpr auto getQuorumHash() const -> const uint256& {
        return quorumHash;
    }

    [[nodiscard]] constexpr auto getId() const -> const uint256& {
        return id;
    }

    [[nodiscard]] constexpr auto getMsgHash() const -> const uint256& {
        return msgHash;
    }

    [[nodiscard]] SignHash buildSignHash() const;
};

class CRecoveredSig : virtual public CSigBase
{
public:
    const CBLSLazySignature sig;

    CRecoveredSig() = default;

    CRecoveredSig(Consensus::LLMQType _llmqType, const uint256& _quorumHash, const uint256& _id, const uint256& _msgHash, const CBLSLazySignature& _sig) :
                  CSigBase(_llmqType, _quorumHash, _id, _msgHash), sig(_sig) {UpdateHash();};
    CRecoveredSig(Consensus::LLMQType _llmqType, const uint256& _quorumHash, const uint256& _id, const uint256& _msgHash, const CBLSSignature& _sig) :
                  CSigBase(_llmqType, _quorumHash, _id, _msgHash) {const_cast<CBLSLazySignature&>(sig).Set(_sig, bls::bls_legacy_scheme.load()); UpdateHash();};

private:
    // only in-memory
    uint256 hash;

    void UpdateHash()
    {
        hash = ::SerializeHash(*this);
    }

public:
    SERIALIZE_METHODS(CRecoveredSig, obj)
    {
        READWRITE(const_cast<Consensus::LLMQType&>(obj.llmqType), const_cast<uint256&>(obj.quorumHash), const_cast<uint256&>(obj.id),
                  const_cast<uint256&>(obj.msgHash), const_cast<CBLSLazySignature&>(obj.sig));
        SER_READ(obj, obj.UpdateHash());
    }

    const uint256& GetHash() const
    {
        assert(!hash.IsNull());
        return hash;
    }

    [[nodiscard]] static RPCResult GetJsonHelp(const std::string& key, bool optional);
    [[nodiscard]] UniValue ToJson() const;
};

class CRecoveredSigsDb
{
private:
    std::unique_ptr<CDBWrapper> db{nullptr};

    mutable Mutex cs_cache;
    mutable unordered_lru_cache<std::pair<Consensus::LLMQType, uint256>, bool, StaticSaltedHasher, 30000> hasSigForIdCache GUARDED_BY(cs_cache);
    mutable Uint256LruHashMap<bool, 30000> hasSigForSessionCache GUARDED_BY(cs_cache);
    mutable Uint256LruHashMap<bool, 30000> hasSigForHashCache GUARDED_BY(cs_cache);

public:
    explicit CRecoveredSigsDb(bool fMemory, bool fWipe);
    ~CRecoveredSigsDb();

    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool HasRecoveredSigForSession(const uint256& signHash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool HasRecoveredSigForHash(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    bool GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret) const;
    bool GetRecoveredSigById(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const;
    void WriteRecoveredSig(const CRecoveredSig& recSig) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    void CleanupOldRecoveredSigs(int64_t maxAge) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);

    // votes are removed when the recovered sig is written to the db
    bool HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id) const;
    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const;
    void WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);

    void CleanupOldVotes(int64_t maxAge);

private:
    bool ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const;
    void RemoveRecoveredSig(CDBBatch& batch, Consensus::LLMQType llmqType, const uint256& id, bool deleteHashKey,
                            bool deleteTimeKey) EXCLUSIVE_LOCKS_REQUIRED(!cs_cache);
};

class CRecoveredSigsListener
{
public:
    virtual ~CRecoveredSigsListener() = default;

    [[nodiscard]] virtual MessageProcessingResult HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) = 0;
};

class CSigningManager
{
private:

    CRecoveredSigsDb db;
    const CActiveMasternodeManager* const m_mn_activeman;
    const CChainState& m_chainstate;
    const CQuorumManager& qman;

    mutable Mutex cs_pending;
    // Incoming and not verified yet
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> pendingRecoveredSigs GUARDED_BY(cs_pending);
    Uint256HashMap<std::shared_ptr<const CRecoveredSig>> pendingReconstructedRecoveredSigs GUARDED_BY(cs_pending);

    FastRandomContext rnd GUARDED_BY(cs_pending);

    int64_t lastCleanupTime{0};

    mutable Mutex cs_listeners;
    std::vector<CRecoveredSigsListener*> recoveredSigsListeners GUARDED_BY(cs_listeners);

public:
    CSigningManager(const CActiveMasternodeManager* const mn_activeman, const CChainState& chainstate,
                    const CQuorumManager& _qman, bool fMemory, bool fWipe);

    bool AlreadyHave(const CInv& inv) const EXCLUSIVE_LOCKS_REQUIRED(!cs_pending);
    bool GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const;

    [[nodiscard]] MessageProcessingResult ProcessMessage(NodeId from, std::string_view msg_type, CDataStream& vRecv)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending);

    // This is called when a recovered signature was was reconstructed from another P2P message and is known to be valid
    // This is the case for example when a signature appears as part of InstantSend or ChainLocks
    void PushReconstructedRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending);

    // This is called when a recovered signature can be safely removed from the DB. This is only safe when some other
    // mechanism prevents possible conflicts. As an example, ChainLocks prevent conflicts in confirmed TXs InstantSend votes
    // This won't completely remove all traces of the recovered sig but instead leave the hash entry in the DB. This
    // allows AlreadyHave to keep returning true. Cleanup will later remove the remains
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

private:
    void CollectPendingRecoveredSigsToVerify(
        size_t maxUniqueSessions, std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
        std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending);
    void ProcessPendingReconstructedRecoveredSigs(PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending, !cs_listeners);
    bool ProcessPendingRecoveredSigs(PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending, !cs_listeners); // called from the worker thread of CSigSharesManager
public:
    // TODO - should not be public!
    void ProcessRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig, PeerManager& peerman)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_pending, !cs_listeners);

private:
    void Cleanup(); // called from the worker thread of CSigSharesManager

public:
    // public interface
    void RegisterRecoveredSigsListener(CRecoveredSigsListener* l) EXCLUSIVE_LOCKS_REQUIRED(!cs_listeners);
    void UnregisterRecoveredSigsListener(CRecoveredSigsListener* l) EXCLUSIVE_LOCKS_REQUIRED(!cs_listeners);

    bool AsyncSignIfMember(Consensus::LLMQType llmqType, CSigSharesManager& shareman, const uint256& id,
                           const uint256& msgHash, const uint256& quorumHash = uint256(), bool allowReSign = false,
                           bool allowDiffMsgHashSigning = false);
    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const;
    bool HasRecoveredSigForSession(const uint256& signHash) const;
    bool GetRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& retRecSig) const;
    bool IsConflicting(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;

    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const;

private:
    std::thread workThread;
    CThreadInterrupt workInterrupt;
    void WorkThreadMain(PeerManager& peerman) EXCLUSIVE_LOCKS_REQUIRED(!cs_pending, !cs_listeners);
    ;

public:
    void StartWorkerThread(PeerManager& peerman);
    void StopWorkerThread();
    void InterruptWorkerThread();
};

template<typename NodesContainer, typename Continue, typename Callback>
void IterateNodesRandom(NodesContainer& nodeStates, Continue&& cont, Callback&& callback, FastRandomContext& rnd)
{
    std::vector<typename NodesContainer::iterator> rndNodes;
    rndNodes.reserve(nodeStates.size());
    for (auto it = nodeStates.begin(); it != nodeStates.end(); ++it) {
        rndNodes.emplace_back(it);
    }
    if (rndNodes.empty()) {
        return;
    }
    Shuffle(rndNodes.begin(), rndNodes.end(), rnd);

    size_t idx = 0;
    while (!rndNodes.empty() && cont()) {
        auto nodeId = rndNodes[idx]->first;
        auto& ns = rndNodes[idx]->second;

        if (callback(nodeId, ns)) {
            idx = (idx + 1) % rndNodes.size();
        } else {
            rndNodes.erase(rndNodes.begin() + idx);
            if (rndNodes.empty()) {
                break;
            }
            idx %= rndNodes.size();
        }
    }
}

bool IsQuorumActive(Consensus::LLMQType llmqType, const CQuorumManager& qman, const uint256& quorumHash);

} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNING_H
