// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNING_H
#define BITCOIN_LLMQ_SIGNING_H

#include <bls/bls.h>
#include <gsl/pointers.h>
#include <llmq/params.h>
#include <net_types.h>
#include <protocol.h>
#include <random.h>
#include <saltedhasher.h>
#include <sync.h>
#include <util/threadinterrupt.h>
#include <unordered_lru_cache.h>

#include <unordered_map>

class CActiveMasternodeManager;
class CChainState;
class CDataStream;
class CDBBatch;
class CDBWrapper;
class CInv;
class CNode;
class PeerManager;
class UniValue;

namespace llmq
{
class CQuorum;
using CQuorumCPtr = std::shared_ptr<const CQuorum>;
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

    [[nodiscard]] uint256 buildSignHash() const;
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

    UniValue ToJson() const;
};

class CRecoveredSigsDb
{
private:
    std::unique_ptr<CDBWrapper> db{nullptr};

    mutable Mutex cs_cache;
    mutable unordered_lru_cache<std::pair<Consensus::LLMQType, uint256>, bool, StaticSaltedHasher, 30000> hasSigForIdCache GUARDED_BY(cs_cache);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForSessionCache GUARDED_BY(cs_cache);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForHashCache GUARDED_BY(cs_cache);

public:
    explicit CRecoveredSigsDb(bool fMemory, bool fWipe);
    ~CRecoveredSigsDb();

    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const;
    bool HasRecoveredSigForSession(const uint256& signHash) const;
    bool HasRecoveredSigForHash(const uint256& hash) const;
    bool GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret) const;
    bool GetRecoveredSigById(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const;
    void WriteRecoveredSig(const CRecoveredSig& recSig);
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

    void CleanupOldRecoveredSigs(int64_t maxAge);

    // votes are removed when the recovered sig is written to the db
    bool HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id) const;
    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const;
    void WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);

    void CleanupOldVotes(int64_t maxAge);

private:
    bool ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const;
    void RemoveRecoveredSig(CDBBatch& batch, Consensus::LLMQType llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey);
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
    std::unordered_map<uint256, std::shared_ptr<const CRecoveredSig>, StaticSaltedHasher> pendingReconstructedRecoveredSigs GUARDED_BY(cs_pending);

    FastRandomContext rnd GUARDED_BY(cs_pending);

    int64_t lastCleanupTime{0};

    mutable Mutex cs_listeners;
    std::vector<CRecoveredSigsListener*> recoveredSigsListeners GUARDED_BY(cs_listeners);

public:
    CSigningManager(const CActiveMasternodeManager* const mn_activeman, const CChainState& chainstate,
                    const CQuorumManager& _qman, bool fMemory, bool fWipe);

    bool AlreadyHave(const CInv& inv) const;
    bool GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const;

    PeerMsgRet ProcessMessage(const CNode& pnode, PeerManager& peerman, const std::string& msg_type, CDataStream& vRecv);

    // This is called when a recovered signature was was reconstructed from another P2P message and is known to be valid
    // This is the case for example when a signature appears as part of InstantSend or ChainLocks
    void PushReconstructedRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig);

    // This is called when a recovered signature can be safely removed from the DB. This is only safe when some other
    // mechanism prevents possible conflicts. As an example, ChainLocks prevent conflicts in confirmed TXs InstantSend votes
    // This won't completely remove all traces of the recovered sig but instead leave the hash entry in the DB. This
    // allows AlreadyHave to keep returning true. Cleanup will later remove the remains
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

private:
    PeerMsgRet ProcessMessageRecoveredSig(const CNode& pfrom, PeerManager& peerman,
                                          const std::shared_ptr<const CRecoveredSig>& recoveredSig);

    void CollectPendingRecoveredSigsToVerify(size_t maxUniqueSessions,
            std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
            std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums);
    void ProcessPendingReconstructedRecoveredSigs(PeerManager& peerman);
    bool ProcessPendingRecoveredSigs(PeerManager& peerman); // called from the worker thread of CSigSharesManager
public:
    // TODO - should not be public!
    void ProcessRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig, PeerManager& peerman);

private:
    void Cleanup(); // called from the worker thread of CSigSharesManager

public:
    // public interface
    void RegisterRecoveredSigsListener(CRecoveredSigsListener* l);
    void UnregisterRecoveredSigsListener(CRecoveredSigsListener* l);

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
    void WorkThreadMain(PeerManager& peerman);

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

uint256 BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash);

bool IsQuorumActive(Consensus::LLMQType llmqType, const CQuorumManager& qman, const uint256& quorumHash);

} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNING_H
