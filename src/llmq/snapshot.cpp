// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/snapshot.h>

#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>

#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/utils.h>

#include <base58.h>
#include <chainparams.h>
#include <serialize.h>
#include <univalue.h>
#include <validation.h>

namespace llmq {

static const std::string DB_QUORUM_SNAPSHOT = "llmq_S";

std::unique_ptr<CQuorumSnapshotManager> quorumSnapshotManager;

void CQuorumSnapshot::ToJson(UniValue& obj) const
{
    //TODO Check this function if correct
    obj.setObject();
    UniValue activeQ(UniValue::VARR);
    for (const auto& h : activeQuorumMembers) {
        activeQ.push_back(h);
    }
    obj.pushKV("activeQuorumMembers", activeQ);
    obj.pushKV("mnSkipListMode", mnSkipListMode);
    UniValue skipList(UniValue::VARR);
    for (const auto& h : mnSkipList) {
        skipList.push_back(h);
    }
    obj.pushKV("mnSkipList", skipList);
}

void CQuorumRotationInfo::ToJson(UniValue& obj) const
{
    obj.setObject();
    obj.pushKV("extraShare", extraShare);

    UniValue objc;
    quorumSnapshotAtHMinusC.ToJson(objc);
    obj.pushKV("quorumSnapshotAtHMinusC", objc);

    UniValue obj2c;
    quorumSnapshotAtHMinus2C.ToJson(obj2c);
    obj.pushKV("quorumSnapshotAtHMinus2C", obj2c);

    UniValue obj3c;
    quorumSnapshotAtHMinus3C.ToJson(obj3c);
    obj.pushKV("quorumSnapshotAtHMinus3C", obj3c);

    if (extraShare && quorumSnapshotAtHMinus4C.has_value()) {
        UniValue obj4c;
        quorumSnapshotAtHMinus4C.value().ToJson(obj4c);
        obj.pushKV("quorumSnapshotAtHMinus4C", obj4c);
    }

    UniValue objdifftip;
    mnListDiffTip.ToJson(objdifftip);
    obj.pushKV("mnListDiffTip", objdifftip);

    UniValue objdiffh;
    mnListDiffH.ToJson(objdiffh);
    obj.pushKV("mnListDiffH", objdiffh);

    UniValue objdiffc;
    mnListDiffAtHMinusC.ToJson(objdiffc);
    obj.pushKV("mnListDiffAtHMinusC", objdiffc);

    UniValue objdiff2c;
    mnListDiffAtHMinus2C.ToJson(objdiff2c);
    obj.pushKV("mnListDiffAtHMinus2C", objdiff2c);

    UniValue objdiff3c;
    mnListDiffAtHMinus3C.ToJson(objdiff3c);
    obj.pushKV("mnListDiffAtHMinus3C", objdiff3c);

    if (extraShare && mnListDiffAtHMinus4C.has_value()) {
        UniValue objdiff4c;
        mnListDiffAtHMinus4C.value().ToJson(objdiff4c);
        obj.pushKV("mnListDiffAtHMinus4C", objdiff4c);
    }
    UniValue hqclists(UniValue::VARR);
    for (const auto& qc : lastCommitmentPerIndex) {
        UniValue objqc;
        qc.ToJson(objqc);
        hqclists.push_back(objqc);
    }
    obj.pushKV("lastCommitmentPerIndex", hqclists);

    UniValue snapshotlist(UniValue::VARR);
    for (const auto& snap : quorumSnapshotList) {
        UniValue o;
        o.setObject();
        snap.ToJson(o);
        snapshotlist.push_back(o);
    }
    obj.pushKV("quorumSnapshotList", snapshotlist);

    UniValue mnlistdifflist(UniValue::VARR);
    for (const auto& mnlist : mnListDiffList) {
        UniValue o;
        o.setObject();
        mnlist.ToJson(o);
        mnlistdifflist.push_back(o);
    }
    obj.pushKV("mnListDiffList", mnlistdifflist);
}

bool BuildQuorumRotationInfo(const CGetQuorumRotationInfo& request, CQuorumRotationInfo& response, std::string& errorRet)
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
            const CBlockIndex* blockIndex = LookupBlockIndex(blockHash);
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
    if (!BuildSimplifiedMNListDiff(baseBlockIndexes.back()->GetBlockHash(), tipBlockIndex->GetBlockHash(), response.mnListDiffTip, errorRet)) {
        return false;
    }

    const CBlockIndex* blockIndex = LookupBlockIndex(request.blockRequestHash);
    if (!blockIndex) {
        errorRet = strprintf("block not found");
        return false;
    }

    //Quorum rotation is enabled only for InstantSend atm.
    Consensus::LLMQType llmqType = CLLMQUtils::GetInstantSendLLMQType(blockIndex);

    // Since the returned quorums are in reversed order, the most recent one is at index 0
    const Consensus::LLMQParams& llmqParams = GetLLMQParams(llmqType);
    const int cycleLength = llmqParams.dkgInterval;
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
    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockIndex), pWorkBlockIndex->GetBlockHash(), response.mnListDiffH, errorRet)) {
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

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinusCIndex), pWorkBlockHMinusCIndex->GetBlockHash(), response.mnListDiffAtHMinusC, errorRet)) {
        return false;
    }

    auto snapshotHMinusC = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinusCIndex);
    if (!snapshotHMinusC.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-C");
        return false;
    } else {
        response.quorumSnapshotAtHMinusC = std::move(snapshotHMinusC.value());
    }

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus2CIndex), pWorkBlockHMinus2CIndex->GetBlockHash(), response.mnListDiffAtHMinus2C, errorRet)) {
        return false;
    }

    auto snapshotHMinus2C = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinus2CIndex);
    if (!snapshotHMinus2C.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-C");
        return false;
    } else {
        response.quorumSnapshotAtHMinus2C = std::move(snapshotHMinus2C.value());
    }

    if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus3CIndex), pWorkBlockHMinus3CIndex->GetBlockHash(), response.mnListDiffAtHMinus3C, errorRet)) {
        return false;
    }

    auto snapshotHMinus3C = quorumSnapshotManager->GetSnapshotForBlock(llmqType, pBlockHMinus3CIndex);
    if (!snapshotHMinus3C.has_value()) {
        errorRet = strprintf("Can not find quorum snapshot at H-C");
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
        if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pWorkBlockHMinus4CIndex), pWorkBlockHMinus4CIndex->GetBlockHash(), mn4c, errorRet)) {
            return false;
        }

        response.mnListDiffAtHMinus4C = std::move(mn4c);
    } else {
        response.extraShare = false;
        response.quorumSnapshotAtHMinus4C = std::nullopt;
        response.mnListDiffAtHMinus4C = std::nullopt;
    }

    std::set<int> snapshotHeightsNeeded;

    std::vector<std::pair<int, const CBlockIndex*>> qdata = quorumBlockProcessor->GetLastMinedCommitmentsPerQuorumIndexUntilBlock(llmqType, blockIndex, 0);

    for (const auto& obj : qdata) {
        uint256 minedBlockHash;
        llmq::CFinalCommitmentPtr qc = llmq::quorumBlockProcessor->GetMinedCommitment(llmqType, obj.second->GetBlockHash(), minedBlockHash);
        if (qc == nullptr) {
            return false;
        }
        response.lastCommitmentPerIndex.push_back(*qc);

        int quorumCycleStartHeight = obj.second->nHeight - (obj.second->nHeight % llmqParams.dkgInterval);
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
        if (!BuildSimplifiedMNListDiff(GetLastBaseBlockHash(baseBlockIndexes, pNeededWorkBlockIndex), pNeededWorkBlockIndex->GetBlockHash(), mnhneeded, errorRet)) {
            return false;
        }

        response.mnListDiffList.push_back(mnhneeded);
    }

    return true;
}

uint256 GetLastBaseBlockHash(const std::vector<const CBlockIndex*>& baseBlockIndexes, const CBlockIndex* blockIndex)
{
    uint256 hash;
    for (const auto baseBlock : baseBlockIndexes) {
        if (baseBlock->nHeight >= blockIndex->nHeight)
            break;
        hash = baseBlock->GetBlockHash();
    }
    return hash;
}

CQuorumSnapshotManager::CQuorumSnapshotManager(CEvoDB& _evoDb) :
    evoDb(_evoDb),
    quorumSnapshotCache(32)
{
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
    LOCK(evoDb.cs);
    if (evoDb.Read(std::make_pair(DB_QUORUM_SNAPSHOT, snapshotHash), snapshot)) {
        quorumSnapshotCache.insert(snapshotHash, snapshot);
        return snapshot;
    }

    return std::nullopt;
}

void CQuorumSnapshotManager::StoreSnapshotForBlock(const Consensus::LLMQType llmqType, const CBlockIndex* pindex, const CQuorumSnapshot& snapshot)
{
    auto snapshotHash = ::SerializeHash(std::make_pair(llmqType, pindex->GetBlockHash()));

    // LOCK(cs_main);
    LOCK2(snapshotCacheCs, evoDb.cs);
    evoDb.GetRawDB().Write(std::make_pair(DB_QUORUM_SNAPSHOT, snapshotHash), snapshot);
    quorumSnapshotCache.insert(snapshotHash, snapshot);
}

} // namespace llmq
