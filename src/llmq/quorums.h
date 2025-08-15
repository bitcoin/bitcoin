// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_H
#define BITCOIN_LLMQ_QUORUMS_H

#include <llmq/params.h>

#include <bls/bls.h>
#include <bls/bls_worker.h>
#include <ctpl_stl.h>
#include <gsl/pointers.h>
#include <protocol.h>
#include <saltedhasher.h>
#include <util/threadinterrupt.h>
#include <unordered_lru_cache.h>
#include <util/time.h>

#include <atomic>
#include <map>
#include <utility>

class CActiveMasternodeManager;
class CBlockIndex;
class CChain;
class CChainState;
class CConnman;
class CDataStream;
class CDeterministicMN;
class CDeterministicMNManager;
class CDBWrapper;
class CEvoDB;
class CMasternodeSync;
class CNode;
class CSporkManager;

using CDeterministicMNCPtr = std::shared_ptr<const CDeterministicMN>;

namespace llmq
{
enum class VerifyRecSigStatus
{
    NoQuorum,
    Invalid,
    Valid,
};

class CDKGSessionManager;
class CQuorumBlockProcessor;
class CQuorumSnapshotManager;

/**
 * Object used as a key to store CQuorumDataRequest
 */
struct CQuorumDataRequestKey
{
    uint256 proRegTx;
    bool m_we_requested;
    uint256 quorumHash;
    Consensus::LLMQType llmqType;

    CQuorumDataRequestKey(const uint256& proRegTxIn, const bool _m_we_requested, const uint256& quorumHashIn, const Consensus::LLMQType llmqTypeIn) :
        proRegTx(proRegTxIn),
        m_we_requested(_m_we_requested),
        quorumHash(quorumHashIn),
        llmqType(llmqTypeIn)
        {}

    bool operator ==(const CQuorumDataRequestKey& obj) const
    {
        return (proRegTx == obj.proRegTx && m_we_requested == obj.m_we_requested && quorumHash == obj.quorumHash && llmqType == obj.llmqType);
    }
};

/**
 * An object of this class represents a QGETDATA request or a QDATA response header
 */
class CQuorumDataRequest
{
public:

    enum Flags : uint16_t {
        QUORUM_VERIFICATION_VECTOR = 0x0001,
        ENCRYPTED_CONTRIBUTIONS = 0x0002,
    };
    enum Errors : uint8_t {
        NONE = 0x00,
        QUORUM_TYPE_INVALID = 0x01,
        QUORUM_BLOCK_NOT_FOUND = 0x02,
        QUORUM_NOT_FOUND = 0x03,
        MASTERNODE_IS_NO_MEMBER = 0x04,
        QUORUM_VERIFICATION_VECTOR_MISSING = 0x05,
        ENCRYPTED_CONTRIBUTIONS_MISSING = 0x06,
        UNDEFINED = 0xFF,
    };

private:
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint16_t nDataMask{0};
    uint256 proTxHash;
    Errors nError{UNDEFINED};

    int64_t nTime{GetTime()};
    bool fProcessed{false};

    static constexpr int64_t EXPIRATION_TIMEOUT{300};
    static constexpr int64_t EXPIRATION_BIAS{60};

public:

    CQuorumDataRequest() : nTime(GetTime()) {}
    CQuorumDataRequest(const Consensus::LLMQType llmqTypeIn, const uint256& quorumHashIn, const uint16_t nDataMaskIn, const uint256& proTxHashIn = uint256()) :
        llmqType(llmqTypeIn),
        quorumHash(quorumHashIn),
        nDataMask(nDataMaskIn),
        proTxHash(proTxHashIn)
        {}

    SERIALIZE_METHODS(CQuorumDataRequest, obj)
    {
        bool fRead{false};
        SER_READ(obj, fRead = true);
        READWRITE(obj.llmqType, obj.quorumHash, obj.nDataMask, obj.proTxHash);
        if (fRead) {
            try {
                READWRITE(obj.nError);
            } catch (...) {
                SER_READ(obj, obj.nError = UNDEFINED);
            }
        } else if (obj.nError != UNDEFINED) {
            READWRITE(obj.nError);
        }
    }

    Consensus::LLMQType GetLLMQType() const { return llmqType; }
    const uint256& GetQuorumHash() const { return quorumHash; }
    uint16_t GetDataMask() const { return nDataMask; }
    const uint256& GetProTxHash() const { return proTxHash; }

    void SetError(Errors nErrorIn) { nError = nErrorIn; }
    Errors GetError() const { return nError; }
    std::string GetErrorString() const;

    bool IsExpired(bool add_bias) const { return (GetTime() - nTime) >= (EXPIRATION_TIMEOUT + (add_bias ? EXPIRATION_BIAS : 0)); }
    bool IsProcessed() const { return fProcessed; }
    void SetProcessed() { fProcessed = true; }

    bool operator==(const CQuorumDataRequest& other) const
    {
        return llmqType == other.llmqType &&
               quorumHash == other.quorumHash &&
               nDataMask == other.nDataMask &&
               proTxHash == other.proTxHash;
    }
    bool operator!=(const CQuorumDataRequest& other) const
    {
        return !(*this == other);
    }
};

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
    const Consensus::LLMQParams params;
    CFinalCommitmentPtr qc;
    const CBlockIndex* m_quorum_base_block_index{nullptr};
    uint256 minedBlockHash;
    std::vector<CDeterministicMNCPtr> members;

private:
    // Recovery of public key shares is very slow, so we start a background thread that pre-populates a cache so that
    // the public key shares are ready when needed later
    mutable CBLSWorkerCache blsCache;
    mutable std::atomic<bool> fQuorumDataRecoveryThreadRunning{false};

    mutable Mutex cs_vvec_shShare;
    // These are only valid when we either participated in the DKG or fully watched it
    BLSVerificationVectorPtr quorumVvec GUARDED_BY(cs_vvec_shShare);
    CBLSSecretKey skShare GUARDED_BY(cs_vvec_shShare);

public:
    CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker);
    ~CQuorum() = default;
    void Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, Span<CDeterministicMNCPtr> _members);

    bool SetVerificationVector(const std::vector<CBLSPublicKey>& quorumVecIn);
    void SetVerificationVector(BLSVerificationVectorPtr vvec_in) {
        LOCK(cs_vvec_shShare);
        quorumVvec = std::move(vvec_in);
    }
    bool SetSecretKeyShare(const CBLSSecretKey& secretKeyShare, const CActiveMasternodeManager& mn_activeman);

    bool HasVerificationVector() const LOCKS_EXCLUDED(cs_vvec_shShare);
    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const;
    CBLSSecretKey GetSkShare() const;

private:
    bool HasVerificationVectorInternal() const EXCLUSIVE_LOCKS_REQUIRED(cs_vvec_shShare);
    void WriteContributions(CDBWrapper& db) const;
    bool ReadContributions(const CDBWrapper& db);
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
    std::unique_ptr<CDBWrapper> db GUARDED_BY(cs_db){nullptr};

    CBLSWorker& blsWorker;
    CChainState& m_chainstate;
    CDeterministicMNManager& m_dmnman;
    CDKGSessionManager& dkgManager;
    CQuorumBlockProcessor& quorumBlockProcessor;
    CQuorumSnapshotManager& m_qsnapman;
    const CActiveMasternodeManager* const m_mn_activeman;
    const CMasternodeSync& m_mn_sync;
    const CSporkManager& m_sporkman;

    mutable Mutex cs_map_quorums;
    mutable std::map<Consensus::LLMQType, unordered_lru_cache<uint256, CQuorumPtr, StaticSaltedHasher>> mapQuorumsCache GUARDED_BY(cs_map_quorums);
    mutable Mutex cs_scan_quorums;
    mutable std::map<Consensus::LLMQType, unordered_lru_cache<uint256, std::vector<CQuorumCPtr>, StaticSaltedHasher>> scanQuorumsCache GUARDED_BY(cs_scan_quorums);
    mutable Mutex cs_cleanup;
    mutable std::map<Consensus::LLMQType, unordered_lru_cache<uint256, uint256, StaticSaltedHasher>> cleanupQuorumsCache GUARDED_BY(cs_cleanup);

    mutable Mutex cs_quorumBaseBlockIndexCache;
    // On mainnet, we have around 62 quorums active at any point; let's cache a little more than double that to be safe.
    mutable unordered_lru_cache<uint256 /*quorum_hash*/, const CBlockIndex* /*pindex*/, StaticSaltedHasher, 128 /*max_size*/> quorumBaseBlockIndexCache;

    mutable ctpl::thread_pool workerPool;
    mutable CThreadInterrupt quorumThreadInterrupt;

public:
    CQuorumManager(CBLSWorker& _blsWorker, CChainState& chainstate, CDeterministicMNManager& dmnman,
                   CDKGSessionManager& _dkgManager, CEvoDB& _evoDb, CQuorumBlockProcessor& _quorumBlockProcessor,
                   CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager* const mn_activeman,
                   const CMasternodeSync& mn_sync, const CSporkManager& sporkman, bool unit_tests, bool wipe);
    ~CQuorumManager();

    void Start();
    void Stop();

    void TriggerQuorumDataRecoveryThreads(CConnman& connman, const CBlockIndex* pIndex) const;

    void UpdatedBlockTip(const CBlockIndex* pindexNew, CConnman& connman, bool fInitialDownload) const;

    [[nodiscard]] MessageProcessingResult ProcessMessage(CNode& pfrom, CConnman& connman, std::string_view msg_type, CDataStream& vRecv);

    static bool HasQuorum(Consensus::LLMQType llmqType, const CQuorumBlockProcessor& quorum_block_processor, const uint256& quorumHash);

    bool RequestQuorumData(CNode* pfrom, CConnman& connman, const CQuorumCPtr pQuorum, uint16_t nDataMask,
                           const uint256& proTxHash = uint256()) const;

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const;
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const;

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t nCountRequested) const;

private:
    // all private methods here are cs_main-free
    void CheckQuorumConnections(CConnman& connman, const Consensus::LLMQParams& llmqParams,
                                const CBlockIndex* pindexNew) const;

    CQuorumPtr BuildQuorumFromCommitment(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool populate_cache) const;
    bool BuildQuorumContributions(const CFinalCommitmentPtr& fqc, const std::shared_ptr<CQuorum>& quorum) const;

    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pindex, bool populate_cache = true) const;
    /// Returns the start offset for the masternode with the given proTxHash. This offset is applied when picking data recovery members of a quorum's
    /// memberlist and is calculated based on a list of all member of all active quorums for the given llmqType in a way that each member
    /// should receive the same number of request if all active llmqType members requests data from one llmqType quorum.
    size_t GetQuorumRecoveryStartOffset(const CQuorumCPtr pQuorum, const CBlockIndex* pIndex) const;

    void StartCachePopulatorThread(const CQuorumCPtr pQuorum) const;
    void StartQuorumDataRecoveryThread(CConnman& connman, const CQuorumCPtr pQuorum, const CBlockIndex* pIndex,
                                       uint16_t nDataMask) const;

    void StartCleanupOldQuorumDataThread(const CBlockIndex* pIndex) const;
    void MigrateOldQuorumDB(CEvoDB& evoDb) const;
};

// when selecting a quorum for signing and verification, we use CQuorumManager::SelectQuorum with this offset as
// starting height for scanning. This is because otherwise the resulting signatures would not be verifiable by nodes
// which are not 100% at the chain tip.
static constexpr int SIGN_HEIGHT_OFFSET{8};

CQuorumCPtr SelectQuorumForSigning(const Consensus::LLMQParams& llmq_params, const CChain& active_chain, const CQuorumManager& qman,
                                   const uint256& selectionHash, int signHeight = -1 /*chain tip*/, int signOffset = SIGN_HEIGHT_OFFSET);

// Verifies a recovered sig that was signed while the chain tip was at signedAtTip
VerifyRecSigStatus VerifyRecoveredSig(Consensus::LLMQType llmqType, const CChain& active_chain, const CQuorumManager& qman,
                                      int signedAtHeight, const uint256& id, const uint256& msgHash, const CBLSSignature& sig,
                                      int signOffset = SIGN_HEIGHT_OFFSET);
} // namespace llmq

template<typename T> struct SaltedHasherImpl;
template<>
struct SaltedHasherImpl<llmq::CQuorumDataRequestKey>
{
    static std::size_t CalcHash(const llmq::CQuorumDataRequestKey& v, uint64_t k0, uint64_t k1)
    {
        CSipHasher c(k0, k1);
        c.Write(reinterpret_cast<const unsigned char*>(&v.proRegTx), sizeof(v.proRegTx));
        c.Write(reinterpret_cast<const unsigned char*>(&v.m_we_requested), sizeof(v.m_we_requested));
        c.Write(reinterpret_cast<const unsigned char*>(&v.quorumHash), sizeof(v.quorumHash));
        c.Write(reinterpret_cast<const unsigned char*>(&v.llmqType), sizeof(v.llmqType));
        return c.Finalize();
    }
};

template<> struct is_serializable_enum<llmq::CQuorumDataRequest::Errors> : std::true_type {};

#endif // BITCOIN_LLMQ_QUORUMS_H
