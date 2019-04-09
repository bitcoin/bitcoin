// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums.h"
#include "quorums_utils.h"

#include "chainparams.h"
#include "random.h"
#include "validation.h"

namespace llmq
{

std::vector<CDeterministicMNCPtr> CLLMQUtils::GetAllQuorumMembers(Consensus::LLMQType llmqType, const uint256& blockHash)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);
    auto allMns = deterministicMNManager->GetListForBlock(blockHash);
    auto modifier = ::SerializeHash(std::make_pair((uint8_t)llmqType, blockHash));
    return allMns.CalculateQuorum(params.size, modifier);
}

uint256 CLLMQUtils::BuildCommitmentHash(uint8_t llmqType, const uint256& blockHash, const std::vector<bool>& validMembers, const CBLSPublicKey& pubKey, const uint256& vvecHash)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << llmqType;
    hw << blockHash;
    hw << DYNBITSET(validMembers);
    hw << pubKey;
    hw << vvecHash;
    return hw.GetHash();
}

uint256 CLLMQUtils::BuildSignHash(Consensus::LLMQType llmqType, const uint256& quorumHash, const uint256& id, const uint256& msgHash)
{
    CHashWriter h(SER_GETHASH, 0);
    h << (uint8_t)llmqType;
    h << quorumHash;
    h << id;
    h << msgHash;
    return h.GetHash();
}

std::set<uint256> CLLMQUtils::GetQuorumConnections(Consensus::LLMQType llmqType, const uint256& blockHash, const uint256& forMember)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    auto mns = GetAllQuorumMembers(llmqType, blockHash);
    std::set<uint256> result;
    for (size_t i = 0; i < mns.size(); i++) {
        auto& dmn = mns[i];
        if (dmn->proTxHash == forMember) {
            // Connect to nodes at indexes (i+2^k)%n, where
            //   k: 0..max(1, floor(log2(n-1))-1)
            //   n: size of the quorum/ring
            int gap = 1;
            int gap_max = (int)mns.size() - 1;
            int k = 0;
            while ((gap_max >>= 1) || k <= 1) {
                size_t idx = (i + gap) % mns.size();
                auto& otherDmn = mns[idx];
                if (otherDmn == dmn) {
                    continue;
                }
                result.emplace(otherDmn->proTxHash);
                gap <<= 1;
                k++;
            }
            // there can be no two members with the same proTxHash, so return early
            break;
        }
    }
    return result;
}

std::set<size_t> CLLMQUtils::CalcDeterministicWatchConnections(Consensus::LLMQType llmqType, const uint256& blockHash, size_t memberCount, size_t connectionCount)
{
    static uint256 qwatchConnectionSeed;
    static std::atomic<bool> qwatchConnectionSeedGenerated{false};
    static CCriticalSection qwatchConnectionSeedCs;
    if (!qwatchConnectionSeedGenerated) {
        LOCK(qwatchConnectionSeedCs);
        if (!qwatchConnectionSeedGenerated) {
            qwatchConnectionSeed = GetRandHash();
            qwatchConnectionSeedGenerated = true;
        }
    }

    std::set<size_t> result;
    uint256 rnd = qwatchConnectionSeed;
    for (size_t i = 0; i < connectionCount; i++) {
        rnd = ::SerializeHash(std::make_pair(rnd, std::make_pair((uint8_t)llmqType, blockHash)));
        result.emplace(rnd.GetUint64(0) % memberCount);
    }
    return result;
}

bool CLLMQUtils::IsQuorumActive(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    // sig shares and recovered sigs are only accepted from recent/active quorums
    // we allow one more active quorum as specified in consensus, as otherwise there is a small window where things could
    // fail while we are on the brink of a new quorum
    auto quorums = quorumManager->ScanQuorums(llmqType, (int)params.signingActiveQuorumCount + 1);
    for (auto& q : quorums) {
        if (q->qc.quorumHash == quorumHash) {
            return true;
        }
    }
    return false;
}


}
