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

namespace llmq_tests
{
class CQuorumManagerTestAccess;
}
namespace llmq
{
enum class VerifyRecSigStatus
{
    NoQuorum,
    Invalid,
    Valid,
};
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

    mutable Mutex cs_vvec_shShare;
    // These are only valid when we either participated in the DKG or fully watched it
    BLSVerificationVectorPtr quorumVvec GUARDED_BY(cs_vvec_shShare);
    CBLSSecretKey skShare GUARDED_BY(cs_vvec_shShare);

public:
    CQuorum(CBLSWorker& _blsWorker);
    ~CQuorum() = default;
    void Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, Span<CDeterministicMNCPtr> _members);

    void SetVerificationVector(BLSVerificationVectorPtr vvec_in) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare) {
        LOCK(cs_vvec_shShare);
        quorumVvec = std::move(vvec_in);
    }
    bool SetSecretKeyShare(const CBLSSecretKey& secretKeyShare) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    bool HasVerificationVector() const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    CBLSSecretKey GetSkShare() const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);

private:
    bool HasVerificationVectorInternal() const EXCLUSIVE_LOCKS_REQUIRED(cs_vvec_shShare);
    void WriteContributions(std::unique_ptr<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>>& evoDb_vvec, std::unique_ptr<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>>& evoDb_sk) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    bool ReadContributions(std::unique_ptr<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>>& evoDb_vvec, std::unique_ptr<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>>& evoDb_sk) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
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
    mutable Mutex cs_db;
    CBLSWorker& blsWorker;
    CDKGSessionManager& dkgManager;
    ChainstateManager& chainman;
    mutable Mutex cs_quorums;
    mutable std::vector<CQuorumCPtr> vecQuorumsCache GUARDED_BY(cs_quorums);
    mutable ctpl::thread_pool workerPool;
    mutable CThreadInterrupt quorumThreadInterrupt;
    static constexpr int QUORUM_CACHE_SIZE = 10;

public:
    std::unique_ptr<CEvoDB<uint256, std::vector<CBLSPublicKey>, StaticSaltedHasher>> evoDb_vvec;
    std::unique_ptr<CEvoDB<uint256, CBLSSecretKey, StaticSaltedHasher>> evoDb_sk;
    explicit CQuorumManager(const DBParams& db_params_vvecs, const DBParams& db_params_sk, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager, ChainstateManager& _chainman);
    ~CQuorumManager();

    void Start();
    void Stop();

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);


    static bool HasQuorum(const uint256& quorumHash);

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(const uint256& quorumHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);
    std::vector<CQuorumCPtr> ScanQuorums(size_t nCountRequested) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(const CBlockIndex* pindexStart, size_t nCountRequested) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);
    bool FlushCacheToDisk(bool bForceFlush, bool fSync = true);
private:
    static uint256 MakeQuorumKey(const CQuorum& quorum);
    static bool IsQuorumMinedOnChain(
        const CQuorum& quorum,
        const CBlockIndex* pindexStart);
    bool DoMaintenance(bool bForceFlush, bool fSync = true);
    std::vector<CQuorumCPtr>::iterator FindQuorum(
        const uint256& quorumHash,
        const uint256& minedBlockHash) EXCLUSIVE_LOCKS_REQUIRED(cs_quorums);
    // all private methods here are cs_main-free
    void EnsureQuorumConnections(const CBlockIndex *pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);

    CQuorumPtr BuildQuorumFromCommitment(
        const CBlockIndex* pQuorumBaseBlockIndex,
        CFinalCommitmentPtr qc,
        const uint256& minedBlockHash) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const EXCLUSIVE_LOCKS_REQUIRED(!cs_db, !cs_quorums);

    CQuorumCPtr GetQuorum(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs_quorums, !cs_db);
    void StartCachePopulatorThread(const CQuorumCPtr pQuorum) const;

    friend class CQuorum;
    friend class llmq_tests::CQuorumManagerTestAccess;
};

// when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
// starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
// which are not 100% at the chain tip.
static constexpr int SIGN_HEIGHT_OFFSET{5};

CQuorumCPtr SelectQuorumForSigning(ChainstateManager& chainman,
                                   const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

// Verifies a recovered sig that was signed while the chain tip was at signedAtTip
VerifyRecSigStatus VerifyRecoveredSig(ChainstateManager& chainman,
                                      int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                                      int signOffset = SIGN_HEIGHT_OFFSET);

extern CQuorumManager* quorumManager;

} // namespace llmq


#endif // SYSCOIN_LLMQ_QUORUMS_H
