// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_dkgsession.h"

#include "quorums_commitment.h"
#include "quorums_dkgsessionmgr.h"
#include "quorums_utils.h"

#include "evo/specialtx.h"

#include "activemasternode.h"
#include "chainparams.h"
#include "init.h"
#include "net.h"
#include "netmessagemaker.h"
#include "spork.h"
#include "univalue.h"
#include "validation.h"

#include "cxxtimer.hpp"

namespace llmq
{

double contributionOmitRate = 0;
double contributionLieRate = 0;
double complainLieRate = 0;
double justifyOmitRate = 0;
double justifyLieRate = 0;
double commitOmitRate = 0;
double commitLieRate = 0;

static bool RandBool(double rate)
{
    const uint64_t v = 100000000;
    uint64_t r = GetRand(v + 1);
    if (r <= v * rate)
        return true;
    return false;
}

CDKGLogger::CDKGLogger(CDKGSession& _quorumDkg, const std::string& _func) :
    CDKGLogger(_quorumDkg.params.type, _quorumDkg.quorumHash, _quorumDkg.height, _quorumDkg.AreWeMember(), _func)
{
}

CDKGLogger::CDKGLogger(Consensus::LLMQType _llmqType, const uint256& _quorumHash, int _height, bool _areWeMember, const std::string& _func) :
    CBatchedLogger(strprintf("QuorumDKG(type=%d, height=%d, member=%d, func=%s)", _llmqType, _height, _areWeMember, _func))
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

bool CDKGSession::Init(int _height, const uint256& _quorumHash, const std::vector<CDeterministicMNCPtr>& mns, const uint256& _myProTxHash)
{
    if (mns.size() < params.minSize) {
        return false;
    }

    height = _height;
    quorumHash = _quorumHash;

    members.resize(mns.size());
    memberIds.resize(members.size());
    receivedVvecs.resize(members.size());
    receivedSkContributions.resize(members.size());

    for (size_t i = 0; i < mns.size(); i++) {
        members[i] = std::unique_ptr<CDKGMember>(new CDKGMember(mns[i], i));
        membersMap.emplace(members[i]->dmn->proTxHash, i);
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

    if (myProTxHash.IsNull()) {
        logger.Printf("initialized as observer. mns=%d\n", mns.size());
    } else {
        logger.Printf("initialized as member. mns=%d\n", mns.size());
    }

    return true;
}

void CDKGSession::Contribute()
{
    CDKGLogger logger(*this, __func__);

    if (!AreWeMember()) {
        return;
    }

    cxxtimer::Timer t1(true);
    logger.Printf("generating contributions\n");
    if (!blsWorker.GenerateContributions(params.threshold, memberIds, vvecContribution, skContributions)) {
        // this should never happen actually
        logger.Printf("GenerateContributions failed\n");
        return;
    }
    logger.Printf("generated contributions. time=%d\n", t1.count());

    SendContributions();
}

void CDKGSession::SendContributions()
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Printf("sending contributions\n");

    if (RandBool(contributionOmitRate)) {
        logger.Printf("omitting\n");
        return;
    }

    CDKGContribution qc;
    qc.llmqType = (uint8_t)params.type;
    qc.quorumHash = quorumHash;
    qc.proTxHash = myProTxHash;
    qc.vvec = vvecContribution;

    cxxtimer::Timer t1(true);
    qc.contributions = std::make_shared<CBLSIESMultiRecipientObjects<CBLSSecretKey>>();
    qc.contributions->InitEncrypt(members.size());

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        CBLSSecretKey skContrib = skContributions[i];

        if (RandBool(contributionLieRate)) {
            logger.Printf("lying for %s\n", m->dmn->proTxHash.ToString());
            skContrib.MakeNewKey();
        }

        if (!qc.contributions->Encrypt(i, m->dmn->pdmnState->pubKeyOperator, skContrib, PROTOCOL_VERSION)) {
            logger.Printf("failed to encrypt contribution for %s\n", m->dmn->proTxHash.ToString());
            return;
        }
    }

    logger.Printf("encrypted contributions. time=%d\n", t1.count());

    qc.sig = activeMasternodeInfo.blsKeyOperator->Sign(qc.GetSignHash());

    logger.Flush();

    uint256 hash = ::SerializeHash(qc);
    bool ban = false;
    if (PreVerifyMessage(hash, qc, ban)) {
        ReceiveMessage(hash, qc, ban);
    }
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const uint256& hash, const CDKGContribution& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    cxxtimer::Timer t1(true);

    retBan = false;

    if (qc.quorumHash != quorumHash) {
        logger.Printf("contribution for wrong quorum, rejecting\n");
        return false;
    }

    if (Seen(hash)) {
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Printf("contributor not a member of this quorum, rejecting contribution\n");
        retBan = true;
        return false;
    }

    if (qc.contributions->blobs.size() != members.size()) {
        logger.Printf("invalid contributions count\n");
        retBan = true;
        return false;
    }
    if (qc.vvec->size() != params.threshold) {
        logger.Printf("invalid verification vector length\n");
        retBan = true;
        return false;
    }

    if (!blsWorker.VerifyVerificationVector(*qc.vvec)) {
        logger.Printf("invalid verification vector\n");
        retBan = true;
        return false;
    }

    if (member->contributions.size() >= 2) {
        // don't do any further processing if we got more than 1 valid contributions already
        // this is a DoS protection against members sending multiple contributions with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Printf("dropping contribution from %s as we already got %d contributions\n", member->dmn->proTxHash.ToString(), member->contributions.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGContribution& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    auto member = GetMember(qc.proTxHash);

    cxxtimer::Timer t1(true);
    logger.Printf("received contribution from %s\n", qc.proTxHash.ToString());

    {
        // relay, no matter if further verification fails
        // This ensures the whole quorum sees the bad behavior
        LOCK(invCs);

        if (member->contributions.size() >= 2) {
            // only relay up to 2 contributions, that's enough to let the other members know about his bad behavior
            return;
        }

        contributions.emplace(hash, qc);
        member->contributions.emplace(hash);

        CInv inv(MSG_QUORUM_CONTRIB, hash);
        invSet.emplace(inv);
        RelayInvToParticipants(inv);

        if (member->contributions.size() > 1) {
            // don't do any further processing if we got more than 1 contribution. we already relayed it,
            // so others know about his bad behavior
            MarkBadMember(member->idx);
            logger.Printf("%s did send multiple contributions\n", member->dmn->proTxHash.ToString());
            return;
        }
    }

    receivedVvecs[member->idx] = qc.vvec;

    int receivedCount = 0;
    for (auto& m : members) {
        if (!m->contributions.empty()) {
            receivedCount++;
        }
    }

    logger.Printf("received and relayed contribution. received=%d/%d, time=%d\n", receivedCount, members.size(), t1.count());

    cxxtimer::Timer t2(true);

    if (!AreWeMember()) {
        // can't further validate
        return;
    }

    dkgManager.WriteVerifiedVvecContribution(params.type, qc.quorumHash, qc.proTxHash, qc.vvec);

    bool complain = false;
    CBLSSecretKey skContribution;
    if (!qc.contributions->Decrypt(myIdx, *activeMasternodeInfo.blsKeyOperator, skContribution, PROTOCOL_VERSION)) {
        logger.Printf("contribution from %s could not be decrypted\n", member->dmn->proTxHash.ToString());
        complain = true;
    } else if (RandBool(complainLieRate)) {
        logger.Printf("lying/complaining for %s\n", member->dmn->proTxHash.ToString());
        complain = true;
    }

    if (complain) {
        member->weComplain = true;
        return;
    }

    logger.Printf("decrypted our contribution share. time=%d\n", t2.count());

    bool verifyPending = false;
    receivedSkContributions[member->idx] = skContribution;
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
    CDKGLogger logger(*this, __func__);

    cxxtimer::Timer t1(true);

    std::vector<size_t> pend = std::move(pendingContributionVerifications);
    if (pend.empty()) {
        return;
    }

    std::vector<size_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;

    for (auto& idx : pend) {
        auto& m = members[idx];
        if (m->bad || m->weComplain) {
            continue;
        }
        memberIndexes.emplace_back(idx);
        vvecs.emplace_back(receivedVvecs[idx]);
        skContributions.emplace_back(receivedSkContributions[idx]);
    }

    auto result = blsWorker.VerifyContributionShares(myId, vvecs, skContributions);
    if (result.size() != memberIndexes.size()) {
        logger.Printf("VerifyContributionShares returned result of size %d but size %d was expected, something is wrong\n", result.size(), memberIndexes.size());
        return;
    }

    for (size_t i = 0; i < memberIndexes.size(); i++) {
        if (!result[i]) {
            auto& m = members[memberIndexes[i]];
            logger.Printf("invalid contribution from %s. will complain later\n", m->dmn->proTxHash.ToString());
            m->weComplain = true;
        } else {
            size_t memberIdx = memberIndexes[i];
            dkgManager.WriteVerifiedSkContribution(params.type, quorumHash, members[memberIdx]->dmn->proTxHash, skContributions[i]);
        }
    }

    logger.Printf("verified %d pending contributions. time=%d\n", pend.size(), t1.count());
}

void CDKGSession::VerifyAndComplain()
{
    if (!AreWeMember()) {
        return;
    }

    VerifyPendingContributions();

    CDKGLogger logger(*this, __func__);

    // we check all members if they sent us their contributions
    // we consider members as bad if they missed to send anything or if they sent multiple
    // in both cases we won't give him a second chance as he is either down, buggy or an adversary
    // we assume that such a participant will be marked as bad by the whole network in most cases,
    // as propagation will ensure that all nodes see the same vvecs/contributions. In case nodes come to
    // different conclusions, the aggregation phase will handle this (most voted quorum key wins)

    cxxtimer::Timer t1(true);

    for (auto& m : members) {
        if (m->bad) {
            continue;
        }
        if (m->contributions.empty()) {
            logger.Printf("%s did not send any contribution\n", m->dmn->proTxHash.ToString());
            MarkBadMember(m->idx);
            continue;
        }
    }

    logger.Printf("verified contributions. time=%d\n", t1.count());
    logger.Flush();

    SendComplaint();
}

void CDKGSession::SendComplaint()
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    CDKGComplaint qc(params);
    qc.llmqType = (uint8_t)params.type;
    qc.quorumHash = quorumHash;
    qc.proTxHash = myProTxHash;

    int badCount = 0;
    int complaintCount = 0;
    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (m->bad) {
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

    logger.Printf("sending complaint. badCount=%d, complaintCount=%d\n", badCount, complaintCount);

    qc.sig = activeMasternodeInfo.blsKeyOperator->Sign(qc.GetSignHash());

    logger.Flush();

    uint256 hash = ::SerializeHash(qc);
    bool ban = false;
    if (PreVerifyMessage(hash, qc, ban)) {
        ReceiveMessage(hash, qc, ban);
    }
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const uint256& hash, const CDKGComplaint& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qc.quorumHash != quorumHash) {
        logger.Printf("complaint for wrong quorum, rejecting\n");
        return false;
    }

    if (Seen(hash)) {
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Printf("complainer not a member of this quorum, rejecting complaint\n");
        retBan = true;
        return false;
    }

    if (qc.badMembers.size() != (size_t)params.size) {
        logger.Printf("invalid badMembers bitset size\n");
        retBan = true;
        return false;
    }

    if (qc.complainForMembers.size() != (size_t)params.size) {
        logger.Printf("invalid complainForMembers bitset size\n");
        retBan = true;
        return false;
    }

    if (member->complaints.size() >= 2) {
        // don't do any further processing if we got more than 1 valid complaints already
        // this is a DoS protection against members sending multiple complaints with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Printf("dropping complaint from %s as we already got %d complaints\n",
                      member->dmn->proTxHash.ToString(), member->complaints.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGComplaint& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    logger.Printf("received complaint from %s\n", qc.proTxHash.ToString());

    auto member = GetMember(qc.proTxHash);

    {
        LOCK(invCs);

        if (member->complaints.size() >= 2) {
            // only relay up to 2 complaints, that's enough to let the other members know about his bad behavior
            return;
        }

        complaints.emplace(hash, qc);
        member->complaints.emplace(hash);

        CInv inv(MSG_QUORUM_COMPLAINT, hash);
        invSet.emplace(inv);
        RelayInvToParticipants(inv);

        if (member->complaints.size() > 1) {
            // don't do any further processing if we got more than 1 complaint. we already relayed it,
            // so others know about his bad behavior
            MarkBadMember(member->idx);
            logger.Printf("%s did send multiple complaints\n", member->dmn->proTxHash.ToString());
            return;
        }
    }

    int receivedCount = 0;
    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (qc.badMembers[i]) {
            logger.Printf("%s voted for %s to be bad\n", member->dmn->proTxHash.ToString(), m->dmn->proTxHash.ToString());
            m->badMemberVotes.emplace(qc.proTxHash);
            if (AreWeMember() && i == myIdx) {
                logger.Printf("%s voted for us to be bad\n", member->dmn->proTxHash.ToString());
            }
        }
        if (qc.complainForMembers[i]) {
            m->complaintsFromOthers.emplace(qc.proTxHash);
            m->someoneComplain = true;
            if (AreWeMember() && i == myIdx) {
                logger.Printf("%s complained about us\n", member->dmn->proTxHash.ToString());
            }
        }
        if (!m->complaints.empty()) {
            receivedCount++;
        }
    }

    logger.Printf("received and relayed complaint. received=%d\n", receivedCount);
}

void CDKGSession::VerifyAndJustify()
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__);

    std::set<uint256> justifyFor;

    for (auto& m : members) {
        if (m->bad) {
            continue;
        }
        if (m->badMemberVotes.size() >= params.dkgBadVotesThreshold) {
            logger.Printf("%s marked as bad as %d other members voted for this\n", m->dmn->proTxHash.ToString(), m->badMemberVotes.size());
            MarkBadMember(m->idx);
            continue;
        }
        if (m->complaints.empty()) {
            continue;
        }
        if (m->complaints.size() != 1) {
            logger.Printf("%s sent multiple complaints\n", m->dmn->proTxHash.ToString());
            MarkBadMember(m->idx);
            continue;
        }

        auto& qc = complaints.at(*m->complaints.begin());
        if (qc.complainForMembers[myIdx]) {
            justifyFor.emplace(qc.proTxHash);
        }
    }

    logger.Flush();
    if (!justifyFor.empty()) {
        SendJustification(justifyFor);
    }
}

void CDKGSession::SendJustification(const std::set<uint256>& forMembers)
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Printf("sending justification for %d members\n", forMembers.size());

    CDKGJustification qj;
    qj.llmqType = (uint8_t)params.type;
    qj.quorumHash = quorumHash;
    qj.proTxHash = myProTxHash;
    qj.contributions.reserve(forMembers.size());

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (!forMembers.count(m->dmn->proTxHash)) {
            continue;
        }
        logger.Printf("justifying for %s\n", m->dmn->proTxHash.ToString());

        CBLSSecretKey skContribution = skContributions[i];

        if (RandBool(justifyLieRate)) {
            logger.Printf("lying for %s\n", m->dmn->proTxHash.ToString());
            skContribution.MakeNewKey();
        }

        qj.contributions.emplace_back(i, skContribution);
    }

    if (RandBool(justifyOmitRate)) {
        logger.Printf("omitting\n");
        return;
    }

    qj.sig = activeMasternodeInfo.blsKeyOperator->Sign(qj.GetSignHash());

    logger.Flush();

    uint256 hash = ::SerializeHash(qj);
    bool ban = false;
    if (PreVerifyMessage(hash, qj, ban)) {
        ReceiveMessage(hash, qj, ban);
    }
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const uint256& hash, const CDKGJustification& qj, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    if (qj.quorumHash != quorumHash) {
        logger.Printf("justification for wrong quorum, rejecting\n");
        return false;
    }

    if (Seen(hash)) {
        return false;
    }

    auto member = GetMember(qj.proTxHash);
    if (!member) {
        logger.Printf("justifier not a member of this quorum, rejecting justification\n");
        retBan = true;
        return false;
    }

    if (qj.contributions.empty()) {
        logger.Printf("justification with no contributions\n");
        retBan = true;
        return false;
    }

    std::set<size_t> contributionsSet;
    for (const auto& p : qj.contributions) {
        if (p.first > members.size()) {
            logger.Printf("invalid contribution index\n");
            retBan = true;
            return false;
        }

        if (!contributionsSet.emplace(p.first).second) {
            logger.Printf("duplicate contribution index\n");
            retBan = true;
            return false;
        }

        auto& skShare = p.second;
        if (!skShare.IsValid()) {
            logger.Printf("invalid contribution\n");
            retBan = true;
            return false;
        }
    }

    if (member->justifications.size() >= 2) {
        // don't do any further processing if we got more than 1 valid justification already
        // this is a DoS protection against members sending multiple justifications with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Printf("dropping justification from %s as we already got %d justifications\n",
                      member->dmn->proTxHash.ToString(), member->justifications.size());
        return false;
    }

    return true;
}

void CDKGSession::ReceiveMessage(const uint256& hash, const CDKGJustification& qj, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    retBan = false;

    logger.Printf("received justification from %s\n", qj.proTxHash.ToString());

    auto member = GetMember(qj.proTxHash);

    {
        LOCK(invCs);

        if (member->justifications.size() >= 2) {
            // only relay up to 2 justifications, that's enough to let the other members know about his bad behavior
            return;
        }

        justifications.emplace(hash, qj);
        member->justifications.emplace(hash);

        // we always relay, even if further verification fails
        CInv inv(MSG_QUORUM_JUSTIFICATION, hash);
        invSet.emplace(inv);
        RelayInvToParticipants(inv);

        if (member->justifications.size() > 1) {
            // don't do any further processing if we got more than 1 justification. we already relayed it,
            // so others know about his bad behavior
            logger.Printf("%s did send multiple justifications\n", member->dmn->proTxHash.ToString());
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
            logger.Printf("got justification from %s for %s even though he didn't complain\n",
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
            logger.Printf("  %s did send an invalid justification for %s\n", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
        } else {
            logger.Printf("  %s justified for %s\n", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            if (AreWeMember() && member2->id == myId) {
                receivedSkContributions[member->idx] = skContribution;
                member->weComplain = false;

                dkgManager.WriteVerifiedSkContribution(params.type, quorumHash, member->dmn->proTxHash, skContribution);
            }
            member->complaintsFromOthers.erase(member2->dmn->proTxHash);
        }
    }

    int receivedCount = 0;
    int expectedCount = 0;

    for (auto& m : members) {
        if (!m->justifications.empty()) {
            receivedCount++;
        }

        if (m->someoneComplain) {
            expectedCount++;
        }
    }

    logger.Printf("verified justification: received=%d/%d time=%d\n", receivedCount, expectedCount, t1.count());
}

void CDKGSession::VerifyAndCommit()
{
    if (!AreWeMember()) {
        return;
    }

    CDKGLogger logger(*this, __func__);

    std::vector<size_t> badMembers;
    std::vector<size_t> openComplaintMembers;

    for (auto& m : members) {
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
        logger.Printf("verification result:\n");
    }
    if (!badMembers.empty()) {
        logger.Printf("  members previously determined as bad:\n");
        for (auto& idx : badMembers) {
            logger.Printf("    %s\n", members[idx]->dmn->proTxHash.ToString());
        }
    }
    if (!openComplaintMembers.empty()) {
        logger.Printf("  members with open complaints and now marked as bad:\n");
        for (auto& idx : openComplaintMembers) {
            logger.Printf("    %s\n", members[idx]->dmn->proTxHash.ToString());
        }
    }

    logger.Flush();

    SendCommitment();
}

void CDKGSession::SendCommitment()
{
    CDKGLogger logger(*this, __func__);

    assert(AreWeMember());

    logger.Printf("sending commitment\n");

    CDKGPrematureCommitment qc(params);
    qc.llmqType = (uint8_t)params.type;
    qc.quorumHash = quorumHash;
    qc.proTxHash = myProTxHash;

    for (size_t i = 0; i < members.size(); i++) {
        auto& m = members[i];
        if (!m->bad) {
            qc.validMembers[i] = true;
        }
    }

    if (qc.CountValidMembers() < params.minSize) {
        logger.Printf("not enough valid members. not sending commitment\n");
        return;
    }

    if (RandBool(commitOmitRate)) {
        logger.Printf("omitting\n");
        return;
    }

    cxxtimer::Timer timerTotal(true);

    cxxtimer::Timer t1(true);
    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    if (!dkgManager.GetVerifiedContributions(params.type, quorumHash, qc.validMembers, memberIndexes, vvecs, skContributions)) {
        logger.Printf("failed to get valid contributions\n");
        return;
    }

    BLSVerificationVectorPtr vvec = cache.BuildQuorumVerificationVector(::SerializeHash(memberIndexes), vvecs);
    if (vvec == nullptr) {
        logger.Printf("failed to build quorum verification vector\n");
        return;
    }
    t1.stop();

    cxxtimer::Timer t2(true);
    CBLSSecretKey skShare = cache.AggregateSecretKeys(::SerializeHash(memberIndexes), skContributions);
    if (!skShare.IsValid()) {
        logger.Printf("failed to build own secret share\n");
        return;
    }
    t2.stop();

    logger.Printf("skShare=%s, pubKeyShare=%s\n", skShare.ToString(), skShare.GetPublicKey().ToString());

    cxxtimer::Timer t3(true);
    qc.quorumPublicKey = (*vvec)[0];
    qc.quorumVvecHash = ::SerializeHash(*vvec);

    int lieType = -1;
    if (RandBool(commitLieRate)) {
        lieType = GetRandInt(5);
        logger.Printf("lying on commitment. lieType=%d\n", lieType);
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
        std::vector<unsigned char> buf;
        qc.sig.GetBuf(buf);
        buf[5]++;
        qc.sig.SetBuf(buf);
    } else if (lieType == 4) {
        std::vector<unsigned char> buf;
        qc.quorumSig.GetBuf(buf);
        buf[5]++;
        qc.quorumSig.SetBuf(buf);
    }

    t3.stop();
    timerTotal.stop();

    logger.Printf("built premature commitment. time1=%d, time2=%d, time3=%d, totalTime=%d\n",
                    t1.count(), t2.count(), t3.count(), timerTotal.count());


    logger.Flush();

    uint256 hash = ::SerializeHash(qc);
    bool ban = false;
    if (PreVerifyMessage(hash, qc, ban)) {
        ReceiveMessage(hash, qc, ban);
    }
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const uint256& hash, const CDKGPrematureCommitment& qc, bool& retBan)
{
    CDKGLogger logger(*this, __func__);

    cxxtimer::Timer t1(true);

    retBan = false;

    if (qc.quorumHash != quorumHash) {
        logger.Printf("commitment for wrong quorum, rejecting\n");
        return false;
    }

    if (Seen(hash)) {
        logger.Printf("already received premature commitment\n");
        return false;
    }

    auto member = GetMember(qc.proTxHash);
    if (!member) {
        logger.Printf("committer not a member of this quorum, rejecting premature commitment\n");
        retBan = true;
        return false;
    }

    if (qc.validMembers.size() != (size_t)params.size) {
        logger.Printf("invalid validMembers bitset size\n");
        retBan = true;
        return false;
    }

    if (qc.CountValidMembers() < params.minSize) {
        logger.Printf("invalid validMembers count. validMembersCount=%d\n", qc.CountValidMembers());
        retBan = true;
        return false;
    }
    if (!qc.sig.IsValid()) {
        logger.Printf("invalid membersSig\n");
        retBan = true;
        return false;
    }
    if (!qc.quorumSig.IsValid()) {
        logger.Printf("invalid quorumSig\n");
        retBan = true;
        return false;
    }

    for (size_t i = members.size(); i < params.size; i++) {
        if (qc.validMembers[i]) {
            retBan = true;
            logger.Printf("invalid validMembers bitset. bit %d should not be set\n", i);
            return false;
        }
    }

    if (member->prematureCommitments.size() >= 2) {
        // don't do any further processing if we got more than 1 valid commitment already
        // this is a DoS protection against members sending multiple commitments with valid signatures to us
        // we must bail out before any expensive BLS verification happens
        logger.Printf("dropping commitment from %s as we already got %d commitments\n",
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

    logger.Printf("received premature commitment from %s. validMembers=%d\n", qc.proTxHash.ToString(), qc.CountValidMembers());

    auto member = GetMember(qc.proTxHash);

    {
        LOCK(invCs);

        // keep track of ALL commitments but only relay valid ones (or if we couldn't build the vvec)
        // relaying is done further down
        prematureCommitments.emplace(hash, qc);
        member->prematureCommitments.emplace(hash);
    }

    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    BLSSecretKeyVector skContributions;
    BLSVerificationVectorPtr quorumVvec;
    if (dkgManager.GetVerifiedContributions(params.type, qc.quorumHash, qc.validMembers, memberIndexes, vvecs, skContributions)) {
        quorumVvec = cache.BuildQuorumVerificationVector(::SerializeHash(memberIndexes), vvecs);
    }

    if (quorumVvec == nullptr) {
        logger.Printf("failed to build quorum verification vector. skipping full verification\n");
        // we might be the unlucky one who didn't receive all contributions, but we still have to relay
        // the premature commitment as others might be luckier
    } else {
        // we got all information that is needed to verify everything (even though we might not be a member of the quorum)
        // if any of this verification fails, we won't relay this message. This ensures that invalid messages are lost
        // in the network. Nodes relaying such invalid messages to us are not punished as they might have not known
        // all contributions. We only handle up to 2 commitments per member, so a DoS shouldn't be possible

        if ((*quorumVvec)[0] != qc.quorumPublicKey) {
            logger.Printf("calculated quorum public key does not match\n");
            return;
        }
        uint256 vvecHash = ::SerializeHash(*quorumVvec);
        if (qc.quorumVvecHash != vvecHash) {
            logger.Printf("calculated quorum vvec hash does not match\n");
            return;
        }

        CBLSPublicKey pubKeyShare = cache.BuildPubKeyShare(::SerializeHash(std::make_pair(memberIndexes, member->id)), quorumVvec, member->id);
        if (!pubKeyShare.IsValid()) {
            logger.Printf("failed to calculate public key share\n");
            return;
        }

        if (!qc.quorumSig.VerifyInsecure(pubKeyShare, qc.GetSignHash())) {
            logger.Printf("failed to verify quorumSig\n");
            return;
        }
    }

    LOCK(invCs);
    validCommitments.emplace(hash);

    CInv inv(MSG_QUORUM_PREMATURE_COMMITMENT, hash);
    invSet.emplace(inv);
    RelayInvToParticipants(inv);

    int receivedCount = 0;
    for (auto& m : members) {
        if (!m->prematureCommitments.empty()) {
            receivedCount++;
        }
    }

    t1.stop();

    logger.Printf("verified premature commitment. received=%d/%d, time=%d\n", receivedCount, members.size(), t1.count());
}

std::vector<CFinalCommitment> CDKGSession::FinalizeCommitments()
{
    if (!AreWeMember()) {
        return {};
    }

    CDKGLogger logger(*this, __func__);

    cxxtimer::Timer totalTimer(true);

    typedef std::vector<bool> Key;
    std::map<Key, std::vector<CDKGPrematureCommitment>> commitmentsMap;

    for (auto& p : prematureCommitments) {
        auto& qc = p.second;
        if (!validCommitments.count(p.first)) {
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

    std::vector<CFinalCommitment> finalCommitments;
    for (const auto& p : commitmentsMap) {
        auto& cvec = p.second;
        if (cvec.size() < params.minSize) {
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

        for (size_t i = 0; i < cvec.size(); i++) {
            auto& qc = cvec[i];

            if (qc.quorumPublicKey != first.quorumPublicKey || qc.quorumVvecHash != first.quorumVvecHash) {
                logger.Printf("quorumPublicKey or quorumVvecHash does not match, skipping");
                continue;
            }

            size_t signerIndex = membersMap[qc.proTxHash];
            const auto& m = members[signerIndex];

            fqc.signers[signerIndex] = true;
            aggSigs.emplace_back(qc.sig);
            aggPks.emplace_back(m->dmn->pdmnState->pubKeyOperator);

            signerIds.emplace_back(m->id);
            thresholdSigs.emplace_back(qc.quorumSig);
        }

        cxxtimer::Timer t1(true);
        fqc.membersSig = CBLSSignature::AggregateSecure(aggSigs, aggPks, commitmentHash);
        t1.stop();

        cxxtimer::Timer t2(true);
        if (!fqc.quorumSig.Recover(thresholdSigs, signerIds)) {
            logger.Printf("failed to recover quorum sig\n");
            continue;
        }
        t2.stop();

        finalCommitments.emplace_back(fqc);

        logger.Printf("final commitment: validMembers=%d, signers=%d, quorumPublicKey=%s, time1=%d, time2=%d\n",
                        fqc.CountValidMembers(), fqc.CountSigners(), fqc.quorumPublicKey.ToString(),
                        t1.count(), t2.count());
    }

    logger.Flush();

    return finalCommitments;
}

CDKGMember* CDKGSession::GetMember(const uint256& proTxHash)
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
    member->bad = true;
}

bool CDKGSession::Seen(const uint256& msgHash)
{
    return !seenMessages.emplace(msgHash).second;
}

void CDKGSession::AddParticipatingNode(NodeId nodeId)
{
    LOCK(invCs);
    g_connman->ForNode(nodeId, [&](CNode* pnode) {
        if (!participatingNodes.emplace(pnode->addr).second) {
            return true;
        }

        for (auto& inv : invSet) {
            pnode->PushInventory(inv);
        }
        return true;
    });
}

void CDKGSession::RelayInvToParticipants(const CInv& inv)
{
    LOCK(invCs);
    g_connman->ForEachNode([&](CNode* pnode) {
        if (participatingNodes.count(pnode->addr)) {
            pnode->PushInventory(inv);
        }
    });
}

}