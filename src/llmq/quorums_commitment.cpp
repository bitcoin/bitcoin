// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_commitment.h"
#include "quorums_utils.h"

#include "chainparams.h"
#include "validation.h"

#include "evo/specialtx.h"

#include <univalue.h>

namespace llmq
{

CFinalCommitment::CFinalCommitment(const Consensus::LLMQParams& params, const uint256& _quorumHash) :
        llmqType(params.type),
        quorumHash(_quorumHash),
        signers(params.size),
        validMembers(params.size)
{
}

#define LogPrintfFinalCommitment(...) do { \
    LogPrintStr(strprintf("CFinalCommitment::%s -- %s", __func__, tinyformat::format(__VA_ARGS__))); \
} while(0)

bool CFinalCommitment::Verify(const std::vector<CDeterministicMNCPtr>& members, bool checkSigs) const
{
    if (nVersion == 0 || nVersion > CURRENT_VERSION) {
        return false;
    }

    if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)llmqType)) {
        LogPrintfFinalCommitment("invalid llmqType=%d\n", llmqType);
        return false;
    }
    const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)llmqType);

    if (!VerifySizes(params)) {
        return false;
    }

    if (CountValidMembers() < params.minSize) {
        LogPrintfFinalCommitment("invalid validMembers count. validMembersCount=%d\n", CountValidMembers());
        return false;
    }
    if (CountSigners() < params.minSize) {
        LogPrintfFinalCommitment("invalid signers count. signersCount=%d\n", CountSigners());
        return false;
    }
    if (!quorumPublicKey.IsValid()) {
        LogPrintfFinalCommitment("invalid quorumPublicKey\n");
        return false;
    }
    if (quorumVvecHash.IsNull()) {
        LogPrintfFinalCommitment("invalid quorumVvecHash\n");
        return false;
    }
    if (!membersSig.IsValid()) {
        LogPrintfFinalCommitment("invalid membersSig\n");
        return false;
    }
    if (!quorumSig.IsValid()) {
        LogPrintfFinalCommitment("invalid vvecSig\n");
        return false;
    }

    for (size_t i = members.size(); i < params.size; i++) {
        if (validMembers[i]) {
            LogPrintfFinalCommitment("invalid validMembers bitset. bit %d should not be set\n", i);
            return false;
        }
        if (signers[i]) {
            LogPrintfFinalCommitment("invalid signers bitset. bit %d should not be set\n", i);
            return false;
        }
    }

    // sigs are only checked when the block is processed
    if (checkSigs) {
        uint256 commitmentHash = CLLMQUtils::BuildCommitmentHash((uint8_t)params.type, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);

        std::vector<CBLSPublicKey> memberPubKeys;
        for (size_t i = 0; i < members.size(); i++) {
            if (!signers[i]) {
                continue;
            }
            memberPubKeys.emplace_back(members[i]->pdmnState->pubKeyOperator.Get());
        }

        if (!membersSig.VerifySecureAggregated(memberPubKeys, commitmentHash)) {
            LogPrintfFinalCommitment("invalid aggregated members signature\n");
            return false;
        }

        if (!quorumSig.VerifyInsecure(quorumPublicKey, commitmentHash)) {
            LogPrintfFinalCommitment("invalid quorum signature\n");
            return false;
        }
    }

    return true;
}

bool CFinalCommitment::VerifyNull() const
{
    if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)llmqType)) {
        LogPrintfFinalCommitment("invalid llmqType=%d\n", llmqType);
        return false;
    }
    const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)llmqType);

    if (!IsNull() || !VerifySizes(params)) {
        return false;
    }

    return true;
}

bool CFinalCommitment::VerifySizes(const Consensus::LLMQParams& params) const
{
    if (signers.size() != params.size) {
        LogPrintfFinalCommitment("invalid signers.size=%d\n", signers.size());
        return false;
    }
    if (validMembers.size() != params.size) {
        LogPrintfFinalCommitment("invalid signers.size=%d\n", signers.size());
        return false;
    }
    return true;
}

void CFinalCommitment::ToJson(UniValue& obj) const
{
    obj.setObject();
    obj.push_back(Pair("version", (int)nVersion));
    obj.push_back(Pair("llmqType", (int)llmqType));
    obj.push_back(Pair("quorumHash", quorumHash.ToString()));
    obj.push_back(Pair("signersCount", CountSigners()));
    obj.push_back(Pair("validMembersCount", CountValidMembers()));
    obj.push_back(Pair("quorumPublicKey", quorumPublicKey.ToString()));
}

void CFinalCommitmentTxPayload::ToJson(UniValue& obj) const
{
    obj.setObject();
    obj.push_back(Pair("version", (int)nVersion));
    obj.push_back(Pair("height", (int)nHeight));

    UniValue qcObj;
    commitment.ToJson(qcObj);
    obj.push_back(Pair("commitment", qcObj));
}

bool CheckLLMQCommitment(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    CFinalCommitmentTxPayload qcTx;
    if (!GetTxPayload(tx, qcTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-payload");
    }

    if (qcTx.nVersion == 0 || qcTx.nVersion > CFinalCommitmentTxPayload::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-version");
    }

    if (qcTx.nHeight != pindexPrev->nHeight + 1) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-height");
    }

    if (!mapBlockIndex.count(qcTx.commitment.quorumHash)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-quorum-hash");
    }

    const CBlockIndex* pindexQuorum = mapBlockIndex[qcTx.commitment.quorumHash];

    if (pindexQuorum != pindexPrev->GetAncestor(pindexQuorum->nHeight)) {
        // not part of active chain
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-quorum-hash");
    }

    if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)qcTx.commitment.llmqType)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-type");
    }
    const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)qcTx.commitment.llmqType);

    if (qcTx.commitment.IsNull()) {
        if (!qcTx.commitment.VerifyNull()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid-null");
        }
        return true;
    }

    auto members = CLLMQUtils::GetAllQuorumMembers(params.type, qcTx.commitment.quorumHash);
    if (!qcTx.commitment.Verify(members, false)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid");
    }

    return true;
}

}
