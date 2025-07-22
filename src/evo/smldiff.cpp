// Copyright (c) 2017-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/smldiff.h>


#include <chainparams.h>
#include <consensus/merkle.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <evo/specialtx.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/quorums.h>
#include <node/blockstorage.h>
#include <serialize.h>
#include <univalue.h>
#include <util/enumerate.h>
#include <validation.h>

using node::ReadBlockFromDisk;

// Forward declaration
std::optional<std::pair<CBLSSignature, uint32_t>> GetNonNullCoinbaseChainlock(const CBlockIndex* pindex);

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
            auto [qc, minedBlockHash] = quorum_block_processor.GetMinedCommitment(llmqType, hash);
            if (minedBlockHash == uint256::ZERO) {
                return false;
            }
            newQuorums.emplace_back(std::move(qc));
        }
    }

    return true;
}

bool CSimplifiedMNListDiff::BuildQuorumChainlockInfo(const llmq::CQuorumManager& qman, const CBlockIndex* blockIndex)
{
    // Group quorums (indexes corresponding to entries of newQuorums) per CBlockIndex containing the expected CL
    // signature in CbTx. We want to avoid to load CbTx now, as more than one quorum will target the same block: hence
    // we want to load CbTxs once per block (heavy operation).
    std::multimap<const CBlockIndex*, uint16_t> workBaseBlockIndexMap;

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

        // In case of rotation, all rotated quorums rely on the CL sig expected in the cycleBlock (the block of the
        // first DKG) - 8 In case of non-rotation, quorums rely on the CL sig expected in the block of the DKG - 8
        const CBlockIndex* pWorkBaseBlockIndex = blockIndex->GetAncestor(quorum->m_quorum_base_block_index->nHeight -
                                                                         quorum->qc->quorumIndex - 8);

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
        std::transform(it_begin, it_end, std::inserter(idx_set, idx_set.end()),
                       [](const auto& pair) { return pair.second; });
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

CSimplifiedMNListDiff BuildSimplifiedDiff(const CDeterministicMNList& from, const CDeterministicMNList& to, bool extended)
{
    CSimplifiedMNListDiff diffRet;
    diffRet.baseBlockHash = from.GetBlockHash();
    diffRet.blockHash = to.GetBlockHash();

    to.ForEachMN(false, [&](const auto& toPtr) {
        auto fromPtr = from.GetMN(toPtr.proTxHash);
        if (fromPtr == nullptr) {
            CSimplifiedMNListEntry sme{toPtr.to_sml_entry()};
            diffRet.mnList.push_back(std::move(sme));
        } else {
            CSimplifiedMNListEntry sme1{toPtr.to_sml_entry()};
            CSimplifiedMNListEntry sme2(fromPtr->to_sml_entry());
            if ((sme1 != sme2) || (extended && (sme1.scriptPayout != sme2.scriptPayout ||
                                                sme1.scriptOperatorPayout != sme2.scriptOperatorPayout))) {
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

bool BuildSimplifiedMNListDiff(CDeterministicMNManager& dmnman, const ChainstateManager& chainman,
                               const llmq::CQuorumBlockProcessor& qblockman, const llmq::CQuorumManager& qman,
                               const uint256& baseBlockHash, const uint256& blockHash,
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
