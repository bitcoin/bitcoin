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
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint16_t nDataMask;
    uint256 proTxHash;
    Errors nError;

    int64_t nTime;
    bool fProcessed;

    static const int64_t EXPIRATION_TIMEOUT{300};

public:

    CQuorumDataRequest() : nTime(GetTime()) {}
    CQuorumDataRequest(const Consensus::LLMQType llmqTypeIn, const uint256& quorumHashIn, const uint16_t nDataMaskIn, const uint256& proTxHashIn = uint256()) :
        llmqType(llmqTypeIn),
        quorumHash(quorumHashIn),
        nDataMask(nDataMaskIn),
        proTxHash(proTxHashIn),
        nError(UNDEFINED),
        nTime(GetTime()),
        fProcessed(false) {}

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(nDataMask);
        READWRITE(proTxHash);
        if (ser_action.ForRead()) {
            try {
                READWRITE(nError);
            } catch (...) {
                nError = UNDEFINED;
            }
        } else if (nError != UNDEFINED) {
            READWRITE(nError);
        }
    }

    const Consensus::LLMQType GetLLMQType() const { return llmqType; }
    const uint256& GetQuorumHash() const { return quorumHash; }
    const uint16_t GetDataMask() const { return nDataMask; }
    const uint256& GetProTxHash() const { return proTxHash; }

    void SetError(Errors nErrorIn) { nError = nErrorIn; }
    const Errors GetError() const { return nError; }

    bool IsExpired() const
    {
        return (GetTime() - nTime) >= EXPIRATION_TIMEOUT;
    }
    bool IsProcessed() const
    {
        return fProcessed;
    }
    void SetProcessed()
    {
        fProcessed = true;
    }

    bool operator==(const CQuorumDataRequest& other)
    {
        return llmqType == other.llmqType &&
               quorumHash == other.quorumHash &&
               nDataMask == other.nDataMask &&
               proTxHash == other.proTxHash;
    }
    bool operator!=(const CQuorumDataRequest& other)
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

    bool SetVerificationVector(const BLSVerificationVector& quorumVecIn);
    bool SetSecretKeyShare(const CBLSSecretKey& secretKeyShare);

    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const;
    const CBLSSecretKey& GetSkShare() const;

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

    mutable CCriticalSection quorumsCacheCs;
    mutable std::map<Consensus::LLMQType, unordered_lru_cache<uint256, CQuorumPtr, StaticSaltedHasher>> mapQuorumsCache;
    mutable std::map<Consensus::LLMQType, unordered_lru_cache<uint256, std::vector<CQuorumCPtr>, StaticSaltedHasher>> scanQuorumsCache;

public:
    CQuorumManager(CEvoDB& _evoDb, CBLSWorker& _blsWorker, CDKGSessionManager& _dkgManager);

    void UpdatedBlockTip(const CBlockIndex *pindexNew, bool fInitialDownload) const;

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);

    static bool HasQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash);

    bool RequestQuorumData(CNode* pFrom, Consensus::LLMQType llmqType, const CBlockIndex* pQuorumIndex, uint16_t nDataMask, const uint256& proTxHash = uint256());

    // all these methods will lock cs_main for a short period of time
    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const uint256& quorumHash) const;
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, size_t nCountRequested) const;

    // this one is cs_main-free
    std::vector<CQuorumCPtr> ScanQuorums(Consensus::LLMQType llmqType, const CBlockIndex* pindexStart, size_t nCountRequested) const;

private:
    // all private methods here are cs_main-free
    void EnsureQuorumConnections(Consensus::LLMQType llmqType, const CBlockIndex *pindexNew) const;

    CQuorumPtr BuildQuorumFromCommitment(Consensus::LLMQType llmqType, const CBlockIndex* pindexQuorum) const;
    bool BuildQuorumContributions(const CFinalCommitment& fqc, std::shared_ptr<CQuorum>& quorum) const;

    CQuorumCPtr GetQuorum(Consensus::LLMQType llmqType, const CBlockIndex* pindex) const;
};

extern CQuorumManager* quorumManager;

} // namespace llmq

template<> struct is_serializable_enum<llmq::CQuorumDataRequest::Errors> : std::true_type {};

#endif // BITCOIN_LLMQ_QUORUMS_H
