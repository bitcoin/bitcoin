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
class CDeterministicMNManager;
class CMNHFManager;
class TxValidationState;
struct MNListUpdates;

namespace Consensus { struct Params; }
namespace llmq {
class CQuorumBlockProcessor;
class CChainLocksHandler;
} // namespace llmq

extern RecursiveMutex cs_main;

bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs,
                    TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

class CSpecialTxProcessor
{
private:
    CDeterministicMNManager& m_dmnman;
    CMNHFManager& m_mnhfman;
    llmq::CQuorumBlockProcessor& m_qblockman;
    const Consensus::Params& m_consensus_params;
    const llmq::CChainLocksHandler& m_clhandler;

private:
    [[nodiscard]] bool ProcessSpecialTx(const CTransaction& tx, const CBlockIndex* pindex, TxValidationState& state);
    [[nodiscard]] bool UndoSpecialTx(const CTransaction& tx, const CBlockIndex* pindex);

public:
    explicit CSpecialTxProcessor(CDeterministicMNManager& dmnman, CMNHFManager& mnhfman, llmq::CQuorumBlockProcessor& qblockman,
                                 const Consensus::Params& consensus_params, const llmq::CChainLocksHandler& clhandler) :
        m_dmnman{dmnman}, m_mnhfman{mnhfman}, m_qblockman{qblockman}, m_consensus_params{consensus_params}, m_clhandler{clhandler} {}

    bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                  bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    bool CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CAmount blockSubsidy, BlockValidationState& state);
};

#endif // BITCOIN_EVO_SPECIALTXMAN_H
