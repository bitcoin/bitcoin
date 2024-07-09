// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_H
#define SYSCOIN_LLMQ_QUORUMS_H

#include <util/threadinterrupt.h>

#include <validationinterface.h>
#include <consensus/params.h>
#include <saltedhasher.h>

#include <bls/bls.h>
#include <bls/bls_worker.h>
#include <evo/evodb.h>
class CNode;
class CConnman;
class CBlockIndex;

class CDeterministicMN;
class ChainstateManager;
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
    CQuorum(CBLSWorker& _blsWorker);
    ~CQuorum() = default;
    void Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, const std::vector<CDeterministicMNCPtr>& _members);

    bool HasVerificationVector() const;
    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const;
    CBLSSecretKey GetSkShare() const;

private:
    void WriteContributions(CEvoDB<uint256, std::vector<CBLSPublicKey>>& evoDb_vvec, CEvoDB<uint256, CBLSSecretKey>& evoDb_sk);
    bool ReadContributions(const CEvoDB<uint256, std::vector<CBLSPublicKey>>& evoDb_vvec, const CEvoDB<uint256, CBLSSecretKey>& evoDb_sk);
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
    CBLSWorker& blsWorker;
    CDKGSessionManager& dkgManager;
    ChainstateManager& chainman;
    mutable RecursiveMutex cs_quorums;
    mutable std::vector<CQuorumCPtr> vecQuorumsCache GUARDED_BY(cs_quorums);
    mutable ctpl::thread_pool workerPool;
    mutable CThreadInterrupt quorumThreadInterrupt;

public:
    CEvoDB<uint256, std::vector<CBLSPublicKey>> evoDb_vvec;
    CEvoDB<uint256, CBLSSecretKey> evoDb_sk;
    explicit CQuorumManager(const DBParams& db_params_vvecs, const DBParams& db_params_sk, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager, ChainstateManager& _chainman);
    ~CQuorumManager() { Stop(); };

    void Start();
    void Stop();

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload);


    static bool HasQuorum(const uint256& quorumHash);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(const uint256& quorumHash);
    std::vector<CQuorumCPtr> ScanQuorums(size_t nCountRequested);

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(const CBlockIndex* pindexStart, size_t nCountRequested);
    bool FlushCacheToDisk();
private:
    std::vector<CQuorumCPtr>::iterator FindQuorumByHash(const uint256& blockHash) EXCLUSIVE_LOCKS_REQUIRED(cs_quorums);
    // all private methods here are cs_main-free
    void EnsureQuorumConnections(const CBlockIndex *pindexNew);

    CQuorumPtr BuildQuorumFromCommitment(const CBlockIndex* pQuorumBaseBlockIndex);
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const;

    CQuorumCPtr GetQuorum(const CBlockIndex* pindex);
    void StartCachePopulatorThread(const CQuorumCPtr pQuorum) const;
    void CleanupOldQuorumData(const CBlockIndex* pIndex);
};

extern CQuorumManager* quorumManager;

} // namespace llmq


#endif // SYSCOIN_LLMQ_QUORUMS_H
