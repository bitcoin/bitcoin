// Copyright (c) 2021-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/snapshot.h>

#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>

#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>

#include <base58.h>
#include <chainparams.h>
#include <serialize.h>
#include <univalue.h>
#include <validation.h>

namespace llmq {

static const std::string DB_QUORUM_SNAPSHOT = "llmq_S";

std::unique_ptr<CQuorumSnapshotManager> quorumSnapshotManager;

UniValue CQuorumSnapshot::ToJson() const
{
    UniValue obj;
    obj.setObject();
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
    UniValue obj;
    obj.setObject();
    obj.pushKV("extraShare", extraShare);

    obj.pushKV("quorumSnapshotAtHMinusC", quorumSnapshotAtHMinusC.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus2C", quorumSnapshotAtHMinus2C.ToJson());
    obj.pushKV("quorumSnapshotAtHMinus3C", quorumSnapshotAtHMinus3C.ToJson());

    if (extraShare && quorumSnapshotAtHMinus4C.has_value()) {
        obj.pushKV("quorumSnapshotAtHMinus4C", quorumSnapshotAtHMinus4C->ToJson());
    }

    obj.pushKV("mnListDiffTip", mnListDiffTip.ToJson());
    obj.pushKV("mnListDiffH", mnListDiffH.ToJson());
    obj.pushKV("mnListDiffAtHMinusC", mnListDiffAtHMinusC.ToJson());
    obj.pushKV("mnListDiffAtHMinus2C", mnListDiffAtHMinus2C.ToJson());
    obj.pushKV("mnListDiffAtHMinus3C", mnListDiffAtHMinus3C.ToJson());

    if (extraShare && mnListDiffAtHMinus4C.has_value()) {
        obj.pushKV("mnListDiffAtHMinus4C", mnListDiffAtHMinus4C->ToJson());
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

bool BuildQuorumRotationInfo(const CGetQuorumRotationInfo& request, CQuorumRotationInfo& response,
                             const CQuorumManager& qman, const CQuorumBlockProcessor& quorumBlockProcessor, std::string& errorRet)
{
    AssertLockHeld(cs_main);

    std::vector<const CBlockIndex*> baseBlockIndexes;
    if (request.baseBlockHashes.size() == 0) {
        const CBlockIndex* blockIndex = ::ChainActive().Genesis();
        if (!blockIndex) {
            errorRet = strprintf("genesis block not found");
            return false;
        }
        baseBlockIndexes.push_back(blockIndex);
    } else {
        for (const auto& blockHash : request.baseBlockHashes) {
            const CBlockIndex* blockIndex = g_chainman.m_blockman.LookupBlockIndex(blockHash);
            if (!blockIndex) {
                errorRet = strprintf("block %s not found", blockHash.ToString());
                return false;
            }
            if (!::ChainActive().Contains(blockIndex)) {
                errorRet = strprintf("block %s is not in the active chain", blockHash.ToString());
                return false;
            }
            baseBlockIndexes.push_back(blockIndex);
        }
        std::sort(baseBlockIndexes.begin(), baseBlockIndexes.end(), [](const CBlockIndex* a, const CBlockIndex* b) {
            return a->nHeight < b->nHeight;
        });
    }

    const CBlockIndex* tipBlockIndex = ::ChainActive().Tip();
    if (!tipBlockIndex) {
        errorRet = strprintf("tip block not found");
        return false;
    }
    //Build MN list Diff always with highest baseblock
    if (!BuildSimplifiedMNListDiff(baseBlockIndexes.back()->GetBlockHash(), tipBlockIndex->GetBlockHash(), response.mnListDiffTip, quorumBlockProcessor, errorRet)) {
        return false;
    }

    const CBlockIndex* blockIndex = g_chainman.m_blockman.LookupBlockIndex(request.blockRequestHash);
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

    const CBlockIndex* pWorkBlockIndex = hBlockIndex->GetAncestor(hBlockIndex->nHeight - workDiff);
    if (!pWorkBlockIndex) {
        errorRet = strprintf("Can not find work block H");
        return false;
    }

    //Build MN list Diff always with highest baseblock
    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockIndex), pWorkBlockIndex->GetBlockHash(), response.mnListDiffH, quorumBlockProcessor, errorRet)) {
        return false;
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

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinusCIndex), pWorkBlockHMinusCIndex->GetBlockHash(), response.mnListDiffAtHMinusC, quorumBlockProcessor, errorRet)) {
        return false;
    }

    auto snapshotHMinusC = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinusCIndex);
    if (!snapshotHMinusC.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-C");
        return false;
    } else {
        response.quorumSnapshotAtHMinusC = std::move(snapshotHMinusC.value());
    }

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus2CIndex), pWorkBlockHMinus2CIndex->GetBlockHash(), response.mnListDiffAtHMinus2C, quorumBlockProcessor, errorRet)) {
        return false;
    }

    auto snapshotHMinus2C = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinus2CIndex);
    if (!snapshotHMinus2C.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-2C");
        return false;
    } else {
        response.quorumSnapshotAtHMinus2C = std::move(snapshotHMinus2C.value());
    }

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus3CIndex), pWorkBlockHMinus3CIndex->GetBlockHash(), response.mnListDiffAtHMinus3C, quorumBlockProcessor, errorRet)) {
        return false;
    }

    auto snapshotHMinus3C = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinus3CIndex);
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

        auto snapshotHMinus4C = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinus4CIndex);
        if (!snapshotHMinus4C.has_value()) {
            errorRet = strprintf("Can not find quorum snapshot at H-4C");
            return false;
        } else {
            response.quorumSnapshotAtHMinus4C = std::move(snapshotHMinus4C);
        }

        CSimplifiedMNListDiff mn4c;
        if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus4CIndex), pWorkBlockHMinus4CIndex->GetBlockHash(), mn4c, quorumBlockProcessor, errorRet)) {
            return false;
        }

        response.mnListDiffAtHMinus4C = std::move(mn4c);
    } else {
        response.extraShare = false;
        response.quorumSnapshotAtHMinus4C.reset();
        response.mnListDiffAtHMinus4C.reset();
    }

    std::set<int> snapshotHeightsNeeded;

    std::vector<std::pair<int, const CBlockIndex*>> qdata = quorumBlockProcessor.GetLastMinedCommitmentsPerQuorumIndexUntilBlock(llmqType, blockIndex, 0);

    for (const auto& obj : qdata) {
        uint256 minedBlockHash;
        llmq::CFinalCommitmentPtr qc = quorumBlockProcessor.GetMinedCommitment(llmqType, obj.second->GetBlockHash(), minedBlockHash);
        if (qc == nullptr) {
            return false;
        }
        response.lastCommitmentPerIndex.push_back(*qc);

        int quorumCycleStartHeight = obj.second->nHeight - (obj.second->nHeight % llmq_params_opt->dkgInterval);
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

        auto snapshotNeededH = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pNeededBlockIndex);
        if (!snapshotNeededH.has_value()) {
            errorRet = strprintf("Can not find quorum snapshot at H(%d)", h);
            return false;
        } else {
            response.quorumSnapshotList.push_back(snapshotNeededH.value());
        }

        CSimplifiedMNListDiff mnhneeded;
        if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pNeededWorkBlockIndex), pNeededWorkBlockIndex->GetBlockHash(), mnhneeded, quorumBlockProcessor, errorRet)) {
            return false;
        }

        response.mnListDiffList.push_back(mnhneeded);
    }

    return true;
}

uint256 GetLastBaseBlockHash(Span<const CBlockIndex*> baseBlockIndexes, const CBlockIndex* blockIndex)
{
    uint256 hash;
    for (const auto baseBlock : baseBlockIndexes) {
        if (baseBlock->nHeight >= blockIndex->nHeight)
            break;
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

    // LOCK(cs_main);
    AssertLockNotHeld(m_evoDb.cs);
    LOCK2(snapshotCacheCs, m_evoDb.cs);
    m_evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SNAPSHOT, snapshotHash), snapshot);
    quorumSnapshotCache.insert(snapshotHash, snapshot);
}

} // namespace llmq
