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
#include <threadsafety.h>
class CNode;
class CBlock;
class PeerManager;
class BlockValidationState;
class ChainstateManager;
class CInv;
namespace llmq
{
class CFinalCommitment;
class CFinalCommitmentTxPayload;
using CFinalCommitmentPtr = std::unique_ptr<CFinalCommitment>;

class CQuorumBlockProcessor
{
private:
    PeerManager& peerman;
    ChainstateManager &chainman;
    mutable Mutex minableCommitmentsCs;
    std::map<uint256, uint256> minableCommitmentsByQuorum GUARDED_BY(minableCommitmentsCs);
    std::map<uint256, CFinalCommitment> minableCommitments GUARDED_BY(minableCommitmentsCs);

public:
    CEvoDB<uint256, std::pair<CFinalCommitment, uint256>, StaticSaltedHasher> m_commitment_evoDb;
    explicit CQuorumBlockProcessor(const DBParams& db_commitment_params, PeerManager &_peerman, ChainstateManager& _chainman);

    void ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv) EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);

    bool ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, llmq::CFinalCommitmentTxPayload& qcTx, bool fJustCheck, bool fBLSChecks) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
    bool UndoBlock(const CBlock& block, const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    std::optional<CInv> AddMineableCommitment(const CFinalCommitment& fqc);
    bool HasMineableCommitment(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    bool GetMineableCommitmentByHash(const uint256& commitmentHash, CFinalCommitment& ret) EXCLUSIVE_LOCKS_REQUIRED(!minableCommitmentsCs);
    bool GetMinableCommitment(int nHeight, CFinalCommitment& ret) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);

    bool HasMinedCommitment(const uint256& quorumHash);
    CFinalCommitmentPtr GetMinedCommitment(const uint256& quorumHash, uint256& retMinedBlockHash);
    bool FlushCacheToDisk(bool fSync = true);
    static bool IsMiningPhase(int nHeight);
private:
    static bool GetCommitmentsFromBlock(const CBlock& block, const uint32_t& nHeight, llmq::CFinalCommitmentTxPayload &qcRet, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state, bool fJustCheck, bool fBLSChecks) EXCLUSIVE_LOCKS_REQUIRED(::cs_main, !minableCommitmentsCs);
    bool IsCommitmentRequired(int nHeight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    static uint256 GetQuorumBlockHash(ChainstateManager& chainman, int nHeight) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

extern CQuorumBlockProcessor* quorumBlockProcessor;

} // namespace llmq

#endif // SYSCOIN_LLMQ_QUORUMS_BLOCKPROCESSOR_H
