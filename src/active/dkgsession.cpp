// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <active/dkgsession.h>

#include <active/masternode.h>
#include <evo/deterministicmns.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionhandler.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <masternode/meta.h>

#include <chain.h>
#include <deploymentstatus.h>
#include <validation.h>

#include <cxxtimer.hpp>

namespace llmq {
ActiveDKGSession::ActiveDKGSession(CBLSWorker& bls_worker, CDeterministicMNManager& dmnman, CDKGDebugManager& dkgdbgman,
                                   CDKGSessionManager& qdkgsman, CMasternodeMetaMan& mn_metaman,
                                   CQuorumSnapshotManager& qsnapman, const CActiveMasternodeManager& mn_activeman,
                                   const ChainstateManager& chainman, const CSporkManager& sporkman,
                                   const CBlockIndex* base_block_index, const Consensus::LLMQParams& params) :
    CDKGSession(bls_worker, dmnman, dkgdbgman, qdkgsman, qsnapman, chainman, base_block_index, params),
    m_mn_metaman{mn_metaman},
    m_mn_activeman{mn_activeman},
    m_sporkman{sporkman},
    m_use_legacy_bls{!DeploymentActiveAfter(m_quorum_base_block_index, Params().GetConsensus(), Consensus::DEPLOYMENT_V19)}
{
}

ActiveDKGSession::~ActiveDKGSession() = default;

void ActiveDKGSession::Contribute(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    if (!AreWeMember()) {
        return;
    }

    assert(params.threshold > 1); // we should not get there with single-node-quorums

    cxxtimer::Timer t1(true);
    logger.Batch("generating contributions");
    if (!blsWorker.GenerateContributions(params.threshold, memberIds, vvecContribution, m_sk_contributions)) {
        // this should never happen actually
        logger.Batch("GenerateContributions failed");
        return;
    }
    logger.Batch("generated contributions. time=%d", t1.count());
    logger.Flush();

    SendContributions(pendingMessages, peerman);
}

void ActiveDKGSession::SendContributions(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    assert(AreWeMember());

    logger.Batch("sending contributions");

    if (ShouldSimulateError(DKGError::type::CONTRIBUTION_OMIT)) {
        logger.Batch("omitting");
        return;
    }

    CDKGContribution qc;
    qc.llmqType = params.type;
    qc.quorumHash = m_quorum_base_block_index->GetBlockHash();
    qc.proTxHash = myProTxHash;
    qc.vvec = vvecContribution;

    cxxtimer::Timer t1(true);
    qc.contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>();
    qc.contributions->InitEncrypt(members.size());

    for (const auto i : irange::range(members.size())) {
        const auto& m = members[i];
        CBLSSecretKey skContrib = m_sk_contributions[i];

        if (i != myIdx && ShouldSimulateError(DKGError::type::CONTRIBUTION_LIE)) {
            logger.Batch("lying for %s", m->dmn->proTxHash.ToString());
            skContrib.MakeNewKey();
        }

        if (!qc.contributions->Encrypt(i, m->dmn->pdmnState->pubKeyOperator.Get(), skContrib, PROTOCOL_VERSION)) {
            logger.Batch("failed to encrypt contribution for %s", m->dmn->proTxHash.ToString());
            return;
        }
    }

    logger.Batch("encrypted contributions. time=%d", t1.count());

    qc.sig = m_mn_activeman.Sign(qc.GetSignHash(), m_use_legacy_bls);

    logger.Flush();

    dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentContributions = true;
        return true;
    });

    pendingMessages.PushPendingMessage(-1, qc, peerman);
}

// Verifies all pending secret key contributions in one batch
// This is done by aggregating the verification vectors belonging to the secret key contributions
// The resulting aggregated vvec is then used to recover a public key share
// The public key share must match the public key belonging to the aggregated secret key contributions
// See CBLSWorker::VerifyContributionShares for more details.
void ActiveDKGSession::VerifyPendingContributions()
{
    AssertLockHeld(cs_pending);

    CDKGLogger logger(*this, __func__, __LINE__);

    cxxtimer::Timer t1(true);

    if (pendingContributionVerifications.empty()) {
        return;
    }

    std::vector<size_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;

    for (const auto& idx : pendingContributionVerifications) {
        const auto& m = members[idx];
        if (m->bad || m->weComplain) {
            continue;
        }
        memberIndexes.emplace_back(idx);
        vvecs.emplace_back(receivedVvecs[idx]);
        skContributions.emplace_back(receivedSkContributions[idx]);
        // Write here to definitely store one contribution for each member no matter if
        // our share is valid or not, could be that others are still correct
        dkgManager.WriteEncryptedContributions(params.type, m_quorum_base_block_index, m->dmn->proTxHash, *vecEncryptedContributions[idx]);
    }

    auto result = blsWorker.VerifyContributionShares(myId, vvecs, skContributions);
    if (result.size() != memberIndexes.size()) {
        logger.Batch("VerifyContributionShares returned result of size %d but size %d was expected, something is wrong", result.size(), memberIndexes.size());
        return;
    }

    for (const auto i : irange::range(memberIndexes.size())) {
        if (!result[i]) {
            const auto& m = members[memberIndexes[i]];
            logger.Batch("invalid contribution from %s. will complain later", m->dmn->proTxHash.ToString());
            m->weComplain = true;
            dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, m->idx, [&](CDKGDebugMemberStatus& status) {
                status.statusBits.weComplain = true;
                return true;
            });
        } else {
            size_t memberIdx = memberIndexes[i];
            dkgManager.WriteVerifiedSkContribution(params.type, m_quorum_base_block_index, members[memberIdx]->dmn->proTxHash, skContributions[i]);
        }
    }

    logger.Batch("verified %d pending contributions. time=%d", pendingContributionVerifications.size(), t1.count());
    pendingContributionVerifications.clear();
}

void ActiveDKGSession::VerifyAndComplain(CConnman& connman, CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    if (!AreWeMember()) {
        return;
    }

    {
        LOCK(cs_pending);
        VerifyPendingContributions();
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    // we check all members if they sent us their contributions
    // we consider members as bad if they missed to send anything or if they sent multiple
    // in both cases we won't give them a second chance as they might be either down, buggy or an adversary
    // we assume that such a participant will be marked as bad by the whole network in most cases,
    // as propagation will ensure that all nodes see the same vvecs/contributions. In case nodes come to
    // different conclusions, the aggregation phase will handle this (most voted quorum key wins).

    cxxtimer::Timer t1(true);

    for (const auto& m : members) {
        if (m->bad) {
            continue;
        }
        if (m->contributions.empty()) {
            logger.Batch("%s did not send any contribution", m->dmn->proTxHash.ToString());
            MarkBadMember(m->idx);
            continue;
        }
    }

    logger.Batch("verified contributions. time=%d", t1.count());
    logger.Flush();

    VerifyConnectionAndMinProtoVersions(connman);

    SendComplaint(pendingMessages, peerman);
}

void ActiveDKGSession::VerifyConnectionAndMinProtoVersions(CConnman& connman) const
{
    assert(m_mn_metaman.IsValid());

    if (!IsQuorumPoseEnabled(params.type, m_sporkman)) {
        return;
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    Uint256HashMap<int> protoMap;
    connman.ForEachNode([&](const CNode* pnode) {
        auto verifiedProRegTxHash = pnode->GetVerifiedProRegTxHash();
        if (verifiedProRegTxHash.IsNull()) {
            return;
        }
        protoMap.emplace(verifiedProRegTxHash, pnode->nVersion);
    });

    bool fShouldAllMembersBeConnected = IsAllMembersConnectedEnabled(params.type, m_sporkman);
    for (const auto& m : members) {
        if (m->dmn->proTxHash == myProTxHash) {
            continue;
        }
        if (auto it = protoMap.find(m->dmn->proTxHash); it == protoMap.end()) {
            m->badConnection = fShouldAllMembersBeConnected;
            if (m->badConnection) {
                logger.Batch("%s is not connected to us, badConnection=1", m->dmn->proTxHash.ToString());
            }
        } else if (it->second < MIN_MASTERNODE_PROTO_VERSION) {
            m->badConnection = true;
            logger.Batch("%s does not have min proto version %d (has %d)", m->dmn->proTxHash.ToString(), MIN_MASTERNODE_PROTO_VERSION, it->second);
        }
        if (m_mn_metaman.OutboundFailedTooManyTimes(m->dmn->proTxHash)) {
            m->badConnection = true;
            logger.Batch("%s failed to connect to it too many times", m->dmn->proTxHash.ToString());
        }
        if (m_mn_metaman.IsPlatformBanned(m->dmn->proTxHash)) {
            m->badConnection = true;
            logger.Batch("%s is Platform PoSe banned", m->dmn->proTxHash.ToString());
        }
    }
}

void ActiveDKGSession::SendComplaint(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    assert(AreWeMember());

    CDKGComplaint qc(params);
    qc.llmqType = params.type;
    qc.quorumHash = m_quorum_base_block_index->GetBlockHash();
    qc.proTxHash = myProTxHash;

    int badCount = 0;
    int complaintCount = 0;
    for (const auto i : irange::range(members.size())) {
        const auto& m = members[i];
        if (m->bad || m->badConnection) {
            qc.badMembers[i] = true;
            badCount++;
        } else if (m->weComplain) {
            qc.complainForMembers[i] = true;
            complaintCount++;
        }
    }

    if (badCount == 0 && complaintCount == 0) {
        return;
    }

    logger.Batch("sending complaint. badCount=%d, complaintCount=%d", badCount, complaintCount);

    qc.sig = m_mn_activeman.Sign(qc.GetSignHash(), m_use_legacy_bls);

    logger.Flush();

    dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentComplaint = true;
        return true;
    });

    pendingMessages.PushPendingMessage(-1, qc, peerman);
}

void ActiveDKGSession::VerifyAndJustify(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    std::set<uint256> justifyFor;

    for (const auto& m : members) {
        if (m->bad) {
            continue;
        }
        if (m->badMemberVotes.size() >= size_t(params.dkgBadVotesThreshold)) {
            logger.Batch("%s marked as bad as %d other members voted for this", m->dmn->proTxHash.ToString(), m->badMemberVotes.size());
            MarkBadMember(m->idx);
            continue;
        }
        if (m->complaints.empty()) {
            continue;
        }
        if (m->complaints.size() != 1) {
            logger.Batch("%s sent multiple complaints", m->dmn->proTxHash.ToString());
            MarkBadMember(m->idx);
            continue;
        }

        LOCK(invCs);
        if (const auto& qc = complaints.at(*m->complaints.begin());
                qc.complainForMembers[*myIdx]) {
            justifyFor.emplace(qc.proTxHash);
        }
    }

    logger.Flush();
    if (!justifyFor.empty()) {
        SendJustification(pendingMessages, peerman, justifyFor);
    }
}

void ActiveDKGSession::SendJustification(CDKGPendingMessages& pendingMessages, PeerManager& peerman,
                                         const std::set<uint256>& forMembers)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    assert(AreWeMember());

    logger.Batch("sending justification for %d members", forMembers.size());

    CDKGJustification qj;
    qj.llmqType = params.type;
    qj.quorumHash = m_quorum_base_block_index->GetBlockHash();
    qj.proTxHash = myProTxHash;
    qj.contributions.reserve(forMembers.size());

    for (const uint32_t i : irange::range(members.size())) {
        const auto& m = members[i];
        if (forMembers.count(m->dmn->proTxHash) == 0) {
            continue;
        }
        logger.Batch("justifying for %s", m->dmn->proTxHash.ToString());

        CBLSSecretKey skContribution = m_sk_contributions[i];

        if (i != myIdx && ShouldSimulateError(DKGError::type::JUSTIFY_LIE)) {
            logger.Batch("lying for %s", m->dmn->proTxHash.ToString());
            skContribution.MakeNewKey();
        }

        qj.contributions.emplace_back(CDKGJustification::Contribution{i, skContribution});
    }

    if (ShouldSimulateError(DKGError::type::JUSTIFY_OMIT)) {
        logger.Batch("omitting");
        return;
    }

    qj.sig = m_mn_activeman.Sign(qj.GetSignHash(), m_use_legacy_bls);

    logger.Flush();

    dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentJustification = true;
        return true;
    });

    pendingMessages.PushPendingMessage(-1, qj, peerman);
}

void ActiveDKGSession::VerifyAndCommit(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    std::vector<size_t> badMembers;
    badMembers.reserve(members.size());
    std::vector<size_t> openComplaintMembers;
    openComplaintMembers.reserve(members.size());

    for (const auto& m : members) {
        if (m->bad) {
            badMembers.emplace_back(m->idx);
            continue;
        }
        if (!m->complaintsFromOthers.empty()) {
            MarkBadMember(m->idx);
            openComplaintMembers.emplace_back(m->idx);
        }
    }

    if (!badMembers.empty() || !openComplaintMembers.empty()) {
        logger.Batch("verification result:");
    }
    if (!badMembers.empty()) {
        logger.Batch("  members previously determined as bad:");
        for (const auto& idx : badMembers) {
            logger.Batch("    %s", members[idx]->dmn->proTxHash.ToString());
        }
    }
    if (!openComplaintMembers.empty()) {
        logger.Batch("  members with open complaints and now marked as bad:");
        for (const auto& idx : openComplaintMembers) {
            logger.Batch("    %s", members[idx]->dmn->proTxHash.ToString());
        }
    }

    logger.Flush();

    SendCommitment(pendingMessages, peerman);
}

void ActiveDKGSession::SendCommitment(CDKGPendingMessages& pendingMessages, PeerManager& peerman)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    assert(AreWeMember());

    logger.Batch("sending commitment");

    CDKGPrematureCommitment qc(params);
    qc.llmqType = params.type;
    qc.quorumHash = m_quorum_base_block_index->GetBlockHash();
    qc.proTxHash = myProTxHash;

    for (const auto i : irange::range(members.size())) {
        const auto& m = members[i];
        if (!m->bad) {
            qc.validMembers[i] = true;
        }
    }

    if (qc.CountValidMembers() < params.minSize) {
        logger.Batch("not enough valid members. not sending commitment");
        return;
    }

    if (ShouldSimulateError(DKGError::type::COMMIT_OMIT)) {
        logger.Batch("omitting");
        return;
    }

    cxxtimer::Timer timerTotal(true);

    cxxtimer::Timer t1(true);
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    if (!dkgManager.GetVerifiedContributions(params.type, m_quorum_base_block_index, qc.validMembers, memberIndexes, vvecs, skContributions)) {
        logger.Batch("failed to get valid contributions");
        return;
    }

    BLSVerificationVectorPtr vvec = cache.BuildQuorumVerificationVector(::SerializeHash(memberIndexes), vvecs);
    if (vvec == nullptr) {
        logger.Batch("failed to build quorum verification vector");
        return;
    }
    t1.stop();

    cxxtimer::Timer t2(true);
    CBLSSecretKey skShare = cache.AggregateSecretKeys(::SerializeHash(memberIndexes), skContributions);
    if (!skShare.IsValid()) {
        logger.Batch("failed to build own secret share");
        return;
    }
    t2.stop();

    logger.Batch("pubKeyShare=%s", skShare.GetPublicKey().ToString());

    cxxtimer::Timer t3(true);
    qc.quorumPublicKey = (*vvec)[0];
    qc.quorumVvecHash = ::SerializeHash(*vvec);

    int lieType = -1;
    if (ShouldSimulateError(DKGError::type::COMMIT_LIE)) {
        lieType = GetRand<int>(/*nMax=*/5);
        logger.Batch("lying on commitment. lieType=%d", lieType);
    }

    if (lieType == 0) {
        CBLSSecretKey k;
        k.MakeNewKey();
        qc.quorumPublicKey = k.GetPublicKey();
    } else if (lieType == 1) {
        (*qc.quorumVvecHash.begin())++;
    }

    uint256 commitmentHash = BuildCommitmentHash(qc.llmqType, qc.quorumHash, qc.validMembers, qc.quorumPublicKey, qc.quorumVvecHash);

    if (lieType == 2) {
        (*commitmentHash.begin())++;
    }

    qc.sig = m_mn_activeman.Sign(commitmentHash, m_use_legacy_bls);
    qc.quorumSig = skShare.Sign(commitmentHash, m_use_legacy_bls);

    if (lieType == 3) {
        auto buf = qc.sig.ToBytes(m_use_legacy_bls);
        buf[5]++;
        qc.sig.SetBytes(buf, m_use_legacy_bls);
    } else if (lieType == 4) {
        auto buf = qc.quorumSig.ToBytes(m_use_legacy_bls);
        buf[5]++;
        qc.quorumSig.SetBytes(buf, m_use_legacy_bls);
    }

    t3.stop();
    timerTotal.stop();

    logger.Batch("built premature commitment. time1=%d, time2=%d, time3=%d, totalTime=%d",
                    t1.count(), t2.count(), t3.count(), timerTotal.count());


    logger.Flush();

    dkgDebugManager.UpdateLocalSessionStatus(params.type, quorumIndex, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentPrematureCommitment = true;
        return true;
    });

    pendingMessages.PushPendingMessage(-1, qc, peerman);
}

std::vector<CFinalCommitment> ActiveDKGSession::FinalizeCommitments()
{
    if (!AreWeMember()) {
        return {};
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    using Key = std::vector<bool>;
    std::map<Key, std::vector<CDKGPrematureCommitment>> commitmentsMap;

    {
        LOCK(invCs);

        for (const auto& p : prematureCommitments) {
            const auto& qc = p.second;
            if (validCommitments.count(p.first) == 0) {
                continue;
            }

            // should have been verified before
            assert(qc.CountValidMembers() >= params.minSize);

            auto it = commitmentsMap.find(qc.validMembers);
            if (it == commitmentsMap.end()) {
                it = commitmentsMap.emplace(qc.validMembers, std::vector<CDKGPrematureCommitment>()).first;
            }

            it->second.emplace_back(qc);
        }
    }

    std::vector<CFinalCommitment> finalCommitments;
    for (const auto& p : commitmentsMap) {
        const auto& cvec = p.second;
        if (cvec.size() < size_t(params.minSize)) {
            // commitment was signed by a minority
            continue;
        }

        std::vector<CBLSId> signerIds;
        std::vector<CBLSSignature> thresholdSigs;

        const auto& first = cvec[0];

        CFinalCommitment fqc(params, first.quorumHash);
        fqc.validMembers = first.validMembers;
        fqc.quorumPublicKey = first.quorumPublicKey;
        fqc.quorumVvecHash = first.quorumVvecHash;

        const bool isQuorumRotationEnabled{IsQuorumRotationEnabled(params, m_quorum_base_block_index)};
        // TODO: always put `true` here: so far as v19 is activated, we always write BASIC now
        fqc.nVersion = CFinalCommitment::GetVersion(isQuorumRotationEnabled, DeploymentActiveAfter(m_quorum_base_block_index, m_chainman.GetConsensus(), Consensus::DEPLOYMENT_V19));
        fqc.quorumIndex = isQuorumRotationEnabled ? quorumIndex : 0;

        uint256 commitmentHash = BuildCommitmentHash(fqc.llmqType, fqc.quorumHash, fqc.validMembers, fqc.quorumPublicKey, fqc.quorumVvecHash);

        std::vector<CBLSSignature> aggSigs;
        std::vector<CBLSPublicKey> aggPks;
        aggSigs.reserve(cvec.size());
        aggPks.reserve(cvec.size());

        for (const auto& qc : cvec) {
            if (qc.quorumPublicKey != first.quorumPublicKey || qc.quorumVvecHash != first.quorumVvecHash) {
                logger.Batch("quorumPublicKey or quorumVvecHash does not match, skipping");
                continue;
            }

            size_t signerIndex = membersMap[qc.proTxHash];
            const auto& m = members[signerIndex];

            fqc.signers[signerIndex] = true;
            aggSigs.emplace_back(qc.sig);
            aggPks.emplace_back(m->dmn->pdmnState->pubKeyOperator.Get());

            signerIds.emplace_back(m->id);
            thresholdSigs.emplace_back(qc.quorumSig);
        }

        cxxtimer::Timer t1(true);
        fqc.membersSig = CBLSSignature::AggregateSecure(aggSigs, aggPks, commitmentHash);
        t1.stop();

        cxxtimer::Timer t2(true);
        if (!fqc.quorumSig.Recover(thresholdSigs, signerIds)) {
            logger.Batch("failed to recover quorum sig");
            continue;
        }
        t2.stop();

        cxxtimer::Timer t3(true);
        if (!fqc.Verify({m_dmnman, m_qsnapman, m_chainman, m_quorum_base_block_index}, true)) {
            logger.Batch("failed to verify final commitment");
            continue;
        }
        t3.stop();

        finalCommitments.emplace_back(fqc);

        logger.Batch("final commitment: validMembers=%d, signers=%d, quorumPublicKey=%s, time1=%d, time2=%d, time3=%d",
                        fqc.CountValidMembers(), fqc.CountSigners(), fqc.quorumPublicKey.ToString(),
                        t1.count(), t2.count(), t3.count());
    }

    logger.Flush();

    return finalCommitments;
}

CFinalCommitment ActiveDKGSession::FinalizeSingleCommitment()
{
    if (!AreWeMember()) {
        return {};
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    std::vector<CBLSId> signerIds;
    std::vector<CBLSSignature> thresholdSigs;

    CFinalCommitment fqc(params, m_quorum_base_block_index->GetBlockHash());


    fqc.signers = {true};
    fqc.validMembers = {true};

    CBLSSecretKey sk1;
    sk1.MakeNewKey();

    fqc.quorumPublicKey = sk1.GetPublicKey();
    fqc.quorumVvecHash = {};

    // use just MN's operator public key as quorum pubkey.
    // TODO: use sk1 here instead and use recovery mechanism from shares, but that's not trivial to do
    const bool workaround_qpublic_key = true;
    if (workaround_qpublic_key) {
        fqc.quorumPublicKey = m_mn_activeman.GetPubKey();
    }
    const bool isQuorumRotationEnabled{false};
    fqc.nVersion = CFinalCommitment::GetVersion(isQuorumRotationEnabled,
                                                DeploymentActiveAfter(m_quorum_base_block_index, m_chainman.GetConsensus(),
                                                                      Consensus::DEPLOYMENT_V19));
    fqc.quorumIndex = 0;

    uint256 commitmentHash = BuildCommitmentHash(fqc.llmqType, fqc.quorumHash, fqc.validMembers, fqc.quorumPublicKey,
                                                 fqc.quorumVvecHash);
    fqc.quorumSig = sk1.Sign(commitmentHash, m_use_legacy_bls);

    fqc.membersSig = m_mn_activeman.Sign(commitmentHash, m_use_legacy_bls);

    if (workaround_qpublic_key) {
        fqc.quorumSig = fqc.membersSig;
    }

    if (!fqc.Verify({m_dmnman, m_qsnapman, m_chainman, m_quorum_base_block_index}, true)) {
        logger.Batch("failed to verify final commitment");
        assert(false);
    }

    logger.Batch("final commitment: validMembers=%d, signers=%d, quorumPublicKey=%s", fqc.CountValidMembers(),
                 fqc.CountSigners(), fqc.quorumPublicKey.ToString());

    logger.Flush();

    return fqc;
}

bool ActiveDKGSession::MaybeDecrypt(const CBLSIESMultiRecipientObjects<CBLSSecretKey>& obj, size_t idx,
                                    CBLSSecretKey& ret_obj, int version)
{
    return m_mn_activeman.Decrypt(obj, idx, ret_obj, version);
}
} // namespace llmq
