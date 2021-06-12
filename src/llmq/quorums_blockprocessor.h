// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
#define SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H

#include <llmq/quorums_utils.h>

#include <unordered_map>
#include <unordered_lru_cache.h>
#include <saltedhasher.h>
extern RecursiveMutex cs_main;
class CNode;
class CConnman;
class PeerManager;
class BlockValidationState;
class ChainstateManager;
namespace llmq
{
class CFinalCommitment;
typedef std::shared_ptr<CFinalCommitment> CFinalCommitmentPtr;
class CQuorumBlockProcessor
{
private:
    CConnman& connman;
    ChainstateManager &chainman;
    // TODO cleanup
    mutable RecursiveMutex minableCommitmentsCs;
    std::map<std::pair<uint8_t, uint256>, uint256> minableCommitmentsByQuorum GUARDED_BY(minableCommitmentsCs);
    std::map<uint256, CFinalCommitment> minableCommitments GUARDED_BY(minableCommitmentsCs);

    std::map<uint8_t, unordered_lru_cache<uint256, bool, StaticSaltedHasher>> mapHasMinedCommitmentCache GUARDED_BY(minableCommitmentsCs);

public:
    explicit CQuorumBlockProcessor(CConnman &_connman, ChainstateManager& _chainman);


    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, PeerManager& peerman);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void AddMineableCommitment(const CFinalCommitment& fqc);
    bool HasMineableCommitment(const uint256& hash);
    bool GetMineableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret);
    bool GetMinableCommitment(uint8_t llmqType, int nHeight, CFinalCommitment& ret) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool HasMinedCommitment(uint8_t llmqType, const uint256& quorumHash);
    CFinalCommitmentPtr GetMinedCommitment(uint8_t llmqType, const uint256& quorumHash, uint256& retMinedBlockHash);

    void GetMinedCommitmentsUntilBlock(uint8_t llmqType, const CBlockIndex* pindex, size_t maxCount, std::vector<const CBlockIndex*> &ret);
    void GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex, std::map<uint8_t, std::vector<const CBlockIndex*>> &ret);

private:
    static bool GetCommitmentsFromBlock(const CBlock& block, const uint32_t& nHeight, std::map<uint8_t, CFinalCommitment>& ret, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state, bool fJustCheck) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    static bool IsMiningPhase(uint8_t llmqType, int nHeight);
    bool IsCommitmentRequired(uint8_t llmqType, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    static uint256 GetQuorumBlockHash(ChainstateManager& chainman, uint8_t llmqType, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
