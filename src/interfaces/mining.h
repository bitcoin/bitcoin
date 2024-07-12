// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <memory>
#include <optional>
#include <uint256.h>
#include <util/mining.h>

namespace node {
struct CBlockTemplate;
struct NodeContext;
} // namespace node

class BlockValidationState;
class CBlock;
class CScript;

namespace interfaces {

//! Block template interface
class BlockTemplate
{
public:
    virtual ~BlockTemplate() = default;

    virtual CBlock getBlock() = 0;

    virtual std::vector<CAmount> getTxFees() = 0;
    virtual std::vector<int64_t> getTxSigops() = 0;

    virtual std::vector<unsigned char> getCoinbaseCommitment() = 0;
};

//! Interface giving clients (RPC, Stratum v2 Template Provider in the future)
//! ability to create block templates.
class Mining
{
public:
    virtual ~Mining() {}

    //! If this chain is exclusively used for testing
    virtual bool isTestChain() = 0;

    //! Returns whether IBD is still in progress.
    virtual bool isInitialBlockDownload() = 0;

    //! Returns the hash for the tip of this chain
    virtual std::optional<uint256> getTipHash() = 0;

   /**
     * Construct a new block template
     *
     * @param[in] script_pub_key the coinbase output
     * @param[in] use_mempool set false to omit mempool transactions
     * @param[in] coinbase_max_additional_weight maximum additional weight which the pool will add to the coinbase transaction
     * @param[in] coinbase_output_max_additional_sigops maximum additional sigops which the pool will add in coinbase transaction outputs
     * @returns a block template interface
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const CScript& script_pub_key, bool use_mempool = true,
                                                                 size_t coinbase_max_additional_weight = DEFAULT_COINBASE_MAX_ADDITIONAL_WEIGHT,
                                                                 size_t coinbase_output_max_additional_sigops = DEFAULT_COINBASE_OUTPUT_MAX_ADDITIONAL_SIGOPS) = 0;

    /**
     * Processes new block. A valid new block is automatically relayed to peers.
     *
     * @param[in]   block The block we want to process.
     * @param[out]  new_block A boolean which is set to indicate if the block was first received via this call
     * @returns     If the block was processed, independently of block validity
     */
    virtual bool processNewBlock(const std::shared_ptr<const CBlock>& block, bool* new_block) = 0;

    //! Return the number of transaction updates in the mempool,
    //! used to decide whether to make a new block template.
    virtual unsigned int getTransactionsUpdated() = 0;

    /**
     * Check a block is completely valid from start to finish.
     * Only works on top of our current best block.
     * Does not check proof-of-work.
     *
     * @param[in] block the block to validate
     * @param[in] check_merkle_root call CheckMerkleRoot()
     * @param[out] state details of why a block failed to validate
     * @returns false if it does not build on the current tip, or any of the checks fail
     */
    virtual bool testBlockValidity(const CBlock& block, bool check_merkle_root, BlockValidationState& state) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
std::unique_ptr<Mining> MakeMining(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
