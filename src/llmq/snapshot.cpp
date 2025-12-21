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
#include <univalue.h>
#include <validation.h>

namespace llmq {

static const std::string DB_QUORUM_SNAPSHOT = "llmq_S";

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

    //Quorum rotation is enabled only for InstantSend atm.
    Consensus::LLMQType llmqType = Params().GetConsensus().llmqTypeDIP0024InstantSend;

    // Since the returned quorums are in reversed order, the most recent one is at index 0
    const auto& llmq_params_opt = Params().GetLLMQ(llmqType);
    assert(llmq_params_opt.has_value());

    const int cycleLength = llmq_params_opt->dkgInterval;
    constexpr int workDiff = 8;

    const CBlockIndex* hBlockIndex = blockIndex->GetAncestor(blockIndex->nHeight - (blockIndex->nHeight % cycleLength));
    if (!hBlockIndex) {
        errorRet = strprintf("Can not find block H");
        return false;
    }

    const CBlockIndex* pWorkBlockHIndex = hBlockIndex->GetAncestor(hBlockIndex->nHeight - workDiff);
    if (!pWorkBlockHIndex) {
        errorRet = strprintf("Can not find work block H");
        return false;
    }

    if (use_legacy_construction) {
        // Build MN list Diff always with highest baseblock
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHIndex, use_legacy_construction),
                                       pWorkBlockHIndex->GetBlockHash(), response.mnListDiffH, errorRet)) {
            return false;
        }
    }

    response.extraShare = request.extraShare;

    auto target_cycles{response.GetCycles()};
    for (size_t idx{0}; idx < target_cycles.size(); idx++) {
        auto* cycle{target_cycles[idx]};
        const auto depth{idx + 1};
        cycle->m_cycle_index = tipBlockIndex->GetAncestor(hBlockIndex->nHeight - (cycleLength * depth));
        if (!cycle->m_cycle_index) {
            errorRet = strprintf("Cannot find block H-%dC", depth);
            return false;
        }
        cycle->m_work_index = cycle->m_cycle_index->GetAncestor(cycle->m_cycle_index->nHeight - workDiff);
        if (!cycle->m_work_index) {
            errorRet = strprintf("Cannot find work block H-%dC", depth);
            return false;
        }
        if (auto opt_snap = qsnapman.GetSnapshotForBlock(llmqType, cycle->m_cycle_index); opt_snap.has_value()) {
            cycle->m_snap = std::move(opt_snap.value());
        } else {
            errorRet = strprintf("Cannot find quorum snapshot at H-%dC", depth);
            return false;
        }
        if (use_legacy_construction) {
            if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                           GetLastBaseBlockHash(baseBlockIndexes, cycle->m_work_index, use_legacy_construction),
                                           cycle->m_work_index->GetBlockHash(), cycle->m_diff, errorRet))
            {
                return false;
            }
        }
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
        const CBlockIndex* pNeededBlockIndex = tipBlockIndex->GetAncestor(h);
        if (!pNeededBlockIndex) {
            errorRet = strprintf("Can not find needed block H(%d)", h);
            return false;
        }
        const CBlockIndex* pNeededWorkBlockIndex = pNeededBlockIndex->GetAncestor(pNeededBlockIndex->nHeight - workDiff);
        if (!pNeededWorkBlockIndex) {
            errorRet = strprintf("Can not find needed work block H(%d)", h);
            return false;
        }

        auto snapshotNeededH = qsnapman.GetSnapshotForBlock(llmqType, pNeededBlockIndex);
        if (!snapshotNeededH.has_value()) {
            errorRet = strprintf("Can not find quorum snapshot at H(%d)", h);
            return false;
        } else {
            response.quorumSnapshotList.push_back(snapshotNeededH.value());
        }

        CSimplifiedMNListDiff mnhneeded;
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pNeededWorkBlockIndex, use_legacy_construction),
                                       pNeededWorkBlockIndex->GetBlockHash(), mnhneeded, errorRet)) {
            return false;
        }
        if (!use_legacy_construction) {
            baseBlockIndexes.push_back(pNeededWorkBlockIndex);
        }
        response.mnListDiffList.push_back(mnhneeded);
    }

    if (!use_legacy_construction) {
        for (size_t idx = target_cycles.size(); idx-- > 0;) {
            auto* cycle{target_cycles[idx]};
            if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                           GetLastBaseBlockHash(baseBlockIndexes, cycle->m_work_index, use_legacy_construction),
                                           cycle->m_work_index->GetBlockHash(), cycle->m_diff, errorRet))
            {
                return false;
            }
            baseBlockIndexes.push_back(cycle->m_work_index);
        }

        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHIndex, use_legacy_construction),
                                       pWorkBlockHIndex->GetBlockHash(), response.mnListDiffH, errorRet)) {
            return false;
        }
        baseBlockIndexes.push_back(pWorkBlockHIndex);

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
