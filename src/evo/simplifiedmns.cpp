// Copyright (c) 2017-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/cbtx.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>

#include <base58.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <univalue.h>
#include <validation.h>

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const CDeterministicMN& dmn) :
    proRegTxHash(dmn.proTxHash),
    confirmedHash(dmn.pdmnState->confirmedHash),
    service(dmn.pdmnState->addr),
    pubKeyOperator(dmn.pdmnState->pubKeyOperator),
    keyIDVoting(dmn.pdmnState->keyIDVoting),
    isValid(!dmn.pdmnState->IsBanned())
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
    return strprintf("CSimplifiedMNListEntry(proRegTxHash=%s, confirmedHash=%s, service=%s, pubKeyOperator=%s, votingAddress=%s, isValid=%d)",
        proRegTxHash.ToString(), confirmedHash.ToString(), service.ToString(false), pubKeyOperator.Get().ToString(), EncodeDestination(keyIDVoting), isValid);
}

void CSimplifiedMNListEntry::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    obj.pushKV("service", service.ToString(false));
    obj.pushKV("pubKeyOperator", pubKeyOperator.Get().ToString());
    obj.pushKV("votingAddress", EncodeDestination(keyIDVoting));
    obj.pushKV("isValid", isValid);
}

CSimplifiedMNList::CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries)
{
    mnList.resize(smlEntries.size());
    for (size_t i = 0; i < smlEntries.size(); i++) {
        mnList[i] = std::make_unique<CSimplifiedMNListEntry>(smlEntries[i]);
    }

    std::sort(mnList.begin(), mnList.end(), [&](const std::unique_ptr<CSimplifiedMNListEntry>& a, const std::unique_ptr<CSimplifiedMNListEntry>& b) {
        return a->proRegTxHash.Compare(b->proRegTxHash) < 0;
    });
}

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList)
{
    mnList.resize(dmnList.GetAllMNsCount());

    size_t i = 0;
    dmnList.ForEachMN(false, [this, &i](const CDeterministicMNCPtr& dmn) {
        mnList[i++] = std::make_unique<CSimplifiedMNListEntry>(*dmn);
    });

    std::sort(mnList.begin(), mnList.end(), [&](const std::unique_ptr<CSimplifiedMNListEntry>& a, const std::unique_ptr<CSimplifiedMNListEntry>& b) {
        return a->proRegTxHash.Compare(b->proRegTxHash) < 0;
    });
}

uint256 CSimplifiedMNList::CalcMerkleRoot(bool* pmutated) const
{
    std::vector<uint256> leaves;
    leaves.reserve(mnList.size());
    for (const auto& e : mnList) {
        leaves.emplace_back(e->CalcHash());
    }
    return ComputeMerkleRoot(leaves, pmutated);
}

CSimplifiedMNListDiff::CSimplifiedMNListDiff() = default;

CSimplifiedMNListDiff::~CSimplifiedMNListDiff() = default;

bool CSimplifiedMNListDiff::BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex)
{
    auto baseQuorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(baseBlockIndex);
    auto quorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(blockIndex);

    std::set<std::pair<Consensus::LLMQType, uint256>> baseQuorumHashes;
    std::set<std::pair<Consensus::LLMQType, uint256>> quorumHashes;
    for (auto& p : baseQuorums) {
        for (auto& p2 : p.second) {
            baseQuorumHashes.emplace(p.first, p2->GetBlockHash());
        }
    }
    for (auto& p : quorums) {
        for (auto& p2 : p.second) {
            quorumHashes.emplace(p.first, p2->GetBlockHash());
        }
    }

    for (auto& p : baseQuorumHashes) {
        if (!quorumHashes.count(p)) {
            deletedQuorums.emplace_back((uint8_t)p.first, p.second);
        }
    }
    for (auto& p : quorumHashes) {
        if (!baseQuorumHashes.count(p)) {
            llmq::CFinalCommitment qc;
            uint256 minedBlockHash;
            if (!llmq::quorumBlockProcessor->GetMinedCommitment(p.first, p.second, qc, minedBlockHash)) {
                return false;
            }
            newQuorums.emplace_back(qc);
        }
    }
    return true;
}

void CSimplifiedMNListDiff::ToJson(UniValue& obj) const
{
    obj.setObject();

    obj.pushKV("baseBlockHash", baseBlockHash.ToString());
    obj.pushKV("blockHash", blockHash.ToString());

    CDataStream ssCbTxMerkleTree(SER_NETWORK, PROTOCOL_VERSION);
    ssCbTxMerkleTree << cbTxMerkleTree;
    obj.pushKV("cbTxMerkleTree", HexStr(ssCbTxMerkleTree.begin(), ssCbTxMerkleTree.end()));

    obj.pushKV("cbTx", EncodeHexTx(*cbTx));

    UniValue deletedMNsArr(UniValue::VARR);
    for (const auto& h : deletedMNs) {
        deletedMNsArr.push_back(h.ToString());
    }
    obj.pushKV("deletedMNs", deletedMNsArr);

    UniValue mnListArr(UniValue::VARR);
    for (const auto& e : mnList) {
        UniValue eObj;
        e.ToJson(eObj);
        mnListArr.push_back(eObj);
    }
    obj.pushKV("mnList", mnListArr);

    UniValue deletedQuorumsArr(UniValue::VARR);
    for (const auto& e : deletedQuorums) {
        UniValue eObj(UniValue::VOBJ);
        eObj.pushKV("llmqType", e.first);
        eObj.pushKV("quorumHash", e.second.ToString());
        deletedQuorumsArr.push_back(eObj);
    }
    obj.pushKV("deletedQuorums", deletedQuorumsArr);

    UniValue newQuorumsArr(UniValue::VARR);
    for (const auto& e : newQuorums) {
        UniValue eObj;
        e.ToJson(eObj);
        newQuorumsArr.push_back(eObj);
    }
    obj.pushKV("newQuorums", newQuorumsArr);

    CCbTx cbTxPayload;
    if (GetTxPayload(*cbTx, cbTxPayload)) {
        obj.pushKV("merkleRootMNList", cbTxPayload.merkleRootMNList.ToString());
        if (cbTxPayload.nVersion >= 2) {
            obj.pushKV("merkleRootQuorums", cbTxPayload.merkleRootQuorums.ToString());
        }
    }
}

bool BuildSimplifiedMNListDiff(const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet)
{
    AssertLockHeld(cs_main);
    mnListDiffRet = CSimplifiedMNListDiff();

    const CBlockIndex* baseBlockIndex = chainActive.Genesis();
    if (!baseBlockHash.IsNull()) {
        baseBlockIndex = LookupBlockIndex(baseBlockHash);
        if (!baseBlockIndex) {
            errorRet = strprintf("block %s not found", baseBlockHash.ToString());
            return false;
        }
    }

    const CBlockIndex* blockIndex = LookupBlockIndex(blockHash);
    if (!blockIndex) {
        errorRet = strprintf("block %s not found", blockHash.ToString());
        return false;
    }

    if (!chainActive.Contains(baseBlockIndex) || !chainActive.Contains(blockIndex)) {
        errorRet = strprintf("block %s and %s are not in the same chain", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }
    if (baseBlockIndex->nHeight > blockIndex->nHeight) {
        errorRet = strprintf("base block %s is higher then block %s", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }

    LOCK(deterministicMNManager->cs);

    auto baseDmnList = deterministicMNManager->GetListForBlock(baseBlockIndex);
    auto dmnList = deterministicMNManager->GetListForBlock(blockIndex);
    mnListDiffRet = baseDmnList.BuildSimplifiedDiff(dmnList);

    // We need to return the value that was provided by the other peer as it otherwise won't be able to recognize the
    // response. This will usually be identical to the block found in baseBlockIndex. The only difference is when a
    // null block hash was provided to get the diff from the genesis block.
    mnListDiffRet.baseBlockHash = baseBlockHash;

    if (!mnListDiffRet.BuildQuorumsDiff(baseBlockIndex, blockIndex)) {
        errorRet = strprintf("failed to build quorums diff");
        return false;
    }

    // TODO store coinbase TX in CBlockIndex
    CBlock block;
    if (!ReadBlockFromDisk(block, blockIndex, Params().GetConsensus())) {
        errorRet = strprintf("failed to read block %s from disk", blockHash.ToString());
        return false;
    }

    mnListDiffRet.cbTx = block.vtx[0];

    std::vector<uint256> vHashes;
    std::vector<bool> vMatch(block.vtx.size(), false);
    for (const auto& tx : block.vtx) {
        vHashes.emplace_back(tx->GetHash());
    }
    vMatch[0] = true; // only coinbase matches
    mnListDiffRet.cbTxMerkleTree = CPartialMerkleTree(vHashes, vMatch);

    return true;
}
