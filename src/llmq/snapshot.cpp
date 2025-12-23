// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/snapshot.h>

#include <chainparams.h>
#include <evo/evodb.h>
#include <evo/simplifiedmns.h>
#include <evo/smldiff.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <validation.h>

#include <univalue.h>

namespace {
constexpr std::string_view DB_QUORUM_SNAPSHOT{"llmq_S"};

//! Constructs a llmq::CycleData and populate it with metadata
std::optional<llmq::CycleData> ConstructCycle(llmq::CQuorumSnapshotManager& qsnapman,
                                              const Consensus::LLMQType& llmq_type, bool skip_snap, int32_t height,
                                              gsl::not_null<const CBlockIndex*> index_tip, std::string& error)
{
    llmq::CycleData ret;
    ret.m_cycle_index = index_tip->GetAncestor(height);
    if (!ret.m_cycle_index) {
        error = "Cannot find block";
        return std::nullopt;
    }
    ret.m_work_index = ret.m_cycle_index->GetAncestor(ret.m_cycle_index->nHeight - llmq::WORK_DIFF_DEPTH);
    if (!ret.m_work_index) {
        error = "Cannot find work block";
        return std::nullopt;
    }
    if (!skip_snap) {
        if (auto opt_snap = qsnapman.GetSnapshotForBlock(llmq_type, ret.m_cycle_index); opt_snap.has_value()) {
            ret.m_snap = opt_snap.value();
        } else {
            error = "Cannot find quorum snapshot";
            return std::nullopt;
        }
    }
    return ret;
}
} // anonymous namespace

namespace llmq {
bool BuildQuorumRotationInfo(CDeterministicMNManager& dmnman, CQuorumSnapshotManager& qsnapman,
                             const ChainstateManager& chainman, const CQuorumManager& qman,
                             const CQuorumBlockProcessor& qblockman, const CGetQuorumRotationInfo& request,
                             bool use_legacy_construction, CQuorumRotationInfo& response, std::string& errorRet)
{
    AssertLockHeld(::cs_main);

    std::vector<const CBlockIndex*> baseBlockIndexes;
    if (request.baseBlockHashes.size() == 0) {
        const CBlockIndex* blockIndex = chainman.ActiveChain().Genesis();
        if (!blockIndex) {
            errorRet = strprintf("genesis block not found");
            return false;
        }
        baseBlockIndexes.push_back(blockIndex);
    } else {
        for (const auto& blockHash : request.baseBlockHashes) {
            const CBlockIndex* blockIndex = chainman.m_blockman.LookupBlockIndex(blockHash);
            if (!blockIndex) {
                errorRet = strprintf("block %s not found", blockHash.ToString());
                return false;
            }
            if (!chainman.ActiveChain().Contains(blockIndex)) {
                errorRet = strprintf("block %s is not in the active chain", blockHash.ToString());
                return false;
            }
            baseBlockIndexes.push_back(blockIndex);
        }
        if (use_legacy_construction) {
            std::sort(baseBlockIndexes.begin(), baseBlockIndexes.end(),
                      [](const CBlockIndex* a, const CBlockIndex* b) { return a->nHeight < b->nHeight; });
        }
    }

    const CBlockIndex* tipBlockIndex = chainman.ActiveChain().Tip();
    if (!tipBlockIndex) {
        errorRet = strprintf("tip block not found");
        return false;
    }
    if (use_legacy_construction) {
        // Build MN list Diff always with highest baseblock
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman, baseBlockIndexes.back()->GetBlockHash(),
                                       tipBlockIndex->GetBlockHash(), response.mnListDiffTip, errorRet)) {
            return false;
        }
    }

    const CBlockIndex* blockIndex = chainman.m_blockman.LookupBlockIndex(request.blockRequestHash);
    if (!blockIndex) {
        errorRet = strprintf("block not found");
        return false;
    }

    // Quorum rotation is enabled only for InstantSend atm.
    Consensus::LLMQType llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;

    // Since the returned quorums are in reversed order, the most recent one is at index 0
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());

    const int cycleLength = llmq_params_opt->dkgInterval;

    auto cycle_base_opt = ConstructCycle(qsnapman, llmqType, /*skip_snap=*/true,
                                         /*height=*/blockIndex->nHeight - (blockIndex->nHeight % cycleLength),
                                         blockIndex, errorRet);
    if (!cycle_base_opt.has_value()) {
        return false;
    }
    if (use_legacy_construction) {
        // Build MN list Diff always with highest baseblock
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, cycle_base_opt->m_work_index,
                                                            use_legacy_construction),
                                       cycle_base_opt->m_work_index->GetBlockHash(), response.mnListDiffH, errorRet)) {
            return false;
        }
    }

    response.extraShare = request.extraShare;

    auto target_cycles{response.GetCycles()};
    for (size_t idx{0}; idx < target_cycles.size(); idx++) {
        auto cycle_opt = ConstructCycle(qsnapman, llmqType, /*skip_snap=*/false,
                                        /*height=*/cycle_base_opt->m_cycle_index->nHeight - (cycleLength * (idx + 1)),
                                        tipBlockIndex, errorRet);
        if (!cycle_opt.has_value()) {
            return false;
        }
        if (use_legacy_construction) {
            if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                           GetLastBaseBlockHash(baseBlockIndexes, cycle_opt->m_work_index,
                                                                use_legacy_construction),
                                           cycle_opt->m_work_index->GetBlockHash(), cycle_opt->m_diff, errorRet)) {
                return false;
            }
        }
        *target_cycles[idx] = cycle_opt.value();
    }

    std::set<int> snapshotHeightsNeeded;
    for (const auto& obj : qblockman.GetLastMinedCommitmentsPerQuorumIndexUntilBlock(llmqType, blockIndex, /*cycle=*/0)) {
        auto [qc, minedBlockHash] = qblockman.GetMinedCommitment(llmqType, obj->GetBlockHash());
        if (minedBlockHash == uint256::ZERO) {
            return false;
        }
        response.lastCommitmentPerIndex.emplace_back(std::move(qc));

        int quorumCycleStartHeight = obj->nHeight - (obj->nHeight % llmq_params_opt->dkgInterval);
        snapshotHeightsNeeded.insert(quorumCycleStartHeight - cycleLength);
        snapshotHeightsNeeded.insert(quorumCycleStartHeight - 2 * cycleLength);
        snapshotHeightsNeeded.insert(quorumCycleStartHeight - 3 * cycleLength);
    }

    for (auto* cycle : target_cycles) {
        snapshotHeightsNeeded.erase(cycle->m_cycle_index->nHeight);
    }

    for (const auto& h : snapshotHeightsNeeded) {
        auto cycle_opt = ConstructCycle(qsnapman, llmqType, /*skip_snap=*/false, /*height=*/h, tipBlockIndex, errorRet);
        if (!cycle_opt.has_value()) {
            return false;
        }
        response.quorumSnapshotList.push_back(cycle_opt->m_snap);
        CSimplifiedMNListDiff mnhneeded;
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, cycle_opt->m_work_index,
                                                            use_legacy_construction),
                                       cycle_opt->m_work_index->GetBlockHash(), mnhneeded, errorRet)) {
            return false;
        }
        if (!use_legacy_construction) {
            baseBlockIndexes.push_back(cycle_opt->m_work_index);
        }
        response.mnListDiffList.push_back(mnhneeded);
    }

    if (!use_legacy_construction) {
        for (size_t idx = target_cycles.size(); idx-- > 0;) {
            auto* cycle{target_cycles[idx]};
            if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                           GetLastBaseBlockHash(baseBlockIndexes, cycle->m_work_index,
                                                                use_legacy_construction),
                                           cycle->m_work_index->GetBlockHash(), cycle->m_diff, errorRet)) {
                return false;
            }
            baseBlockIndexes.push_back(cycle->m_work_index);
        }

        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, cycle_base_opt->m_work_index,
                                                            use_legacy_construction),
                                       cycle_base_opt->m_work_index->GetBlockHash(), response.mnListDiffH, errorRet)) {
            return false;
        }
        baseBlockIndexes.push_back(cycle_base_opt->m_work_index);

        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, tipBlockIndex, use_legacy_construction),
                                       tipBlockIndex->GetBlockHash(), response.mnListDiffTip, errorRet)) {
            return false;
        }
    }
    return true;
}

uint256 GetLastBaseBlockHash(Span<const CBlockIndex*> baseBlockIndexes, const CBlockIndex* blockIndex,
                             bool use_legacy_construction)
{
    if (!use_legacy_construction) {
        std::sort(baseBlockIndexes.begin(), baseBlockIndexes.end(),
                  [](const CBlockIndex* a, const CBlockIndex* b) { return a->nHeight < b->nHeight; });
    }
    // default to genesis block
    uint256 hash{Params().GenesisBlock().GetHash()};
    for (const auto baseBlock : baseBlockIndexes) {
        if (baseBlock->nHeight > blockIndex->nHeight) break;
        hash = baseBlock->GetBlockHash();
    }
    return hash;
}

CQuorumSnapshot::CQuorumSnapshot() = default;

CQuorumSnapshot::CQuorumSnapshot(std::vector<bool> active_quorum_members, SnapshotSkipMode skip_mode,
                                 std::vector<int> skip_list) :
    activeQuorumMembers(std::move(active_quorum_members)),
    mnSkipListMode(skip_mode),
    mnSkipList(std::move(skip_list))
{
}

CQuorumSnapshot::~CQuorumSnapshot() = default;

CQuorumRotationInfo::CQuorumRotationInfo() = default;

CQuorumRotationInfo::~CQuorumRotationInfo() = default;

std::vector<CycleData*> CQuorumRotationInfo::GetCycles()
{
    std::vector<CycleData*> ret{&cycleHMinusC, &cycleHMinus2C, &cycleHMinus3C};
    if (extraShare) {
        if (!cycleHMinus4C.has_value()) { cycleHMinus4C = CycleData{}; }
        ret.emplace_back(&(cycleHMinus4C.value()));
    }
    return ret;
}

CQuorumSnapshotManager::CQuorumSnapshotManager(CEvoDB& evoDb) :
    m_evoDb{evoDb},
    quorumSnapshotCache{32}
{
}

CQuorumSnapshotManager::~CQuorumSnapshotManager() = default;

std::optional<CQuorumSnapshot> CQuorumSnapshotManager::GetSnapshotForBlock(const Consensus::LLMQType llmqType, const CBlockIndex* pindex)
{
    CQuorumSnapshot snapshot = {};

    auto snapshotHash = ::SerializeHash(std::make_pair(llmqType, pindex->GetBlockHash()));

    LOCK(snapshotCacheCs);
    // try using cache before reading from disk
    if (quorumSnapshotCache.get(snapshotHash, snapshot)) {
        return snapshot;
    }
    if (m_evoDb.Read(std::make_pair(DB_QUORUM_SNAPSHOT, snapshotHash), snapshot)) {
        quorumSnapshotCache.insert(snapshotHash, snapshot);
        return snapshot;
    }

    return std::nullopt;
}

void CQuorumSnapshotManager::StoreSnapshotForBlock(const Consensus::LLMQType llmqType, const CBlockIndex* pindex, const CQuorumSnapshot& snapshot)
{
    auto snapshotHash = ::SerializeHash(std::make_pair(llmqType, pindex->GetBlockHash()));

    // LOCK(::cs_main);
    AssertLockNotHeld(m_evoDb.cs);
    LOCK2(snapshotCacheCs, m_evoDb.cs);
    m_evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SNAPSHOT, snapshotHash), snapshot);
    quorumSnapshotCache.insert(snapshotHash, snapshot);
}
} // namespace llmq
