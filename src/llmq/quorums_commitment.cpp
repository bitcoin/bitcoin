// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_commitment.h"
#include "quorums_utils.h"

#include "chainparams.h"

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

bool CFinalCommitment::Verify(const std::vector<CDeterministicMNCPtr>& members) const
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

    uint256 commitmentHash = CLLMQUtils::BuildCommitmentHash((uint8_t)params.type, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);

    std::vector<CBLSPublicKey> memberPubKeys;
    for (size_t i = 0; i < members.size(); i++) {
        if (!signers[i]) {
            continue;
        }
        memberPubKeys.emplace_back(members[i]->pdmnState->pubKeyOperator);
    }

    if (!membersSig.VerifySecureAggregated(memberPubKeys, commitmentHash)) {
        LogPrintfFinalCommitment("invalid aggregated members signature\n");
        return false;
    }

    if (!quorumSig.VerifyInsecure(quorumPublicKey, commitmentHash)) {
        LogPrintfFinalCommitment("invalid quorum signature\n");
        return false;
    }

    return true;
}

bool CFinalCommitment::VerifyNull(int nHeight) const
{
    if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)llmqType)) {
        LogPrintfFinalCommitment("invalid llmqType=%d\n", llmqType);
        return false;
    }
    const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)llmqType);

    if (!IsNull() || !VerifySizes(params)) {
        return false;
    }

    uint256 expectedQuorumVvecHash = ::SerializeHash(std::make_pair(quorumHash, nHeight));
    if (quorumVvecHash != expectedQuorumVvecHash) {
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

}
