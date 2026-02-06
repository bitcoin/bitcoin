// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATE_H
#define BITCOIN_NODE_CHAINSTATE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class CChainstateHelper;
class CDeterministicMNManager;
class CEvoDB;
class CGovernanceManager;
class ChainstateManager;
class CMasternodeMetaMan;
class CMasternodeSync;
class CSporkManager;
class CTxMemPool;
struct LLMQContext;

namespace chainlock { class Chainlocks; }
namespace Consensus {
struct Params;
} // namespace Consensus
namespace fs {
class path;
} // namespace fs

namespace node {
enum class ChainstateLoadingError {
    ERROR_LOADING_BLOCK_DB,
    ERROR_BAD_GENESIS_BLOCK,
    ERROR_BAD_DEVNET_GENESIS_BLOCK,
    ERROR_ADDRIDX_NEEDS_REINDEX,
    ERROR_SPENTIDX_NEEDS_REINDEX,
    ERROR_TIMEIDX_NEEDS_REINDEX,
    ERROR_PRUNED_NEEDS_REINDEX,
    ERROR_LOAD_GENESIS_BLOCK_FAILED,
    ERROR_CHAINSTATE_UPGRADE_FAILED,
    ERROR_REPLAYBLOCKS_FAILED,
    ERROR_LOADCHAINTIP_FAILED,
    ERROR_GENERIC_BLOCKDB_OPEN_FAILED,
    ERROR_COMMITING_EVO_DB,
    ERROR_UPGRADING_EVO_DB,
    ERROR_UPGRADING_SIGNALS_DB,
    SHUTDOWN_PROBED,
};

/** This sequence can have 4 types of outcomes:
 *
 *  1. Success
 *  2. Shutdown requested
 *    - nothing failed but a shutdown was triggered in the middle of the
 *      sequence
 *  3. Soft failure
 *    - a failure that might be recovered from with a reindex
 *  4. Hard failure
 *    - a failure that definitively cannot be recovered from with a reindex
 *
 *  Currently, LoadChainstate returns a std::optional<ChainstateLoadingError>
 *  which:
 *
 *  - if has_value()
 *      - Either "Soft failure", "Hard failure", or "Shutdown requested",
 *        differentiable by the specific enumerator.
 *
 *        Note that a return value of SHUTDOWN_PROBED means ONLY that "during
 *        this sequence, when we explicitly checked shutdown_requested() at
 *        arbitrary points, one of those calls returned true". Therefore, a
 *        return value other than SHUTDOWN_PROBED does not guarantee that
 *        shutdown hasn't been called indirectly.
 *  - else
 *      - Success!
 */
std::optional<ChainstateLoadingError> LoadChainstate(bool fReset,
                                                     ChainstateManager& chainman,
                                                     CGovernanceManager& govman,
                                                     CMasternodeMetaMan& mn_metaman,
                                                     CMasternodeSync& mn_sync,
                                                     CSporkManager& sporkman,
                                                     chainlock::Chainlocks& chainlocks,
                                                     std::unique_ptr<CChainstateHelper>& chain_helper,
                                                     std::unique_ptr<CDeterministicMNManager>& dmnman,
                                                     std::unique_ptr<CEvoDB>& evodb,
                                                     std::unique_ptr<LLMQContext>& llmq_ctx,
                                                     CTxMemPool* mempool,
                                                     const fs::path& data_dir,
                                                     bool fPruneMode,
                                                     bool is_addrindex_enabled,
                                                     bool is_spentindex_enabled,
                                                     bool is_timeindex_enabled,
                                                     const Consensus::Params& consensus_params,
                                                     bool fReindexChainState,
                                                     int64_t nBlockTreeDBCache,
                                                     int64_t nCoinDBCache,
                                                     int64_t nCoinCacheUsage,
                                                     bool block_tree_db_in_memory,
                                                     bool coins_db_in_memory,
                                                     bool dash_dbs_in_memory,
                                                     int8_t bls_threads,
                                                     int64_t max_recsigs_age,
                                                     std::function<bool()> shutdown_requested = nullptr,
                                                     std::function<void()> coins_error_cb = nullptr);

/** Initialize Dash-specific components during chainstate initialization */
void DashChainstateSetup(ChainstateManager& chainman,
                         CGovernanceManager& govman,
                         CMasternodeMetaMan& mn_metaman,
                         CMasternodeSync& mn_sync,
                         CSporkManager& sporkman,
                         chainlock::Chainlocks& chainlocks,
                         std::unique_ptr<CChainstateHelper>& chain_helper,
                         std::unique_ptr<CDeterministicMNManager>& dmnman,
                         CEvoDB& evodb,
                         std::unique_ptr<LLMQContext>& llmq_ctx,
                         CTxMemPool* mempool,
                         const fs::path& data_dir,
                         bool llmq_dbs_in_memory,
                         bool llmq_dbs_wipe,
                         int8_t bls_threads,
                         int64_t max_recsigs_age,
                         const Consensus::Params& consensus_params);

void DashChainstateSetupClose(std::unique_ptr<CChainstateHelper>& chain_helper,
                              std::unique_ptr<CDeterministicMNManager>& dmnman,
                              std::unique_ptr<LLMQContext>& llmq_ctx,
                              CTxMemPool* mempool);

enum class ChainstateLoadVerifyError {
    ERROR_BLOCK_FROM_FUTURE,
    ERROR_CORRUPTED_BLOCK_DB,
    ERROR_EVO_DB_SANITY_FAILED,
    ERROR_GENERIC_FAILURE,
};

std::optional<ChainstateLoadVerifyError> VerifyLoadedChainstate(ChainstateManager& chainman,
                                                                CEvoDB& evodb,
                                                                bool fReset,
                                                                bool fReindexChainState,
                                                                const Consensus::Params& consensus_params,
                                                                int check_blocks,
                                                                int check_level,
                                                                std::function<int64_t()> get_unix_time_seconds,
                                                                std::function<void(bool)> notify_bls_state = nullptr);
} // namespace node

#endif // BITCOIN_NODE_CHAINSTATE_H
