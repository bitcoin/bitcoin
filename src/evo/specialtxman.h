// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTXMAN_H
#define BITCOIN_EVO_SPECIALTXMAN_H

#include <gsl/pointers.h>
#include <sync.h>
#include <threadsafety.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CCbTx;
class CCoinsViewCache;
class CCreditPoolManager;
class CDeterministicMNList;
class CDeterministicMNManager;
class CTransaction;
class ChainstateManager;
class CMNHFManager;
class TxValidationState;
struct MNListUpdates;

namespace chainlock {
class Chainlocks;
}
namespace Consensus { struct Params; }
namespace llmq {
class CQuorumBlockProcessor;
class CQuorumManager;
class CQuorumSnapshotManager;
} // namespace llmq

extern RecursiveMutex cs_main; // NOLINT(readability-redundant-declaration)

class CSpecialTxProcessor
{
private:
    CCreditPoolManager& m_cpoolman;
    CDeterministicMNManager& m_dmnman;
    CMNHFManager& m_mnhfman;
    llmq::CQuorumBlockProcessor& m_qblockman;
    llmq::CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const Consensus::Params& m_consensus_params;
    const chainlock::Chainlocks& m_chainlocks;
    const llmq::CQuorumManager& m_qman;

public:
    explicit CSpecialTxProcessor(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman, CMNHFManager& mnhfman,
                                 llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumSnapshotManager& qsnapman,
                                 const ChainstateManager& chainman, const Consensus::Params& consensus_params,
                                 const chainlock::Chainlocks& chainlocks, const llmq::CQuorumManager& qman) :
        m_cpoolman(cpoolman),
        m_dmnman{dmnman},
        m_mnhfman{mnhfman},
        m_qblockman{qblockman},
        m_qsnapman{qsnapman},
        m_chainman(chainman),
        m_consensus_params{consensus_params},
        m_chainlocks{chainlocks},
        m_qman{qman}
    {
    }

    bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, const CCoinsViewCache& view, bool fJustCheck,
                                  bool fCheckCbTxMerkleRoots, BlockValidationState& state, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, std::optional<MNListUpdates>& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);


    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                               const CCoinsViewCache& view, bool debugLogs, BlockValidationState& state,
                               CDeterministicMNList& mnListRet) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Variant that takes an explicit starting list instead of loading from GetListForBlock
    // Used for rebuilding diffs from trusted snapshots
    bool RebuildListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                              const CDeterministicMNList& prevList, const CCoinsViewCache& view, bool debugLogs,
                              BlockValidationState& state, CDeterministicMNList& mnListRet);

private:
    bool CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CCbTx& cbTx,
                                     BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};


bool CheckProRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                   CDeterministicMNManager& dmnman, const CCoinsViewCache& view, const ChainstateManager& chainman,
                   TxValidationState& state, bool check_sigs);
bool CheckProUpServTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                      const ChainstateManager& chainman, TxValidationState& state, bool check_sigs);
bool CheckProUpRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                     CDeterministicMNManager& dmnman, const CCoinsViewCache& view, const ChainstateManager& chainman,
                     TxValidationState& state, bool check_sigs);
bool CheckProUpRevTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                     const ChainstateManager& chainman, TxValidationState& state, bool check_sigs);

#endif // BITCOIN_EVO_SPECIALTXMAN_H
