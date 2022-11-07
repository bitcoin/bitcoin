// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_BLOCKCHAIN_H
#define BITCOIN_RPC_BLOCKCHAIN_H

#include <amount.h>
#include <context.h>
#include <sync.h>

#include <stdint.h>
#include <vector>

extern RecursiveMutex cs_main;

class CBlock;
class CBlockIndex;
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
void RPCNotifyBlockChange(bool ibd, const CBlockIndex *);

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

NodeContext& EnsureNodeContext(const CoreContext& context);
LLMQContext& EnsureLLMQContext(const CoreContext& context);
CTxMemPool& EnsureMemPool(const CoreContext& context);
ChainstateManager& EnsureChainman(const CoreContext& context);

#endif
