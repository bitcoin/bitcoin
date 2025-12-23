// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/commitment.h>

#include <evo/deterministicmns.h>
#include <evo/specialtx.h>

#include <chainparams.h>
#include <checkqueue.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <logging.h>
#include <util/underlying.h>
#include <validation.h>

namespace llmq
{

CFinalCommitment::CFinalCommitment(const Consensus::LLMQParams& params, const uint256& _quorumHash) :
        llmqType(params.type),
        quorumHash(_quorumHash),
        signers(params.size),
        validMembers(params.size)
{
}

bool CFinalCommitment::VerifySignatureAsync(const llmq::UtilParameters& util_params,
                                            CCheckQueueControl<utils::BlsCheck>* queue_control) const
{
    auto members = utils::GetAllQuorumMembers(llmqType, util_params);
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid llmqType=%d\n", quorumHash.ToString(),
                 ToUnderlying(llmqType));
        return false;
    }
    const auto& llmq_params = llmq_params_opt.value();

    uint256 commitmentHash = BuildCommitmentHash(llmq_params.type, quorumHash, validMembers, quorumPublicKey,
                                                 quorumVvecHash);
    if (LogAcceptDebug(BCLog::LLMQ)) {
        std::stringstream ss3;
        for (const auto& mn : members) {
            ss3 << mn->proTxHash.ToString().substr(0, 4) << " | ";
        }
        LogPrint(BCLog::LLMQ, "CFinalCommitment::%s members[%s] quorumPublicKey[%s] commitmentHash[%s]\n", __func__,
                 ss3.str(), quorumPublicKey.ToString(), commitmentHash.ToString());
    }
    if (llmq_params.is_single_member()) {
        LogPrintf("pubkey operator: %s\n", members[0]->pdmnState->pubKeyOperator.Get().ToString());
        if (!membersSig.VerifyInsecure(members[0]->pdmnState->pubKeyOperator.Get(), commitmentHash)) {
            LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid member signature\n", quorumHash.ToString());
            return false;
        }
    } else {
        std::vector<CBLSPublicKey> memberPubKeys;
        for (const auto i : irange::range(members.size())) {
            if (!signers[i]) {
                continue;
            }
            memberPubKeys.emplace_back(members[i]->pdmnState->pubKeyOperator.Get());
        }
        std::string members_id_string{
            strprintf("CFinalCommitment -- q[%s] invalid aggregated members signature", quorumHash.ToString())};
        if (queue_control) {
            std::vector<utils::BlsCheck> vChecks;
            vChecks.emplace_back(membersSig, memberPubKeys, commitmentHash, members_id_string);
            queue_control->Add(vChecks);
        } else {
            if (!membersSig.VerifySecureAggregated(memberPubKeys, commitmentHash)) {
                LogPrint(BCLog::LLMQ, "%s\n", members_id_string);
                return false;
            }
        }
    }
    std::string qsig_id_string{strprintf("CFinalCommitment -- q[%s] invalid quorum signature", quorumHash.ToString())};
    if (queue_control) {
        std::vector<utils::BlsCheck> vChecks;
        std::vector<CBLSPublicKey> public_keys;
        public_keys.push_back(quorumPublicKey);
        vChecks.emplace_back(quorumSig, public_keys, commitmentHash, qsig_id_string);
        queue_control->Add(vChecks);
    } else {
        if (!quorumSig.VerifyInsecure(quorumPublicKey, commitmentHash)) {
            LogPrint(BCLog::LLMQ, "%s\n", qsig_id_string);
            return false;
        }
    }
    return true;
}


bool CFinalCommitment::Verify(const llmq::UtilParameters& util_params, bool checkSigs) const
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid llmqType=%d\n", quorumHash.ToString(), ToUnderlying(llmqType));
        return false;
    }
    const auto& llmq_params = llmq_params_opt.value();

    const uint16_t expected_nversion{
        CFinalCommitment::GetVersion(IsQuorumRotationEnabled(llmq_params, util_params.m_base_index),
                                     DeploymentActiveAfter(util_params.m_base_index, util_params.m_chainman.GetConsensus(),
                                                           Consensus::DEPLOYMENT_V19))};
    if (nVersion == 0 || nVersion != expected_nversion) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid nVersion=%d expected=%d\n", quorumHash.ToString(), nVersion, expected_nversion);
        return false;
    }

    if (util_params.m_base_index->GetBlockHash() != quorumHash) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid quorumHash\n", quorumHash.ToString());
        return false;
    }

    if ((util_params.m_base_index->nHeight % llmq_params.dkgInterval) != quorumIndex) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid quorumIndex=%d\n", quorumHash.ToString(), quorumIndex);
        return false;
    }

    if (!VerifySizes(llmq_params)) {
        return false;
    }

    if (CountValidMembers() < llmq_params.minSize) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid validMembers count. validMembersCount=%d\n", quorumHash.ToString(), CountValidMembers());
        return false;
    }
    if (CountSigners() < llmq_params.minSize) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid signers count. signersCount=%d\n", quorumHash.ToString(), CountSigners());
        return false;
    }
    if (!quorumPublicKey.IsValid()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid quorumPublicKey\n", quorumHash.ToString());
        return false;
    }
    if (!llmq_params.is_single_member() && quorumVvecHash.IsNull()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid quorumVvecHash\n", quorumHash.ToString());
        return false;
    }
    if (!membersSig.IsValid()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid membersSig\n", quorumHash.ToString());
        return false;
    }
    if (!quorumSig.IsValid()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid vvecSig\n", quorumHash.ToString());
        return false;
    }
    auto members = utils::GetAllQuorumMembers(llmqType, util_params);
    if (LogAcceptDebug(BCLog::LLMQ)) {
        std::stringstream ss;
        std::stringstream ss2;
        for (const auto i: irange::range(llmq_params.size)) {
            ss << "v[" << i << "]=" << validMembers[i];
            ss2 << "s[" << i << "]=" << signers[i];
        }
        LogPrint(BCLog::LLMQ, "CFinalCommitment::%s mns[%d] validMembers[%s] signers[%s]\n", __func__, members.size(), ss.str(), ss2.str());
    }

    for (const auto i : irange::range(members.size(), size_t(llmq_params.size))) {
        if (validMembers[i]) {
            LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid validMembers bitset. bit %d should not be set\n", quorumHash.ToString(), i);
            return false;
        }
        if (signers[i]) {
            LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid signers bitset. bit %d should not be set\n", quorumHash.ToString(), i);
            return false;
        }
    }

    // sigs are only checked when the block is processed
    if (checkSigs) {
        if (!VerifySignatureAsync(util_params, /*queue_control=*/nullptr)) {
            return false;
        }
    }

    LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] VALID QUORUM\n", quorumHash.ToString());

    return true;
}

bool CFinalCommitment::VerifyNull() const
{
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s]invalid llmqType=%d\n", quorumHash.ToString(), ToUnderlying(llmqType));
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
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid signers.size=%d\n", quorumHash.ToString(), signers.size());
        return false;
    }
    if (validMembers.size() != size_t(params.size)) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- q[%s] invalid validMembers.size=%d\n", quorumHash.ToString(),
                 validMembers.size());
        return false;
    }
    return true;
}

bool CheckLLMQCommitment(const llmq::UtilParameters& util_params, const CTransaction& tx, TxValidationState& state)
{
    const auto opt_qcTx = GetTxPayload<CFinalCommitmentTxPayload>(tx);
    if (!opt_qcTx) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] GetTxPayload LLMQCommitment failed\n",
                 util_params.m_base_index->nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-payload");
    }
    auto& qcTx = *opt_qcTx;

    const auto& llmq_params_opt = Params().GetLLMQ(qcTx.commitment.llmqType);
    if (!llmq_params_opt.has_value()) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] GetLLMQ failed for llmqType[%d]\n",
                 util_params.m_base_index->nHeight, ToUnderlying(qcTx.commitment.llmqType));
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-commitment-type");
    }

    if (LogAcceptDebug(BCLog::LLMQ)) {
        std::stringstream ss;
        for (const auto i: irange::range(llmq_params_opt->size)) {
            ss << "v[" << i << "]=" << qcTx.commitment.validMembers[i];
        }
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- %s llmqType[%d] validMembers[%s] signers[]\n", __func__,
                                 int(qcTx.commitment.llmqType), ss.str());
    }

    if (qcTx.nVersion == 0 || qcTx.nVersion > CFinalCommitmentTxPayload::CURRENT_VERSION) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] invalid qcTx.nVersion[%d]\n",
                 util_params.m_base_index->nHeight, qcTx.nVersion);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-version");
    }

    if (qcTx.nHeight != uint32_t(util_params.m_base_index->nHeight + 1)) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] invalid qcTx.nHeight[%d]\n", util_params.m_base_index->nHeight,
                 qcTx.nHeight);
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-height");
    }

    const CBlockIndex* pQuorumBaseBlockIndex =
        WITH_LOCK(::cs_main, return util_params.m_chainman.m_blockman.LookupBlockIndex(qcTx.commitment.quorumHash));
    if (pQuorumBaseBlockIndex == nullptr) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-quorum-hash");
    }

    if (pQuorumBaseBlockIndex != util_params.m_base_index->GetAncestor(pQuorumBaseBlockIndex->nHeight)) {
        // not part of active chain
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-quorum-hash");
    }

    if (qcTx.commitment.IsNull()) {
        if (!qcTx.commitment.VerifyNull()) {
            LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] invalid qcTx.commitment[%s] VerifyNull failed\n",
                     util_params.m_base_index->nHeight, qcTx.commitment.quorumHash.ToString());
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-invalid-null");
        }
        return true;
    }

    if (!qcTx.commitment.Verify(util_params.replace_index(pQuorumBaseBlockIndex), false)) {
        LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] invalid qcTx.commitment[%s] Verify failed\n",
                 util_params.m_base_index->nHeight, qcTx.commitment.quorumHash.ToString());
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-qc-invalid");
    }

    LogPrint(BCLog::LLMQ, "CFinalCommitment -- h[%d] CheckLLMQCommitment VALID\n", util_params.m_base_index->nHeight);

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
