// Copyright (c) 2018 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITGREEN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
#define BITGREEN_LLMQ_QUORUMS_BLOCKPROCESSOR_H

#include <llmq/quorums_commitment.h>
#include <llmq/quorums_utils.h>

#include <consensus/params.h>
#include <primitives/transaction.h>
#include <saltedhasher.h>
#include <sync.h>

#include <map>
#include <unordered_map>

class CNode;
class CConnman;

namespace llmq
{

class CQuorumBlockProcessor
{
private:
    CSpecialDB& specialDb;

    // TODO cleanup
    CCriticalSection minableCommitmentsCs;
    std::map<std::pair<Consensus::LLMQType, uint256>, uint256> minableCommitmentsByQuorum;
    std::map<uint256, CFinalCommitment> minableCommitments;

    std::unordered_map<std::pair<Consensus::LLMQType, uint256>, bool, StaticSaltedHasher> hasMinedCommitmentCache;

public:
    CQuorumBlockProcessor(CSpecialDB& _specialDb) : specialDb(_specialDb) {}

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex);

    void AddMinableCommitment(const CFinalCommitment& fqc);
    bool HasMinableCommitment(const uint256& hash);
    bool GetMinableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret);
    bool GetMinableCommitment(Consensus::LLMQType llmqType, int nHeight, CFinalCommitment& ret);
    bool GetMinableCommitmentTx(Consensus::LLMQType llmqType, int nHeight, CTransactionRef& ret);

    bool HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash);
    bool GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, CFinalCommitment& ret, uint256& retMinedBlockHash);

    std::vector<const CBlockIndex*> GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount);
    std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex);

private:
    bool GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindex, std::map<Consensus::LLMQType, CFinalCommitment>& ret, CValidationState& state);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, CValidationState& state);
    bool IsMiningPhase(Consensus::LLMQType llmqType, int nHeight);
    bool IsCommitmentRequired(Consensus::LLMQType llmqType, int nHeight);
    uint256 GetQuorumBlockHash(Consensus::LLMQType llmqType, int nHeight);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

}

#endif//BITGREEN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
