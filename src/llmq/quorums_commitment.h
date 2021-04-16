// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_COMMITMENT_H
#define BITCOIN_LLMQ_QUORUMS_COMMITMENT_H

#include <llmq/quorums_utils.h>
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
    static const uint16_t CURRENT_VERSION = 1;

public:
    uint16_t nVersion{CURRENT_VERSION};
    Consensus::LLMQType llmqType{Consensus::LLMQ_NONE};
    uint256 quorumHash;
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

    bool Verify(const CBlockIndex* pQuorumIndex, bool checkSigs) const;
    bool VerifyNull() const;
    bool VerifySizes(const Consensus::LLMQParams& params) const;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nVersion);
        READWRITE(llmqType);
        READWRITE(quorumHash);
        READWRITE(DYNBITSET(signers));
        READWRITE(DYNBITSET(validMembers));
        READWRITE(quorumPublicKey);
        READWRITE(quorumVvecHash);
        READWRITE(quorumSig);
        READWRITE(membersSig);
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
typedef std::shared_ptr<CFinalCommitment> CFinalCommitmentPtr;

class CFinalCommitmentTxPayload
{
public:
    static const uint16_t CURRENT_VERSION = 1;

public:
    uint16_t nVersion{CURRENT_VERSION};
    uint32_t nHeight{(uint32_t)-1};
    CFinalCommitment commitment;

public:
    ADD_SERIALIZE_METHODS

    template<typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nVersion);
        READWRITE(nHeight);
        READWRITE(commitment);
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

#endif // BITCOIN_LLMQ_QUORUMS_COMMITMENT_H
