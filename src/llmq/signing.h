// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_SIGNING_H
#define BITCOIN_LLMQ_SIGNING_H

#include <bls/bls.h>
#include <unordered_lru_cache.h>

#include <consensus/params.h>
#include <random.h>
#include <saltedhasher.h>
#include <sync.h>
#include <univalue.h>

#include <unordered_map>

class CConnman;
class CDataStream;
class CDBBatch;
class CDBWrapper;
class CInv;
class CNode;
class PeerManager;

using NodeId = int64_t;

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

    mutable RecursiveMutex cs;
    mutable unordered_lru_cache<std::pair<Consensus::LLMQType, uint256>, bool, StaticSaltedHasher, 30000> hasSigForIdCache GUARDED_BY(cs);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForSessionCache GUARDED_BY(cs);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForHashCache GUARDED_BY(cs);

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
    void MigrateRecoveredSigs();

    bool ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret) const;
    void RemoveRecoveredSig(CDBBatch& batch, Consensus::LLMQType llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

class CRecoveredSigsListener
{
public:
    virtual ~CRecoveredSigsListener() = default;

    virtual void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) = 0;
};

class CSigningManager
{
    friend class CSigSharesManager;

    // when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
    // starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
    // which are not 100% at the chain tip.
    static constexpr int SIGN_HEIGHT_OFFSET{8};

private:
    mutable RecursiveMutex cs;

    CRecoveredSigsDb db;
    CConnman& connman;
    const CQuorumManager& qman;

    const std::unique_ptr<PeerManager>& m_peerman;

    // Incoming and not verified yet
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> pendingRecoveredSigs GUARDED_BY(cs);
    std::unordered_map<uint256, std::shared_ptr<const CRecoveredSig>, StaticSaltedHasher> pendingReconstructedRecoveredSigs GUARDED_BY(cs);

    FastRandomContext rnd GUARDED_BY(cs);

    int64_t lastCleanupTime{0};

    std::vector<CRecoveredSigsListener*> recoveredSigsListeners GUARDED_BY(cs);

public:
    CSigningManager(CConnman& _connman, const CQuorumManager& _qman,
                    const std::unique_ptr<PeerManager>& peerman, bool fMemory, bool fWipe);

    bool AlreadyHave(const CInv& inv) const;
    bool GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const;

    void ProcessMessage(const CNode& pnode, const std::string& msg_type, CDataStream& vRecv);

    // This is called when a recovered signature was was reconstructed from another P2P message and is known to be valid
    // This is the case for example when a signature appears as part of InstantSend or ChainLocks
    void PushReconstructedRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig);

    // This is called when a recovered signature can be safely removed from the DB. This is only safe when some other
    // mechanism prevents possible conflicts. As an example, ChainLocks prevent conflicts in confirmed TXs InstantSend votes
    // This won't completely remove all traces of the recovered sig but instead leave the hash entry in the DB. This
    // allows AlreadyHave to keep returning true. Cleanup will later remove the remains
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

private:
    void ProcessMessageRecoveredSig(const CNode& pfrom, const std::shared_ptr<const CRecoveredSig>& recoveredSig);
    static bool PreVerifyRecoveredSig(const CQuorumManager& quorum_manager, const CRecoveredSig& recoveredSig, bool& retBan);

    void CollectPendingRecoveredSigsToVerify(size_t maxUniqueSessions,
            std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
            std::unordered_map<std::pair<Consensus::LLMQType, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums);
    void ProcessPendingReconstructedRecoveredSigs();
    bool ProcessPendingRecoveredSigs(); // called from the worker thread of CSigSharesManager
    void ProcessRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig);
    void Cleanup(); // called from the worker thread of CSigSharesManager

public:
    // public interface
    void RegisterRecoveredSigsListener(CRecoveredSigsListener* l);
    void UnregisterRecoveredSigsListener(CRecoveredSigsListener* l);

    bool AsyncSignIfMember(Consensus::LLMQType llmqType, CSigSharesManager& shareman, const uint256& id, const uint256& msgHash, const uint256& quorumHash = uint256(), bool allowReSign = false);
    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id) const;
    bool HasRecoveredSigForSession(const uint256& signHash) const;
    bool GetRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& retRecSig) const;
    bool IsConflicting(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash) const;

    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet) const;

    static std::vector<CQuorumCPtr> GetActiveQuorumSet(Consensus::LLMQType llmqType, int signHeight);
    static CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CQuorumManager& quorum_manager, const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

    // Verifies a recovered sig that was signed while the chain tip was at signedAtTip
    static bool VerifyRecoveredSig(Consensus::LLMQType llmqType, const CQuorumManager& quorum_manager, int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig, int signOffset = SIGN_HEIGHT_OFFSET);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_SIGNING_H
