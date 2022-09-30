// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_H
#define SYSCOIN_LLMQ_QUORUMS_H

#include <util/threadinterrupt.h>

#include <validationinterface.h>
#include <consensus/params.h>
#include <saltedhasher.h>
#include <unordered_lru_cache.h>

#include <bls/bls.h>
#include <bls/bls_worker.h>

class CNode;
class CConnman;
class CBlockIndex;

class CDeterministicMN;
class ChainstateManager;
class CEvoDB;
using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;


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

class CQuorum;
using CQuorumPtr = std::shared_ptr<CQuorum>;
using CQuorumCPtr = std::shared_ptr<const CQuorum>;

class CFinalCommitment;
using CFinalCommitmentPtr = std::unique_ptr<CFinalCommitment>;


class CQuorum
{
    friend class CQuorumManager;
public:
    const Consensus::LLMQParams& params;
    CFinalCommitmentPtr qc;
    const CBlockIndex* m_quorum_base_block_index{nullptr};
    uint256 minedBlockHash;
    std::vector<CDeterministicMNCPtr> members;

private:
    // Recovery of public key shares is very slow, so we start a background thread that pre-populates a cache so that
    // the public key shares are ready when needed later
    mutable CBLSWorkerCache blsCache;

    mutable RecursiveMutex cs;
    // These are only valid when we either participated in the DKG or fully watched it
    BLSVerificationVectorPtr quorumVvec GUARDED_BY(cs);
    CBLSSecretKey skShare GUARDED_BY(cs);

public:
    CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker);
    ~CQuorum() = default;
    void Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members);

    bool HasVerificationVector() const;
    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const;
    CBLSSecretKey GetSkShare() const;

private:
    void WriteContributions(CEvoDB& evoDb) const;
    bool ReadContributions(CEvoDB& evoDb);
};

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
    ChainstateManager& chainman;
    mutable RecursiveMutex cs_map_quorums;
    mutable RecursiveMutex cs_scan_quorums;
    mutable std::map<uint8_t, unordered_lru_cache<uint256, CQuorumPtr, StaticSaltedHasher>> mapQuorumsCache GUARDED_BY(cs_map_quorums);
    mutable std::map<uint8_t, unordered_lru_cache<uint256, std::vector<CQuorumCPtr>, StaticSaltedHasher>> scanQuorumsCache GUARDED_BY(cs_scan_quorums);

    mutable ctpl::thread_pool workerPool;
    mutable CThreadInterrupt quorumThreadInterrupt;

public:
    CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager, ChainstateManager& _chainman);
    ~CQuorumManager() { Stop(); };

    void Start();
    void Stop();

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload) const;


    static bool HasQuorum(uint8_t llmqType, const uint256& quorumHash);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(uint8_t llmqType, const uint256& quorumHash) const;
    std::vector<CQuorumCPtr> ScanQuorums(uint8_t llmqType, size_t nCountRequested) const;

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(uint8_t llmqType, const CBlockIndex* pindexStart, size_t nCountRequested) const;

private:
    // all private methods here are cs_main-free
    void EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, const CBlockIndex *pindexNew) const;

    CQuorumPtr BuildQuorumFromCommitment(uint8_t llmqType, const CBlockIndex* pQuorumBaseBlockIndex) const EXCLUSIVE_LOCKS_REQUIRED(cs_map_quorums);
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const;

    CQuorumCPtr GetQuorum(uint8_t llmqType, const CBlockIndex* pindex) const;
    void StartCachePopulatorThread(const CQuorumCPtr pQuorum) const;
};

extern CQuorumManager* quorumManager;

} // namespace llmq


#endif // SYSCOIN_LLMQ_QUORUMS_H
