// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTXMAN_H
#define BITCOIN_EVO_SPECIALTXMAN_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CCoinsViewCache;
class CCreditPoolManager;
class CDeterministicMNManager;
class ChainstateManager;
class CMNHFManager;
class TxValidationState;
struct MNListUpdates;

namespace Consensus { struct Params; }
namespace llmq {
class CQuorumBlockProcessor;
class CQuorumManager;
class CChainLocksHandler;
} // namespace llmq

extern RecursiveMutex cs_main;

class CSpecialTxProcessor
{
private:
    CCreditPoolManager& m_cpoolman;
    CDeterministicMNManager& m_dmnman;
    CMNHFManager& m_mnhfman;
    llmq::CQuorumBlockProcessor& m_qblockman;
    const ChainstateManager& m_chainman;
    const Consensus::Params& m_consensus_params;
    const llmq::CChainLocksHandler& m_clhandler;
    const llmq::CQuorumManager& m_qman;

private:
    [[nodiscard]] bool ProcessSpecialTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state);
    [[nodiscard]] bool UndoSpecialTx(const CTransaction& tx, const CBlockIndex* pindex);

public:
    explicit CSpecialTxProcessor(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman, CMNHFManager& mnhfman,
                                 llmq::CQuorumBlockProcessor& qblockman, const ChainstateManager& chainman, const Consensus::Params& consensus_params,
                                 const llmq::CChainLocksHandler& clhandler, const llmq::CQuorumManager& qman) :
        m_cpoolman(cpoolman), m_dmnman{dmnman}, m_mnhfman{mnhfman}, m_qblockman{qblockman}, m_chainman(chainman), m_consensus_params{consensus_params},
        m_clhandler{clhandler}, m_qman{qman} {}

    bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                  bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CAmount blockSubsidy, BlockValidationState& state)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};

#endif // BITCOIN_EVO_SPECIALTXMAN_H
