// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_MINING_H
#define BITCOIN_TEST_UTIL_MINING_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CBlock;
class CBlockIndex;
class CChainParams;
class COutPoint;
class CScript;
namespace Consensus {
struct Params;
} // namespace Consensus
namespace node {
struct BlockCreateOptions;
struct NodeContext;
} // namespace node

/** Create a blockchain, starting from genesis */
std::vector<std::shared_ptr<CBlock>> CreateBlockChain(size_t total_height, const CChainParams& params);

/** Rebuild a block template for a different parent and timestamp */
void RebuildBlockForParent(CBlock& block, const CBlockIndex& parent, uint32_t time, const Consensus::Params& params);

/**
 * Build a chain of `length` coinbase-only blocks on top of `pindex` (which need
 * not be the active tip, allowing forks to be created) and submit their headers.
 * The blocks themselves are returned in `chain` for the caller to process.
 */
bool BuildChain(const node::NodeContext& node, const CBlockIndex* pindex, const CScript& coinbase_script_pub_key, size_t length, std::vector<std::shared_ptr<CBlock>>& chain);

/** Returns the generated coin */
COutPoint MineBlock(const node::NodeContext&,
                    const node::BlockCreateOptions& assembler_options);

/**
 * Returns the generated coin (or Null if the block was invalid).
 * It is recommended to call RegenerateCommitments before mining the block to avoid merkle tree mismatches.
 **/
COutPoint MineBlock(const node::NodeContext&, std::shared_ptr<CBlock>& block);

/**
 * Returns the generated coin (or Null if the block was invalid).
 */
COutPoint ProcessBlock(const node::NodeContext&, const std::shared_ptr<CBlock>& block);

/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const node::NodeContext& node,
                                     const node::BlockCreateOptions& assembler_options);

/** RPC-like helper function, returns the generated coin */
COutPoint generatetoaddress(const node::NodeContext&, const std::string& address);

#endif // BITCOIN_TEST_UTIL_MINING_H
