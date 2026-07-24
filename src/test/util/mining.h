// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_MINING_H
#define BITCOIN_TEST_UTIL_MINING_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class CBlock;
class CBlockHeader;
class CChainParams;
class COutPoint;
namespace Consensus {
struct Params;
} // namespace Consensus
namespace node {
struct BlockCreateOptions;
struct NodeContext;
} // namespace node

/** Increment the nonce until the block header satisfies proof of work. */
void GrindBlock(CBlockHeader& block, const Consensus::Params& consensus);

/** Create a blockchain, starting from genesis */
std::vector<std::shared_ptr<CBlock>> CreateBlockChain(size_t total_height, const CChainParams& params);

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
