// Copyright (c) 2017-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <consensus/amount.h>
#include <core_io.h>
#include <streams.h>
#include <sync.h>
#include <validation.h>

#include <stdint.h>
#include <vector>

extern RecursiveMutex cs_main;

class CBlock;
class CBlockIndex;
class CChainState;
class UniValue;
struct NodeContext;
namespace llmq {
class CChainLocksHandler;
class CInstantSendManager;
} // namespace llmq

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
UniValue blockToJSON(BlockManager& blockman, const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, const llmq::CChainLocksHandler& clhandler, const llmq::CInstantSendManager& isman, bool txDetails = false) LOCKS_EXCLUDED(cs_main);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex, const llmq::CChainLocksHandler& clhandler) LOCKS_EXCLUDED(cs_main);

/** Used by getblockstats to get feerates at different percentiles by weight  */
void CalculatePercentilesBySize(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_size);

void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_hex = true, const CTxUndo* txundo = nullptr, const CSpentIndexTxInfo* ptxSpentInfo = nullptr);

/**
 * Helper to create UTXO snapshots given a chainstate and a file handle.
 * @return a UniValue map containing metadata about the snapshot.
 */
UniValue CreateUTXOSnapshot(NodeContext& node, CChainState& chainstate, CAutoFile& afile);

#endif // BITCOIN_RPC_BLOCKCHAIN_H
