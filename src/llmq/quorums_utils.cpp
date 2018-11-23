// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_utils.h"

#include "chainparams.h"
#include "random.h"

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


}
