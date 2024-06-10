// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <uint256.h>

namespace node {
struct CBlockTemplate;
struct NodeContext;
} // namespace node

class BlockValidationState;
class CBlock;
class CScript;

namespace interfaces {

//! Interface giving clients (RPC, Stratum v2 Template Provider in the future)
//! ability to create block templates.

class Mining
{
public:
    virtual ~Mining() {}

    //! If this chain is exclusively used for testing
    virtual bool isTestChain() = 0;

    //! Returns the hash for the tip of this chain, 0 if none
    virtual uint256 getTipHash() = 0;

   /**
     * Construct a new block template
     *
     * @param[in] script_pub_key the coinbase output
     * @param[in] use_mempool set false to omit mempool transactions
     * @returns a block template
     */
    virtual std::unique_ptr<node::CBlockTemplate> createNewBlock(const CScript& script_pub_key, bool use_mempool = true) = 0;

    /**
     * Check a block is completely valid from start to finish.
     * Only works on top of our current best block.
     * Does not check proof-of-work.
     *
     * @param[out] state details of why a block failed to validate
     * @param[in] block the block to validate
     * @param[in] check_merkle_root call CheckMerkleRoot()
     * @returns false if any of the checks fail
     */
    virtual bool testBlockValidity(BlockValidationState& state, const CBlock& block, bool check_merkle_root = true) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
std::unique_ptr<Mining> MakeMining(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
