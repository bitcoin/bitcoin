// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_dkgsession.h>

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_dkgsessionmgr.h>
#include <llmq/quorums_utils.h>

#include <evo/deterministicmns.h>

#include <evo/specialtx.h>

#include <masternode/activemasternode.h>
#include <masternode/masternodemeta.h>
#include <chainparams.h>
#include <netmessagemaker.h>
#include <univalue.h>
#include <validation.h>

#include <cxxtimer.hpp>
#include <memory>

namespace llmq
{

// Supported error types:
// - contribution-omit
// - contribution-lie
// - complain-lie
// - justify-lie
// - justify-omit
// - commit-omit
// - commit-lie

static RecursiveMutex cs_simDkgError;
static std::map<std::string, double> simDkgErrorMap;

void SetSimulatedDKGErrorRate(const std::string& type, double rate)
{
    LOCK(cs_simDkgError);
    simDkgErrorMap[type] = rate;
}

static double GetSimulatedErrorRate(const std::string& type)
{
    LOCK(cs_simDkgError);
    auto it = simDkgErrorMap.find(type);
    if (it != simDkgErrorMap.end()) {
        return it->second;
    }
    return 0;
}

static bool ShouldSimulateError(const std::string& type)
{
    double rate = GetSimulatedErrorRate(type);
    return GetRandBool(rate);
}

CDKGLogger::CDKGLogger(const CDKGSession& _quorumDkg, const std::string& _func) :
    CDKGLogger(_quorumDkg.params.name, _quorumDkg.pindexQuorum->GetBlockHash(), _quorumDkg.pindexQuorum->nHeight, _quorumDkg.AreWeMember(), _func)
{
}

CDKGLogger::CDKGLogger(const std::string& _llmqTypeName, const uint256& _quorumHash, int _height, bool _areWeMember, const std::string& _func) :
    CBatchedLogger(BCLog::LLMQ_DKG, strprintf("QuorumDKG(type=%s, height=%d, member=%d, func=%s)", _llmqTypeName, _height, _areWeMember, _func))
{
}


CDKGComplaint::CDKGComplaint(const Consensus::LLMQParams& params) :
    badMembers((size_t)params.size), complainForMembers((size_t)params.size)
{
}

CDKGPrematureCommitment::CDKGPrematureCommitment(const Consensus::LLMQParams& params) :
    validMembers((size_t)params.size)
{
}

CDKGMember::CDKGMember(CDeterministicMNCPtr _dmn, size_t _idx) :
    dmn(_dmn),
    idx(_idx),
    id(CBLSId::FromHash(_dmn->proTxHash))
{

}

bool CDKGSession::Init(const CBlockIndex* _pindexQuorum, const std::vector<CDeterministicMNCPtr>& mns, const uint256& _myProTxHash)
{
    pindexQuorum = _pindexQuorum;

    members.resize(mns.size());
    memberIds.resize(members.size());
    receivedVvecs.resize(members.size());
    receivedSkContributions.resize(members.size());
    vecEncryptedContributions.resize(members.size());

    for (size_t i = 0; i < mns.size(); i++) {
        members[i] = std::make_unique<CDKGMember>(mns[i], i);
        membersMap.try_emplace(members[i]->dmn->proTxHash, i);
        memberIds[i] = members[i]->id;
    }

    if (!_myProTxHash.IsNull()) {
        for (size_t i = 0; i < members.size(); i++) {
            auto& m = members[i];
            if (m->dmn->proTxHash == _myProTxHash) {
                myIdx = i;
                myProTxHash = _myProTxHash;
                myId = m->id;
                break;
            }
        }
    }

    CDKGLogger logger(*this, __func__);

    if (mns.size() < (size_t)params.minSize) {
        logger.Batch("not enough members (%d < %d), aborting init", mns.size(), (size_t)params.minSize);
        return false;
    }

    if (!myProTxHash.IsNull()) {
        quorumDKGDebugManager->InitLocalSessionStatus(params.type, pindexQuorum->GetBlockHash(), pindexQuorum->nHeight);
        relayMembers = CLLMQUtils::GetQuorumRelayMembers(params.type, pindexQuorum, myProTxHash, true);
    }

    if (myProTxHash.IsNull()) {
        logger.Batch("initialized as observer. mns=%d", mns.size());
    } else {
        logger.Batch("initialized as member. mns=%d", mns.size());
    }

    return true;
}

void CDKGSession::Contribute(CDKGPendingMessages& pendingMessages)
{
    CDKGLogger logger(*this, __func__);

    if (!AreWeMember()) {
        return;
    }

    cxxtimer::Timer t1(true);
    logger.Batch("generating contributions");
    if (!blsWorker.GenerateContributions((size_t)params.threshold, memberIds, vvecContribution, skContributions)) {
        // this should never happen actually
        logger.Batch("GenerateContributions failed");
        return;
    }
    logger.Batch("generated contributions. time=%d", t1.count());

    SendContributions(pendingMessages);
}

void CDKGSession::SendContributions(CDKGPendingMessages& pendingMessages)
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Batch("sending contributions");

    if (ShouldSimulateError("contribution-omit")) {
        logger.Batch("omitting");
        return;
    }

    CDKGContribution qc;
    qc.llmqType = params.type;
    qc.quorumHash = pindexQuorum->GetBlockHash();
    qc.proTxHash = myProTxHash;
    qc.vvec = vvecContribution;

    cxxtimer::Timer t1(true);
    qc.contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>();
    qc.contributions->InitEncrypt(members.size());

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        CBLSSecretKey skContrib = skContributions[i];

        if (i != myIdx && ShouldSimulateError("contribution-lie")) {
            logger.Batch("lying for %s", m->dmn->proTxHash.ToString());
            skContrib.MakeNewKey();
        }

        if (!qc.contributions->Encrypt(i, m->dmn->pdmnState->pubKeyOperator.Get(), skContrib, PROTOCOL_VERSION)) {
            logger.Batch("failed to encrypt contribution for %s", m->dmn->proTxHash.ToString());
            return;
        }
    }

    logger.Batch("encrypted contributions. time=%d", t1.count());

    qc.sig = activeMasternodeInfo.blsKeyOperator->Sign(qc.GetSignHash());

    logger.Flush();

    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentContributions = true;
        return true;
    });

    pendingMessages.PushPendingMessage(nullptr, qc);
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGContribution& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qc.quorumHash != pindexQuorum->GetBlockHash()) {
        logger.Batch("contribution for wrong quorum, rejecting");
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Batch("contributor not a member of this quorum, rejecting contribution");
        retBan = true;
        return false;
    }

    if (qc.contributions->blobs.size() != members.size()) {
        logger.Batch("invalid contributions count");
        retBan = true;
        return false;
    }
    if (qc.vvec->size() != (size_t)params.threshold) {
        logger.Batch("invalid verification vector length");
        retBan = true;
        return false;
    }

    if (!blsWorker.VerifyVerificationVector(*qc.vvec)) {
        logger.Batch("invalid verification vector");
        retBan = true;
        return false;
    }

    if (member->contributions.size() >= 2) {
        // don't do any further processing if we got more than 1 valid contributions already
        // this is a DoS protection against members sending multiple contributions with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Batch("dropping contribution from %s as we already got %d contributions", member->dmn->proTxHash.ToString(), member->contributions.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGContribution& qc, bool& retBan)
{
    LOCK(cs_pending);

    CDKGLogger logger(*this, __func__);

    retBan = false;

    auto member = GetMember(qc.proTxHash);

    cxxtimer::Timer t1(true);
    logger.Batch("received contribution from %s", qc.proTxHash.ToString());

    {
        // relay, no matter if further verification fails
        // This ensures the whole quorum sees the bad behavior
        LOCK(invCs);

        if (member->contributions.size() >= 2) {
            // only relay up to 2 contributions, that's enough to let the other members know about his bad behavior
            return;
        }

        contributions.try_emplace(hash, qc);
        member->contributions.emplace(hash);

        CInv inv(MSG_QUORUM_CONTRIB, hash);
        RelayOtherInvToParticipants(inv);

        quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, member->idx, [&](CDKGDebugMemberStatus& status) {
            status.statusBits.receivedContribution = true;
            return true;
        });

        if (member->contributions.size() > 1) {
            // don't do any further processing if we got more than 1 contribution. we already relayed it,
            // so others know about his bad behavior
            MarkBadMember(member->idx);
            logger.Batch("%s did send multiple contributions", member->dmn->proTxHash.ToString());
            return;
        }
    }

    receivedVvecs[member->idx] = qc.vvec;

    int receivedCount = 0;
    for (const auto& m : members) {
        if (!m->contributions.empty()) {
            receivedCount++;
        }
    }

    logger.Batch("received and relayed contribution. received=%d/%d, time=%d", receivedCount, members.size(), t1.count());

    cxxtimer::Timer t2(true);

    if (!AreWeMember()) {
        // can't further validate
        return;
    }

    dkgManager.WriteVerifiedVvecContribution(params.type, pindexQuorum->GetBlockHash(), qc.proTxHash, qc.vvec);

    bool complain = false;
    CBLSSecretKey skContribution;
    if (!qc.contributions->Decrypt(myIdx, *activeMasternodeInfo.blsKeyOperator, skContribution, PROTOCOL_VERSION)) {
        logger.Batch("contribution from %s could not be decrypted", member->dmn->proTxHash.ToString());
        complain = true;
    } else if (member->idx != myIdx && ShouldSimulateError("complain-lie")) {
        logger.Batch("lying/complaining for %s", member->dmn->proTxHash.ToString());
        complain = true;
    }

    if (complain) {
        member->weComplain = true;
        quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, member->idx, [&](CDKGDebugMemberStatus& status) {
            status.statusBits.weComplain = true;
            return true;
        });
        return;
    }

    logger.Batch("decrypted our contribution share. time=%d", t2.count());

    bool verifyPending = false;
    receivedSkContributions[member->idx] = skContribution;
    vecEncryptedContributions[member->idx] = qc.contributions;
    pendingContributionVerifications.emplace_back(member->idx);
    if (pendingContributionVerifications.size() >= 32) {
        verifyPending = true;
    }

    if (verifyPending) {
        VerifyPendingContributions();
    }
}

// Verifies all pending secret key contributions in one batch
// This is done by aggregating the verification vectors belonging to the secret key contributions
// The resulting aggregated vvec is then used to recover a public key share
// The public key share must match the public key belonging to the aggregated secret key contributions
// See CBLSWorker::VerifyContributionShares for more details.
void CDKGSession::VerifyPendingContributions()
{
    AssertLockHeld(cs_pending);

    CDKGLogger logger(*this, __func__);

    cxxtimer::Timer t1(true);

    std::vector<size_t> pend = std::move(pendingContributionVerifications);
    if (pend.empty()) {
        return;
    }

    std::vector<size_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;

    for (const auto& idx : pend) {
        auto& m = members[idx];
        if (m->bad || m->weComplain) {
            continue;
        }
        memberIndexes.emplace_back(idx);
        vvecs.emplace_back(receivedVvecs[idx]);
        skContributions.emplace_back(receivedSkContributions[idx]);
        // Write here to definitely store one contribution for each member no matter if
        // our share is valid or not, could be that others are still correct
        dkgManager.WriteEncryptedContributions(params.type, pindexQuorum, m->dmn->proTxHash, *vecEncryptedContributions[idx]);
    }

    auto result = blsWorker.VerifyContributionShares(myId, vvecs, skContributions);
    if (result.size() != memberIndexes.size()) {
        logger.Batch("VerifyContributionShares returned result of size %d but size %d was expected, something is wrong", result.size(), memberIndexes.size());
        return;
    }

    for (size_t i = 0; i < memberIndexes.size(); i++) {
        if (!result[i]) {
            auto& m = members[memberIndexes[i]];
            logger.Batch("invalid contribution from %s. will complain later", m->dmn->proTxHash.ToString());
            m->weComplain = true;
            quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, m->idx, [&](CDKGDebugMemberStatus& status) {
                status.statusBits.weComplain = true;
                return true;
            });
        } else {
            size_t memberIdx = memberIndexes[i];
            dkgManager.WriteVerifiedSkContribution(params.type, pindexQuorum->GetBlockHash(), members[memberIdx]->dmn->proTxHash, skContributions[i]);
        }
    }

    logger.Batch("verified %d pending contributions. time=%d", pend.size(), t1.count());
}

void CDKGSession::VerifyAndComplain(CDKGPendingMessages& pendingMessages)
{
    if (!AreWeMember()) {
        return;
    }

    {
        LOCK(cs_pending);
        VerifyPendingContributions();
    }

    CDKGLogger logger(*this, __func__);

    // we check all members if they sent us their contributions
    // we consider members as bad if they missed to send anything or if they sent multiple
    // in both cases we won't give him a second chance as he is either down, buggy or an adversary
    // we assume that such a participant will be marked as bad by the whole network in most cases,
    // as propagation will ensure that all nodes see the same vvecs/contributions. In case nodes come to
    // different conclusions, the aggregation phase will handle this (most voted quorum key wins)

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

    VerifyConnectionAndMinProtoVersions();

    SendComplaint(pendingMessages);
}

void CDKGSession::VerifyConnectionAndMinProtoVersions()
{
    if (!CLLMQUtils::IsQuorumPoseEnabled(params.type)) {
        return;
    }

    CDKGLogger logger(*this, __func__);

    std::unordered_map<uint256, int, StaticSaltedHasher> protoMap;
    dkgManager.connman.ForEachNode([&](const CNode* pnode) {
        LOCK(pnode->cs_mnauth);
        if (pnode->verifiedProRegTxHash.IsNull()) {
            return;
        }
        protoMap.try_emplace(pnode->verifiedProRegTxHash, pnode->nVersion);
    });

    bool fShouldAllMembersBeConnected = CLLMQUtils::IsAllMembersConnectedEnabled(params.type);
    for (auto& m : members) {
        if (m->dmn->proTxHash == myProTxHash) {
            continue;
        }

        auto it = protoMap.find(m->dmn->proTxHash);
        if (it == protoMap.end()) {
            m->badConnection = fShouldAllMembersBeConnected;
            logger.Batch("%s is not connected to us, badConnection=%b", m->dmn->proTxHash.ToString(), m->badConnection);
        } else if (it != protoMap.end() && it->second < MIN_MASTERNODE_PROTO_VERSION) {
            m->badConnection = true;
            logger.Batch("%s does not have min proto version %d (has %d)", m->dmn->proTxHash.ToString(), MIN_MASTERNODE_PROTO_VERSION, it->second);
        }

        auto lastOutbound = mmetaman.GetMetaInfo(m->dmn->proTxHash)->GetLastOutboundSuccess();
        if (GetAdjustedTime() - lastOutbound > 60 * 60) {
            m->badConnection = true;
            logger.Batch("%s no outbound connection since %d seconds", m->dmn->proTxHash.ToString(), GetAdjustedTime() - lastOutbound);
        }
    }
}

void CDKGSession::SendComplaint(CDKGPendingMessages& pendingMessages)
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    CDKGComplaint qc(params);
    qc.llmqType = params.type;
    qc.quorumHash = pindexQuorum->GetBlockHash();
    qc.proTxHash = myProTxHash;

    int badCount = 0;
    int complaintCount = 0;
    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
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

    qc.sig = activeMasternodeInfo.blsKeyOperator->Sign(qc.GetSignHash());

    logger.Flush();

    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentComplaint = true;
        return true;
    });

    pendingMessages.PushPendingMessage(nullptr, qc);
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGComplaint& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qc.quorumHash != pindexQuorum->GetBlockHash()) {
        logger.Batch("complaint for wrong quorum, rejecting");
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Batch("complainer not a member of this quorum, rejecting complaint");
        retBan = true;
        return false;
    }

    if (qc.badMembers.size() != (size_t)params.size) {
        logger.Batch("invalid badMembers bitset size");
        retBan = true;
        return false;
    }

    if (qc.complainForMembers.size() != (size_t)params.size) {
        logger.Batch("invalid complainForMembers bitset size");
        retBan = true;
        return false;
    }

    if (member->complaints.size() >= 2) {
        // don't do any further processing if we got more than 1 valid complaints already
        // this is a DoS protection against members sending multiple complaints with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Batch("dropping complaint from %s as we already got %d complaints",
                      member->dmn->proTxHash.ToString(), member->complaints.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGComplaint& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    logger.Batch("received complaint from %s", qc.proTxHash.ToString());

    auto member = GetMember(qc.proTxHash);

    {
        LOCK(invCs);

        if (member->complaints.size() >= 2) {
            // only relay up to 2 complaints, that's enough to let the other members know about his bad behavior
            return;
        }

        complaints.try_emplace(hash, qc);
        member->complaints.emplace(hash);

        CInv inv(MSG_QUORUM_COMPLAINT, hash);
        RelayOtherInvToParticipants(inv);

        quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, member->idx, [&](CDKGDebugMemberStatus& status) {
            status.statusBits.receivedComplaint = true;
            return true;
        });

        if (member->complaints.size() > 1) {
            // don't do any further processing if we got more than 1 complaint. we already relayed it,
            // so others know about his bad behavior
            MarkBadMember(member->idx);
            logger.Batch("%s did send multiple complaints", member->dmn->proTxHash.ToString());
            return;
        }
    }

    int receivedCount = 0;
    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (qc.badMembers[i]) {
            logger.Batch("%s voted for %s to be bad", member->dmn->proTxHash.ToString(), m->dmn->proTxHash.ToString());
            m->badMemberVotes.emplace(qc.proTxHash);
            if (AreWeMember() && i == myIdx) {
                logger.Batch("%s voted for us to be bad", member->dmn->proTxHash.ToString());
            }
        }
        if (qc.complainForMembers[i]) {
            m->complaintsFromOthers.emplace(qc.proTxHash);
            m->someoneComplain = true;
            quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, m->idx, [&](CDKGDebugMemberStatus& status) {
                return status.complaintsFromMembers.emplace(member->idx).second;
            });
            if (AreWeMember() && i == myIdx) {
                logger.Batch("%s complained about us", member->dmn->proTxHash.ToString());
            }
        }
        if (!m->complaints.empty()) {
            receivedCount++;
        }
    }

    logger.Batch("received and relayed complaint. received=%d", receivedCount);
}

void CDKGSession::VerifyAndJustify(CDKGPendingMessages& pendingMessages)
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__);

    std::set<uint256> justifyFor;

    for (const auto& m : members) {
        if (m->bad) {
            continue;
        }
        if (m->badMemberVotes.size() >= (size_t)params.dkgBadVotesThreshold) {
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
        {
            LOCK(invCs);
            auto& qc = complaints.at(*m->complaints.begin());
            if (qc.complainForMembers[myIdx]) {
                justifyFor.emplace(qc.proTxHash);
            }
        }
    }

    logger.Flush();
    if (!justifyFor.empty()) {
        SendJustification(pendingMessages, justifyFor);
    }
}

void CDKGSession::SendJustification(CDKGPendingMessages& pendingMessages, const std::set<uint256>& forMembers)
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Batch("sending justification for %d members", forMembers.size());

    CDKGJustification qj;
    qj.llmqType = params.type;
    qj.quorumHash = pindexQuorum->GetBlockHash();
    qj.proTxHash = myProTxHash;
    qj.contributions.reserve(forMembers.size());

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (!forMembers.count(m->dmn->proTxHash)) {
            continue;
        }
        logger.Batch("justifying for %s", m->dmn->proTxHash.ToString());

        CBLSSecretKey skContribution = skContributions[i];

        if (i != myIdx && ShouldSimulateError("justify-lie")) {
            logger.Batch("lying for %s", m->dmn->proTxHash.ToString());
            skContribution.MakeNewKey();
        }

        qj.contributions.emplace_back(i, skContribution);
    }

    if (ShouldSimulateError("justify-omit")) {
        logger.Batch("omitting");
        return;
    }

    qj.sig = activeMasternodeInfo.blsKeyOperator->Sign(qj.GetSignHash());

    logger.Flush();

    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentJustification = true;
        return true;
    });

    pendingMessages.PushPendingMessage(nullptr, qj);
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGJustification& qj, bool& retBan) const
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qj.quorumHash != pindexQuorum->GetBlockHash()) {
        logger.Batch("justification for wrong quorum, rejecting");
        return false;
    }

    auto member = GetMember(qj.proTxHash);
    if (!member) {
        logger.Batch("justifier not a member of this quorum, rejecting justification");
        retBan = true;
        return false;
    }

    if (qj.contributions.empty()) {
        logger.Batch("justification with no contributions");
        retBan = true;
        return false;
    }

    std::set<size_t> contributionsSet;
    for (const auto& p : qj.contributions) {
        if (p.first > members.size()) {
            logger.Batch("invalid contribution index");
            retBan = true;
            return false;
        }

        if (!contributionsSet.emplace(p.first).second) {
            logger.Batch("duplicate contribution index");
            retBan = true;
            return false;
        }

        auto& skShare = p.second;
        if (!skShare.IsValid()) {
            logger.Batch("invalid contribution");
            retBan = true;
            return false;
        }
    }

    if (member->justifications.size() >= 2) {
        // don't do any further processing if we got more than 1 valid justification already
        // this is a DoS protection against members sending multiple justifications with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Batch("dropping justification from %s as we already got %d justifications",
                      member->dmn->proTxHash.ToString(), member->justifications.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGJustification& qj, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    logger.Batch("received justification from %s", qj.proTxHash.ToString());

    auto member = GetMember(qj.proTxHash);

    {
        LOCK(invCs);

        if (member->justifications.size() >= 2) {
            // only relay up to 2 justifications, that's enough to let the other members know about his bad behavior
            return;
        }

        justifications.try_emplace(hash, qj);
        member->justifications.emplace(hash);

        // we always relay, even if further verification fails
        CInv inv(MSG_QUORUM_JUSTIFICATION, hash);
        RelayOtherInvToParticipants(inv);

        quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, member->idx, [&](CDKGDebugMemberStatus& status) {
            status.statusBits.receivedJustification = true;
            return true;
        });

        if (member->justifications.size() > 1) {
            // don't do any further processing if we got more than 1 justification. we already relayed it,
            // so others know about his bad behavior
            logger.Batch("%s did send multiple justifications", member->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
            return;
        }

        if (member->bad) {
            // we locally determined him to be bad (sent none or more then one contributions)
            // don't give him a second chance (but we relay the justification in case other members disagree)
            return;
        }
    }

    for (const auto& p : qj.contributions) {
        auto& member2 = members[p.first];

        if (!member->complaintsFromOthers.count(member2->dmn->proTxHash)) {
            logger.Batch("got justification from %s for %s even though he didn't complain",
                            member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
        }
    }
    if (member->bad) {
        return;
    }

    cxxtimer::Timer t1(true);

    std::list<std::future<bool>> futures;
    for (const auto& p : qj.contributions) {
        auto& member2 = members[p.first];
        auto& skContribution = p.second;

        // watch out to not bail out before these async calls finish (they rely on valid references)
        futures.emplace_back(blsWorker.AsyncVerifyContributionShare(member2->id, receivedVvecs[member->idx], skContribution));
    }
    auto resultIt = futures.begin();
    for (const auto& p : qj.contributions) {
        auto& member2 = members[p.first];
        auto& skContribution = p.second;

        bool result = (resultIt++)->get();
        if (!result) {
            logger.Batch("  %s did send an invalid justification for %s", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
        } else {
            logger.Batch("  %s justified for %s", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            if (AreWeMember() && member2->id == myId) {
                receivedSkContributions[member->idx] = skContribution;
                member->weComplain = false;

                dkgManager.WriteVerifiedSkContribution(params.type, pindexQuorum->GetBlockHash(), member->dmn->proTxHash, skContribution);
            }
            member->complaintsFromOthers.erase(member2->dmn->proTxHash);
        }
    }

    int receivedCount = 0;
    int expectedCount = 0;

    for (const auto& m : members) {
        if (!m->justifications.empty()) {
            receivedCount++;
        }

        if (m->someoneComplain) {
            expectedCount++;
        }
    }

    logger.Batch("verified justification: received=%d/%d time=%d", receivedCount, expectedCount, t1.count());
}

void CDKGSession::VerifyAndCommit(CDKGPendingMessages& pendingMessages)
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__);

    std::vector<size_t> badMembers;
    std::vector<size_t> openComplaintMembers;

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

    SendCommitment(pendingMessages);
}

void CDKGSession::SendCommitment(CDKGPendingMessages& pendingMessages)
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Batch("sending commitment");

    CDKGPrematureCommitment qc(params);
    qc.llmqType = params.type;
    qc.quorumHash = pindexQuorum->GetBlockHash();
    qc.proTxHash = myProTxHash;

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (!m->bad) {
            qc.validMembers[i] = true;
        }
    }

    if (qc.CountValidMembers() < params.minSize) {
        logger.Batch("not enough valid members. not sending commitment");
        return;
    }

    if (ShouldSimulateError("commit-omit")) {
        logger.Batch("omitting");
        return;
    }

    cxxtimer::Timer timerTotal(true);

    cxxtimer::Timer t1(true);
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions(params.type, pindexQuorum, qc.validMembers, memberIndexes, vvecs, skContributions)) {
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
    if (ShouldSimulateError("commit-lie")) {
        lieType = GetRandInt(5);
        logger.Batch("lying on commitment. lieType=%d", lieType);
    }

    if (lieType == 0) {
        CBLSSecretKey k;
        k.MakeNewKey();
        qc.quorumPublicKey = k.GetPublicKey();
    } else if (lieType == 1) {
        (*qc.quorumVvecHash.begin())++;
    }

    uint256 commitmentHash = CLLMQUtils::BuildCommitmentHash(qc.llmqType, qc.quorumHash, qc.validMembers, qc.quorumPublicKey, qc.quorumVvecHash);

    if (lieType == 2) {
        (*commitmentHash.begin())++;
    }

    qc.sig = activeMasternodeInfo.blsKeyOperator->Sign(commitmentHash);
    qc.quorumSig = skShare.Sign(commitmentHash);

    if (lieType == 3) {
        std::vector<uint8_t> buf = qc.sig.ToByteVector();
        buf[5]++;
        qc.sig.SetByteVector(buf);
    } else if (lieType == 4) {
        std::vector<uint8_t> buf = qc.quorumSig.ToByteVector();
        buf[5]++;
        qc.quorumSig.SetByteVector(buf);
    }

    t3.stop();
    timerTotal.stop();

    logger.Batch("built premature commitment. time1=%d, time2=%d, time3=%d, totalTime=%d",
                    t1.count(), t2.count(), t3.count(), timerTotal.count());


    logger.Flush();

    quorumDKGDebugManager->UpdateLocalSessionStatus(params.type, [&](CDKGDebugSessionStatus& status) {
        status.statusBits.sentPrematureCommitment = true;
        return true;
    });

    pendingMessages.PushPendingMessage(nullptr, qc);
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGPrematureCommitment& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qc.quorumHash != pindexQuorum->GetBlockHash()) {
        logger.Batch("commitment for wrong quorum, rejecting");
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Batch("committer not a member of this quorum, rejecting premature commitment");
        retBan = true;
        return false;
    }

    if (qc.validMembers.size() != (size_t)params.size) {
        logger.Batch("invalid validMembers bitset size");
        retBan = true;
        return false;
    }

    if (qc.CountValidMembers() < params.minSize) {
        logger.Batch("invalid validMembers count. validMembersCount=%d", qc.CountValidMembers());
        retBan = true;
        return false;
    }
    if (!qc.sig.IsValid()) {
        logger.Batch("invalid membersSig");
        retBan = true;
        return false;
    }
    if (!qc.quorumSig.IsValid()) {
        logger.Batch("invalid quorumSig");
        retBan = true;
        return false;
    }

    for (size_t i = members.size(); i < (size_t)params.size; i++) {
        if (qc.validMembers[i]) {
            retBan = true;
            logger.Batch("invalid validMembers bitset. bit %d should not be set", i);
            return false;
        }
    }

    if (member->prematureCommitments.size() >= 2) {
        // don't do any further processing if we got more than 1 valid commitment already
        // this is a DoS protection against members sending multiple commitments with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Batch("dropping commitment from %s as we already got %d commitments",
                      member->dmn->proTxHash.ToString(), member->prematureCommitments.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGPrematureCommitment& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    cxxtimer::Timer t1(true);

    logger.Batch("received premature commitment from %s. validMembers=%d", qc.proTxHash.ToString(), qc.CountValidMembers());

    auto member = GetMember(qc.proTxHash);

    {
        LOCK(invCs);

        // keep track of ALL commitments but only relay valid ones (or if we couldn't build the vvec)
        // relaying is done further down
        prematureCommitments.try_emplace(hash, qc);
        member->prematureCommitments.emplace(hash);
    }

    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    BLSVerificationVectorPtr quorumVvec;
    if (dkgManager.GetVerifiedContributions(params.type, pindexQuorum, qc.validMembers, memberIndexes, vvecs, skContributions)) {
        quorumVvec = cache.BuildQuorumVerificationVector(::SerializeHash(memberIndexes), vvecs);
    }

    if (quorumVvec == nullptr) {
        logger.Batch("failed to build quorum verification vector. skipping full verification");
        // we might be the unlucky one who didn't receive all contributions, but we still have to relay
        // the premature commitment as others might be luckier
    } else {
        // we got all information that is needed to verify everything (even though we might not be a member of the quorum)
        // if any of this verification fails, we won't relay this message. This ensures that invalid messages are lost
        // in the network. Nodes relaying such invalid messages to us are not punished as they might have not known
        // all contributions. We only handle up to 2 commitments per member, so a DoS shouldn't be possible

        if ((*quorumVvec)[0] != qc.quorumPublicKey) {
            logger.Batch("calculated quorum public key does not match");
            return;
        }
        uint256 vvecHash = ::SerializeHash(*quorumVvec);
        if (qc.quorumVvecHash != vvecHash) {
            logger.Batch("calculated quorum vvec hash does not match");
            return;
        }

        CBLSPublicKey pubKeyShare = cache.BuildPubKeyShare(::SerializeHash(std::make_pair(memberIndexes, member->id)), quorumVvec, member->id);
        if (!pubKeyShare.IsValid()) {
            logger.Batch("failed to calculate public key share");
            return;
        }

        if (!qc.quorumSig.VerifyInsecure(pubKeyShare, qc.GetSignHash())) {
            logger.Batch("failed to verify quorumSig");
            return;
        }
    }

    LOCK(invCs);
    validCommitments.emplace(hash);

    CInv inv(MSG_QUORUM_PREMATURE_COMMITMENT, hash);
    RelayOtherInvToParticipants(inv);

    quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, member->idx, [&](CDKGDebugMemberStatus& status) {
        status.statusBits.receivedPrematureCommitment = true;
        return true;
    });

    int receivedCount = 0;
    for (const auto& m : members) {
        if (!m->prematureCommitments.empty()) {
            receivedCount++;
        }
    }

    t1.stop();

    logger.Batch("verified premature commitment. received=%d/%d, time=%d", receivedCount, members.size(), t1.count());
}

std::vector<CFinalCommitment> CDKGSession::FinalizeCommitments()
{
    if (!AreWeMember()) {
        return {};
    }

    CDKGLogger logger(*this, __func__);

    typedef std::vector<bool> Key;
    std::map<Key, std::vector<CDKGPrematureCommitment>> commitmentsMap;
    {
        LOCK(invCs);
        for (const auto& p : prematureCommitments) {
            auto& qc = p.second;
            if (!validCommitments.count(p.first)) {
                continue;
            }

            // should have been verified before
            assert(qc.CountValidMembers() >= params.minSize);

            auto it = commitmentsMap.find(qc.validMembers);
            if (it == commitmentsMap.end()) {
                it = commitmentsMap.try_emplace(qc.validMembers, std::vector<CDKGPrematureCommitment>()).first;
            }

            it->second.emplace_back(qc);
        }
    }

    std::vector<CFinalCommitment> finalCommitments;
    for (const auto& p : commitmentsMap) {
        auto& cvec = p.second;
        if (cvec.size() < (size_t)params.minSize) {
            // commitment was signed by a minority
            continue;
        }

        std::vector<CBLSId> signerIds;
        std::vector<CBLSSignature> thresholdSigs;

        auto& first = cvec[0];

        CFinalCommitment fqc(params, first.quorumHash);
        fqc.validMembers = first.validMembers;
        fqc.quorumPublicKey = first.quorumPublicKey;
        fqc.quorumVvecHash = first.quorumVvecHash;

        uint256 commitmentHash = CLLMQUtils::BuildCommitmentHash(fqc.llmqType, fqc.quorumHash, fqc.validMembers, fqc.quorumPublicKey, fqc.quorumVvecHash);

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
        if (!fqc.Verify(pindexQuorum, true)) {
            logger.Batch("failed to verify final commitment");
            continue;
        }

        t2.stop();

        finalCommitments.emplace_back(fqc);

        logger.Batch("final commitment: validMembers=%d, signers=%d, quorumPublicKey=%s, time1=%d, time2=%d",
                        fqc.CountValidMembers(), fqc.CountSigners(), fqc.quorumPublicKey.ToString(),
                        t1.count(), t2.count());
    }

    logger.Flush();

    return finalCommitments;
}

CDKGMember* CDKGSession::GetMember(const uint256& proTxHash) const
{
    auto it = membersMap.find(proTxHash);
    if (it == membersMap.end()) {
        return nullptr;
    }
    return members[it->second].get();
}

void CDKGSession::MarkBadMember(size_t idx)
{
    auto member = members.at(idx).get();
    if (member->bad) {
        return;
    }
    quorumDKGDebugManager->UpdateLocalMemberStatus(params.type, idx, [&](CDKGDebugMemberStatus& status) {
        status.statusBits.bad = true;
        return true;
    });
    member->bad = true;
}

void CDKGSession::RelayOtherInvToParticipants(const CInv& inv) const
{
    LOCK(invCs);
    dkgManager.connman.ForEachNode([&](CNode* pnode) {
        bool relay = false;
        {
            LOCK(pnode->cs_mnauth);
            if (pnode->qwatch) {
                relay = true;
            } else if (!pnode->verifiedProRegTxHash.IsNull() && relayMembers.count(pnode->verifiedProRegTxHash)) {
                relay = true;
            }
        }
        if (relay) {
            pnode->PushOtherInventory(inv);
        }
    });
}

} // namespace llmq
