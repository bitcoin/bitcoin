// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>

#include <chainparams.h>
#include <validation.h>

#include <evo/cbtx.h>
#include <logging.h>
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
    LogInstance().LogPrintStr(strprintf("CFinalCommitment::%s -- %s", __func__, tinyformat::format(__VA_ARGS__)), __func__, "", 0); \
} while(0)

bool CFinalCommitment::Verify(const CBlockIndex* pQuorumIndex, bool checkSigs) const
{
    if (nVersion == 0 || nVersion > CURRENT_VERSION) {
        return false;
    }

    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        LogPrintfFinalCommitment("invalid llmqType=%d\n", llmqType);
        return false;
    }
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

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
    std::vector<CDeterministicMNCPtr> members;
    CLLMQUtils::GetAllQuorumMembers(llmqType, pQuorumIndex, members);

    for (size_t i = members.size(); i < (size_t)params.size; i++) {
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
        uint256 commitmentHash = CLLMQUtils::BuildCommitmentHash(params.type, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);

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
    if (!Params().GetConsensus().llmqs.count(llmqType)) {
        LogPrintfFinalCommitment("invalid llmqType=%d\n", llmqType);
        return false;
    }
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);

    if (!IsNull() || !VerifySizes(params)) {
        return false;
    }

    return true;
}

bool CFinalCommitment::VerifySizes(const Consensus::LLMQParams& params) const
{
    if (signers.size() != (size_t)params.size) {
        LogPrintfFinalCommitment("invalid signers.size=%d\n", signers.size());
        return false;
    }
    if (validMembers.size() != (size_t)params.size) {
        LogPrintfFinalCommitment("invalid signers.size=%d\n", signers.size());
        return false;
    }
    return true;
}

bool CheckLLMQCommitment(BlockManager &blockman, const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state, bool fJustCheck)
{
    AssertLockHeld(cs_main);
    if (!tx.IsCoinBase()) {
        return FormatSyscoinErrorMessage(state, "bad-qctx-invalid", fJustCheck);
    }
    CFinalCommitmentTxPayload qcTx;
    if (!GetTxPayload(tx, qcTx)) {
        return FormatSyscoinErrorMessage(state, "bad-qc-payload", fJustCheck);
    }
    
    if (!CheckCbTx(qcTx.cbTx, pindexPrev, state, fJustCheck)) {
        return FormatSyscoinErrorMessage(state, "bad-qc-cbtx", fJustCheck);
    }
    for(const auto& commitment: qcTx.commitments) {
        const CBlockIndex* pindexQuorum = blockman.LookupBlockIndex(commitment.quorumHash);
        if(!pindexQuorum) {
            return FormatSyscoinErrorMessage(state, "bad-qc-quorum-hash", fJustCheck);
        }
        if (pindexQuorum != pindexPrev->GetAncestor(pindexQuorum->nHeight)) {
            // not part of active chain
            return FormatSyscoinErrorMessage(state, "bad-qc-quorum-hash", fJustCheck);
        }

        if (!Params().GetConsensus().llmqs.count(commitment.llmqType)) {
            return FormatSyscoinErrorMessage(state, "bad-qc-type", fJustCheck);
        }

        if (commitment.IsNull()) {
            if (!commitment.VerifyNull()) {
                return FormatSyscoinErrorMessage(state, "bad-qc-invalid-null", fJustCheck);
            }
            return true;
        }
        if (!commitment.Verify(pindexQuorum, false)) {
            return FormatSyscoinErrorMessage(state, "bad-qc-invalid", fJustCheck);
        }
    }

    return true;
}

} // namespace llmq
