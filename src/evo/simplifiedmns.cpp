// Copyright (c) 2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "simplifiedmns.h"
#include "specialtx.h"
#include "deterministicmns.h"
#include "cbtx.h"

#include "validation.h"
#include "univalue.h"
#include "consensus/merkle.h"
#include "chainparams.h"

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const CDeterministicMN& dmn) :
    proRegTxHash(dmn.proTxHash),
    service(dmn.pdmnState->addr),
    keyIDOperator(dmn.pdmnState->keyIDOperator),
    keyIDVoting(dmn.pdmnState->keyIDVoting),
    isValid(dmn.pdmnState->nPoSeBanHeight == -1)
{
}

uint256 CSimplifiedMNListEntry::CalcHash() const
{
    CHashWriter hw(SER_GETHASH, CLIENT_VERSION);
    hw << *this;
    return hw.GetHash();
}

std::string CSimplifiedMNListEntry::ToString() const
{
    return strprintf("CSimplifiedMNListEntry(proRegTxHash=%s, service=%s, keyIDOperator=%s, keyIDVoting=%s, isValie=%d)",
        proRegTxHash.ToString(), service.ToString(false), keyIDOperator.ToString(), keyIDVoting.ToString(), isValid);
}

void CSimplifiedMNListEntry::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("proRegTxHash", proRegTxHash.ToString()));
    obj.push_back(Pair("service", service.ToString(false)));
    obj.push_back(Pair("keyIDOperator", keyIDOperator.ToString()));
    obj.push_back(Pair("keyIDVoting", keyIDOperator.ToString()));
    obj.push_back(Pair("isValid", isValid));
}

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList)
{
    mnList.reserve(dmnList.all_count());

    for (const auto& dmn : dmnList.all_range()) {
        mnList.emplace_back(*dmn);
    }

    std::sort(mnList.begin(), mnList.end(), [&](const CSimplifiedMNListEntry& a, const CSimplifiedMNListEntry& b) {
        return a.proRegTxHash.Compare(b.proRegTxHash) < 0;
    });
}

uint256 CSimplifiedMNList::CalcMerkleRoot(bool *pmutated) const
{
    std::vector<uint256> leaves;
    leaves.reserve(mnList.size());
    for (const auto& e : mnList) {
        leaves.emplace_back(e.CalcHash());
    }
    return ComputeMerkleRoot(leaves, pmutated);
}

void CSimplifiedMNListDiff::ToJson(UniValue& obj) const
{
    obj.setObject();

    obj.push_back(Pair("baseBlockHash", baseBlockHash.ToString()));
    obj.push_back(Pair("blockHash", blockHash.ToString()));

    UniValue deletedMNsArr(UniValue::VARR);
    for (const auto& h : deletedMNs) {
        deletedMNsArr.push_back(h.ToString());
    }
    obj.push_back(Pair("deletedMNs", deletedMNsArr));

    UniValue mnListArr(UniValue::VARR);
    for (const auto& e : mnList) {
        UniValue eObj;
        e.ToJson(eObj);
        mnListArr.push_back(eObj);
    }
    obj.push_back(Pair("mnList", mnListArr));

    CCbTx cbTxPayload;
    if (GetTxPayload(*cbTx, cbTxPayload)) {
        obj.push_back(Pair("merkleRootMNList", cbTxPayload.merkleRootMNList.ToString()));
    }
}

bool BuildSimplifiedMNListDiff(const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet)
{
    AssertLockHeld(cs_main);
    mnListDiffRet = CSimplifiedMNListDiff();

    BlockMap::iterator baseBlockIt = mapBlockIndex.begin();
    if (!baseBlockHash.IsNull()) {
        baseBlockIt = mapBlockIndex.find(baseBlockHash);
    }
    auto blockIt = mapBlockIndex.find(blockHash);
    if (baseBlockIt == mapBlockIndex.end()) {
        errorRet = strprintf("block %s not found", baseBlockHash.ToString());
        return false;
    }
    if (blockIt == mapBlockIndex.end()) {
        errorRet = strprintf("block %s not found", blockHash.ToString());
        return false;
    }

    if (!chainActive.Contains(baseBlockIt->second) || !chainActive.Contains(blockIt->second)) {
        errorRet = strprintf("block %s and %s are not in the same chain", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }
    if (baseBlockIt->second->nHeight > blockIt->second->nHeight) {
        errorRet = strprintf("base block %s is higher then block %s", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }

    LOCK(deterministicMNManager->cs);

    auto baseDmnList = deterministicMNManager->GetListForBlock(baseBlockHash);
    auto dmnList = deterministicMNManager->GetListForBlock(blockHash);
    auto dmnDiff = baseDmnList.BuildDiff(dmnList);

    // TODO store coinbase TX in CBlockIndex
    CBlock block;
    if (!ReadBlockFromDisk(block, blockIt->second, Params().GetConsensus())) {
        errorRet = strprintf("failed to read block %s from disk", blockHash.ToString());
        return false;
    }

    mnListDiffRet.baseBlockHash = baseBlockHash;
    mnListDiffRet.blockHash = blockHash;
    mnListDiffRet.cbTx = block.vtx[0];

    std::vector<uint256> vHashes;
    std::vector<bool> vMatch(block.vtx.size(), false);
    for (const auto& tx : block.vtx) {
        vHashes.emplace_back(tx->GetHash());
    }
    vMatch[0] = true; // only coinbase matches
    mnListDiffRet.cbTxMerkleTree = CPartialMerkleTree(vHashes, vMatch);
    mnListDiffRet.deletedMNs.assign(dmnDiff.removedMns.begin(), dmnDiff.removedMns.end());

    for (const auto& p : dmnDiff.addedMNs) {
        mnListDiffRet.mnList.emplace_back(*p.second);
    }
    for (const auto& p : dmnDiff.updatedMNs) {
        const auto& dmn = dmnList.GetMN(p.first);
        CDeterministicMN newDmn(*dmn);
        newDmn.pdmnState = p.second;
        mnListDiffRet.mnList.emplace_back(newDmn);
    }

    return true;
}
