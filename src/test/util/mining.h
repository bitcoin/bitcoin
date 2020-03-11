// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_MINING_H
#define BITCOIN_TEST_UTIL_MINING_H

#include <memory>
#include <string>

class CBlock;
class COutPoint;
class CScript;
struct NodeContext;

void ReGenerateCommitments(CBlock& block);

/** Returns the generated coin */
COutPoint MineBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/**
 * Returns the generated coin (or Null if the block was invalid).
 * It is recommended to call ReGenerateCommitments before mining the block to avoid merkle tree mismatches.
 **/
COutPoint MineBlock(std::shared_ptr<CBlock>& block);

/** Prepare a block to be mined */
std::shared_ptr<CBlock> PrepareBlock(const NodeContext&, const CScript& coinbase_scriptPubKey);

/** RPC-like helper function, returns the generated coin */
COutPoint generatetoaddress(const NodeContext&, const std::string& address);

#endif // BITCOIN_TEST_UTIL_MINING_H
