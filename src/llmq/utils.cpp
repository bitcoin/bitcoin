// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/utils.h>

#include <bls/bls.h>
#include <evo/deterministicmns.h>
#include <llmq/options.h>
#include <llmq/snapshot.h>
#include <llmq/types.h>
#include <masternode/meta.h>
#include <util/irange.h>
#include <util/ranges.h>

#include <chainparams.h>
#include <deploymentstatus.h>
#include <net.h>
#include <random.h>
#include <util/time.h>
#include <util/underlying.h>
#include <validation.h>

#include <atomic>
#include <map>
#include <optional>

/**
 * Forward declarations
 */
std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex);

namespace {
using QuorumMembers = std::vector<CDeterministicMNCPtr>;

std::string ToString(const QuorumMembers& members)
{
    std::stringstream ss;
    for (const auto& mn : members) {
        ss << mn->proTxHash.ToString().substr(0, 4) << "|";
    }
    return ss.str();
}

struct MasternodeScore {
    arith_uint256 m_score;
    CDeterministicMNCPtr m_node;
};

struct QuorumQuarter : public llmq::CycleBase {
    std::vector<QuorumMembers> m_members;

public:
    explicit QuorumQuarter(size_t size) : m_members(size) {}
};

// QuorumMembers per quorumIndex at heights H-Cycle, H-2Cycles, H-3Cycles
struct PreviousQuorumQuarters {
    QuorumQuarter quarterHMinusC;
    QuorumQuarter quarterHMinus2C;
    QuorumQuarter quarterHMinus3C;

public:
    explicit PreviousQuorumQuarters(size_t s) : quarterHMinusC(s), quarterHMinus2C(s), quarterHMinus3C(s) {}

    std::vector<QuorumQuarter*> GetCycles() { return {&quarterHMinusC, &quarterHMinus2C, &quarterHMinus3C}; }
    std::vector<const QuorumQuarter*> GetCycles() const
    {
        return {&quarterHMinusC, &quarterHMinus2C, &quarterHMinus3C};
    }
};

arith_uint256 calculateQuorumScore(const CDeterministicMNCPtr& dmn, const uint256& modifier)
{
    // calculate sha256(sha256(proTxHash, confirmedHash), modifier) per MN
    // Please note that this is not a double-sha256 but a single-sha256
    // The first part is already precalculated (confirmedHashWithProRegTxHash)
    // TODO When https://github.com/bitcoin/bitcoin/pull/13191 gets backported, implement something that is similar but for single-sha256
    uint256 h;
    CSHA256 sha256;
    sha256.Write(dmn->pdmnState->confirmedHashWithProRegTxHash.begin(),
                 dmn->pdmnState->confirmedHashWithProRegTxHash.size());
    sha256.Write(modifier.begin(), modifier.size());
    sha256.Finalize(h.begin());
    return UintToArith256(h);
}

uint256 GetHashModifier(const Consensus::LLMQParams& llmqParams, const Consensus::Params& consensus_params,
                        gsl::not_null<const CBlockIndex*> pCycleQuorumBaseBlockIndex)
{
    ASSERT_IF_DEBUG(pCycleQuorumBaseBlockIndex->nHeight % llmqParams.dkgInterval == 0);
    const CBlockIndex* pWorkBlockIndex = pCycleQuorumBaseBlockIndex->GetAncestor(pCycleQuorumBaseBlockIndex->nHeight - llmq::WORK_DIFF_DEPTH);

    if (DeploymentActiveAfter(pWorkBlockIndex, consensus_params, Consensus::DEPLOYMENT_V20)) {
        // v20 is active: calculate modifier using the new way.
        auto cbcl = GetNonNullCoinbaseChainlock(pWorkBlockIndex);
        if (cbcl.has_value()) {
            // We have a non-null CL signature: calculate modifier using this CL signature
            auto& [bestCLSignature, bestCLHeightDiff] = cbcl.value();
            return ::SerializeHash(std::make_tuple(llmqParams.type, pWorkBlockIndex->nHeight, bestCLSignature));
        }
        // No non-null CL signature found in coinbase: calculate modifier using block hash only
        return ::SerializeHash(std::make_pair(llmqParams.type, pWorkBlockIndex->GetBlockHash()));
    }

    // v20 isn't active yet: calculate modifier using the usual way
    if (llmqParams.useRotation) {
        return ::SerializeHash(std::make_pair(llmqParams.type, pWorkBlockIndex->GetBlockHash()));
    }
    return ::SerializeHash(std::make_pair(llmqParams.type, pCycleQuorumBaseBlockIndex->GetBlockHash()));
}

std::vector<MasternodeScore> CalculateScoresForQuorum(QuorumMembers&& dmns, const uint256& modifier, const bool onlyEvoNodes)
{
    std::vector<MasternodeScore> scores;
    scores.reserve(dmns.size());

    for (auto& dmn : dmns) {
        if (dmn->pdmnState->IsBanned()) continue;
        if (dmn->pdmnState->confirmedHash.IsNull()) {
            // we only take confirmed MNs into account to avoid hash grinding on the ProRegTxHash to sneak MNs into a
            // future quorums
            continue;
        }
        if (onlyEvoNodes && dmn->nType != MnType::Evo) {
            continue;
        }
        scores.emplace_back(calculateQuorumScore(dmn, modifier), std::move(dmn));
    };
    return scores;
}

std::vector<MasternodeScore> CalculateScoresForQuorum(const CDeterministicMNList& mn_list, const uint256& modifier,
                                                      const bool onlyEvoNodes)
{
    std::vector<MasternodeScore> scores;
    scores.reserve(mn_list.GetAllMNsCount());

    mn_list.ForEachMNShared(/*onlyValid=*/true, [&](const auto& dmn) {
        if (dmn->pdmnState->confirmedHash.IsNull()) {
            // we only take confirmed MNs into account to avoid hash grinding on the ProRegTxHash to sneak MNs into a
            // future quorums
            return;
        }
        if (onlyEvoNodes && dmn->nType != MnType::Evo) {
            return;
        }
        scores.emplace_back(calculateQuorumScore(dmn, modifier), dmn);
    });
    return scores;
}

/**
 * Calculate a quorum based on the modifier. The resulting list is deterministically sorted by score
 */
template <typename List>
QuorumMembers CalculateQuorum(List&& mn_list, const uint256& modifier, size_t maxSize = 0, const bool onlyEvoNodes = false)
{
    auto scores = CalculateScoresForQuorum(std::forward<List>(mn_list), modifier, onlyEvoNodes);

    // sort is descending order
    std::sort(scores.rbegin(), scores.rend(), [](const MasternodeScore& a, const MasternodeScore& b) {
        if (a.m_score == b.m_score) {
            // this should actually never happen, but we should stay compatible with how the non-deterministic MNs did the sorting
            // TODO - add assert ?
            return a.m_node->collateralOutpoint < b.m_node->collateralOutpoint;
        }
        return a.m_score < b.m_score;
    });

    // return top maxSize entries only (if specified)
    if (maxSize > 0 && scores.size() > maxSize) {
        scores.resize(maxSize);
    }

    QuorumMembers result;
    result.reserve(scores.size());
    for (auto& [_, node] : scores) {
        result.emplace_back(std::move(node));
    }
    return result;
}

std::vector<QuorumMembers> GetQuorumQuarterMembersBySnapshot(const Consensus::LLMQParams& llmqParams,
                                                             CDeterministicMNManager& dmnman,
                                                             const Consensus::Params& consensus_params,
                                                             const CBlockIndex* pCycleQuorumBaseBlockIndex,
                                                             const llmq::CQuorumSnapshot& snapshot, int nHeight)
{
    if (!llmqParams.useRotation || pCycleQuorumBaseBlockIndex->nHeight % llmqParams.dkgInterval != 0) {
        ASSERT_IF_DEBUG(false);
        return {};
    }

    std::vector<CDeterministicMNCPtr> sortedCombinedMns;
    {
        const CBlockIndex* pWorkBlockIndex = pCycleQuorumBaseBlockIndex->GetAncestor(
            pCycleQuorumBaseBlockIndex->nHeight - llmq::WORK_DIFF_DEPTH);
        auto mn_list = dmnman.GetListForBlock(pWorkBlockIndex);
        const auto modifier = GetHashModifier(llmqParams, consensus_params, pCycleQuorumBaseBlockIndex);
        auto sortedAllMns = CalculateQuorum(mn_list, modifier);

        std::vector<CDeterministicMNCPtr> usedMNs;
        size_t i{0};
        for (const auto& dmn : sortedAllMns) {
            if (snapshot.activeQuorumMembers[i]) {
                usedMNs.push_back(dmn);
            } else {
                if (!dmn->pdmnState->IsBanned()) {
                    // the list begins with all the unused MNs
                    sortedCombinedMns.push_back(dmn);
                }
            }
            i++;
        }

        // Now add the already used MNs to the end of the list
        std::move(usedMNs.begin(), usedMNs.end(), std::back_inserter(sortedCombinedMns));
    }

    if (LogAcceptDebug(BCLog::LLMQ)) {
        LogPrint(BCLog::LLMQ, "%s h[%d] from[%d] sortedCombinedMns[%s]\n", __func__,
                 pCycleQuorumBaseBlockIndex->nHeight, nHeight, ToString(sortedCombinedMns));
    }

    size_t numQuorums = static_cast<size_t>(llmqParams.signingActiveQuorumCount);
    size_t quorumSize = static_cast<size_t>(llmqParams.size);
    auto quarterSize{quorumSize / 4};

    std::vector<QuorumMembers> quarterQuorumMembers(numQuorums);

    if (sortedCombinedMns.empty()) {
        return quarterQuorumMembers;
    }

    switch (snapshot.mnSkipListMode) {
    case SnapshotSkipMode::MODE_NO_SKIPPING: {
        auto itm = sortedCombinedMns.begin();
        for (const size_t i : irange::range(numQuorums)) {
            while (quarterQuorumMembers[i].size() < quarterSize) {
                quarterQuorumMembers[i].push_back(*itm);
                itm++;
                if (itm == sortedCombinedMns.end()) {
                    itm = sortedCombinedMns.begin();
                }
            }
        }
        return quarterQuorumMembers;
    }
    case SnapshotSkipMode::MODE_SKIPPING_ENTRIES: // List holds entries to be skipped
    {
        size_t first_entry_index{0};
        std::vector<int> processesdSkipList;
        for (const auto& s : snapshot.mnSkipList) {
            if (first_entry_index == 0) {
                first_entry_index = s;
                processesdSkipList.push_back(s);
            } else {
                processesdSkipList.push_back(first_entry_index + s);
            }
        }

        int idx = 0;
        auto itsk = processesdSkipList.begin();
        for (const size_t i : irange::range(numQuorums)) {
            while (quarterQuorumMembers[i].size() < quarterSize) {
                if (itsk != processesdSkipList.end() && idx == *itsk) {
                    itsk++;
                } else {
                    quarterQuorumMembers[i].push_back(sortedCombinedMns[idx]);
                }
                idx++;
                if (idx == static_cast<int>(sortedCombinedMns.size())) {
                    idx = 0;
                }
            }
        }
        return quarterQuorumMembers;
    }
    case SnapshotSkipMode::MODE_NO_SKIPPING_ENTRIES: // List holds entries to be kept
    case SnapshotSkipMode::MODE_ALL_SKIPPED:         // Every node was skipped. Returning empty quarterQuorumMembers
    default:
        return quarterQuorumMembers;
    }
}

QuorumMembers ComputeQuorumMembers(Consensus::LLMQType llmqType, const CChainParams& chainparams,
                                   const CDeterministicMNList& mn_list, const CBlockIndex* pQuorumBaseBlockIndex)
{
    bool EvoOnly = (chainparams.GetConsensus().llmqTypePlatform == llmqType) &&
                   DeploymentActiveAfter(pQuorumBaseBlockIndex, chainparams.GetConsensus(), Consensus::DEPLOYMENT_V19);
    const auto& llmq_params_opt = chainparams.GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    if (llmq_params_opt->useRotation || pQuorumBaseBlockIndex->nHeight % llmq_params_opt->dkgInterval != 0) {
        ASSERT_IF_DEBUG(false);
        return {};
    }

    const auto modifier = GetHashModifier(llmq_params_opt.value(), chainparams.GetConsensus(), pQuorumBaseBlockIndex);
    return CalculateQuorum(mn_list, modifier, llmq_params_opt->size, EvoOnly);
}

void BuildQuorumSnapshot(const Consensus::LLMQParams& llmqParams, const Consensus::Params& consensus_params,
                         const CDeterministicMNList& allMns, const CDeterministicMNList& mnUsedAtH,
                         std::vector<CDeterministicMNCPtr>& sortedCombinedMns, llmq::CQuorumSnapshot& quorumSnapshot,
                         std::vector<int>& skipList, const CBlockIndex* pCycleQuorumBaseBlockIndex)
{
    if (!llmqParams.useRotation || pCycleQuorumBaseBlockIndex->nHeight % llmqParams.dkgInterval != 0) {
        ASSERT_IF_DEBUG(false);
        return;
    }

    quorumSnapshot.activeQuorumMembers.resize(allMns.GetAllMNsCount());
    const auto modifier = GetHashModifier(llmqParams, consensus_params, pCycleQuorumBaseBlockIndex);
    auto sortedAllMns = CalculateQuorum(allMns, modifier);

    LogPrint(BCLog::LLMQ, "BuildQuorumSnapshot h[%d] numMns[%d]\n", pCycleQuorumBaseBlockIndex->nHeight,
             allMns.GetAllMNsCount());

    std::fill(quorumSnapshot.activeQuorumMembers.begin(), quorumSnapshot.activeQuorumMembers.end(), false);
    size_t index = {};
    for (const auto& dmn : sortedAllMns) {
        if (mnUsedAtH.HasMN(dmn->proTxHash)) {
            quorumSnapshot.activeQuorumMembers[index] = true;
        }
        index++;
    }

    if (skipList.empty()) {
        quorumSnapshot.mnSkipListMode = SnapshotSkipMode::MODE_NO_SKIPPING;
        quorumSnapshot.mnSkipList.clear();
    } else {
        quorumSnapshot.mnSkipListMode = SnapshotSkipMode::MODE_SKIPPING_ENTRIES;
        quorumSnapshot.mnSkipList = std::move(skipList);
    }
}

std::vector<QuorumMembers> BuildNewQuorumQuarterMembers(const Consensus::LLMQParams& llmqParams,
                                                        const llmq::UtilParameters& util_params,
                                                        const CDeterministicMNList& allMns,
                                                        const PreviousQuorumQuarters& previousQuarters)
{
    if (!llmqParams.useRotation || util_params.m_base_index->nHeight % llmqParams.dkgInterval != 0) {
        ASSERT_IF_DEBUG(false);
        return {};
    }

    size_t nQuorums = static_cast<size_t>(llmqParams.signingActiveQuorumCount);
    std::vector<QuorumMembers> quarterQuorumMembers{nQuorums};

    size_t quorumSize = static_cast<size_t>(llmqParams.size);
    auto quarterSize{quorumSize / 4};
    const auto modifier = GetHashModifier(llmqParams, util_params.m_chainman.GetConsensus(), util_params.m_base_index);

    if (allMns.GetValidMNsCount() < quarterSize) {
        return quarterQuorumMembers;
    }

    auto MnsUsedAtH = CDeterministicMNList();
    std::vector<CDeterministicMNList> MnsUsedAtHIndexed{nQuorums};

    bool skipRemovedMNs = DeploymentActiveAfter(util_params.m_base_index, util_params.m_chainman.GetConsensus(),
                                                Consensus::DEPLOYMENT_V19) ||
                          (util_params.m_chainman.GetParams().NetworkIDString() == CBaseChainParams::TESTNET);

    for (const size_t idx : irange::range(nQuorums)) {
        for (auto* prev_cycle : previousQuarters.GetCycles()) {
            for (const auto& mn : prev_cycle->m_members[idx]) {
                if (skipRemovedMNs && !allMns.HasMN(mn->proTxHash)) {
                    continue;
                }
                if (allMns.IsMNPoSeBanned(mn->proTxHash)) {
                    continue;
                }
                try {
                    MnsUsedAtH.AddMN(mn);
                } catch (const std::runtime_error& e) {
                }
                try {
                    MnsUsedAtHIndexed[idx].AddMN(mn);
                } catch (const std::runtime_error& e) {
                }
            }
        }
    }

    std::vector<CDeterministicMNCPtr> MnsNotUsedAtH;
    allMns.ForEachMNShared(/*onlyValid=*/false, [&MnsUsedAtH, &MnsNotUsedAtH](const auto& dmn) {
        if (!MnsUsedAtH.HasMN(dmn->proTxHash)) {
            if (!dmn->pdmnState->IsBanned()) {
                MnsNotUsedAtH.push_back(dmn);
            }
        }
    });

    auto sortedMnsUsedAtHM = CalculateQuorum(MnsUsedAtH, modifier);
    auto sortedCombinedMnsList = CalculateQuorum(std::move(MnsNotUsedAtH), modifier);
    for (auto& m : sortedMnsUsedAtHM) {
        sortedCombinedMnsList.push_back(std::move(m));
    }

    if (LogAcceptDebug(BCLog::LLMQ)) {
        LogPrint(BCLog::LLMQ, "%s h[%d] sortedCombinedMns[%s]\n", __func__, util_params.m_base_index->nHeight,
                 ToString(sortedCombinedMnsList));
    }

    std::vector<int> skipList;
    size_t firstSkippedIndex = 0;
    size_t idx{0};
    for (const size_t i : irange::range(nQuorums)) {
        auto usedMNsCount = MnsUsedAtHIndexed[i].GetAllMNsCount();
        bool updated{false};
        size_t initial_loop_idx = idx;
        while (quarterQuorumMembers[i].size() < quarterSize && (usedMNsCount + quarterQuorumMembers[i].size() < sortedCombinedMnsList.size())) {
            bool skip{true};
            if (!MnsUsedAtHIndexed[i].HasMN(sortedCombinedMnsList[idx]->proTxHash)) {
                try {
                    // NOTE: AddMN is the one that can throw exceptions, must be exicuted first
                    MnsUsedAtHIndexed[i].AddMN(sortedCombinedMnsList[idx]);
                    quarterQuorumMembers[i].push_back(sortedCombinedMnsList[idx]);
                    updated = true;
                    skip = false;
                } catch (const std::runtime_error& e) {
                }
            }
            if (skip) {
                if (firstSkippedIndex == 0) {
                    firstSkippedIndex = idx;
                    skipList.push_back(idx);
                } else {
                    skipList.push_back(idx - firstSkippedIndex);
                }
            }
            if (++idx == sortedCombinedMnsList.size()) {
                idx = 0;
            }
            if (idx == initial_loop_idx) {
                // we made full "while" loop
                if (!updated) {
                    // there are not enough MNs, there is nothing we can do here
                    return std::vector<QuorumMembers>(nQuorums);
                }
                // reset and try again
                updated = false;
            }
        }
    }

    llmq::CQuorumSnapshot quorumSnapshot{};
    BuildQuorumSnapshot(llmqParams, util_params.m_chainman.GetConsensus(), allMns, MnsUsedAtH, sortedCombinedMnsList,
                        quorumSnapshot, skipList, util_params.m_base_index);
    util_params.m_qsnapman.StoreSnapshotForBlock(llmqParams.type, util_params.m_base_index, quorumSnapshot);

    return quarterQuorumMembers;
}

std::vector<QuorumMembers> ComputeQuorumMembersByQuarterRotation(const Consensus::LLMQParams& llmqParams,
                                                                 const llmq::UtilParameters& util_params)
{
    const int cycleLength = llmqParams.dkgInterval;
    if (!llmqParams.useRotation || util_params.m_base_index->nHeight % llmqParams.dkgInterval != 0) {
        ASSERT_IF_DEBUG(false);
        return {};
    }
    const auto nQuorums{static_cast<size_t>(llmqParams.signingActiveQuorumCount)};

    const CBlockIndex* pWorkBlockIndex = util_params.m_base_index->GetAncestor(util_params.m_base_index->nHeight -
                                                                               llmq::WORK_DIFF_DEPTH);
    CDeterministicMNList allMns = util_params.m_dmnman.GetListForBlock(pWorkBlockIndex);
    LogPrint(BCLog::LLMQ, "ComputeQuorumMembersByQuarterRotation llmqType[%d] nHeight[%d] allMns[%d]\n",
             ToUnderlying(llmqParams.type), util_params.m_base_index->nHeight, allMns.GetValidMNsCount());

    PreviousQuorumQuarters previousQuarters(nQuorums);
    auto prev_cycles{previousQuarters.GetCycles()};
    for (size_t idx{0}; idx < prev_cycles.size(); idx++) {
        prev_cycles[idx]->m_cycle_index = util_params.m_base_index->GetAncestor(util_params.m_base_index->nHeight -
                                                                                (cycleLength * (idx + 1)));
        if (auto opt_snap = util_params.m_qsnapman.GetSnapshotForBlock(llmqParams.type, prev_cycles[idx]->m_cycle_index);
            opt_snap.has_value()) {
            prev_cycles[idx]->m_snap = opt_snap.value();
        } else {
            // TODO: Check if it is triggered from outside (P2P, block validation) and maybe throw an exception
            // assert(false);
            break;
        }
        prev_cycles[idx]->m_members = GetQuorumQuarterMembersBySnapshot(llmqParams, util_params.m_dmnman,
                                                                        util_params.m_chainman.GetConsensus(),
                                                                        prev_cycles[idx]->m_cycle_index,
                                                                        prev_cycles[idx]->m_snap,
                                                                        util_params.m_base_index->nHeight);
    }

    auto newQuarterMembers = BuildNewQuorumQuarterMembers(llmqParams, util_params, allMns, previousQuarters);
    // TODO: Check if it is triggered from outside (P2P, block validation) and maybe throw an exception
    // assert (!newQuarterMembers.empty());

    if (LogAcceptDebug(BCLog::LLMQ)) {
        for (const size_t i : irange::range(nQuorums)) {
            std::stringstream ss;
            for (size_t idx = prev_cycles.size(); idx-- > 0;) {
                ss << strprintf(" %dCmns[%s]", idx, ToString(prev_cycles[idx]->m_members[i]));
            }
            ss << strprintf(" new[%s]", ToString(newQuarterMembers[i]));
            LogPrint(BCLog::LLMQ, "QuarterComposition h[%d] i[%d]:%s\n", util_params.m_base_index->nHeight, i, ss.str());
        }
    }

    std::vector<QuorumMembers> quorumMembers(nQuorums);
    for (const size_t i : irange::range(nQuorums)) {
        // Move elements from previous quarters into quorumMembers
        for (auto* prev_cycle : prev_cycles | std::views::reverse) {
            std::move(prev_cycle->m_members[i].begin(), prev_cycle->m_members[i].end(),
                      std::back_inserter(quorumMembers[i]));
        }
        std::move(newQuarterMembers[i].begin(), newQuarterMembers[i].end(), std::back_inserter(quorumMembers[i]));
        if (LogAcceptDebug(BCLog::LLMQ)) {
            LogPrint(BCLog::LLMQ, "QuorumComposition h[%d] i[%d]: [%s]\n", util_params.m_base_index->nHeight, i,
                     ToString(quorumMembers[i]));
        }
    }

    return quorumMembers;
}
} // anonymous namespace

namespace llmq {
namespace utils {
BlsCheck::BlsCheck() = default;

BlsCheck::BlsCheck(CBLSSignature sig, std::vector<CBLSPublicKey> pubkeys, uint256 msg_hash, std::string id_string) :
    m_sig(sig),
    m_pubkeys(pubkeys),
    m_msg_hash(msg_hash),
    m_id_string(id_string)
{
}

BlsCheck::~BlsCheck() = default;

bool BlsCheck::operator()()
{
    if (m_pubkeys.size() > 1) {
        if (!m_sig.VerifySecureAggregated(m_pubkeys, m_msg_hash)) {
            LogPrint(BCLog::LLMQ, "%s\n", m_id_string);
            return false;
        }
    } else if (m_pubkeys.size() == 1) {
        if (!m_sig.VerifyInsecure(m_pubkeys.back(), m_msg_hash)) {
            LogPrint(BCLog::LLMQ, "%s\n", m_id_string);
            return false;
        }
    } else {
        // we should not get there ever
        LogPrint(BCLog::LLMQ, "%s - no public keys are provided\n", m_id_string);
        return false;
    }
    return true;
}

void BlsCheck::swap(BlsCheck& obj)
{
    std::swap(m_sig, obj.m_sig);
    std::swap(m_pubkeys, obj.m_pubkeys);
    std::swap(m_msg_hash, obj.m_msg_hash);
    std::swap(m_id_string, obj.m_id_string);
}

QuorumMembers GetAllQuorumMembers(Consensus::LLMQType llmqType, const UtilParameters& util_params, bool reset_cache)
{
    static RecursiveMutex cs_members;
    static std::map<Consensus::LLMQType, Uint256LruHashMap<QuorumMembers>> mapQuorumMembers GUARDED_BY(cs_members);
    static RecursiveMutex cs_indexed_members;
    static std::map<Consensus::LLMQType, unordered_lru_cache<std::pair<uint256, int>, QuorumMembers, StaticSaltedHasher>> mapIndexedQuorumMembers GUARDED_BY(cs_indexed_members);

    if (!util_params.m_chainman.IsQuorumTypeEnabled(llmqType, util_params.m_base_index->pprev)) {
        return {};
    }

    std::vector<CDeterministicMNCPtr> quorumMembers;
    {
        LOCK(cs_members);
        if (mapQuorumMembers.empty()) {
            InitQuorumsCache(mapQuorumMembers, util_params.m_chainman.GetConsensus());
        }
        if (reset_cache) {
            mapQuorumMembers[llmqType].clear();
        } else if (mapQuorumMembers[llmqType].get(util_params.m_base_index->GetBlockHash(), quorumMembers)) {
            return quorumMembers;
        }
    }

    const auto& llmq_params_opt = util_params.m_chainman.GetParams().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());
    const auto& llmq_params = llmq_params_opt.value();

    if (IsQuorumRotationEnabled(llmq_params, util_params.m_base_index)) {
        if (LOCK(cs_indexed_members); mapIndexedQuorumMembers.empty()) {
            InitQuorumsCache(mapIndexedQuorumMembers, util_params.m_chainman.GetConsensus());
        }
        /*
         * Quorums created with rotation are now created in a different way. All signingActiveQuorumCount are created
         * during the period of dkgInterval. But they are not created exactly in the same block, they are spread
         * overtime: one quorum in each block until all signingActiveQuorumCount are created. The new concept of
         * quorumIndex is introduced in order to identify them. In every dkgInterval blocks (also called
         * CycleQuorumBaseBlock), the spread quorum creation starts like this: For quorumIndex = 0 :
         * signingActiveQuorumCount Quorum Q with quorumIndex is created at height CycleQuorumBaseBlock + quorumIndex
         */

        int quorumIndex = util_params.m_base_index->nHeight % llmq_params.dkgInterval;
        if (quorumIndex >= llmq_params.signingActiveQuorumCount) {
            return {};
        }
        int cycleQuorumBaseHeight = util_params.m_base_index->nHeight - quorumIndex;
        const CBlockIndex* pCycleQuorumBaseBlockIndex = util_params.m_base_index->GetAncestor(cycleQuorumBaseHeight);

        /*
         * Since mapQuorumMembers stores Quorum members per block hash, and we don't know yet the block hashes of blocks
         * for all quorumIndexes (since these blocks are not created yet) We store them in a second cache
         * mapIndexedQuorumMembers which stores them by {CycleQuorumBaseBlockHash, quorumIndex}
         */
        if (reset_cache) {
            LOCK(cs_indexed_members);
            mapIndexedQuorumMembers[llmqType].clear();
        } else if (LOCK(cs_indexed_members); mapIndexedQuorumMembers[llmqType].get(
                       std::pair(pCycleQuorumBaseBlockIndex->GetBlockHash(), quorumIndex), quorumMembers)) {
            LOCK(cs_members);
            mapQuorumMembers[llmqType].insert(util_params.m_base_index->GetBlockHash(), quorumMembers);
            return quorumMembers;
        }

        auto q = ComputeQuorumMembersByQuarterRotation(llmq_params, util_params.replace_index(pCycleQuorumBaseBlockIndex));
        quorumMembers = q[quorumIndex];

        LOCK(cs_indexed_members);
        for (const size_t i : irange::range(q.size())) {
            mapIndexedQuorumMembers[llmqType].emplace(std::make_pair(pCycleQuorumBaseBlockIndex->GetBlockHash(), i),
                                                      std::move(q[i]));
        }
    } else {
        const CBlockIndex* pWorkBlockIndex = DeploymentActiveAfter(util_params.m_base_index,
                                                                   util_params.m_chainman.GetConsensus(),
                                                                   Consensus::DEPLOYMENT_V20)
                                                 ? util_params.m_base_index->GetAncestor(
                                                       util_params.m_base_index->nHeight - WORK_DIFF_DEPTH)
                                                 : util_params.m_base_index.get();
        CDeterministicMNList mn_list = util_params.m_dmnman.GetListForBlock(pWorkBlockIndex);
        quorumMembers = ComputeQuorumMembers(llmqType, util_params.m_chainman.GetParams(), mn_list,
                                             util_params.m_base_index);
    }

    LOCK(cs_members);
    mapQuorumMembers[llmqType].insert(util_params.m_base_index->GetBlockHash(), quorumMembers);
    return quorumMembers;
}

uint256 DeterministicOutboundConnection(const uint256& proTxHash1, const uint256& proTxHash2)
{
    // We need to deterministically select who is going to initiate the connection. The naive way would be to simply
    // return the min(proTxHash1, proTxHash2), but this would create a bias towards MNs with a numerically low
    // hash. To fix this, we return the proTxHash that has the lowest value of:
    //   hash(min(proTxHash1, proTxHash2), max(proTxHash1, proTxHash2), proTxHashX)
    // where proTxHashX is the proTxHash to compare
    uint256 h1;
    uint256 h2;
    if (proTxHash1 < proTxHash2) {
        h1 = ::SerializeHash(std::make_tuple(proTxHash1, proTxHash2, proTxHash1));
        h2 = ::SerializeHash(std::make_tuple(proTxHash1, proTxHash2, proTxHash2));
    } else {
        h1 = ::SerializeHash(std::make_tuple(proTxHash2, proTxHash1, proTxHash1));
        h2 = ::SerializeHash(std::make_tuple(proTxHash2, proTxHash1, proTxHash2));
    }
    if (h1 < h2) {
        return proTxHash1;
    }
    return proTxHash2;
}

Uint256HashSet GetQuorumConnections(const Consensus::LLMQParams& llmqParams, const CSporkManager& sporkman,
                                    const UtilParameters& util_params, const uint256& forMember, bool onlyOutbound)
{
    if (IsAllMembersConnectedEnabled(llmqParams.type, sporkman)) {
        auto mns = GetAllQuorumMembers(llmqParams.type, util_params);
        Uint256HashSet result;

        for (const auto& dmn : mns) {
            if (dmn->proTxHash == forMember) {
                continue;
            }
            // Determine which of the two MNs (forMember vs dmn) should initiate the outbound connection and which
            // one should wait for the inbound connection. We do this in a deterministic way, so that even when we
            // end up with both connecting to each other, we know which one to disconnect
            uint256 deterministicOutbound = DeterministicOutboundConnection(forMember, dmn->proTxHash);
            if (!onlyOutbound || deterministicOutbound == dmn->proTxHash) {
                result.emplace(dmn->proTxHash);
            }
        }
        return result;
    }
    return GetQuorumRelayMembers(llmqParams, util_params, forMember, onlyOutbound);
}

Uint256HashSet GetQuorumRelayMembers(const Consensus::LLMQParams& llmqParams, const UtilParameters& util_params,
                                     const uint256& forMember, bool onlyOutbound)
{
    auto mns = GetAllQuorumMembers(llmqParams.type, util_params);
    Uint256HashSet result;

    auto calcOutbound = [&](size_t i, const uint256& proTxHash) {
        // Relay to nodes at indexes (i+2^k)%n, where
        //   k: 0..max(1, floor(log2(n-1))-1)
        //   n: size of the quorum/ring
        Uint256HashSet r{};
        if (mns.size() == 1) {
            // No outbound connections are needed when there is one MN only.
            // Also note that trying to calculate results via the algorithm below
            // would result in an endless loop.
            return r;
        }
        int gap = 1;
        int gap_max = (int)mns.size() - 1;
        int k = 0;
        while ((gap_max >>= 1) || k <= 1) {
            size_t idx = (i + gap) % mns.size();
            // It doesn't matter if this node is going to be added to the resulting set or not,
            // we should always bump the gap and the k (step count) regardless.
            // Refusing to bump the gap results in an incomplete set in the best case scenario
            // (idx won't ever change again once we hit `==`). Not bumping k guarantees an endless
            // loop when the first or the second node we check is the one that should be skipped
            // (k <= 1 forever).
            gap <<= 1;
            k++;
            const auto& otherDmn = mns[idx];
            if (otherDmn->proTxHash == proTxHash) {
                continue;
            }
            r.emplace(otherDmn->proTxHash);
        }
        return r;
    };

    for (const auto i : irange::range(mns.size())) {
        const auto& dmn = mns[i];
        if (dmn->proTxHash == forMember) {
            auto r = calcOutbound(i, dmn->proTxHash);
            result.insert(r.begin(), r.end());
        } else if (!onlyOutbound) {
            auto r = calcOutbound(i, dmn->proTxHash);
            if (r.count(forMember)) {
                result.emplace(dmn->proTxHash);
            }
        }
    }

    return result;
}

std::set<size_t> CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, gsl::not_null<const CBlockIndex*> pQuorumBaseBlockIndex,
                                                   size_t memberCount, size_t connectionCount)
{
    static uint256 qwatchConnectionSeed;
    static std::atomic<bool> qwatchConnectionSeedGenerated{false};
    static RecursiveMutex qwatchConnectionSeedCs;
    if (!qwatchConnectionSeedGenerated) {
        LOCK(qwatchConnectionSeedCs);
        qwatchConnectionSeed = GetRandHash();
        qwatchConnectionSeedGenerated = true;
    }

    std::set<size_t> result;
    uint256 rnd = qwatchConnectionSeed;
    for ([[maybe_unused]] const auto _ : irange::range(connectionCount)) {
        rnd = ::SerializeHash(std::make_pair(rnd, std::make_pair(llmqType, pQuorumBaseBlockIndex->GetBlockHash())));
        result.emplace(rnd.GetUint64(0) % memberCount);
    }
    return result;
}

bool EnsureQuorumConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, const CSporkManager& sporkman,
                             const UtilParameters& util_params, const CDeterministicMNList& tip_mn_list,
                             const uint256& myProTxHash, bool is_masternode, bool quorums_watch)
{
    if (!is_masternode && !quorums_watch) {
        return false;
    }

    auto members = GetAllQuorumMembers(llmqParams.type, util_params);
    if (members.empty()) {
        return false;
    }

    bool isMember = ranges::find_if(members, [&](const auto& dmn) { return dmn->proTxHash == myProTxHash; }) != members.end();

    if (!isMember && !quorums_watch) {
        return false;
    }

    LogPrint(BCLog::NET_NETCONN, "%s -- isMember=%d for quorum %s:\n", __func__, isMember,
             util_params.m_base_index->GetBlockHash().ToString());

    Uint256HashSet connections;
    if (isMember) {
        connections = GetQuorumConnections(llmqParams, sporkman, util_params, myProTxHash, /*onlyOutbound=*/true);
    } else {
        auto cindexes = CalcDeterministicWatchConnections(llmqParams.type, util_params.m_base_index, members.size(), 1);
        for (auto idx : cindexes) {
            connections.emplace(members[idx]->proTxHash);
        }
    }
    if (!connections.empty()) {
        if (!connman.HasMasternodeQuorumNodes(llmqParams.type, util_params.m_base_index->GetBlockHash()) &&
            LogAcceptDebug(BCLog::LLMQ)) {
            std::string debugMsg = strprintf("%s -- adding masternodes quorum connections for quorum %s:\n", __func__,
                                             util_params.m_base_index->GetBlockHash().ToString());
            for (const auto& c : connections) {
                auto dmn = tip_mn_list.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)\n", c.ToString(),
                                          dmn->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
                }
            }
            LogPrint(BCLog::NET_NETCONN, debugMsg.c_str()); /* Continued */
        }
        connman.SetMasternodeQuorumNodes(llmqParams.type, util_params.m_base_index->GetBlockHash(), connections);
        connman.SetMasternodeQuorumRelayMembers(llmqParams.type, util_params.m_base_index->GetBlockHash(), connections);
    }
    return true;
}

void AddQuorumProbeConnections(const Consensus::LLMQParams& llmqParams, CConnman& connman, CMasternodeMetaMan& mn_metaman,
                               const CSporkManager& sporkman, const UtilParameters& util_params,
                               const CDeterministicMNList& tip_mn_list, const uint256& myProTxHash)
{
    assert(mn_metaman.IsValid());

    if (!IsQuorumPoseEnabled(llmqParams.type, sporkman)) {
        return;
    }

    auto members = GetAllQuorumMembers(llmqParams.type, util_params);
    auto curTime = GetTime<std::chrono::seconds>().count();

    std::set<uint256> probeConnections;
    for (const auto& dmn : members) {
        if (dmn->proTxHash == myProTxHash) {
            continue;
        }
        auto lastOutbound = mn_metaman.GetLastOutboundSuccess(dmn->proTxHash);
        if (curTime - lastOutbound < 10 * 60) {
            // avoid re-probing nodes too often
            continue;
        }
        probeConnections.emplace(dmn->proTxHash);
    }

    if (!probeConnections.empty()) {
        if (LogAcceptDebug(BCLog::LLMQ)) {
            std::string debugMsg = strprintf("%s -- adding masternodes probes for quorum %s:\n", __func__,
                                             util_params.m_base_index->GetBlockHash().ToString());
            for (const auto& c : probeConnections) {
                auto dmn = tip_mn_list.GetValidMN(c);
                if (!dmn) {
                    debugMsg += strprintf("  %s (not in valid MN set anymore)\n", c.ToString());
                } else {
                    debugMsg += strprintf("  %s (%s)\n", c.ToString(),
                                          dmn->pdmnState->netInfo->GetPrimary().ToStringAddrPort());
                }
            }
            LogPrint(BCLog::NET_NETCONN, debugMsg.c_str()); /* Continued */
        }
        connman.AddPendingProbeConnections(probeConnections);
    }
}
} // namespace utils
} // namespace llmq
