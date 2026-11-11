// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTXMAN_H
#define BITCOIN_EVO_SPECIALTXMAN_H

#include <consensus/amount.h>
#include <evo/deterministicmns.h>
#include <gsl/pointers.h>
#include <kernel/cs_main.h> // IWYU pragma: export
#include <sync.h>
#include <threadsafety.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CCbTx;
class CChain;
class CCoinsViewCache;
class CCreditPoolManager;
class CDeterministicMNManager;
class CProDisTx;
class CRangesSet;
class CTransaction;
class ChainstateManager;
class Chainstate;
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
namespace node {
class BlockManager;
} // namespace node

/** Activation state of the deployments that special transaction validation depends on, evaluated by the
 *  caller for the block after pindexPrev (see GetSpecialTxRules() in validation.h) */
struct SpecialTxRules {
    bool v24{false};
    //! EvoNodes may register with shared collateral and have multiple owner payouts
    bool evo_shares{false};
    //! A masternode's primary addresses need not be IPv4
    bool ipv4_optional{false};
};

class CSpecialTxProcessor
{
private:
    CCreditPoolManager& m_cpoolman;
    CDeterministicMNManager& m_dmnman;
    CMNHFManager& m_mnhfman;
    llmq::CQuorumBlockProcessor& m_qblockman;
    llmq::CQuorumSnapshotManager& m_qsnapman;
    const ChainstateManager& m_chainman;
    const node::BlockManager& m_blockman;
    const Consensus::Params& m_consensus_params;
    const chainlock::Chainlocks& m_chainlocks;
    const llmq::CQuorumManager& m_qman;

public:
    explicit CSpecialTxProcessor(CCreditPoolManager& cpoolman, CDeterministicMNManager& dmnman, CMNHFManager& mnhfman,
                                 llmq::CQuorumBlockProcessor& qblockman, llmq::CQuorumSnapshotManager& qsnapman,
                                 const ChainstateManager& chainman, const node::BlockManager& blockman,
                                 const Consensus::Params& consensus_params, const chainlock::Chainlocks& chainlocks,
                                 const llmq::CQuorumManager& qman) :
        m_cpoolman(cpoolman),
        m_dmnman{dmnman},
        m_mnhfman{mnhfman},
        m_qblockman{qblockman},
        m_qsnapman{qsnapman},
        m_chainman(chainman),
        m_blockman{blockman},
        m_consensus_params{consensus_params},
        m_chainlocks{chainlocks},
        m_qman{qman}
    {
    }

    bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, SpecialTxRules rules,
                        const CCoinsViewCache& view, bool check_sigs, TxValidationState& state)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool ProcessSpecialTxsInBlock(Chainstate& chainstate, const CChain& chain, const CBlock& block,
                                  const CBlockIndex* pindex, SpecialTxRules rules, const CCoinsViewCache& view,
                                  CAmount blockSubsidy, bool fJustCheck, bool fCheckCbTxMerkleRoots,
                                  BlockValidationState& state, MNListUpdates& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool UndoSpecialTxsInBlock(const Chainstate& chainstate, const CBlock& block, const CBlockIndex* pindex, MNListUpdates& updatesRet)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    void RemoveMineableCommitmentsForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindex)
        EXCLUSIVE_LOCKS_REQUIRED(::cs_main);


    // the returned list will not contain the correct block hash (we can't know it yet as the coinbase TX is not updated yet)
    bool BuildNewListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev, bool is_v24_active,
                               const CCoinsViewCache& view, bool debugLogs, BlockValidationState& state,
                               CDeterministicMNList& mnListRet) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    // Variant that takes an explicit starting list instead of loading from GetListForBlock
    // Used for rebuilding diffs from trusted snapshots
    bool RebuildListFromBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev, bool is_v24_active,
                              const CDeterministicMNList& prevList, const CCoinsViewCache& view, bool debugLogs,
                              BlockValidationState& state, CDeterministicMNList& mnListRet);

    /** Passthrough to CDeterministicMNManager::UpdatedBlockTip */
    void UpdatedMNListTip(gsl::not_null<const CBlockIndex*> pindex);

private:
    friend class CMNListBuilder;

    // The three phases of RebuildListFromBlock(): per-block updates, one provider transaction, and
    // collateral spends plus payee bookkeeping once every transaction has been applied.
    CDeterministicMNList BeginListForBlock(gsl::not_null<const CBlockIndex*> pindexPrev,
                                           const CDeterministicMNList& prevList) const;
    bool ApplyTxToList(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, bool is_v24_active,
                       const CCoinsViewCache& view, bool debugLogs, CDeterministicMNList& newList,
                       BlockValidationState& state) const;
    bool FinishListForBlock(const CBlock& block, gsl::not_null<const CBlockIndex*> pindexPrev,
                            const CDeterministicMNList& prevList, bool debugLogs, CDeterministicMNList newList,
                            BlockValidationState& state, CDeterministicMNList& mnListRet) const;

    bool CheckSpecialTxInner(const CChain* chain, const CTransaction& tx, const CBlockIndex* pindexPrev,
                             SpecialTxRules rules, const CCoinsViewCache& view, const std::optional<CRangesSet>& indexes,
                             bool check_sigs, TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
    bool CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const CCbTx& cbTx,
                                     CAmount blockSubsidy, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);
};

/**
 * Builds the masternode list for a block one transaction at a time, with the same result as
 * CSpecialTxProcessor::RebuildListFromBlock() over the same transactions. Block assembly uses it to
 * try a package on a copy and keep the copy only if every transaction in it applies, so conflicting
 * transactions are left out instead of failing the whole template. Copying is cheap: the list is
 * backed by persistent maps.
 */
class CMNListBuilder
{
public:
    CMNListBuilder(const CSpecialTxProcessor& processor, const CDeterministicMNList& prevList,
                   gsl::not_null<const CBlockIndex*> pindexPrev, bool is_v24_active, const CCoinsViewCache& view,
                   bool debugLogs);
    //! Starts from the list at pindexPrev, as BuildNewListFromBlock() does.
    CMNListBuilder(const CSpecialTxProcessor& processor, gsl::not_null<const CBlockIndex*> pindexPrev,
                   bool is_v24_active, const CCoinsViewCache& view, bool debugLogs) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

    //! Same result, and same exceptions, as RebuildListFromBlock() processing this transaction next.
    [[nodiscard]] bool ApplyTx(const CTransaction& tx, BlockValidationState& state);
    //! Collateral spends and payee bookkeeping for the whole block. Does not modify the builder.
    [[nodiscard]] bool Finish(const CBlock& block, BlockValidationState& state, CDeterministicMNList& mnListRet) const;

private:
    const CSpecialTxProcessor* m_processor;
    CDeterministicMNList m_prev_list;
    gsl::not_null<const CBlockIndex*> m_pindex_prev;
    bool m_is_v24_active;
    const CCoinsViewCache* m_view;
    bool m_debug_logs;
    CDeterministicMNList m_list;
};

/**
 * This helper does some trivial validations that doesn't depends on collateral and
 * masternode list. Use CheckPro*Tx for the full validation of transaction,
 * including bls signatures, list of masternodes and collateral
 */
template <typename ProTx>
std::optional<ProTx> GetValidatedPayload(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                                         const Consensus::Params& consensus_params, bool is_v24_active,
                                         TxValidationState& state);

/** Validates the bestCLSignature / bestCLHeightDiff fields embedded in a CbTx payload. */
bool CheckCbTxBestChainlock(const CCbTx& cbTx, const CBlockIndex* pindex, const Consensus::Params& consensus_params,
                            const CChain& chain, const llmq::CQuorumManager& qman,
                            const chainlock::Chainlocks& chainlocks, BlockValidationState& state);

bool CheckProRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                   const CCoinsViewCache& view, const Consensus::Params& consensus_params, SpecialTxRules rules,
                   TxValidationState& state, bool check_sigs);
bool CheckProUpServTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                      CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, SpecialTxRules rules,
                      TxValidationState& state, bool check_sigs);
bool CheckProUpRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                     const CCoinsViewCache& view, const Consensus::Params& consensus_params, SpecialTxRules rules,
                     TxValidationState& state, bool check_sigs);
bool CheckProUpRevTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                     CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, bool is_v24_active,
                     TxValidationState& state, bool check_sigs);
bool CheckProDisTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev, CDeterministicMNManager& dmnman,
                   const Consensus::Params& consensus_params, bool is_v24_active, TxValidationState& state,
                   bool check_sigs);
bool CheckProUpShareTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                       CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params, bool is_v24_active,
                       TxValidationState& state, bool check_sigs);
bool CheckProUpSharedRegTx(const CTransaction& tx, gsl::not_null<const CBlockIndex*> pindexPrev,
                           CDeterministicMNManager& dmnman, const Consensus::Params& consensus_params,
                           bool is_v24_active, TxValidationState& state, bool check_sigs);
/** Full ProDisTx validation against a given masternode list and spend height. The masternode must
 *  already be in the list at the previous block: registering and dissolving a shared masternode in
 *  the same block is deliberately invalid, which lets block validation reuse the mempool path. */
bool CheckProDisTxForList(const CTransaction& tx, const CProDisTx& ptx, const CDeterministicMNList& mnList,
                          int nSpendHeight, TxValidationState& state, bool check_sigs);

/** Consensus rule (v24): an input whose prevout pays the shared-collateral template script may
 *  only be spent by a ProDisTx. Applies to every transaction; callers gate on v24 activation. */
bool CheckSharedCollateralSpends(const CTransaction& tx, const CCoinsViewCache& view, TxValidationState& state);
/** Consensus rule (v24): an output paying the shared-collateral template script is only valid as
 *  the collateral output of a shared registration. Applies to every transaction, including the
 *  coinbase; callers gate on v24 activation. */
bool CheckSharedCollateralTemplateOutputs(const CTransaction& tx, TxValidationState& state);


/**
 * Asset lock transactions with more than 100 inputs (and so over ~20 kB) can not
 * be processed by Platform, so Dash Core nodes should not relay them: they are
 * marked non-standard, which keeps the network from propagating them over p2p.
 *
 * Asset lock v2 is enabled by the v24 fork, but Platform can not process it yet.
 * It is kept non-standard so it can be enabled later without another hard fork.
 *
 * These are relay/mempool checks only: a rejected transaction stays valid inside
 * a block.
 *
 * Returns false (with `reason` set) for a non-standard asset lock, and
 * true for any transaction that is not an asset lock or not special-tx.
 */
bool IsStandardSpecialTx(const CTransaction& tx, std::string& reason);
#endif // BITCOIN_EVO_SPECIALTXMAN_H
