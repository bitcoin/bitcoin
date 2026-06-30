// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_DKGMESSAGES_H
#define BITCOIN_LLMQ_DKGMESSAGES_H

#include <llmq/commitment.h>

#include <bls/bls_ies.h>
#include <hash.h>
#include <serialize.h>
#include <util/underlying.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace llmq {
class CDKGContribution
{
public:
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint256 proTxHash;
    BLSVerificationVectorPtr vvec;
    std::shared_ptr<CBLSIESMultiRecipientObjects<CBLSSecretKey>> contributions;
    CBLSSignature sig;

public:
    template <typename Stream>
    inline void SerializeWithoutSig(Stream& s) const
    {
        s << ToUnderlying(llmqType);
        s << quorumHash;
        s << proTxHash;
        s << *vvec;
        s << *contributions;
    }
    template <typename Stream>
    inline void Serialize(Stream& s) const
    {
        SerializeWithoutSig(s);
        s << sig;
    }
    template <typename Stream>
    inline void Unserialize(Stream& s)
    {
        std::vector<CBLSPublicKey> tmp1;
        CBLSIESMultiRecipientObjects<CBLSSecretKey> tmp2;

        s >> llmqType;
        s >> quorumHash;
        s >> proTxHash;
        s >> tmp1;
        s >> tmp2;
        s >> sig;

        vvec = std::make_shared<std::vector<CBLSPublicKey>>(std::move(tmp1));
        contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>(std::move(tmp2));
    }

    [[nodiscard]] uint256 GetSignHash() const
    {
        CHashWriter hw(SER_GETHASH, 0);
        SerializeWithoutSig(hw);
        hw << CBLSSignature();
        return hw.GetHash();
    }
};

class CDKGComplaint
{
public:
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> badMembers;
    std::vector<bool> complainForMembers;
    CBLSSignature sig;

public:
    CDKGComplaint() = default;
    explicit CDKGComplaint(const Consensus::LLMQParams& params) :
            badMembers((size_t)params.size), complainForMembers((size_t)params.size) {};

    SERIALIZE_METHODS(CDKGComplaint, obj)
    {
        READWRITE(
                obj.llmqType,
                obj.quorumHash,
                obj.proTxHash,
                DYNBITSET(obj.badMembers),
                DYNBITSET(obj.complainForMembers),
                obj.sig
                );
    }

    [[nodiscard]] uint256 GetSignHash() const
    {
        CDKGComplaint tmp(*this);
        tmp.sig = CBLSSignature();
        return ::SerializeHash(tmp);
    }
};

class CDKGJustification
{
public:
    Consensus::LLMQType llmqType;
    uint256 quorumHash;
    uint256 proTxHash;
    struct Contribution {
        uint32_t index;
        CBLSSecretKey key;
        SERIALIZE_METHODS(Contribution, obj)
        {
            READWRITE(obj.index, obj.key);
        }
    };
    std::vector<Contribution> contributions;
    CBLSSignature sig;

public:
    SERIALIZE_METHODS(CDKGJustification, obj)
    {
        READWRITE(obj.llmqType, obj.quorumHash, obj.proTxHash, obj.contributions, obj.sig);
    }

    [[nodiscard]] uint256 GetSignHash() const
    {
        CDKGJustification tmp(*this);
        tmp.sig = CBLSSignature();
        return ::SerializeHash(tmp);
    }
};

// each member commits to a single set of valid members with this message
// then each node aggregate all received premature commitments
// into a single CFinalCommitment, which is only valid if
// enough (>=minSize) premature commitments were aggregated
class CDKGPrematureCommitment
{
public:
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    uint256 proTxHash;
    std::vector<bool> validMembers;

    CBLSPublicKey quorumPublicKey;
    uint256 quorumVvecHash;

    CBLSSignature quorumSig; // threshold sig share of quorumHash+validMembers+pubKeyHash+vvecHash
    CBLSSignature sig; // single member sig of quorumHash+validMembers+pubKeyHash+vvecHash

public:
    CDKGPrematureCommitment() = default;
    explicit CDKGPrematureCommitment(const Consensus::LLMQParams& params) :
            validMembers((size_t)params.size) {};

    [[nodiscard]] int CountValidMembers() const
    {
        return int(std::count(validMembers.begin(), validMembers.end(), true));
    }

public:
    SERIALIZE_METHODS(CDKGPrematureCommitment, obj)
    {
        READWRITE(
                obj.llmqType,
                obj.quorumHash,
                obj.proTxHash,
                DYNBITSET(obj.validMembers),
                obj.quorumPublicKey,
                obj.quorumVvecHash,
                obj.quorumSig,
                obj.sig
                );
    }

    [[nodiscard]] uint256 GetSignHash() const
    {
        return BuildCommitmentHash(llmqType, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);
    }
};
} // namespace llmq

#endif // BITCOIN_LLMQ_DKGMESSAGES_H
