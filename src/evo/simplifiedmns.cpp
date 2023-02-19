// Copyright (c) 2017-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <bls/bls.h>
#include <evo/cbtx.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <pubkey.h>
#include <serialize.h>
#include <version.h>

#include <base58.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <univalue.h>
#include <validation.h>
#include <node/blockstorage.h>
using node::ReadBlockFromDisk;
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
        proRegTxHash.ToString(), confirmedHash.ToString(), service.ToStringAddrPort(), pubKeyOperator.Get().ToString(), EncodeDestination(WitnessV0KeyHash(keyIDVoting)), isValid);
}

void CSimplifiedMNListEntry::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    obj.pushKV("service", service.ToStringAddrPort());
    obj.pushKV("pubKeyOperator", pubKeyOperator.Get().ToString());
    obj.pushKV("votingAddress", EncodeDestination(WitnessV0KeyHash(keyIDVoting)));
    obj.pushKV("isValid", isValid);
    obj.pushKV("nVersion", nVersion);

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

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList, bool isV19Active)
{
    mnList.resize(dmnList.GetAllMNsCount());

    size_t i = 0;
    dmnList.ForEachMN(false, [this, &i, isV19Active](auto& dmn) {
        auto sme = std::make_unique<CSimplifiedMNListEntry>(dmn);
        sme->nVersion = isV19Active ? CSimplifiedMNListEntry::BASIC_BLS_VERSION : CSimplifiedMNListEntry::LEGACY_BLS_VERSION;
        mnList[i++] = std::move(sme);
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
bool CSimplifiedMNList::operator==(const CSimplifiedMNList& rhs) const
{
    return mnList.size() == rhs.mnList.size() &&
            std::equal(mnList.begin(), mnList.end(), rhs.mnList.begin(),
                [](const std::unique_ptr<CSimplifiedMNListEntry>& left, const std::unique_ptr<CSimplifiedMNListEntry>& right)
                {
                    return *left == *right;
                }
            );
}
CSimplifiedMNListDiff::CSimplifiedMNListDiff() = default;

CSimplifiedMNListDiff::~CSimplifiedMNListDiff() = default;

bool CSimplifiedMNListDiff::BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex)
{
    auto baseQuorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(baseBlockIndex);
    auto quorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(blockIndex);

    std::set<std::pair<uint8_t, uint256>> baseQuorumHashes;
    std::set<std::pair<uint8_t, uint256>> quorumHashes;
    for (const auto& [llmqType, vecBlockIndex] : baseQuorums) {
        for (const auto& blockindex : vecBlockIndex) {
            baseQuorumHashes.emplace(llmqType, blockindex->GetBlockHash());
        }
    }
    for (const auto& [llmqType, vecBlockIndex] : quorums) {
        for (const auto& blockindex : vecBlockIndex) {
            quorumHashes.emplace(llmqType, blockindex->GetBlockHash());
        }
    }

    for (const auto& p : baseQuorumHashes) {
        if (!quorumHashes.count(p)) {
            deletedQuorums.emplace_back(p.first, p.second);
        }
    }
    for (const auto& p : quorumHashes) {
        const auto& [llmqType, hash] = p;
        if (!baseQuorumHashes.count(p)) {
            uint256 minedBlockHash;
            llmq::CFinalCommitmentPtr qc = llmq::quorumBlockProcessor->GetMinedCommitment(llmqType, hash, minedBlockHash);
            if (qc == nullptr) {
                return false;
            }
            newQuorums.emplace_back(*qc);
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
    obj.pushKV("cbTxMerkleTree", HexStr(ssCbTxMerkleTree));

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
    obj.pushKV("nVersion", nVersion);

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
    obj.pushKV("merkleRootMNList", merkleRootMNList.ToString());
    obj.pushKV("merkleRootQuorums", merkleRootQuorums.ToString()); 
}

bool BuildSimplifiedMNListDiff(ChainstateManager& chainman, const uint256& baseBlockHash, const uint256& blockHash, CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet)
{
    if(!deterministicMNManager)
        return false;
    LOCK(cs_main);
    mnListDiffRet = CSimplifiedMNListDiff();
    const CBlockIndex* baseBlockIndex = chainman.ActiveChain().Genesis();
    if (!baseBlockHash.IsNull()) {
        baseBlockIndex = chainman.m_blockman.LookupBlockIndex(baseBlockHash);
        if (!baseBlockIndex) {
            errorRet = strprintf("block %s not found", baseBlockHash.ToString());
            return false;
        }
    }
    
    const CBlockIndex* blockIndex = chainman.m_blockman.LookupBlockIndex(blockHash);
    if (!blockIndex) {
        errorRet = strprintf("block %s not found", blockHash.ToString());
        return false;
    }
    if (!chainman.ActiveChain().Contains(baseBlockIndex) || !chainman.ActiveChain().Contains(blockIndex)) {
        errorRet = strprintf("block %s and %s are not in the same chain", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }
    if (baseBlockIndex->nHeight > blockIndex->nHeight) {
        errorRet = strprintf("base block %s is higher then block %s", baseBlockHash.ToString(), blockHash.ToString());
        return false;
    }
    const int nHeight = chainman.ActiveHeight();
    LOCK(deterministicMNManager->cs);
    auto baseDmnList = deterministicMNManager->GetListForBlock(baseBlockIndex);
    auto dmnList = deterministicMNManager->GetListForBlock(blockIndex);
    mnListDiffRet = baseDmnList.BuildSimplifiedDiff(dmnList, nHeight);
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
    CCbTx cbTxPayload;
    if(GetTxPayload(*block.vtx[0], cbTxPayload)) {
        mnListDiffRet.merkleRootMNList = cbTxPayload.merkleRootMNList;
        mnListDiffRet.merkleRootQuorums = cbTxPayload.merkleRootQuorums;
    } else {
        mnListDiffRet.merkleRootMNList.SetNull();
        mnListDiffRet.merkleRootQuorums.SetNull();
    }
    
    std::vector<uint256> vHashes;
    std::vector<bool> vMatch(block.vtx.size(), false);
    for (const auto& tx : block.vtx) {
        vHashes.emplace_back(tx->GetHash());
    }
    vMatch[0] = true; // only coinbase matches
    mnListDiffRet.cbTxMerkleTree = CPartialMerkleTree(vHashes, vMatch);
    return true;
}
