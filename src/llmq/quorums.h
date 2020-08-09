// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_H
#define BITCOIN_LLMQ_QUORUMS_H

#include <evo/evodb.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_commitment.h>

#include <validationinterface.h>
#include <consensus/params.h>
#include <saltedhasher.h>
#include <unordered_lru_cache.h>

#include <bls/bls.h>
#include <bls/bls_worker.h>

namespace llmq
{

class CDKGSessionManager;

/**
 * An object of this class represents a quorum which was mined on-chain (through a quorum commitment)
 * It at least contains information about the members and the quorum public key which is needed to verify recovered
 * signatures from this quorum.
 *
 * In case the local node is a member of the same quorum and successfully participated in the DKG, the quorum object
 * will also contain the secret key share and the quorum verification vector. The quorum vvec is then used to recover
 * the public key shares of individual members, which are needed to verify signature shares of these members.
 */
class CQuorum
{
    friend class CQuorumManager;
public:
    const Consensus::LLMQParams& params;
    CFinalCommitment qc;
    const CBlockIndex* pindexQuorum;
    uint256 minedBlockHash;
    std::vector<CDeterministicMNCPtr> members;

    // These are only valid when we either participated in the DKG or fully watched it
    BLSVerificationVectorPtr quorumVvec;
    CBLSSecretKey skShare;

private:
    // Recovery of public key shares is very slow, so we start a background thread that pre-populates a cache so that
    // the public key shares are ready when needed later
    mutable CBLSWorkerCache blsCache;
    std::atomic<bool> stopCachePopulatorThread;
    std::thread cachePopulatorThread;

public:
    CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker) : params(_params), blsCache(_blsWorker), stopCachePopulatorThread(false) {}
    ~CQuorum();
    void Init(const CFinalCommitment& _qc, const CBlockIndex* _pindexQuorum, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members);

    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const;
    CBLSSecretKey GetSkShare() const;

private:
    void WriteContributions(CEvoDB& evoDb);
    bool ReadContributions(CEvoDB& evoDb);
    static void StartCachePopulatorThread(std::shared_ptr<CQuorum> _this);
};
typedef std::shared_ptr<CQuorum> CQuorumPtr;
typedef std::shared_ptr<const CQuorum> CQuorumCPtr;

/**
 * The quorum manager maintains quorums which were mined on chain. When a quorum is requested from the manager,
 * it will lookup the commitment (through CQuorumBlockProcessor) and build a CQuorum object from it.
 *
 * It is also responsible for initialization of the intra-quorum connections for new quorums.
 */
class CQuorumManager
{
private:
    CEvoDB& evoDb;
    CBLSWorker& blsWorker;
    CDKGSessionManager& dkgManager;

    CCriticalSection quorumsCacheCs;
    std::map<std::pair<Consensus::LLMQType, uint256>, CQuorumPtr> quorumsCache;
    unordered_lru_cache<std::pair<Consensus::LLMQType, uint256>, std::vector<CQuorumCPtr>, StaticSaltedHasher, 32> scanQuorumsCache;

public:
    CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager);

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload);

    static bool HasQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash);
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, size_t maxCount);

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t maxCount);

private:
    // all private methods here are cs_main-free
    void EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex *pindexNew);

    bool BuildQuorumFromCommitment(const CFinalCommitment& qc, const CBlockIndex* pindexQuorum, const uint256& minedBlockHash, std::shared_ptr<CQuorum>& quorum) const;
    bool BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const;

    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pindex);
};

extern CQuorumManager* quorumManager;

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_H
