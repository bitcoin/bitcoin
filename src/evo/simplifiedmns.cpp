// Copyright (c) 2017-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/simplifiedmns.h>

#include <evo/cbtx.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <node/blockstorage.h>
#include <evo/specialtx.h>

#include <pubkey.h>
#include <serialize.h>
#include <version.h>

#include <base58.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <univalue.h>
#include <validation.h>
#include <key_io.h>
#include <util/underlying.h>
#include <util/enumerate.h>

using node::ReadBlockFromDisk;

CSimplifiedMNListEntry::CSimplifiedMNListEntry(const CDeterministicMN& dmn) :
    proRegTxHash(dmn.proTxHash),
    confirmedHash(dmn.pdmnState->confirmedHash),
    netInfo(dmn.pdmnState->netInfo),
    pubKeyOperator(dmn.pdmnState->pubKeyOperator),
    keyIDVoting(dmn.pdmnState->keyIDVoting),
    isValid(!dmn.pdmnState->IsBanned()),
    platformHTTPPort(dmn.pdmnState->platformHTTPPort),
    platformNodeID(dmn.pdmnState->platformNodeID),
    scriptPayout(dmn.pdmnState->scriptPayout),
    scriptOperatorPayout(dmn.pdmnState->scriptOperatorPayout),
    nVersion(dmn.pdmnState->nVersion),
    nType(dmn.nType)
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
    CTxDestination dest;
    std::string payoutAddress = "unknown";
    std::string operatorPayoutAddress = "none";
    if (ExtractDestination(scriptPayout, dest)) {
        payoutAddress = EncodeDestination(dest);
    }
    if (ExtractDestination(scriptOperatorPayout, dest)) {
        operatorPayoutAddress = EncodeDestination(dest);
    }

    return strprintf("CSimplifiedMNListEntry(nVersion=%d, nType=%d, proRegTxHash=%s, confirmedHash=%s, "
                     "pubKeyOperator=%s, votingAddress=%s, isValid=%d, payoutAddress=%s, operatorPayoutAddress=%s, "
                     "platformHTTPPort=%d, platformNodeID=%s)\n"
                     "  %s",
                     nVersion, ToUnderlying(nType), proRegTxHash.ToString(), confirmedHash.ToString(),
                     pubKeyOperator.ToString(), EncodeDestination(PKHash(keyIDVoting)), isValid, payoutAddress,
                     operatorPayoutAddress, platformHTTPPort, platformNodeID.ToString(), netInfo.ToString());
}

UniValue CSimplifiedMNListEntry::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("nVersion", nVersion);
    obj.pushKV("nType", ToUnderlying(nType));
    obj.pushKV("proRegTxHash", proRegTxHash.ToString());
    obj.pushKV("confirmedHash", confirmedHash.ToString());
    obj.pushKV("service", netInfo.GetPrimary().ToStringAddrPort());
    obj.pushKV("pubKeyOperator", pubKeyOperator.ToString());
    obj.pushKV("votingAddress", EncodeDestination(PKHash(keyIDVoting)));
    obj.pushKV("isValid", isValid);
    if (nType == MnType::Evo) {
        obj.pushKV("platformHTTPPort", platformHTTPPort);
        obj.pushKV("platformNodeID", platformNodeID.ToString());
    }

    if (extended) {
        CTxDestination dest;
        if (ExtractDestination(scriptPayout, dest)) {
            obj.pushKV("payoutAddress", EncodeDestination(dest));
        }
        if (ExtractDestination(scriptOperatorPayout, dest)) {
            obj.pushKV("operatorPayoutAddress", EncodeDestination(dest));
        }
    }
    return obj;
}

CSimplifiedMNList::CSimplifiedMNList(const std::vector<CSimplifiedMNListEntry>& smlEntries)
{
    mnList.reserve(smlEntries.size());
    for (const auto& entry : smlEntries) {
        mnList.emplace_back(std::make_unique<CSimplifiedMNListEntry>(entry));
    }

    std::sort(mnList.begin(), mnList.end(), [&](const std::unique_ptr<CSimplifiedMNListEntry>& a, const std::unique_ptr<CSimplifiedMNListEntry>& b) {
        return a->proRegTxHash.Compare(b->proRegTxHash) < 0;
    });
}

CSimplifiedMNList::CSimplifiedMNList(const CDeterministicMNList& dmnList)
{
    mnList.reserve(dmnList.GetAllMNsCount());
    dmnList.ForEachMN(false, [this](auto& dmn) {
        mnList.emplace_back(std::make_unique<CSimplifiedMNListEntry>(dmn));
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

bool CSimplifiedMNListDiff::BuildQuorumsDiff(const CBlockIndex* baseBlockIndex, const CBlockIndex* blockIndex,
                                             const llmq::CQuorumBlockProcessor& quorum_block_processor)
{
    auto baseQuorums = quorum_block_processor.GetMinedAndActiveCommitmentsUntilBlock(baseBlockIndex);
    auto quorums = quorum_block_processor.GetMinedAndActiveCommitmentsUntilBlock(blockIndex);

    std::set<std::pair<Consensus::LLMQType, uint256>> baseQuorumHashes;
    std::set<std::pair<Consensus::LLMQType, uint256>> quorumHashes;
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
            deletedQuorums.emplace_back((uint8_t)p.first, p.second);
        }
    }
    for (const auto& p : quorumHashes) {
        const auto& [llmqType, hash] = p;
        if (!baseQuorumHashes.count(p)) {
            uint256 minedBlockHash;
            llmq::CFinalCommitmentPtr qc = quorum_block_processor.GetMinedCommitment(llmqType, hash, minedBlockHash);
            if (qc == nullptr) {
                return false;
            }
            newQuorums.emplace_back(*qc);
        }
    }

    return true;
}

bool CSimplifiedMNListDiff::BuildQuorumChainlockInfo(const llmq::CQuorumManager& qman, const CBlockIndex* blockIndex)
{
    // Group quorums (indexes corresponding to entries of newQuorums) per CBlockIndex containing the expected CL signature in CbTx.
    // We want to avoid to load CbTx now, as more than one quorum will target the same block: hence we want to load CbTxs once per block (heavy operation).
    std::multimap<const CBlockIndex*, uint16_t>  workBaseBlockIndexMap;

    for (const auto [idx, e] : enumerate(newQuorums)) {
        // We assume that we have on hand, quorums that correspond to the hashes queried.
        // If we cannot find them, something must have gone wrong and we should cease trying
        // to build any further.
        auto quorum = qman.GetQuorum(e.llmqType, e.quorumHash);
        if (!quorum) {
            LogPrintf("%s: ERROR! Unexpected missing quorum with llmqType=%d, quorumHash=%s\n", __func__,
                      ToUnderlying(e.llmqType), e.quorumHash.ToString());
            return false;
        }

        // In case of rotation, all rotated quorums rely on the CL sig expected in the cycleBlock (the block of the first DKG) - 8
        // In case of non-rotation, quorums rely on the CL sig expected in the block of the DKG - 8
        const CBlockIndex* pWorkBaseBlockIndex =
                blockIndex->GetAncestor(quorum->m_quorum_base_block_index->nHeight - quorum->qc->quorumIndex - 8);

        workBaseBlockIndexMap.insert(std::make_pair(pWorkBaseBlockIndex, idx));
    }

    for (auto it = workBaseBlockIndexMap.begin(); it != workBaseBlockIndexMap.end();) {
        // Process each key (CBlockIndex containing the expected CL signature in CbTx) of the std::multimap once
        const CBlockIndex* pWorkBaseBlockIndex = it->first;
        const auto cbcl = GetNonNullCoinbaseChainlock(pWorkBaseBlockIndex);
        CBLSSignature sig;
        if (cbcl.has_value()) {
            sig = cbcl.value().first;
        }
        // Get the range of indexes (values) for the current key and merge them into a single std::set
        const auto [it_begin, it_end] = workBaseBlockIndexMap.equal_range(it->first);
        std::set<uint16_t> idx_set;
        std::transform(it_begin, it_end, std::inserter(idx_set, idx_set.end()), [](const auto& pair) { return pair.second; });
        // Advance the iterator to the next key
        it = it_end;

        // Different CBlockIndex can contain the same CL sig in CbTx (both non-null or null during the first blocks after v20 activation)
        // Hence, we need to merge the std::set if another std::set already exists for the same sig.
        if (auto [it_sig, inserted] = quorumsCLSigs.insert({sig, idx_set}); !inserted) {
            it_sig->second.insert(idx_set.begin(), idx_set.end());
        }
    }

    return true;
}

UniValue CSimplifiedMNListDiff::ToJson(bool extended) const
{
    UniValue obj(UniValue::VOBJ);

    obj.pushKV("nVersion", nVersion);
    obj.pushKV("baseBlockHash", baseBlockHash.ToString());
    obj.pushKV("blockHash", blockHash.ToString());

    CDataStream ssCbTxMerkleTree(SER_NETWORK, PROTOCOL_VERSION);
    ssCbTxMerkleTree << cbTxMerkleTree;
    obj.pushKV("cbTxMerkleTree", HexStr(ssCbTxMerkleTree));

    obj.pushKV("cbTx", EncodeHexTx(*cbTx));

    UniValue deletedMNsArr(UniValue::VARR);
    for (const auto& h : deletedMNs) {
        deletedMNsArr.push_back(h.ToString());
    }
    obj.pushKV("deletedMNs", deletedMNsArr);

    UniValue mnListArr(UniValue::VARR);
    for (const auto& e : mnList) {
        mnListArr.push_back(e.ToJson(extended));
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
        newQuorumsArr.push_back(e.ToJson());
    }
    obj.pushKV("newQuorums", newQuorumsArr);

    // Do not assert special tx type here since this can be called prior to DIP0003 activation
    if (const auto opt_cbTxPayload = GetTxPayload<CCbTx>(*cbTx, /*assert_type=*/false)) {
        obj.pushKV("merkleRootMNList", opt_cbTxPayload->merkleRootMNList.ToString());
        if (opt_cbTxPayload->nVersion >= CCbTx::Version::MERKLE_ROOT_QUORUMS) {
            obj.pushKV("merkleRootQuorums", opt_cbTxPayload->merkleRootQuorums.ToString());
        }
    }

    UniValue quorumsCLSigsArr(UniValue::VARR);
    for (const auto& [signature, quorumsIndexes] : quorumsCLSigs) {
        UniValue j(UniValue::VOBJ);
        UniValue idxArr(UniValue::VARR);
        for (const auto& idx : quorumsIndexes) {
            idxArr.push_back(idx);
        }
        j.pushKV(signature.ToString(),idxArr);
        quorumsCLSigsArr.push_back(j);
    }
    obj.pushKV("quorumsCLSigs", quorumsCLSigsArr);
    return obj;
}

CSimplifiedMNListDiff BuildSimplifiedDiff(const CDeterministicMNList& from, const CDeterministicMNList& to, bool extended)
{
    CSimplifiedMNListDiff diffRet;
    diffRet.baseBlockHash = from.GetBlockHash();
    diffRet.blockHash = to.GetBlockHash();

    to.ForEachMN(false, [&](const auto& toPtr) {
        auto fromPtr = from.GetMN(toPtr.proTxHash);
        if (fromPtr == nullptr) {
            CSimplifiedMNListEntry sme(toPtr);
            diffRet.mnList.push_back(std::move(sme));
        } else {
            CSimplifiedMNListEntry sme1(toPtr);
            CSimplifiedMNListEntry sme2(*fromPtr);
            if ((sme1 != sme2) ||
                (extended && (sme1.scriptPayout != sme2.scriptPayout || sme1.scriptOperatorPayout != sme2.scriptOperatorPayout))) {
                    diffRet.mnList.push_back(std::move(sme1));
            }
        }
    });

    from.ForEachMN(false, [&](auto& fromPtr) {
        auto toPtr = to.GetMN(fromPtr.proTxHash);
        if (toPtr == nullptr) {
            diffRet.deletedMNs.emplace_back(fromPtr.proTxHash);
        }
    });

    return diffRet;
}

bool BuildSimplifiedMNListDiff(CDeterministicMNManager& dmnman, const ChainstateManager& chainman, const llmq::CQuorumBlockProcessor& qblockman,
                               const llmq::CQuorumManager& qman, const uint256& baseBlockHash, const uint256& blockHash,
                               CSimplifiedMNListDiff& mnListDiffRet, std::string& errorRet, bool extended)
{
    AssertLockHeld(::cs_main);
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

    auto baseDmnList = dmnman.GetListForBlock(baseBlockIndex);
    auto dmnList = dmnman.GetListForBlock(blockIndex);
    mnListDiffRet = BuildSimplifiedDiff(baseDmnList, dmnList, extended);

    // We need to return the value that was provided by the other peer as it otherwise won't be able to recognize the
    // response. This will usually be identical to the block found in baseBlockIndex. The only difference is when a
    // null block hash was provided to get the diff from the genesis block.
    mnListDiffRet.baseBlockHash = baseBlockHash;

    if (!mnListDiffRet.BuildQuorumsDiff(baseBlockIndex, blockIndex, qblockman)) {
        errorRet = strprintf("failed to build quorums diff");
        return false;
    }

    if (DeploymentActiveAfter(blockIndex, Params().GetConsensus(), Consensus::DEPLOYMENT_V20)) {
        if (!mnListDiffRet.BuildQuorumChainlockInfo(qman, blockIndex)) {
            errorRet = strprintf("failed to build quorum chainlock info");
            return false;
        }
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
