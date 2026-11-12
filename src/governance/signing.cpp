// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/signing.h>

#include <active/masternode.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <governance/superblock.h>
#include <masternode/sync.h>

#include <chainparams.h>
#include <logging.h>
#include <timedata.h>
#include <util/time.h>
#include <validation.h>

#include <algorithm>
#include <ranges>

namespace {
constexpr std::chrono::seconds GOVERNANCE_FUDGE_WINDOW{2h};
constexpr int NO_VOTE_DEADLINE_BLOCKS{3};
} // anonymous namespace

GovernanceSigner::GovernanceSigner(CDeterministicMNManager& dmnman, CGovernanceManager& govman,
                                   governance::SuperblockManager& superblocks, const CActiveMasternodeManager& mn_activeman,
                                   const ChainstateManager& chainman, const CMasternodeSync& mn_sync) :
    m_dmnman{dmnman},
    m_govman{govman},
    m_superblocks{superblocks},
    m_mn_activeman{mn_activeman},
    m_chainman{chainman},
    m_mn_sync{mn_sync}
{
}

std::optional<const CSuperblock> GovernanceSigner::CreateSuperblockCandidate(int nHeight) const
{
    if (!m_govman.IsValid()) return std::nullopt;
    if (!m_mn_sync.IsSynced()) return std::nullopt;
    if (nHeight % Params().GetConsensus().nSuperblockCycle <
        Params().GetConsensus().nSuperblockCycle - Params().GetConsensus().nSuperblockMaturityWindow)
        return std::nullopt;
    if (HasAlreadyVotedFundingTrigger()) return std::nullopt;

    const auto approvedProposals = m_govman.GetApprovedProposals(m_dmnman.GetListAtChainTip());
    if (approvedProposals.empty()) {
        LogPrint(BCLog::GOBJECT, "%s -- nHeight:%d empty approvedProposals\n", __func__, nHeight);
        return std::nullopt;
    }

    std::vector<CGovernancePayment> payments;
    int nLastSuperblock;
    int nNextSuperblock;

    CSuperblock::GetNearestSuperblocksHeights(nHeight, nLastSuperblock, nNextSuperblock);
    auto SBEpochTime = static_cast<int64_t>(GetTime<std::chrono::seconds>().count() +
                                            (nNextSuperblock - nHeight) * 2.62 * 60);
    auto governanceBudget = WITH_LOCK(::cs_main, return CSuperblock::GetPaymentsLimit(m_chainman.ActiveChain(), nNextSuperblock));

    CAmount budgetAllocated{};
    for (const auto& proposal : approvedProposals) {
        // Extract payment address and amount from proposal
        UniValue jproposal = proposal->GetJSONObject();

        CTxDestination dest = DecodeDestination(jproposal["payment_address"].getValStr());
        if (!IsValidDestination(dest)) continue;

        CAmount nAmount{};
        try {
            nAmount = ParsePaymentAmount(jproposal["payment_amount"].getValStr());
        } catch (const std::runtime_error& e) {
            LogPrint(BCLog::GOBJECT, "%s -- nHeight:%d Skipping payment exception:%s\n", __func__, nHeight, e.what());
            continue;
        }

        // Construct CGovernancePayment object and make sure it is valid
        CGovernancePayment payment(dest, nAmount, proposal->GetHash());
        if (!payment.IsValid()) continue;

        // Skip proposals that are too expensive
        if (budgetAllocated + payment.nAmount > governanceBudget) continue;

        int64_t windowStart = jproposal["start_epoch"].getInt<int64_t>() - count_seconds(GOVERNANCE_FUDGE_WINDOW);
        int64_t windowEnd = jproposal["end_epoch"].getInt<int64_t>() + count_seconds(GOVERNANCE_FUDGE_WINDOW);

        // Skip proposals if the SB isn't within the proposal time window
        if (SBEpochTime < windowStart) {
            LogPrint(BCLog::GOBJECT, "%s -- nHeight:%d SB:%d windowStart:%d\n", __func__, nHeight, SBEpochTime,
                     windowStart);
            continue;
        }
        if (SBEpochTime > windowEnd) {
            LogPrint(BCLog::GOBJECT, "%s -- nHeight:%d SB:%d windowEnd:%d\n", __func__, nHeight, SBEpochTime, windowEnd);
            continue;
        }

        // Keep track of total budget allocation
        budgetAllocated += payment.nAmount;

        // Add the payment
        payments.push_back(payment);
    }

    // No proposals made the cut
    if (payments.empty()) {
        LogPrint(BCLog::GOBJECT, "%s -- CreateSuperblockCandidate nHeight:%d empty payments\n", __func__, nHeight);
        return std::nullopt;
    }

    // Sort by proposal hash descending
    std::sort(payments.begin(), payments.end(), [](const CGovernancePayment& a, const CGovernancePayment& b) {
        return UintToArith256(a.proposalHash) > UintToArith256(b.proposalHash);
    });

    // Create Superblock
    return CSuperblock(nNextSuperblock, std::move(payments));
}

std::optional<const CGovernanceObject> GovernanceSigner::CreateGovernanceTrigger(const std::optional<const CSuperblock>& sb_opt)
{
    // no sb_opt, no trigger
    if (!sb_opt.has_value()) return std::nullopt;

    // TODO: Check if nHashParentIn, nRevision and nCollateralHashIn are correct
    LOCK(::cs_main);

    // Check if identical trigger (equal DataHash()) is already created (signed by other masternode)
    CGovernanceObject gov_sb(uint256(), 1, GetAdjustedTime(), uint256(), sb_opt.value().GetHexStrData());
    if (auto identical_sb = m_govman.FindGovernanceObjectByDataHash(gov_sb.GetDataHash())) {
        // Somebody submitted a trigger with the same data, support it instead of submitting a duplicate
        return std::make_optional<CGovernanceObject>(*identical_sb);
    }

    // Nobody submitted a trigger we'd like to see, so let's do it but only if we are the payee
    const CBlockIndex* tip = m_chainman.ActiveChain().Tip();
    CDeterministicMNList mnList;
    try {
        mnList = m_dmnman.GetListForBlock(tip);
    } catch (const BlockDataUnavailableError& e) {
        // This runs on the scheduler thread, where an uncaught exception
        // terminates the node. Unavailable history is expected while a
        // snapshot's background chainstate is still catching up, so skip this
        // trigger attempt. Any other exception means local EvoDB/list
        // corruption and must not be hidden, so it deliberately stays
        // unhandled.
        LogPrint(BCLog::GOBJECT, "%s -- masternode list unavailable: %s\n", __func__, e.what());
        return std::nullopt;
    }
    const auto mn_payees = mnList.GetProjectedMNPayees(tip);

    if (mn_payees.empty()) {
        LogPrint(BCLog::GOBJECT, "%s -- payee list is empty\n", __func__);
        return std::nullopt;
    }

    if (mn_payees.front()->proTxHash != m_mn_activeman.GetProTxHash()) {
        LogPrint(BCLog::GOBJECT, "%s -- we are not the payee, skipping\n", __func__);
        return std::nullopt;
    }
    gov_sb.SetMasternodeOutpoint(m_mn_activeman.GetOutPoint());
    gov_sb.SetSignature(m_mn_activeman.SignBasic(gov_sb.GetSignatureHash()));

    if (std::string strError; !gov_sb.IsValidLocally(m_dmnman.GetListAtChainTip(), m_chainman, strError, true)) {
        LogPrint(BCLog::GOBJECT, "%s -- Created trigger is invalid:%s\n", __func__, strError);
        return std::nullopt;
    }

    if (!m_govman.MasternodeRateCheck(gov_sb)) {
        LogPrint(BCLog::GOBJECT, "%s -- Trigger rejected because of rate check failure hash(%s)\n", __func__,
                 gov_sb.GetHash().ToString());
        return std::nullopt;
    }

    // The trigger we just created looks good, submit it
    m_govman.AddGovernanceObject(gov_sb, "<local>");
    return std::make_optional<CGovernanceObject>(gov_sb);
}

void GovernanceSigner::VoteGovernanceTriggers(const std::optional<const CGovernanceObject>& trigger_opt,
                                              std::optional<int> superblock_height, int nHeight)
{
    // only active masternodes can vote on triggers
    if (m_mn_activeman.GetProTxHash().IsNull()) return;
    // Until we are synced we can't build a candidate, so we have nothing to compare triggers to
    if (!m_mn_sync.IsSynced()) return;

    LOCK(::cs_main);

    if (trigger_opt.has_value()) {
        // We should never vote "yes" on another trigger or the same trigger twice
        assert(!votedFundingYesTrigger.has_value());
        // Vote YES-FUNDING for the trigger we like, unless we already did
        const uint256 gov_sb_hash = trigger_opt.value().GetHash();
        bool voted_already{false};
        if (vote_rec_t voteRecord; trigger_opt.value().GetCurrentMNVotes(m_mn_activeman.GetOutPoint(), voteRecord)) {
            const auto& strFunc = __func__;
            // Let's see if there is a VOTE_SIGNAL_FUNDING vote from us already
            voted_already = std::ranges::any_of(voteRecord.mapInstances, [&](const auto& voteInstancePair) {
                if (voteInstancePair.first == VOTE_SIGNAL_FUNDING) {
                    if (voteInstancePair.second.eOutcome == VOTE_OUTCOME_YES) {
                        votedFundingYesTrigger = FundingTriggerVote{gov_sb_hash, superblock_height.value()};
                    }
                    LogPrint(BCLog::GOBJECT, /* Continued */
                             "%s -- Not voting YES-FUNDING for trigger:%s, we voted %s for it already\n", strFunc,
                             gov_sb_hash.ToString(),
                             CGovernanceVoting::ConvertOutcomeToString(voteInstancePair.second.eOutcome));
                    return true;
                }
                return false;
            });
        }
        if (!voted_already) {
            // No previous VOTE_SIGNAL_FUNDING was found, vote now
            if (VoteFundingTrigger(gov_sb_hash, VOTE_OUTCOME_YES)) {
                LogPrint(BCLog::GOBJECT, "%s -- Voting YES-FUNDING for new trigger:%s success\n", __func__,
                         gov_sb_hash.ToString());
                votedFundingYesTrigger = FundingTriggerVote{gov_sb_hash, superblock_height.value()};
            } else {
                LogPrint(BCLog::GOBJECT, "%s -- Voting YES-FUNDING for new trigger:%s failed\n", __func__,
                         gov_sb_hash.ToString());
                // this should never happen, bail out
                return;
            }
        }
    }

    if (superblock_height.has_value() && !trigger_opt.has_value() &&
        nHeight + NO_VOTE_DEADLINE_BLOCKS < superblock_height.value()) {
        LogPrint(BCLog::GOBJECT, "%s -- Not voting NO-FUNDING at height:%d, waiting for a trigger for superblock:%d\n",
                 __func__, nHeight, superblock_height.value());
        return;
    }

    // Vote NO-FUNDING for the rest of the active triggers
    const auto activeTriggers = m_superblocks.GetActiveTriggers();
    for (const auto& trigger : activeTriggers) {
        auto govobj = m_govman.FindGovernanceObject(trigger->GetGovernanceObjHash());
        if (!govobj) {
            LogPrint(BCLog::GOBJECT, "%s -- Not voting NO-FUNDING for unknown trigger %s\n", __func__,
                     trigger->GetGovernanceObjHash().ToString());
            continue;
        }

        const uint256 trigger_hash = govobj->GetHash();
        if (trigger->GetBlockHeight() <= m_govman.GetCachedBlockHeight()) {
            // ignore triggers from the past
            LogPrint(BCLog::GOBJECT, "%s -- Not voting NO-FUNDING for outdated trigger:%s\n", __func__,
                     trigger_hash.ToString());
            continue;
        }
        if (nHeight < trigger->GetBlockHeight() - Params().GetConsensus().nSuperblockMaturityWindow) {
            // We can't build a candidate for this trigger's superblock yet, so we have nothing to compare it to
            LogPrint(BCLog::GOBJECT, "%s -- Not voting NO-FUNDING for trigger:%s, its maturity window has not started\n",
                     __func__, trigger_hash.ToString());
            continue;
        }
        if (votedFundingYesTrigger.has_value() && trigger_hash == votedFundingYesTrigger->hash) {
            // Skip actual trigger
            LogPrint(BCLog::GOBJECT, "%s -- Not voting NO-FUNDING for trigger:%s, we voted yes for it already\n",
                     __func__, trigger_hash.ToString());
            continue;
        }
        if (vote_rec_t voteRecord; govobj->GetCurrentMNVotes(m_mn_activeman.GetOutPoint(), voteRecord)) {
            const auto& strFunc = __func__;
            if (std::ranges::any_of(voteRecord.mapInstances, [&](const auto& voteInstancePair) {
                    if (voteInstancePair.first == VOTE_SIGNAL_FUNDING) {
                        LogPrint(BCLog::GOBJECT, /* Continued */
                                 "%s -- Not voting NO-FUNDING for trigger:%s, we voted %s for it already\n", strFunc,
                                 trigger_hash.ToString(),
                                 CGovernanceVoting::ConvertOutcomeToString(voteInstancePair.second.eOutcome));
                        return true;
                    }
                    return false;
                })) {
                continue;
            }
        }
        if (!VoteFundingTrigger(trigger_hash, VOTE_OUTCOME_NO)) {
            LogPrint(BCLog::GOBJECT, "%s -- Voting NO-FUNDING for trigger:%s failed\n", __func__, trigger_hash.ToString());
            // failing here is ok-ish
            continue;
        }
        LogPrint(BCLog::GOBJECT, "%s -- Voting NO-FUNDING for trigger:%s success\n", __func__, trigger_hash.ToString());
    }
}

bool GovernanceSigner::VoteFundingTrigger(const uint256& nHash, const vote_outcome_enum_t outcome)
{
    CGovernanceVote vote(m_mn_activeman.GetOutPoint(), nHash, VOTE_SIGNAL_FUNDING, outcome);
    vote.SetTime(GetAdjustedTime());
    vote.SetSignature(m_mn_activeman.SignBasic(vote.GetSignatureHash()));

    CGovernanceException exception;
    if (!m_govman.ProcessVoteAndRelay(vote, exception)) {
        LogPrint(BCLog::GOBJECT, "%s -- Vote FUNDING %d for trigger:%s failed:%s\n", __func__, outcome,
                 nHash.ToString(), exception.what());
        return false;
    }

    return true;
}

bool GovernanceSigner::HasAlreadyVotedFundingTrigger() const
{
    return votedFundingYesTrigger.has_value();
}

void GovernanceSigner::ResetVotedFundingTrigger()
{
    votedFundingYesTrigger = std::nullopt;
}

void GovernanceSigner::UpdatedBlockTip(const CBlockIndex* pindex)
{
    if (votedFundingYesTrigger.has_value() && pindex->nHeight >= votedFundingYesTrigger->superblock_height) {
        ResetVotedFundingTrigger();
    }
    const auto sb_opt = CreateSuperblockCandidate(pindex->nHeight);
    const auto trigger_opt = CreateGovernanceTrigger(sb_opt);
    VoteGovernanceTriggers(trigger_opt, sb_opt ? std::make_optional(sb_opt->GetBlockHeight()) : std::nullopt,
                           pindex->nHeight);
    CSuperblock_sptr pSuperblock;
    if (m_superblocks.GetBestSuperblock(m_dmnman.GetListAtChainTip(), pSuperblock, pindex->nHeight)) {
        ResetVotedFundingTrigger();
    }
}
