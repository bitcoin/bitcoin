// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_SIGNING_H
#define BITCOIN_LLMQ_QUORUMS_SIGNING_H

#include <bls/bls.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <univalue.h>
#include <unordered_lru_cache.h>

#include <evo/evodb.h>

#include <unordered_map>
#include <sync.h>
#include <random.h>

typedef int64_t NodeId;
class CInv;
class CNode;

namespace llmq
{

class CQuorum;
typedef std::shared_ptr<const CQuorum> CQuorumCPtr;

// Keep recovered signatures for a week. This is a "-maxrecsigsage" option default.
static const int64_t DEFAULT_MAX_RECOVERED_SIGS_AGE = 60 * 60 * 24 * 7;


class CRecoveredSig
{
public:
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint256 id;
    uint256 msgHash;
    CBLSLazySignature sig;

    // only in-memory
    uint256 hash;

public:

    SERIALIZE_METHODS(CRecoveredSig, obj)
    {
        READWRITE(obj.llmqType, obj.quorumHash, obj.id, obj.msgHash, obj.sig);
        SER_READ(obj, obj.UpdateHash());
    }

    void UpdateHash()
    {
        hash = ::SerializeHash(*this);
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
    CDBWrapper& db;

    CCriticalSection cs;
    unordered_lru_cache<std::pair<Consensus::LLMQType, uint256>, bool, StaticSaltedHasher, 30000> hasSigForIdCache GUARDED_BY(cs);
    unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForSessionCache GUARDED_BY(cs);
    unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForHashCache GUARDED_BY(cs);

public:
    explicit CRecoveredSigsDb(CDBWrapper& _db);

    void ConvertInvalidTimeKeys();
    void AddVoteTimeKeys();

    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id);
    bool HasRecoveredSigForSession(const uint256& signHash);
    bool HasRecoveredSigForHash(const uint256& hash);
    bool GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret);
    bool GetRecoveredSigById(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret);
    void WriteRecoveredSig(const CRecoveredSig& recSig);
    void RemoveRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

    void CleanupOldRecoveredSigs(int64_t maxAge);

    // votes are removed when the recovered sig is written to the db
    bool HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id);
    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet);
    void WriteVoteForId(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);

    void CleanupOldVotes(int64_t maxAge);

private:
    bool ReadRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& ret);
    void RemoveRecoveredSig(CDBBatch& batch, Consensus::LLMQType llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey);
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
    static const int SIGN_HEIGHT_OFFSET = 8;

private:
    CCriticalSection cs;

    CRecoveredSigsDb db;

    // Incoming and not verified yet
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> pendingRecoveredSigs GUARDED_BY(cs);
    std::unordered_map<uint256, std::shared_ptr<const CRecoveredSig>, StaticSaltedHasher> pendingReconstructedRecoveredSigs GUARDED_BY(cs);

    FastRandomContext rnd GUARDED_BY(cs);

    int64_t lastCleanupTime{0};

    std::vector<CRecoveredSigsListener*> recoveredSigsListeners GUARDED_BY(cs);

public:
    CSigningManager(CDBWrapper& llmqDb, bool fMemory);

    bool AlreadyHave(const CInv& inv);
    bool GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret);

    void ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv);

    // This is called when a recovered signature was was reconstructed from another P2P message and is known to be valid
    // This is the case for example when a signature appears as part of InstantSend or ChainLocks
    void PushReconstructedRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig);

    // This is called when a recovered signature can be safely removed from the DB. This is only safe when some other
    // mechanism prevents possible conflicts. As an example, ChainLocks prevent conflicts in confirmed TXs InstantSend votes
    // This won't completely remove all traces of the recovered sig but instead leave the hash entry in the DB. This
    // allows AlreadyHave to keep returning true. Cleanup will later remove the remains
    void TruncateRecoveredSig(Consensus::LLMQType llmqType, const uint256& id);

private:
    void ProcessMessageRecoveredSig(CNode* pfrom, const std::shared_ptr<const CRecoveredSig>& recoveredSig);
    static bool PreVerifyRecoveredSig(const CRecoveredSig& recoveredSig, bool& retBan);

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

    bool AsyncSignIfMember(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash, const uint256& quorumHash = uint256(), bool allowReSign = false);
    bool HasRecoveredSig(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);
    bool HasRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id);
    bool HasRecoveredSigForSession(const uint256& signHash);
    bool GetRecoveredSigForId(Consensus::LLMQType llmqType, const uint256& id, CRecoveredSig& retRecSig);
    bool IsConflicting(Consensus::LLMQType llmqType, const uint256& id, const uint256& msgHash);

    bool HasVotedOnId(Consensus::LLMQType llmqType, const uint256& id);
    bool GetVoteForId(Consensus::LLMQType llmqType, const uint256& id, uint256& msgHashRet);

    static std::vector<CQuorumCPtr> GetActiveQuorumSet(Consensus::LLMQType llmqType, int signHeight);
    static CQuorumCPtr SelectQuorumForSigning(Consensus::LLMQType llmqType, const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

    // Verifies a recovered sig that was signed while the chain tip was at signedAtTip
    static bool VerifyRecoveredSig(Consensus::LLMQType llmqType, int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig, int signOffset = SIGN_HEIGHT_OFFSET);
};

extern CSigningManager* quorumSigningManager;

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_SIGNING_H
