// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/dkgsession.h>

#include <batchedlogger.h>
#include <bls/bls_worker.h>
#include <evo/deterministicmns.h>
#include <llmq/commitment.h>
#include <llmq/debug.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/options.h>
#include <llmq/utils.h>
#include <masternode/meta.h>
#include <util/helpers.h>
#include <util/std23.h>

#include <chainparams.h>
#include <deploymentstatus.h>
#include <logging.h>
#include <validation.h>

#include <cxxtimer.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <ranges>
#include <type_traits>

namespace llmq {
CDKGLogger::CDKGLogger(const CDKGSession& _quorumDkg, std::string_view _func, int source_line) :
    CBatchedLogger(BCLog::LLMQ_DKG, BCLog::Level::Debug,
                   strprintf("QuorumDKG(type=%s, qIndex=%d, h=%d, member=%d)", _quorumDkg.params.name, _quorumDkg.quorumIndex,
                             _quorumDkg.m_quorum_base_block_index->nHeight, _quorumDkg.AreWeMember()),
                   __FILE__, source_line)
{
}

static std::array<std::atomic<double>, std23::to_underlying(DKGError::type::_COUNT)> simDkgErrorMap{};

void SetSimulatedDKGErrorRate(DKGError::type type, double rate)
{
    if (type >= DKGError::type::_COUNT) return;
    simDkgErrorMap[std23::to_underlying(type)] = rate;
}

double GetSimulatedErrorRate(DKGError::type type)
{
    if (type >= DKGError::type::_COUNT) return 0;
    return simDkgErrorMap[std23::to_underlying(type)];
}

bool CDKGSession::ShouldSimulateError(DKGError::type type) const
{
    if (params.type != Consensus::LLMQType::LLMQ_TEST) {
        return false;
    }
    double rate = GetSimulatedErrorRate(type);
    return GetRandBool(rate);
}

CDKGMember::CDKGMember(const CDeterministicMNCPtr& _dmn, size_t _idx) :
    dmn(_dmn),
    idx(_idx),
    id(_dmn->proTxHash)
{
}

CDKGSession::CDKGSession(CBLSWorker& _blsWorker, CDeterministicMNManager& dmnman, CDKGDebugManager& _dkgDebugManager,
                         CDKGSessionManager& _dkgManager, CQuorumSnapshotManager& qsnapman,
                         const ChainstateManager& chainman, const CBlockIndex* pQuorumBaseBlockIndex,
                         const Consensus::LLMQParams& _params) :
    blsWorker{_blsWorker},
    cache{_blsWorker},
    m_dmnman{dmnman},
    dkgDebugManager{_dkgDebugManager},
    dkgManager{_dkgManager},
    m_qsnapman{qsnapman},
    m_chainman{chainman},
    params{_params},
    m_quorum_base_block_index{pQuorumBaseBlockIndex}
{
}

CDKGSession::~CDKGSession() = default;

bool CDKGSession::Init(const uint256& _myProTxHash, int _quorumIndex)
{
    const auto mns = utils::GetAllQuorumMembers(params.type, {m_dmnman, m_qsnapman, m_chainman, m_quorum_base_block_index});
    quorumIndex = _quorumIndex;
    members.resize(mns.size());
    memberIds.resize(members.size());
    receivedVvecs.resize(members.size());
    receivedVvecsLegacyScheme.resize(members.size());
    receivedSkContributions.resize(members.size());
    vecEncryptedContributions.resize(members.size());

    for (const auto i : util::irange(mns.size())) {
        members[i] = std::make_unique<CDKGMember>(mns[i], i);
        membersMap.emplace(members[i]->dmn->proTxHash, i);
        memberIds[i] = members[i]->id;
    }

    if (!_myProTxHash.IsNull()) {
        for (const auto i : util::irange(members.size())) {
            const auto& m = members[i];
            if (m->dmn->proTxHash == _myProTxHash) {
                myIdx = i;
                myProTxHash = _myProTxHash;
                myId = m->id;
                break;
            }
        }
    }

    CDKGLogger logger(*this, __func__, __LINE__);

    if (LogAcceptDebug(BCLog::LLMQ) && IsQuorumRotationEnabled(params, m_quorum_base_block_index)) {
        int cycleQuorumBaseHeight = m_quorum_base_block_index->nHeight - quorumIndex;
        const CBlockIndex* pCycleQuorumBaseBlockIndex = m_quorum_base_block_index->GetAncestor(cycleQuorumBaseHeight);
        std::stringstream ss;
        for (const auto& mn : members) {
            ss << mn->dmn->proTxHash.ToString().substr(0, 4) << " | ";
        }
        logger.Batch("DKGComposition h[%d] i[%d] DKG:[%s]", pCycleQuorumBaseBlockIndex->nHeight, quorumIndex, ss.str());
    }

    if (mns.size() < static_cast<size_t>(params.minSize)) {
        logger.Batch("not enough members (%d < %d), aborting init", mns.size(), params.minSize);
        return false;
    }

    if (!myProTxHash.IsNull()) {
        dkgDebugManager.InitLocalSessionStatus(params, quorumIndex, m_quorum_base_block_index->GetBlockHash(), m_quorum_base_block_index->nHeight);
        relayMembers = utils::GetQuorumRelayMembers(params, {m_dmnman, m_qsnapman, m_chainman, m_quorum_base_block_index}, myProTxHash,
                                                    /*onlyOutbound=*/true);
        if (LogAcceptDebug(BCLog::LLMQ)) {
            std::stringstream ss;
            for (const auto& r : relayMembers) {
                ss << r.ToString().substr(0, 4) << " | ";
            }
            logger.Batch("forMember[%s] relayMembers[%s]", myProTxHash.ToString().substr(0, 4), ss.str());
        }
    }

    if (myProTxHash.IsNull()) {
        logger.Batch("initialized as observer. mns=%d", mns.size());
    } else {
        logger.Batch("initialized as member. mns=%d", mns.size());
    }

    return true;
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGContribution& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__, __LINE__);

    retBan = false;

    if (qc.quorumHash != m_quorum_base_block_index->GetBlockHash()) {
        logger.Batch("contribution for wrong quorum, rejecting");
        return false;
    }

    auto* member = GetMember(qc.proTxHash);
    if (member == nullptr) {
        logger.Batch("contributor not a member of this quorum, rejecting contribution");
        retBan = true;
        return false;
    }

    // Reject a structurally-invalid (e.g. all-zero) signature here. This is a cheap
    // validity check, not the batched signature verification; it ensures the message
    // never reaches CBLSSignature::AggregateInsecure(), which asserts validity.
    if (!qc.sig.IsValid()) {
        logger.Batch("invalid contribution signature");
        retBan = true;
        return false;
    }

    if (qc.contributions->blobs.size() != members.size()) {
        logger.Batch("invalid contributions count");
        retBan = true;
        return false;
    }
    if (qc.vvec->size() != static_cast<size_t>(params.threshold)) {
        logger.Batch("invalid verification vector length");
        retBan = true;
        return false;
    }

    if (!CBLSWorker::VerifyVerificationVector(*qc.vvec)) {
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

std::optional<CInv> CDKGSession::ReceiveMessage(const CDKGContribution& qc)
{
    CDKGLogger logger(*this, __func__, __LINE__);
    cxxtimer::Timer t1(true);

    auto state = WITH_LOCK(invCs, return ReceiveMessagePreamble(qc, MsgPhase::Contribution, logger));
    if (!state) return std::nullopt;
    auto& [member, hash, inv, should_process] = *state;
    if (!should_process) return inv;

    receivedVvecs[member->idx] = qc.vvec;
    receivedVvecsLegacyScheme[member->idx] = qc.vvecLegacyScheme;

    int receivedCount = std::ranges::count_if(members, [](const auto& m) { return !m->contributions.empty(); });

    logger.Batch("received and relayed contribution. received=%d/%d, time=%d", receivedCount, members.size(), t1.count());

    if (!AreWeMember()) {
        return inv;
    }

    cxxtimer::Timer t2(true);
    dkgManager.WriteVerifiedVvecContribution(params.type, m_quorum_base_block_index, qc.proTxHash, qc.vvec);

    bool complain = false;
    CBLSSecretKey skContribution;
    if (!MaybeDecrypt(*qc.contributions, *myIdx, skContribution, PROTOCOL_VERSION)) {
        logger.Batch("contribution from %s could not be decrypted", member->dmn->proTxHash.ToString());
        complain = true;
    } else if (!skContribution.IsValid()) {
        // A decryptable but structurally-invalid (e.g. all-zero) share must never be
        // retained: it later reaches CBLSSecretKey::AggregateInsecure() in the batched
        // contribution verification, whose assert(IsValid()) would abort an honest node.
        // Treat it as a bad contribution and complain.
        logger.Batch("contribution from %s decrypted to an invalid share", member->dmn->proTxHash.ToString());
        complain = true;
    } else if (member->idx != myIdx && ShouldSimulateError(DKGError::type::COMPLAIN_LIE)) {
        logger.Batch("lying/complaining for %s", member->dmn->proTxHash.ToString());
        complain = true;
    }

    if (complain) {
        member->weComplain = true;
        dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, member->idx, [&](CDKGDebugMemberStatus& status) {
            status.statusBits.weComplain = true;
            return true;
        });
        return inv;
    }

    logger.Batch("decrypted our contribution share. time=%d", t2.count());

    receivedSkContributions[member->idx] = skContribution;
    vecEncryptedContributions[member->idx] = qc.contributions;
    LOCK(cs_pending);
    pendingContributionVerifications.emplace_back(member->idx);
    if (pendingContributionVerifications.size() >= 32) {
        VerifyPendingContributions();
    }
    return inv;
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGComplaint& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__, __LINE__);

    retBan = false;

    if (qc.quorumHash != m_quorum_base_block_index->GetBlockHash()) {
        logger.Batch("complaint for wrong quorum, rejecting");
        return false;
    }

    auto* member = GetMember(qc.proTxHash);
    if (member == nullptr) {
        logger.Batch("complainer not a member of this quorum, rejecting complaint");
        retBan = true;
        return false;
    }

    // Cheap validity check (not the batched signature verification): reject a
    // structurally-invalid signature before it can reach AggregateInsecure().
    if (!qc.sig.IsValid()) {
        logger.Batch("invalid complaint signature");
        retBan = true;
        return false;
    }

    if (qc.badMembers.size() != static_cast<size_t>(params.size)) {
        logger.Batch("invalid badMembers bitset size");
        retBan = true;
        return false;
    }

    if (qc.complainForMembers.size() != static_cast<size_t>(params.size)) {
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

std::optional<CInv> CDKGSession::ReceiveMessage(const CDKGComplaint& qc)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    auto state = WITH_LOCK(invCs, return ReceiveMessagePreamble(qc, MsgPhase::Complaint, logger));
    if (!state) return std::nullopt;
    auto& [member, hash, inv, should_process] = *state;
    if (!should_process) return inv;

    int receivedCount = 0;
    for (const auto i : util::irange(members.size())) {
        const auto& m = members[i];
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
            dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, m->idx, [&](CDKGDebugMemberStatus& status) {
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
    return inv;
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGJustification& qj, bool& retBan) const
{
    CDKGLogger logger(*this, __func__, __LINE__);

    retBan = false;

    if (qj.quorumHash != m_quorum_base_block_index->GetBlockHash()) {
        logger.Batch("justification for wrong quorum, rejecting");
        return false;
    }

    auto* member = GetMember(qj.proTxHash);
    if (member == nullptr) {
        logger.Batch("justifier not a member of this quorum, rejecting justification");
        retBan = true;
        return false;
    }

    // Cheap validity check (not the batched signature verification): reject a
    // structurally-invalid signature before it can reach AggregateInsecure().
    if (!qj.sig.IsValid()) {
        logger.Batch("invalid justification signature");
        retBan = true;
        return false;
    }

    if (qj.contributions.empty()) {
        logger.Batch("justification with no contributions");
        retBan = true;
        return false;
    }

    std::unordered_set<size_t> contributionsSet;
    for (const auto& [index, skContribution] : qj.contributions) {
        if (GetMemberAtIndex(index) == nullptr) {
            logger.Batch("invalid contribution index");
            retBan = true;
            return false;
        }

        if (!contributionsSet.emplace(index).second) {
            logger.Batch("duplicate contribution index");
            retBan = true;
            return false;
        }

        if (!skContribution.IsValid()) {
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

std::optional<CInv> CDKGSession::ReceiveMessage(const CDKGJustification& qj)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    auto state = WITH_LOCK(invCs, return ReceiveMessagePreamble(qj, MsgPhase::Justification, logger));
    if (!state) return std::nullopt;
    auto& [member, hash, inv, should_process] = *state;
    if (!should_process) return inv;

    if (member->bad) {
        // we locally determined him to be bad (sent none or more then one contributions)
        // don't give him a second chance (but we relay the justification in case other members disagree)
        return inv;
    }

    for (const auto& [index, skContribution] : qj.contributions) {
        const auto* member2 = GetMemberAtIndex(index);
        assert(member2);

        if (member->complaintsFromOthers.count(member2->dmn->proTxHash) == 0) {
            logger.Batch("got justification from %s for %s even though he didn't complain",
                            member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
        }
    }
    if (member->bad) {
        return inv;
    }

    cxxtimer::Timer t1(true);

    std::list<std::future<bool>> futures;
    for (const auto& [index, skContribution] : qj.contributions) {
        const auto* member2 = GetMemberAtIndex(index);
        assert(member2);

        // watch out to not bail out before these async calls finish (they rely on valid references)
        futures.emplace_back(blsWorker.AsyncVerifyContributionShare(member2->id, receivedVvecs[member->idx], skContribution));
    }
    auto resultIt = futures.begin();
    for (const auto& [index, skContribution] : qj.contributions) {
        const auto* member2 = GetMemberAtIndex(index);
        assert(member2);

        bool result = (resultIt++)->get();
        if (!result) {
            logger.Batch("  %s did send an invalid justification for %s", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            MarkBadMember(member->idx);
        } else {
            logger.Batch("  %s justified for %s", member->dmn->proTxHash.ToString(), member2->dmn->proTxHash.ToString());
            if (AreWeMember() && member2->id == myId) {
                receivedSkContributions[member->idx] = skContribution;
                member->weComplain = false;

                dkgManager.WriteVerifiedSkContribution(params.type, m_quorum_base_block_index, member->dmn->proTxHash, skContribution);
            }
            member->complaintsFromOthers.erase(member2->dmn->proTxHash);
        }
    }

    auto receivedCount = std::count_if(members.cbegin(), members.cend(), [](const auto& m){
        return !m->justifications.empty();
    });
    auto expectedCount = std::count_if(members.cbegin(), members.cend(), [](const auto& m){
        return m->someoneComplain;
    });

    logger.Batch("verified justification: received=%d/%d time=%d", receivedCount, expectedCount, t1.count());
    return inv;
}

// only performs cheap verifications, but not the signature of the message. this is checked with batched verification
bool CDKGSession::PreVerifyMessage(const CDKGPrematureCommitment& qc, bool& retBan) const
{
    CDKGLogger logger(*this, __func__, __LINE__);

    retBan = false;

    if (qc.quorumHash != m_quorum_base_block_index->GetBlockHash()) {
        logger.Batch("commitment for wrong quorum, rejecting");
        return false;
    }

    auto* member = GetMember(qc.proTxHash);
    if (member == nullptr) {
        logger.Batch("committer not a member of this quorum, rejecting premature commitment");
        retBan = true;
        return false;
    }

    if (qc.validMembers.size() != static_cast<size_t>(params.size)) {
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

    for (const auto i : std::views::iota(members.size(), size_t(params.size))) {
        // cppcheck-suppress useStlAlgorithm
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

std::optional<CInv> CDKGSession::ReceiveMessage(const CDKGPrematureCommitment& qc)
{
    CDKGLogger logger(*this, __func__, __LINE__);

    cxxtimer::Timer t1(true);

    logger.Batch("received premature commitment from %s. validMembers=%d", qc.proTxHash.ToString(), qc.CountValidMembers());

    auto* member = GetMember(qc.proTxHash);
    const uint256 hash = ::SerializeHash(qc);

    {
        LOCK(invCs);

        // keep track of ALL commitments but only relay valid ones (or if we couldn't build the vvec)
        // relaying is done further down
        prematureCommitments.emplace(hash, qc);
        member->prematureCommitments.emplace(hash);
    }

    std::vector<uint16_t> memberIndexes;
    std::vector<BLSVerificationVectorPtr> vvecs;
    std::vector<CBLSSecretKey> skContributions;
    BLSVerificationVectorPtr quorumVvec;
    if (dkgManager.GetVerifiedContributions(params.type, m_quorum_base_block_index, qc.validMembers, memberIndexes, vvecs, skContributions)) {
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
            return std::nullopt;
        }
        uint256 vvecHash = ::SerializeHash(*quorumVvec);
        if (qc.quorumVvecHash != vvecHash) {
            logger.Batch("calculated quorum vvec hash does not match");
            return std::nullopt;
        }

        CBLSPublicKey pubKeyShare = cache.BuildPubKeyShare(::SerializeHash(std::make_pair(memberIndexes, member->id)), quorumVvec, member->id);
        if (!pubKeyShare.IsValid()) {
            logger.Batch("failed to calculate public key share");
            return std::nullopt;
        }

        if (!qc.quorumSig.VerifyInsecure(pubKeyShare, qc.GetSignHash())) {
            logger.Batch("failed to verify quorumSig");
            return std::nullopt;
        }
    }

    WITH_LOCK(invCs, validCommitments.emplace(hash));

    CInv inv(MSG_QUORUM_PREMATURE_COMMITMENT, hash);

    dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, member->idx, [&](CDKGDebugMemberStatus& status) {
        status.statusBits.receivedPrematureCommitment = true;
        return true;
    });

    int receivedCount = std::ranges::count_if(members, [](const auto& m) { return !m->prematureCommitments.empty(); });

    t1.stop();

    logger.Batch("verified premature commitment. received=%d/%d, time=%d", receivedCount, members.size(), t1.count());
    return inv;
}

CDKGMember* CDKGSession::GetMember(const uint256& proTxHash) const
{
    auto it = membersMap.find(proTxHash);
    if (it == membersMap.end()) {
        return nullptr;
    }
    return members[it->second].get();
}

CDKGMember* CDKGSession::GetMemberAtIndex(size_t index) const
{
    if (index >= members.size()) return nullptr;
    return members[index].get();
}

std::vector<CFinalCommitment> CDKGSession::FinalizeCommitments() { return {}; }

CFinalCommitment CDKGSession::FinalizeSingleCommitment() { return {}; }

bool CDKGSession::GetContribution(const uint256& hash, CDKGContribution& ret) const
{
    LOCK(invCs);
    auto it = contributions.find(hash);
    if (it == contributions.end()) return false;
    ret = it->second;
    return true;
}

bool CDKGSession::GetComplaint(const uint256& hash, CDKGComplaint& ret) const
{
    LOCK(invCs);
    auto it = complaints.find(hash);
    if (it == complaints.end()) return false;
    ret = it->second;
    return true;
}

bool CDKGSession::GetJustification(const uint256& hash, CDKGJustification& ret) const
{
    LOCK(invCs);
    auto it = justifications.find(hash);
    if (it == justifications.end()) return false;
    ret = it->second;
    return true;
}

bool CDKGSession::GetPrematureCommitment(const uint256& hash, CDKGPrematureCommitment& ret) const
{
    LOCK(invCs);
    auto it = prematureCommitments.find(hash);
    if (it == prematureCommitments.end() || !validCommitments.count(hash)) return false;
    ret = it->second;
    return true;
}

template <typename MsgType>
std::optional<CDKGSession::ReceiveMessageState> CDKGSession::ReceiveMessagePreamble(const MsgType& msg, MsgPhase phase, CDKGLogger& logger)
{
    auto* member = GetMember(msg.proTxHash);
    if (member == nullptr) {
        logger.Batch("message from non-member %s", msg.proTxHash.ToString());
        return std::nullopt;
    }

    GetDataMsg inv_type{0};
    std::string msg_name;

    // Select member set, inv type, and name based on phase
    auto& member_set = [&]() -> Uint256HashSet& {
        switch (phase) {
        case MsgPhase::Contribution:
            inv_type = MSG_QUORUM_CONTRIB;
            msg_name = "contribution";
            return member->contributions;
        case MsgPhase::Complaint:
            inv_type = MSG_QUORUM_COMPLAINT;
            msg_name = "complaint";
            return member->complaints;
        case MsgPhase::Justification:
            inv_type = MSG_QUORUM_JUSTIFICATION;
            msg_name = "justification";
            return member->justifications;
        }
        assert(false);
    }();

    logger.Batch("received %s from %s", msg_name, msg.proTxHash.ToString());

    if (member_set.size() >= 2) {
        // only relay up to 2 messages, that's enough to let the other members know about his bad behavior
        return std::nullopt;
    }

    const uint256 hash = ::SerializeHash(msg);
    member_set.emplace(hash);
    if constexpr (std::is_same_v<MsgType, CDKGContribution>) {
        contributions.emplace(hash, msg);
    } else if constexpr (std::is_same_v<MsgType, CDKGComplaint>) {
        complaints.emplace(hash, msg);
    } else if constexpr (std::is_same_v<MsgType, CDKGJustification>) {
        justifications.emplace(hash, msg);
    }

    dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, member->idx, [phase](CDKGDebugMemberStatus& status) {
        switch (phase) {
        case MsgPhase::Contribution: status.statusBits.receivedContribution = true; break;
        case MsgPhase::Complaint: status.statusBits.receivedComplaint = true; break;
        case MsgPhase::Justification: status.statusBits.receivedJustification = true; break;
        }
        return true;
    });

    bool should_process{true};
    if (member_set.size() > 1) {
        // don't do any further processing if we got more than 1 justification. we already relayed it,
        // so others know about his bad behavior
        MarkBadMember(member->idx);
        logger.Batch("%s did send multiple %ss", member->dmn->proTxHash.ToString(), msg_name);
        should_process = false;
    }

    // we always relay, even if further verification fails
    return ReceiveMessageState{member, hash, CInv{inv_type, hash}, should_process};
}

template std::optional<CDKGSession::ReceiveMessageState> CDKGSession::ReceiveMessagePreamble<CDKGContribution>(const CDKGContribution&, MsgPhase, CDKGLogger&);
template std::optional<CDKGSession::ReceiveMessageState> CDKGSession::ReceiveMessagePreamble<CDKGComplaint>(const CDKGComplaint&, MsgPhase, CDKGLogger&);
template std::optional<CDKGSession::ReceiveMessageState> CDKGSession::ReceiveMessagePreamble<CDKGJustification>(const CDKGJustification&, MsgPhase, CDKGLogger&);

void CDKGSession::MarkBadMember(size_t idx)
{
    auto* member = members.at(idx).get();
    if (member->bad) {
        return;
    }
    dkgDebugManager.UpdateLocalMemberStatus(params.type, quorumIndex, idx, [&](CDKGDebugMemberStatus& status) {
        status.statusBits.bad = true;
        return true;
    });
    member->bad = true;
}
} // namespace llmq
