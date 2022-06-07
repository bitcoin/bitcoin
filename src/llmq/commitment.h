// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_COMMITMENT_H
#define BITCOIN_LLMQ_COMMITMENT_H

#include <llmq/utils.h>
#include <bls/bls.h>

#include <univalue.h>

class CValidationState;

namespace llmq
{

// This message is an aggregation of all received premature commitments and only valid if
// enough (>=threshold) premature commitments were aggregated
// This is mined on-chain as part of TRANSACTION_QUORUM_COMMITMENT
class CFinalCommitment
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;
    static constexpr uint16_t INDEXED_QUORUM_VERSION = 2;

public:
    uint16_t nVersion{CURRENT_VERSION};
    Consensus::LLMQType llmqType{Consensus::LLMQType::LLMQ_NONE};
    uint256 quorumHash;
    int16_t quorumIndex{0};
    std::vector<bool> signers;
    std::vector<bool> validMembers;

    CBLSPublicKey quorumPublicKey;
    uint256 quorumVvecHash;

    CBLSSignature quorumSig; // recovered threshold sig of blockHash+validMembers+pubKeyHash+vvecHash
    CBLSSignature membersSig; // aggregated member sig of blockHash+validMembers+pubKeyHash+vvecHash

public:
    CFinalCommitment() = default;
    CFinalCommitment(const Consensus::LLMQParams& params, const uint256& _quorumHash);

    int CountSigners() const
    {
        return (int)std::count(signers.begin(), signers.end(), true);
    }
    int CountValidMembers() const
    {
        return (int)std::count(validMembers.begin(), validMembers.end(), true);
    }

    bool Verify(const CBlockIndex* pQuorumBaseBlockIndex, bool checkSigs) const;
    bool VerifyNull() const;
    bool VerifySizes(const Consensus::LLMQParams& params) const;

public:
    SERIALIZE_METHODS(CFinalCommitment, obj)
    {
        READWRITE(
                obj.nVersion,
                obj.llmqType,
                obj.quorumHash
                );

        int16_t _quorumIndex = 0;
        SER_WRITE(obj, _quorumIndex = obj.quorumIndex);
        if (obj.nVersion == CFinalCommitment::INDEXED_QUORUM_VERSION) {
            READWRITE(_quorumIndex);
        }
        SER_READ(obj, obj.quorumIndex = _quorumIndex);

        READWRITE(
                DYNBITSET(obj.signers),
                DYNBITSET(obj.validMembers),
                obj.quorumPublicKey,
                obj.quorumVvecHash,
                obj.quorumSig,
                obj.membersSig
                );
    }

public:
    bool IsNull() const
    {
        if (std::count(signers.begin(), signers.end(), true) ||
            std::count(validMembers.begin(), validMembers.end(), true)) {
            return false;
        }
        if (quorumPublicKey.IsValid() ||
            !quorumVvecHash.IsNull() ||
            membersSig.IsValid() ||
            quorumSig.IsValid()) {
            return false;
        }
        return true;
    }

    void ToJson(UniValue& obj) const
    {
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("llmqType", (int)llmqType);
        obj.pushKV("quorumHash", quorumHash.ToString());
        obj.pushKV("quorumIndex", quorumIndex);
        obj.pushKV("signersCount", CountSigners());
        obj.pushKV("signers", CLLMQUtils::ToHexStr(signers));
        obj.pushKV("validMembersCount", CountValidMembers());
        obj.pushKV("validMembers", CLLMQUtils::ToHexStr(validMembers));
        obj.pushKV("quorumPublicKey", quorumPublicKey.ToString());
        obj.pushKV("quorumVvecHash", quorumVvecHash.ToString());
        obj.pushKV("quorumSig", quorumSig.ToString());
        obj.pushKV("membersSig", membersSig.ToString());
    }
};
using CFinalCommitmentPtr = std::unique_ptr<CFinalCommitment>;

class CFinalCommitmentTxPayload
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_QUORUM_COMMITMENT;
    static constexpr uint16_t CURRENT_VERSION = 1;
public:
    uint16_t nVersion{CURRENT_VERSION};
    uint32_t nHeight{std::numeric_limits<uint32_t>::max()};
    CFinalCommitment commitment;

public:
    SERIALIZE_METHODS(CFinalCommitmentTxPayload, obj)
    {
        READWRITE(obj.nVersion, obj.nHeight, obj.commitment);
    }

    void ToJson(UniValue& obj) const
    {
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("height", (int)nHeight);

        UniValue qcObj;
        commitment.ToJson(qcObj);
        obj.pushKV("commitment", qcObj);
    }
};

bool CheckLLMQCommitment(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state);

} // namespace llmq

#endif // BITCOIN_LLMQ_COMMITMENT_H
