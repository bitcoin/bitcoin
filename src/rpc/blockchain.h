// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <amount.h>
#include <context.h>
#include <streams.h>
#include <sync.h>

#include <stdint.h>
#include <vector>

extern RecursiveMutex cs_main;

class CBlock;
class CBlockIndex;
class CBlockPolicyEstimator;
class CChainState;
class CTxMemPool;
class ChainstateManager;
class UniValue;
struct LLMQContext;
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
UniValue blockToJSON(const CBlock& block, const CBlockIndex* tip, const CBlockIndex* blockindex, llmq::CChainLocksHandler& clhandler, llmq::CInstantSendManager& isman, bool txDetails = false) LOCKS_EXCLUDED(cs_main);

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool, llmq::CInstantSendManager& isman);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, llmq::CInstantSendManager* isman, bool verbose = false);

/** Block header to JSON */
UniValue blockheaderToJSON(const CBlockIndex* tip, const CBlockIndex* blockindex, llmq::CChainLocksHandler& clhandler, llmq::CInstantSendManager& isman) LOCKS_EXCLUDED(cs_main);

/** Used by getblockstats to get feerates at different percentiles by weight  */
void CalculatePercentilesBySize(CAmount result[NUM_GETBLOCKSTATS_PERCENTILES], std::vector<std::pair<CAmount, int64_t>>& scores, int64_t total_size);

NodeContext& EnsureAnyNodeContext(const CoreContext& context);
CTxMemPool& EnsureMemPool(const NodeContext& node);
CTxMemPool& EnsureAnyMemPool(const CoreContext& context);
ChainstateManager& EnsureChainman(const NodeContext& node);
ChainstateManager& EnsureAnyChainman(const CoreContext& context);
CBlockPolicyEstimator& EnsureFeeEstimator(const NodeContext& node);
CBlockPolicyEstimator& EnsureAnyFeeEstimator(const CoreContext& context);
LLMQContext& EnsureLLMQContext(const NodeContext& node);
LLMQContext& EnsureAnyLLMQContext(const CoreContext& context);

/**
 * Helper to create UTXO snapshots given a chainstate and a file handle.
 * @return a UniValue map containing metadata about the snapshot.
 */
UniValue CreateUTXOSnapshot(NodeContext& node, CChainState& chainstate, CAutoFile& afile);

#endif
