// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_SIGNING_H
#define SYSCOIN_LLMQ_QUORUMS_SIGNING_H

#include <bls/bls.h>

#include <consensus/params.h>
#include <saltedhasher.h>
#include <univalue.h>
#include <unordered_lru_cache.h>

#include <evo/evodb.h>

#include <unordered_map>
#include <sync.h>
#include <random.h>
#include <streams.h>
#include <threadsafety.h>
using NodeId = int64_t;
class CNode;
class CConnman;
class PeerManager;
class ChainstateManager;
namespace llmq
{

class CQuorum;
using CQuorumCPtr = std::shared_ptr<const CQuorum>;
class CRecoveredSig
{
public:
    const uint8_t llmqType{Consensus::LLMQ_NONE};
    const uint256 quorumHash{};
    const uint256 id{};
    const uint256 msgHash{};
    const CBLSLazySignature sig;

    CRecoveredSig() = default;

    CRecoveredSig(uint8_t _llmqType, const uint256& _quorumHash, const uint256& _id, const uint256& _msgHash, const CBLSLazySignature& _sig) :
                  llmqType(_llmqType), quorumHash(_quorumHash), id(_id), msgHash(_msgHash), sig(_sig) {UpdateHash();};
    CRecoveredSig(uint8_t _llmqType, const uint256& _quorumHash, const uint256& _id, const uint256& _msgHash, const CBLSSignature& _sig) :
                  llmqType(_llmqType), quorumHash(_quorumHash), id(_id), msgHash(_msgHash) {const_cast<CBLSLazySignature&>(sig).Set(_sig); UpdateHash();};

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
        READWRITE(const_cast<uint8_t&>(obj.llmqType), const_cast<uint256&>(obj.quorumHash), const_cast<uint256&>(obj.id),
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
    mutable unordered_lru_cache<std::pair<uint8_t, uint256>, bool, StaticSaltedHasher, 30000> hasSigForIdCache GUARDED_BY(cs);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForSessionCache GUARDED_BY(cs);
    mutable unordered_lru_cache<uint256, bool, StaticSaltedHasher, 30000> hasSigForHashCache GUARDED_BY(cs);

public:
    explicit CRecoveredSigsDb(bool fMemory, bool fWipe);

    bool HasRecoveredSig(uint8_t llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(uint8_t llmqType, const uint256& id) const ;
    bool HasRecoveredSigForSession(const uint256& signHash) const;
    bool HasRecoveredSigForHash(const uint256& hash) const;
    bool GetRecoveredSigByHash(const uint256& hash, CRecoveredSig& ret) const;
    bool GetRecoveredSigById(uint8_t llmqType, const uint256& id, CRecoveredSig& ret) const;
    void WriteRecoveredSig(const CRecoveredSig& recSig);
    void RemoveRecoveredSig(uint8_t llmqType, const uint256& id) EXCLUSIVE_LOCKS_REQUIRED(cs);
    void TruncateRecoveredSig(uint8_t llmqType, const uint256& id);

    void CleanupOldRecoveredSigs(int64_t maxAge);

    // votes are removed when the recovered sig is written to the db
    bool HasVotedOnId(uint8_t llmqType, const uint256& id) const;
    bool GetVoteForId(uint8_t llmqType, const uint256& id, uint256& msgHashRet) const;
    void WriteVoteForId(uint8_t llmqType, const uint256& id, const uint256& msgHash);

    void CleanupOldVotes(int64_t maxAge);

private:
    void MigrateRecoveredSigs();

    bool ReadRecoveredSig(uint8_t llmqType, const uint256& id, CRecoveredSig& ret) const;
    void RemoveRecoveredSig(CDBBatch& batch, uint8_t llmqType, const uint256& id, bool deleteHashKey, bool deleteTimeKey) EXCLUSIVE_LOCKS_REQUIRED(cs);
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


private:
    mutable RecursiveMutex cs;

    CRecoveredSigsDb db;
    CConnman& connman;
    PeerManager& peerman;
    ChainstateManager& chainman;
    // Incoming and not verified yet
    std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>> pendingRecoveredSigs GUARDED_BY(cs);
    std::unordered_map<uint256, std::shared_ptr<const CRecoveredSig>, StaticSaltedHasher> pendingReconstructedRecoveredSigs GUARDED_BY(cs);

    FastRandomContext rnd GUARDED_BY(cs);

    int64_t lastCleanupTime{0};

    std::vector<CRecoveredSigsListener*> recoveredSigsListeners GUARDED_BY(cs);

public:
    // when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
    // starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
    // which are not 100% at the chain tip.
    static constexpr int SIGN_HEIGHT_LOOKBACK{5};
    CSigningManager(bool fMemory, CConnman& _connman, PeerManager& _peerman, ChainstateManager& _chainman, bool fWipe);

    bool AlreadyHave(const uint256& hash) const;
    bool GetRecoveredSigForGetData(const uint256& hash, CRecoveredSig& ret) const;

    void ProcessMessage(CNode* pnode, const std::string& strCommand, CDataStream& vRecv);

    // This is called when a recovered signature was was reconstructed from another P2P message and is known to be valid
    // This is the case for example when a signature appears as part of InstantSend or ChainLocks
    void PushReconstructedRecoveredSig(const std::shared_ptr<const CRecoveredSig>& recoveredSig);

    // This is called when a recovered signature can be safely removed from the DB. This is only safe when some other
    // mechanism prevents possible conflicts. As an example, ChainLocks prevent conflicts in confirmed TXs InstantSend votes
    // This won't completely remove all traces of the recovered sig but instead leave the hash entry in the DB. This
    // allows AlreadyHave to keep returning true. Cleanup will later remove the remains
    void TruncateRecoveredSig(uint8_t llmqType, const uint256& id);

private:
    void ProcessMessageRecoveredSig(CNode* pfrom, const std::shared_ptr<const CRecoveredSig>& recoveredSig);
    static bool PreVerifyRecoveredSig(const CRecoveredSig& recoveredSig, bool& retBan);

    void CollectPendingRecoveredSigsToVerify(size_t maxUniqueSessions,
            std::unordered_map<NodeId, std::list<std::shared_ptr<const CRecoveredSig>>>& retSigShares,
            std::unordered_map<std::pair<uint8_t, uint256>, CQuorumCPtr, StaticSaltedHasher>& retQuorums);
    void ProcessPendingReconstructedRecoveredSigs();
    bool ProcessPendingRecoveredSigs(); // called from the worker thread of CSigSharesManager
    void ProcessRecoveredSig(NodeId nodeId, const std::shared_ptr<const CRecoveredSig>& recoveredSig);
    void Cleanup(); // called from the worker thread of CSigSharesManager

public:
    // public interface
    void RegisterRecoveredSigsListener(CRecoveredSigsListener* l);
    void UnregisterRecoveredSigsListener(CRecoveredSigsListener* l);

    bool AsyncSignIfMember(uint8_t llmqType, const uint256& id, const uint256& msgHash, const uint256& quorumHash = uint256(), bool allowReSign = false);
    bool HasRecoveredSig(uint8_t llmqType, const uint256& id, const uint256& msgHash) const;
    bool HasRecoveredSigForId(uint8_t llmqType, const uint256& id) const;
    bool HasRecoveredSigForSession(const uint256& signHash) const;
    bool GetRecoveredSigForId(uint8_t llmqType, const uint256& id, CRecoveredSig& retRecSig) const;
    bool IsConflicting(uint8_t llmqType, const uint256& id, const uint256& msgHash) const;

    bool HasVotedOnId(uint8_t llmqType, const uint256& id) const;
    bool GetVoteForId(uint8_t llmqType, const uint256& id, uint256& msgHashRet) const;
    void Clear();
    static std::vector<CQuorumCPtr> GetActiveQuorumSet(uint8_t llmqType, int signHeight);
    static CQuorumCPtr SelectQuorumForSigning(ChainstateManager& chainman, uint8_t llmqType, const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_LOOKBACK);
    // Verifies a recovered sig that was signed while the chain tip was at signedAtTip
    static bool VerifyRecoveredSig(ChainstateManager& chainman, uint8_t llmqType, int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig, int signOffset = SIGN_HEIGHT_LOOKBACK);
};

extern CSigningManager* quorumSigningManager;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_SIGNING_H
