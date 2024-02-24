// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/commitment.h>

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>

#include <chainparams.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <logging.h>
#include <validation.h>
#include <util/underlying.h>

namespace llmq
{

CFinalCommitment::CFinalCommitment(const Consensus::LLMQParams& params, const uint256& _quorumHash) :
        llmqType(params.type),
        quorumHash(_quorumHash),
        signers(params.size),
        validMembers(params.size)
{
}

template<typename... Types>
void LogPrintfFinalCommitment(Types... out) {
    if (LogAcceptCategory(BCLog::LLMQ)) {
        LogInstance().LogPrintStr(strprintf("CFinalCommitment::%s -- %s", __func__, tinyformat::format(out...)));
    }
}

bool CFinalCommitment::Verify(gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex, bool checkSigs) const
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintfFinalCommitment("q[%s] invalid llmqType=%d\n", quorumHash.ToString(), ToUnderlying(llmqType));
        return false;
    }
    const auto& llmq_params = llmq_params_opt.value();

    const uint16_t expected_nversion{CFinalCommitment::GetVersion(IsQuorumRotationEnabled(llmq_params, pQuorumBaseBlockIndex),
            DeploymentActiveAfter(pQuorumBaseBlockIndex, Params().GetConsensus(), Consensus::DEPLOYMENT_V19))};
    if (nVersion == 0 || nVersion != expected_nversion) {
        LogPrintfFinalCommitment("q[%s] invalid nVersion=%d expectednVersion\n", quorumHash.ToString(), nVersion, expected_nversion);
        return false;
    }

    if (pQuorumBaseBlockIndex->GetBlockHash() != quorumHash) {
        LogPrintfFinalCommitment("q[%s] invalid quorumHash\n", quorumHash.ToString());
        return false;
    }

    if ((pQuorumBaseBlockIndex->nHeight % llmq_params.dkgInterval) != quorumIndex) {
        LogPrintfFinalCommitment("q[%s] invalid quorumIndex=%d\n", quorumHash.ToString(), quorumIndex);
        return false;
    }

    if (!VerifySizes(llmq_params)) {
        return false;
    }

    if (CountValidMembers() < llmq_params.minSize) {
        LogPrintfFinalCommitment("q[%s] invalid validMembers count. validMembersCount=%d\n", quorumHash.ToString(), CountValidMembers());
        return false;
    }
    if (CountSigners() < llmq_params.minSize) {
        LogPrintfFinalCommitment("q[%s] invalid signers count. signersCount=%d\n", quorumHash.ToString(), CountSigners());
        return false;
    }
    if (!quorumPublicKey.IsValid()) {
        LogPrintfFinalCommitment("q[%s] invalid quorumPublicKey\n", quorumHash.ToString());
        return false;
    }
    if (quorumVvecHash.IsNull()) {
        LogPrintfFinalCommitment("q[%s] invalid quorumVvecHash\n", quorumHash.ToString());
        return false;
    }
    if (!membersSig.IsValid()) {
        LogPrintfFinalCommitment("q[%s] invalid membersSig\n", quorumHash.ToString());
        return false;
    }
    if (!quorumSig.IsValid()) {
        LogPrintfFinalCommitment("q[%s] invalid vvecSig\n");
        return false;
    }
    auto members = utils::GetAllQuorumMembers(llmqType, pQuorumBaseBlockIndex);
    if (LogAcceptCategory(BCLog::LLMQ)) {
        std::stringstream ss;
        std::stringstream ss2;
        for (const auto i: irange::range(llmq_params.size)) {
            ss << "v[" << i << "]=" << validMembers[i];
            ss2 << "s[" << i << "]=" << signers[i];
        }
        LogPrintfFinalCommitment("CFinalCommitment::%s mns[%d] validMembers[%s] signers[%s]\n", __func__, members.size(), ss.str(), ss2.str());
    }

    for (const auto i : irange::range(members.size(), size_t(llmq_params.size))) {
        if (validMembers[i]) {
            LogPrintfFinalCommitment("q[%s] invalid validMembers bitset. bit %d should not be set\n", quorumHash.ToString(), i);
            return false;
        }
        if (signers[i]) {
            LogPrintfFinalCommitment("q[%s] invalid signers bitset. bit %d should not be set\n", quorumHash.ToString(), i);
            return false;
        }
    }

    // sigs are only checked when the block is processed
    if (checkSigs) {
        uint256 commitmentHash = BuildCommitmentHash(llmq_params.type, quorumHash, validMembers, quorumPublicKey, quorumVvecHash);
        if (LogAcceptCategory(BCLog::LLMQ)) {
            std::stringstream ss3;
            for (const auto &mn: members) {
                ss3 << mn->proTxHash.ToString().substr(0, 4) << " | ";
            }
            LogPrintfFinalCommitment("CFinalCommitment::%s members[%s] quorumPublicKey[%s] commitmentHash[%s]\n",
                                     __func__, ss3.str(), quorumPublicKey.ToString(), commitmentHash.ToString());
        }
        std::vector<CBLSPublicKey> memberPubKeys;
        for (const auto i : irange::range(members.size())) {
            if (!signers[i]) {
                continue;
            }
            memberPubKeys.emplace_back(members[i]->pdmnState->pubKeyOperator.Get());
        }

        if (!membersSig.VerifySecureAggregated(memberPubKeys, commitmentHash)) {
            LogPrintfFinalCommitment("q[%s] invalid aggregated members signature\n", quorumHash.ToString());
            return false;
        }

        if (!quorumSig.VerifyInsecure(quorumPublicKey, commitmentHash)) {
            LogPrintfFinalCommitment("q[%s] invalid quorum signature\n", quorumHash.ToString());
            return false;
        }
    }

    LogPrintfFinalCommitment("q[%s] VALID\n", quorumHash.ToString());

    return true;
}

bool CFinalCommitment::VerifyNull() const
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintfFinalCommitment("q[%s]invalid llmqType=%d\n", quorumHash.ToString(), ToUnderlying(llmqType));
        return false;
    }

    if (!IsNull() || !VerifySizes(llmq_params_opt.value())) {
        return false;
    }

    return true;
}

bool CFinalCommitment::VerifySizes(const Consensus::LLMQParams& params) const
{
    if (signers.size() != size_t(params.size)) {
        LogPrintfFinalCommitment("q[%s] invalid signers.size=%d\n", quorumHash.ToString(), signers.size());
        return false;
    }
    if (validMembers.size() != size_t(params.size)) {
        LogPrintfFinalCommitment("q[%s] invalid signers.size=%d\n", quorumHash.ToString(), signers.size());
        return false;
    }
    return true;
}

bool CheckLLMQCommitment(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, TxValidationState& state)
{
    const auto opt_qcTx = GetTxPayload<CFinalCommitmentTxPayload>(tx);
    if (!opt_qcTx) {
        LogPrintfFinalCommitment("h[%d] GetTxPayload failed\n", pindexPrev->nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-payload");
    }
    auto& qcTx = *opt_qcTx;

    const auto& llmq_params_opt = Params().GetLLMQ(qcTx.commitment.llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrintfFinalCommitment("h[%d] GetLLMQ failed for llmqType[%d]\n", pindexPrev->nHeight, ToUnderlying(qcTx.commitment.llmqType));
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-commitment-type");
    }

    if (LogAcceptCategory(BCLog::LLMQ)) {
        std::stringstream ss;
        for (const auto i: irange::range(llmq_params_opt->size)) {
            ss << "v[" << i << "]=" << qcTx.commitment.validMembers[i];
        }
        LogPrintfFinalCommitment("%s llmqType[%d] validMembers[%s] signers[]\n", __func__,
                                 int(qcTx.commitment.llmqType), ss.str());
    }

    if (qcTx.nVersion == 0 || qcTx.nVersion > CFinalCommitmentTxPayload::CURRENT_VERSION) {
        LogPrintfFinalCommitment("h[%d] invalid qcTx.nVersion[%d]\n", pindexPrev->nHeight, qcTx.nVersion);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-version");
    }

    if (qcTx.nHeight != uint32_t(pindexPrev->nHeight + 1)) {
        LogPrintfFinalCommitment("h[%d] invalid qcTx.nHeight[%d]\n", pindexPrev->nHeight, qcTx.nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-height");
    }

    const CBlockIndex* pQuorumBaseBlockIndex = WITH_LOCK(cs_main, return g_chainman.m_blockman.LookupBlockIndex(qcTx.commitment.quorumHash));
    if (pQuorumBaseBlockIndex == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-quorum-hash");
    }


    if (pQuorumBaseBlockIndex != pindexPrev->GetAncestor(pQuorumBaseBlockIndex->nHeight)) {
        // not part of active chain
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-quorum-hash");
    }

    if (qcTx.commitment.IsNull()) {
        if (!qcTx.commitment.VerifyNull()) {
            LogPrintfFinalCommitment("h[%d] invalid qcTx.commitment[%s] VerifyNull failed\n", pindexPrev->nHeight, qcTx.commitment.quorumHash.ToString());
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-invalid-null");
        }
        return true;
    }

    if (!qcTx.commitment.Verify(pQuorumBaseBlockIndex, false)) {
        LogPrintfFinalCommitment("h[%d] invalid qcTx.commitment[%s] Verify failed\n", pindexPrev->nHeight, qcTx.commitment.quorumHash.ToString());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-invalid");
    }

    LogPrintfFinalCommitment("h[%d] CheckLLMQCommitment VALID\n", pindexPrev->nHeight);

    return true;
}

uint256 BuildCommitmentHash(Consensus::LLMQType llmqType, const uint256& blockHash,
                                        const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey,
                                        const uint256& vvecHash)
{
    CHashWriter hw(SER_GETHASH, 0);
    hw << llmqType;
    hw << blockHash;
    hw << DYNBITSET(validMembers);
    hw << pubKey;
    hw << vvecHash;
    return hw.GetHash();
}

} // namespace llmq
