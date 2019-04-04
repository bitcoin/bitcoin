// Copyright (c) 2017-2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cbtx.h"
#include "deterministicmns.h"
#include "llmq/quorums.h"
#include "llmq/quorums_blockprocessor.h"
#include "llmq/quorums_commitment.h"
#include "simplifiedmns.h"
#include "specialtx.h"

#include "chainparams.h"
#include "consensus/merkle.h"
#include "univalue.h"
#include "validation.h"

bool CheckCbTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state)
{
    if (tx.nType != TRANSACTION_COINBASE) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-type");
    }

    if (!tx.IsCoinBase()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-invalid");
    }

    CCbTx cbTx;
    if (!GetTxPayload(tx, cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    if (cbTx.nVersion == 0 || cbTx.nVersion > CCbTx::CURRENT_VERSION) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
    }

    if (pindexPrev && pindexPrev->nHeight + 1 != cbTx.nHeight) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-height");
    }

    if (pindexPrev) {
        bool fDIP0008Active = VersionBitsState(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0008, versionbitscache) == THRESHOLD_ACTIVE;
        if (fDIP0008Active && cbTx.nVersion < 2) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-version");
        }
    }

    return true;
}

// This can only be done after the block has been fully processed, as otherwise we won't have the finished MN list
bool CheckCbTxMerkleRoots(const CBlock& block, const CBlockIndex* pindex, CValidationState& state)
{
    if (block.vtx[0]->nType != TRANSACTION_COINBASE) {
        return true;
    }

    CCbTx cbTx;
    if (!GetTxPayload(*block.vtx[0], cbTx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-payload");
    }

    if (pindex) {
        uint256 calculatedMerkleRoot;
        if (!CalcCbTxMerkleRootMNList(block, pindex->pprev, calculatedMerkleRoot, state)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }
        if (calculatedMerkleRoot != cbTx.merkleRootMNList) {
            return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-mnmerkleroot");
        }
        if (cbTx.nVersion >= 2) {
            if (!CalcCbTxMerkleRootQuorums(block, pindex->pprev, calculatedMerkleRoot, state)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-quorummerkleroot");
            }
            if (calculatedMerkleRoot != cbTx.merkleRootQuorums) {
                return state.DoS(100, false, REJECT_INVALID, "bad-cbtx-quorummerkleroot");
            }
        }
    }

    return true;
}

bool CalcCbTxMerkleRootMNList(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state)
{
    LOCK(deterministicMNManager->cs);

    CDeterministicMNList tmpMNList;
    if (!deterministicMNManager->BuildNewListFromBlock(block, pindexPrev, state, tmpMNList, false)) {
        return false;
    }

    CSimplifiedMNList sml(tmpMNList);

    bool mutated = false;
    merkleRootRet = sml.CalcMerkleRoot(&mutated);
    return !mutated;
}

bool CalcCbTxMerkleRootQuorums(const CBlock& block, const CBlockIndex* pindexPrev, uint256& merkleRootRet, CValidationState& state)
{
    auto quorums = llmq::quorumBlockProcessor->GetMinedAndActiveCommitmentsUntilBlock(pindexPrev);
    std::map<Consensus::LLMQType, std::vector<uint256>> qcHashes;
    size_t hashCount = 0;
    for (const auto& p : quorums) {
        auto& v = qcHashes[p.first];
        v.reserve(p.second.size());
        for (const auto& p2 : p.second) {
            llmq::CFinalCommitment qc;
            uint256 minedBlockHash;
            bool found = llmq::quorumBlockProcessor->GetMinedCommitment(p.first, p2->GetBlockHash(), qc, minedBlockHash);
            assert(found);
            v.emplace_back(::SerializeHash(qc));
            hashCount++;
        }
    }

    // now add the commitments from the current block, which are not returned by GetMinedAndActiveCommitmentsUntilBlock
    // due to the use of pindexPrev (we don't have the tip index here)
    for (size_t i = 1; i < block.vtx.size(); i++) {
        auto& tx = block.vtx[i];

        if (tx->nVersion == 3 && tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            llmq::CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(*tx, qc)) {
                assert(false);
            }
            if (qc.commitment.IsNull()) {
                continue;
            }
            auto qcHash = ::SerializeHash(qc.commitment);
            const auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)qc.commitment.llmqType);
            auto& v = qcHashes[params.type];
            if (v.size() == params.signingActiveQuorumCount) {
                v.pop_back();
            }
            v.emplace_back(qcHash);
            hashCount++;
            assert(v.size() <= params.signingActiveQuorumCount);
        }
    }

    std::vector<uint256> qcHashesVec;
    qcHashesVec.reserve(hashCount);

    for (const auto& p : qcHashes) {
        for (const auto& h : p.second) {
            qcHashesVec.emplace_back(h);
        }
    }
    std::sort(qcHashesVec.begin(), qcHashesVec.end());

    bool mutated = false;
    merkleRootRet = ComputeMerkleRoot(qcHashesVec, &mutated);
    return !mutated;
}

std::string CCbTx::ToString() const
{
    return strprintf("CCbTx(nHeight=%d, nVersion=%d, merkleRootMNList=%s, merkleRootQuorums=%s)",
        nVersion, nHeight, merkleRootMNList.ToString(), merkleRootQuorums.ToString());
}

void CCbTx::ToJson(UniValue& obj) const
{
    obj.clear();
    obj.setObject();
    obj.push_back(Pair("version", (int)nVersion));
    obj.push_back(Pair("height", (int)nHeight));
    obj.push_back(Pair("merkleRootMNList", merkleRootMNList.ToString()));
    if (nVersion >= 2) {
        obj.push_back(Pair("merkleRootQuorums", merkleRootQuorums.ToString()));
    }
}
