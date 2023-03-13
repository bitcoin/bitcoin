// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <evo/mnhftx.h>
#include <evo/specialtx.h>
#include <llmq/commitment.h>
#include <llmq/signing.h>
#include <llmq/utils.h>
#include <llmq/quorums.h>

#include <chain.h>
#include <chainparams.h>
#include <validation.h>

#include <string>

extern const std::string CBLSIG_REQUESTID_PREFIX = "clsig";

bool MNHFTx::Verify(const CBlockIndex* pQuorumIndex) const
{
    if (nVersion == 0 || nVersion > (llmq::utils::IsV19Active(pQuorumIndex) ? BASIC_BLS_VERSION : LEGACY_BLS_VERSION)) {
        return false;
    }

    Consensus::LLMQType llmqType = Params().GetConsensus().llmqTypeMnhf;
    const auto& llmq_params_opt = llmq::GetLLMQParams(llmqType);
    assert(llmq_params_opt.has_value());
    int signOffset{llmq_params_opt->dkgInterval};

    const uint256 requestId = ::SerializeHash(std::make_pair(CBLSIG_REQUESTID_PREFIX, pQuorumIndex->nHeight));
    return llmq::CSigningManager::VerifyRecoveredSig(llmqType, *llmq::quorumManager, pQuorumIndex->nHeight, requestId, pQuorumIndex->GetBlockHash(), sig, 0) ||
           llmq::CSigningManager::VerifyRecoveredSig(llmqType, *llmq::quorumManager, pQuorumIndex->nHeight, requestId, pQuorumIndex->GetBlockHash(), sig, signOffset);
}

bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    MNHFTxPayload mnhfTx;
    if (!GetTxPayload(tx, mnhfTx)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-payload");
    }

    if (mnhfTx.nVersion == 0 || mnhfTx.nVersion > MNHFTxPayload::CURRENT_VERSION) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-version");
    }

    const CBlockIndex* pindexQuorum = LookupBlockIndex(mnhfTx.signal.quorumHash);
    if (!pindexQuorum) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-quorum-hash");
    }

    if (pindexQuorum != pindexPrev->GetAncestor(pindexQuorum->nHeight)) {
        // not part of active chain
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-quorum-hash");
    }

    if (!llmq::GetLLMQParams(Params().GetConsensus().llmqTypeMnhf).has_value()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-type");
    }

    if (!mnhfTx.signal.Verify(pindexQuorum)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-mnhf-invalid");
    }

    return true;
}

std::string MNHFTx::ToString() const
{
    return strprintf("MNHFTx(nVersion=%d, quorumHash=%s, sig=%s)",
                     nVersion, quorumHash.ToString(), sig.ToString());
}

