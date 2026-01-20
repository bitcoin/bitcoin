// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_H
#define BITCOIN_LLMQ_QUORUMS_H

#include <bls/bls_worker.h>
#include <evo/types.h>
#include <llmq/params.h>
#include <llmq/types.h>
#include <saltedhasher.h>

#include <serialize.h>
#include <sync.h>
#include <threadsafety.h>
#include <uint256.h>
#include <util/time.h>

#include <set>
#include <string>
#include <vector>

class CBlockIndex;
class CDBWrapper;
namespace llmq {
class CQuorum;
class CQuorumManager;
class QuorumObserver;
class QuorumParticipant;
}; // namespace llmq

namespace llmq {
extern const std::string DB_QUORUM_SK_SHARE;
extern const std::string DB_QUORUM_QUORUM_VVEC;

uint256 MakeQuorumKey(const CQuorum& q);
void DataCleanupHelper(CDBWrapper& db, const std::set<uint256>& skip_list, bool compact = false);

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
class CQuorum
{
    friend class CQuorumManager;
    friend class llmq::QuorumObserver;
    friend class llmq::QuorumParticipant;

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

    bool SetVerificationVector(const std::vector<CBLSPublicKey>& quorumVecIn) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    void SetVerificationVector(BLSVerificationVectorPtr vvec_in) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare)
    {
        LOCK(cs_vvec_shShare);
        quorumVvec = std::move(vvec_in);
    }
    bool SetSecretKeyShare(const CBLSSecretKey& secretKeyShare, const uint256& protx_hash)
        EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);

    bool HasVerificationVector() const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    bool IsMember(const uint256& proTxHash) const;
    bool IsValidMember(const uint256& proTxHash) const;
    int GetMemberIndex(const uint256& proTxHash) const;

    CBLSPublicKey GetPubKeyShare(size_t memberIdx) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    CBLSSecretKey GetSkShare() const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);

private:
    bool HasVerificationVectorInternal() const EXCLUSIVE_LOCKS_REQUIRED(cs_vvec_shShare);
    void WriteContributions(CDBWrapper& db) const EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
    bool ReadContributions(const CDBWrapper& db) EXCLUSIVE_LOCKS_REQUIRED(!cs_vvec_shShare);
};
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
