// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <coins.h>
#include <consensus/amount.h>
#include <core_io.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <util/fs.h>
#include <validation.h>

#include <any>
#include <stdint.h>
#include <vector>

class CBlock;
class CBlockIndex;
class Chainstate;
class UniValue;
namespace node {
class BlockManager;
struct NodeContext;
} // namespace node

static constexpr int NUM_GETBLOCKSTATS_PERCENTILES = 5;
using coinascii_cb_t = std::function<std::string(const COutPoint&, const Coin&)>;

/**
 * Get the difficulty of the net wrt to the given block index.
 *
 * @return A floating point number that is a multiple of the main net minimum
 * difficulty (4295032833 hashes).
 */
double GetDifficulty(const CBlockIndex& blockindex);

/** Callback for when block tip changed. */
void RPCNotifyBlockChange(const CBlockIndex*);

/** Block description to JSON */
UniValue blockToJSON(node::BlockManager& blockman, const CBlock& block, const CBlockIndex& tip, const CBlockIndex& blockindex, TxVerbosity verbosity) LOCKS_EXCLUDED(cs_main);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex& tip, const CBlockIndex& blockindex) LOCKS_EXCLUDED(cs_main);

/** Used by getblockstats to get feerates at different percentiles by weight  */
void CalculatePercentilesByWeight(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_weight);

/**
 * Helper to create UTXO snapshots given a chainstate and a file handle.
 * @return a UniValue map containing metadata about the snapshot.
 */
UniValue CreateUTXOSnapshot(
    const bool is_human_readable,
    const bool show_header,
    const Span<const std::byte>& separator,
    const std::vector<std::pair<std::string, coinascii_cb_t>>& requested,
    node::NodeContext& node,
    Chainstate& chainstate,
    AutoFile& afile,
    const fs::path& path,
    const fs::path& tmppath);

//! Return height of highest block that has been pruned, or std::nullopt if no blocks have been pruned
std::optional<int> GetPruneHeight(const node::BlockManager& blockman, const CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(::cs_main);

#endif // BITCOIN_RPC_BLOCKCHAIN_H
