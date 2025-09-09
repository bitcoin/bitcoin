// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_COMMITMENT_H
#define BITCOIN_LLMQ_COMMITMENT_H

#include <bls/bls.h>
#include <llmq/params.h>
#include <llmq/types.h>

#include <primitives/transaction.h>
#include <util/irange.h>
#include <util/strencodings.h>
#include <util/underlying.h>

#include <gsl/pointers.h>
#include <univalue.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <vector>

class CBlockIndex;
class CDeterministicMNManager;
class ChainstateManager;
class TxValidationState;
template <typename T>
class CCheckQueueControl;

namespace llmq
{
class CQuorumSnapshotManager;

namespace utils {
struct BlsCheck;
} // namespace utils
// This message is an aggregation of all received premature commitments and only valid if
// enough (>=threshold) premature commitments were aggregated
// This is mined on-chain as part of TRANSACTION_QUORUM_COMMITMENT
class CFinalCommitment
{
public:
    static constexpr uint16_t LEGACY_BLS_NON_INDEXED_QUORUM_VERSION = 1;
    static constexpr uint16_t LEGACY_BLS_INDEXED_QUORUM_VERSION = 2;
    static constexpr uint16_t BASIC_BLS_NON_INDEXED_QUORUM_VERSION = 3;
    static constexpr uint16_t BASIC_BLS_INDEXED_QUORUM_VERSION = 4;

    uint16_t nVersion{LEGACY_BLS_NON_INDEXED_QUORUM_VERSION};
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
        return int(std::count(signers.begin(), signers.end(), true));
    }
    int CountValidMembers() const
    {
        return int(std::count(validMembers.begin(), validMembers.end(), true));
    }

    bool VerifySignatureAsync(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                              gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                              CCheckQueueControl<utils::BlsCheck>* queue_control) const;
    bool Verify(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool checkSigs) const;
    bool VerifyNull() const;
    bool VerifySizes(const Consensus::LLMQParams& params) const;

    [[nodiscard]] static constexpr uint16_t GetVersion(const bool is_rotation_enabled, const bool is_basic_scheme_active)
    {
        if (is_rotation_enabled)
            return is_basic_scheme_active ? BASIC_BLS_INDEXED_QUORUM_VERSION : LEGACY_BLS_INDEXED_QUORUM_VERSION;
        else
            return is_basic_scheme_active ? BASIC_BLS_NON_INDEXED_QUORUM_VERSION : LEGACY_BLS_NON_INDEXED_QUORUM_VERSION;
    }

public:

    SERIALIZE_METHODS(CFinalCommitment, obj)
    {
        READWRITE(
                obj.nVersion,
                obj.llmqType,
                obj.quorumHash
        );
        if (obj.nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION || obj.nVersion == BASIC_BLS_INDEXED_QUORUM_VERSION) {
            READWRITE(
                    obj.quorumIndex
            );
        }
        READWRITE(
                DYNBITSET(obj.signers),
                DYNBITSET(obj.validMembers),
                CBLSPublicKeyVersionWrapper(const_cast<CBLSPublicKey&>(obj.quorumPublicKey), (obj.nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || obj.nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION)),
                obj.quorumVvecHash,
                CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.quorumSig), (obj.nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || obj.nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION)),
                CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.membersSig), (obj.nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || obj.nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION))
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

    [[nodiscard]] UniValue ToJson() const
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("version", nVersion);
        obj.pushKV("llmqType", ToUnderlying(llmqType));
        obj.pushKV("quorumHash", quorumHash.ToString());
        obj.pushKV("quorumIndex", quorumIndex);
        obj.pushKV("signersCount", CountSigners());
        obj.pushKV("signers", BitsVectorToHexStr(signers));
        obj.pushKV("validMembersCount", CountValidMembers());
        obj.pushKV("validMembers", BitsVectorToHexStr(validMembers));
        obj.pushKV("quorumPublicKey", quorumPublicKey.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
        obj.pushKV("quorumVvecHash", quorumVvecHash.ToString());
        obj.pushKV("quorumSig", quorumSig.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
        obj.pushKV("membersSig", membersSig.ToString(nVersion == LEGACY_BLS_NON_INDEXED_QUORUM_VERSION || nVersion == LEGACY_BLS_INDEXED_QUORUM_VERSION));
        return obj;
    }

private:
    static std::string BitsVectorToHexStr(const std::vector<bool>& vBits)
    {
        std::vector<uint8_t> vBytes((vBits.size() + 7) / 8);
        for (const auto i : irange::range(vBits.size())) {
            vBytes[i / 8] |= vBits[i] << (i % 8);
        }
        return HexStr(vBytes);
    }
};

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

    [[nodiscard]] UniValue ToJson() const;
};

bool CheckLLMQCommitment(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                         const ChainstateManager& chainman, const CTransaction& tx,
                         gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state);

uint256 BuildCommitmentHash(Consensus::LLMQType llmqType, const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash);

} // namespace llmq

#endif // BITCOIN_LLMQ_COMMITMENT_H
