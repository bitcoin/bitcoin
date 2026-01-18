// Copyright (c) 2017-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <consensus/amount.h>
#include <core_io.h>
#include <fs.h>
#include <kernel/coinstats.h>
#include <streams.h>
#include <sync.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

extern RecursiveMutex cs_main; // NOLINT(readability-redundant-declaration)

class CBlock;
class CBlockIndex;
class CChainState;
class CCoinsView;
namespace chainlock { class Chainlocks; }
namespace kernel {
enum class CoinStatsHashType : uint8_t;
}
namespace llmq {
class CInstantSendManager;
} // namespace llmq
namespace node {
class BlockManager;
struct NodeContext;
} // namespace node

class UniValue;

static constexpr int NUM_GETBLOCKSTATS_PERCENTILES = 5;

/**
 * Get the difficulty of the net wrt to the given block index.
 *
 * @return A floating point number that is a multiple of the main net minimum
 * difficulty (4295032833 hashes).
 */
double GetDifficulty(const CBlockIndex* blockindex);

/** Callback for when block tip changed. */
void RPCNotifyBlockChange(const CBlockIndex*);

/** Block description to JSON */
UniValue blockToJSON(node::BlockManager& blockman, const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, const chainlock::Chainlocks& chainlocks, const llmq::CInstantSendManager& isman, TxVerbosity verbosity) LOCKS_EXCLUDED(cs_main);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex, const chainlock::Chainlocks& chainlocks) LOCKS_EXCLUDED(cs_main);

/** Used by getblockstats to get feerates at different percentiles by weight  */
void CalculatePercentilesBySize(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_size);

/**
 * Helper to create UTXO snapshots given a chainstate and a file handle.
 * @return a UniValue map containing metadata about the snapshot.
 */
UniValue CreateUTXOSnapshot(
    node::NodeContext& node,
    CChainState& chainstate,
    AutoFile& afile,
    const fs::path& path,
    const fs::path& tmppath);

/**
 * Calculate statistics about the unspent transaction output set
 *
 * @param[in] index_requested Signals if the coinstatsindex should be used (when available).
 */
std::optional<kernel::CCoinsStats> GetUTXOStats(CCoinsView* view, node::BlockManager& blockman,
                                                kernel::CoinStatsHashType hash_type,
                                                const std::function<void()>& interruption_point = {},
                                                const CBlockIndex* pindex = nullptr,
                                                bool index_requested = true);

#endif // BITCOIN_RPC_BLOCKCHAIN_H
