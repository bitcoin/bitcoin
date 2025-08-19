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

UniValue CQuorumSnapshot::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    UniValue activeQ(UniValue::VARR);
    for (const bool h : activeQuorumMembers) {
        // cppcheck-suppress useStlAlgorithm
        activeQ.push_back(h);
    }
    obj.pushKV("activeQuorumMembers", activeQ);
    obj.pushKV("mnSkipListMode", mnSkipListMode);
    UniValue skipList(UniValue::VARR);
    for (const auto& h : mnSkipList) {
        // cppcheck-suppress useStlAlgorithm
        skipList.push_back(h);
    }
    obj.pushKV("mnSkipList", skipList);
    return obj;
}

UniValue CQuorumRotationInfo::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("extraShare", extraShare);

    obj.pushKV("quorumSnapshotAtHMinusC", quorumSnapshotAtHMinusC.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus2C", quorumSnapshotAtHMinus2C.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus3C", quorumSnapshotAtHMinus3C.ToJson());

    if (extraShare) {
        obj.pushKV("quorumSnapshotAtHMinus4C", quorumSnapshotAtHMinus4C.ToJson());
    }

    obj.pushKV("mnListDiffTip", mnListDiffTip.ToJson());
    obj.pushKV("mnListDiffH", mnListDiffH.ToJson());
    obj.pushKV("mnListDiffAtHMinusC", mnListDiffAtHMinusC.ToJson());
    obj.pushKV("mnListDiffAtHMinus2C", mnListDiffAtHMinus2C.ToJson());
    obj.pushKV("mnListDiffAtHMinus3C", mnListDiffAtHMinus3C.ToJson());

    if (extraShare) {
        obj.pushKV("mnListDiffAtHMinus4C", mnListDiffAtHMinus4C.ToJson());
    }
    UniValue hqclists(UniValue::VARR);
    for (const auto& qc : lastCommitmentPerIndex) {
        hqclists.push_back(qc.ToJson());
    }
    obj.pushKV("lastCommitmentPerIndex", hqclists);

    UniValue snapshotlist(UniValue::VARR);
    for (const auto& snap : quorumSnapshotList) {
        snapshotlist.push_back(snap.ToJson());
    }
    obj.pushKV("quorumSnapshotList", snapshotlist);

    UniValue mnlistdifflist(UniValue::VARR);
    for (const auto& mnlist : mnListDiffList) {
        mnlistdifflist.push_back(mnlist.ToJson());
    }
    obj.pushKV("mnListDiffList", mnlistdifflist);
    return obj;
}

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

    const CBlockIndex* pBlockHMinusCIndex = tipBlockIndex->GetAncestor(hBlockIndex->nHeight - cycleLength);
    if (!pBlockHMinusCIndex) {
        errorRet = strprintf("Can not find block H-C");
        return false;
    }
    const CBlockIndex* pWorkBlockHMinusCIndex = pBlockHMinusCIndex->GetAncestor(pBlockHMinusCIndex->nHeight - workDiff);
    if (!pWorkBlockHMinusCIndex) {
        errorRet = strprintf("Can not find work block H-C");
        return false;
    }

    const CBlockIndex* pBlockHMinus2CIndex = pBlockHMinusCIndex->GetAncestor(hBlockIndex->nHeight - 2 * cycleLength);
    if (!pBlockHMinus2CIndex) {
        errorRet = strprintf("Can not find block H-2C");
        return false;
    }
    const CBlockIndex* pWorkBlockHMinus2CIndex = pBlockHMinus2CIndex->GetAncestor(pBlockHMinus2CIndex->nHeight - workDiff);
    if (!pWorkBlockHMinus2CIndex) {
        errorRet = strprintf("Can not find work block H-2C");
        return false;
    }

    const CBlockIndex* pBlockHMinus3CIndex = pBlockHMinusCIndex->GetAncestor(hBlockIndex->nHeight - 3 * cycleLength);
    if (!pBlockHMinus3CIndex) {
        errorRet = strprintf("Can not find block H-3C");
        return false;
    }
    const CBlockIndex* pWorkBlockHMinus3CIndex = pBlockHMinus3CIndex->GetAncestor(pBlockHMinus3CIndex->nHeight - workDiff);
    if (!pWorkBlockHMinus3CIndex) {
        errorRet = strprintf("Can not find work block H-3C");
        return false;
    }

    const CBlockIndex* pBlockHMinus4CIndex = pBlockHMinusCIndex->GetAncestor(hBlockIndex->nHeight - 4 * cycleLength);
    if (!pBlockHMinus4CIndex) {
        errorRet = strprintf("Can not find block H-4C");
        return false;
    }

    const CBlockIndex* pWorkBlockHMinus4CIndex = pBlockHMinus4CIndex->GetAncestor(pBlockHMinus4CIndex->nHeight - workDiff);
    //Checked later if extraShare is on

    if (use_legacy_construction) {
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinusCIndex, use_legacy_construction),
                                       pWorkBlockHMinusCIndex->GetBlockHash(), response.mnListDiffAtHMinusC, errorRet)) {
            return false;
        }
    }

    auto snapshotHMinusC = qsnapman.GetSnapshotForBlock(llmqType, pBlockHMinusCIndex);
    if (!snapshotHMinusC.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-C");
        return false;
    } else {
        response.quorumSnapshotAtHMinusC = std::move(snapshotHMinusC.value());
    }

    if (use_legacy_construction) {
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus2CIndex,
                                                            use_legacy_construction),
                                       pWorkBlockHMinus2CIndex->GetBlockHash(), response.mnListDiffAtHMinus2C, errorRet)) {
            return false;
        }
    }

    auto snapshotHMinus2C = qsnapman.GetSnapshotForBlock(llmqType, pBlockHMinus2CIndex);
    if (!snapshotHMinus2C.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-2C");
        return false;
    } else {
        response.quorumSnapshotAtHMinus2C = std::move(snapshotHMinus2C.value());
    }

    if (use_legacy_construction) {
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus3CIndex,
                                                            use_legacy_construction),
                                       pWorkBlockHMinus3CIndex->GetBlockHash(), response.mnListDiffAtHMinus3C, errorRet)) {
            return false;
        }
    }

    auto snapshotHMinus3C = qsnapman.GetSnapshotForBlock(llmqType, pBlockHMinus3CIndex);
    if (!snapshotHMinus3C.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-3C");
        return false;
    } else {
        response.quorumSnapshotAtHMinus3C = std::move(snapshotHMinus3C.value());
    }

    if (request.extraShare) {
        response.extraShare = true;

        if (!pWorkBlockHMinus4CIndex) {
            errorRet = strprintf("Can not find work block H-4C");
            return false;
        }

        auto snapshotHMinus4C = qsnapman.GetSnapshotForBlock(llmqType, pBlockHMinus4CIndex);
        if (!snapshotHMinus4C.has_value()) {
            errorRet = strprintf("Can not find quorum snapshot at H-4C");
            return false;
        } else {
            response.quorumSnapshotAtHMinus4C = std::move(snapshotHMinus4C.value());
        }

        CSimplifiedMNListDiff mn4c;
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus4CIndex,
                                                            use_legacy_construction),
                                       pWorkBlockHMinus4CIndex->GetBlockHash(), mn4c, errorRet)) {
            return false;
        }
        if (!use_legacy_construction) {
            baseBlockIndexes.push_back(pWorkBlockHMinus4CIndex);
        }
        response.mnListDiffAtHMinus4C = std::move(mn4c);
    } else {
        response.extraShare = false;
    }

    std::set<int> snapshotHeightsNeeded;

    std::vector<const CBlockIndex*> qdata = qblockman.GetLastMinedCommitmentsPerQuorumIndexUntilBlock(llmqType,
                                                                                                      blockIndex, 0);

    for (const auto& obj : qdata) {
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

    snapshotHeightsNeeded.erase(pBlockHMinusCIndex->nHeight);
    snapshotHeightsNeeded.erase(pBlockHMinus2CIndex->nHeight);
    snapshotHeightsNeeded.erase(pBlockHMinus3CIndex->nHeight);
    if (request.extraShare)
        snapshotHeightsNeeded.erase(pBlockHMinus4CIndex->nHeight);

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
        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus3CIndex,
                                                            use_legacy_construction),
                                       pWorkBlockHMinus3CIndex->GetBlockHash(), response.mnListDiffAtHMinus3C, errorRet)) {
            return false;
        }
        baseBlockIndexes.push_back(pWorkBlockHMinus3CIndex);

        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus2CIndex,
                                                            use_legacy_construction),
                                       pWorkBlockHMinus2CIndex->GetBlockHash(), response.mnListDiffAtHMinus2C, errorRet)) {
            return false;
        }
        baseBlockIndexes.push_back(pWorkBlockHMinus2CIndex);

        if (!BuildSimplifiedMNListDiff(dmnman, chainman, qblockman, qman,
                                       GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinusCIndex, use_legacy_construction),
                                       pWorkBlockHMinusCIndex->GetBlockHash(), response.mnListDiffAtHMinusC, errorRet)) {
            return false;
        }
        baseBlockIndexes.push_back(pWorkBlockHMinusCIndex);

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
