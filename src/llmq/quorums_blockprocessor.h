// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
#define SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H

#include <llmq/quorums_utils.h>
#include <evo/evodb.h>
#include <unordered_map>
#include <saltedhasher.h>
#include <kernel/cs_main.h>
class CNode;
class PeerManager;
class BlockValidationState;
class ChainstateManager;
namespace llmq
{
class CFinalCommitment;
using CFinalCommitmentPtr = std::unique_ptr<CFinalCommitment>;

class CQuorumBlockProcessor
{
private:
    PeerManager& peerman;
    ChainstateManager &chainman;
    mutable RecursiveMutex minableCommitmentsCs;
    std::map<uint256, uint256> minableCommitmentsByQuorum GUARDED_BY(minableCommitmentsCs);
    std::map<uint256, CFinalCommitment> minableCommitments GUARDED_BY(minableCommitmentsCs);

public:
    CEvoDB<uint256, std::pair<CFinalCommitment, uint256>> m_commitment_evoDb;
    explicit CQuorumBlockProcessor(const DBParams& db_commitment_params, PeerManager &_peerman, ChainstateManager& _chainman);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, PeerManager& peerman);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, bool fJustCheck, bool fBLSChecks) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    void AddMineableCommitment(const CFinalCommitment& fqc);
    bool HasMineableCommitment(const uint256& hash) const;
    bool GetMineableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret);
    bool GetMinableCommitment(int nHeight, CFinalCommitment& ret) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    bool HasMinedCommitment(const uint256& quorumHash) const;
    CFinalCommitmentPtr GetMinedCommitment(const uint256& quorumHash, uint256& retMinedBlockHash) const;

    bool FlushCacheToDisk();
private:
    static bool GetCommitmentsFromBlock(const CBlock& block, const uint32_t& nHeight, CFinalCommitment& ret, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state, bool fJustCheck, bool fBLSChecks) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    static bool IsMiningPhase(int nHeight);
    bool IsCommitmentRequired(int nHeight) const EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    static uint256 GetQuorumBlockHash(ChainstateManager& chainman, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
