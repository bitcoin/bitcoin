// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLQM_QUORUMS_BLOCKPROCESSOR_H
#define SYSCOIN_LLQM_QUORUMS_BLOCKPROCESSOR_H

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
    CEvoDB& evoDb;
    CConnman& connman;
    // TODO cleanup
    RecursiveMutex minableCommitmentsCs;
    std::map<std::pair<uint8_t, uint256>, uint256> minableCommitmentsByQuorum;
    std::map<uint256, CFinalCommitment> minableCommitments;

    std::unordered_map<std::pair<uint8_t, uint256>, bool, StaticSaltedHasher> hasMinedCommitmentCache;

public:
    explicit CQuorumBlockProcessor(CEvoDB& _evoDb, CConnman &_connman) : evoDb(_evoDb), connman(_connman){}

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex);

    void AddMinableCommitment(const CFinalCommitment& fqc);
    bool HasMinableCommitment(const uint256& hash);
    bool GetMinableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret);
    bool GetMinableCommitment(uint8_t llmqType, int nHeight, CFinalCommitment& ret);

    bool HasMinedCommitment(uint8_t llmqType, const uint256& quorumHash);
    bool GetMinedCommitment(uint8_t llmqType, const uint256& quorumHash, CFinalCommitment& ret, uint256& retMinedBlockHash);

    std::vector<const CBlockIndex*> GetMinedCommitmentsUntilBlock(uint8_t llmqType, const CBlockIndex* pindex, size_t maxCount);
    std::map<uint8_t, std::vector<const CBlockIndex*>> GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex);

private:
    static bool GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindex, std::map<uint8_t, CFinalCommitment>& ret, BlockValidationState& state);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state);
    static bool IsMiningPhase(uint8_t llmqType, int nHeight);
    bool IsCommitmentRequired(uint8_t llmqType, int nHeight);
    static uint256 GetQuorumBlockHash(uint8_t llmqType, int nHeight);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

} // namespace llmq

#endif//SYSCOIN_LLQM_QUORUMS_BLOCKPROCESSOR_H
