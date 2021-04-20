// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
#define BITCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H

#include <llmq/quorums_utils.h>

#include <unordered_map>
#include <unordered_lru_cache.h>
#include <saltedhasher.h>

class CNode;
class CConnman;
class CValidationState;
class CEvoDB;

namespace llmq
{

class CFinalCommitment;
typedef std::shared_ptr<CFinalCommitment> CFinalCommitmentPtr;

class CQuorumBlockProcessor
{
private:
    CEvoDB& evoDb;

    // TODO cleanup
    CCriticalSection minableCommitmentsCs;
    std::map<std::pair<Consensus::LLMQType, uint256>, uint256> minableCommitmentsByQuorum;
    std::map<uint256, CFinalCommitment> minableCommitments;

    std::map<Consensus::LLMQType, unordered_lru_cache<uint256, bool, StaticSaltedHasher>> mapHasMinedCommitmentCache;

public:
    explicit CQuorumBlockProcessor(CEvoDB& _evoDb);

    bool UpgradeDB();

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state, bool fJustCheck);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex);

    void AddMineableCommitment(const CFinalCommitment& fqc);
    bool HasMineableCommitment(const uint256& hash);
    bool GetMineableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret);
    bool GetMineableCommitment(Consensus::LLMQType llmqType, int nHeight, CFinalCommitment& ret);
    bool GetMineableCommitmentTx(Consensus::LLMQType llmqType, int nHeight, CTransactionRef& ret);

    bool HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash);
    CFinalCommitmentPtr GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, uint256& retMinedBlockHash);

    std::vector<const CBlockIndex*> GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount);
    std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex);

private:
    static bool GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindex, std::map<Consensus::LLMQType, CFinalCommitment>& ret, CValidationState& state);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, CValidationState& state, bool fJustCheck);
    static bool IsMiningPhase(Consensus::LLMQType llmqType, int nHeight);
    bool IsCommitmentRequired(Consensus::LLMQType llmqType, int nHeight);
    static uint256 GetQuorumBlockHash(Consensus::LLMQType llmqType, int nHeight);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

} // namespace llmq

#endif // BITCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
