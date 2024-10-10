// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <consensus/amount.h>       // for CAmount
#include <interfaces/types.h>       // for BlockRef
#include <node/types.h>             // for BlockCreateOptions
#include <primitives/block.h>       // for CBlock, CBlockHeader
#include <primitives/transaction.h> // for CTransactionRef
#include <stdint.h>                 // for int64_t
#include <uint256.h>                // for uint256
#include <util/time.h>              // for MillisecondsDouble

#include <memory>   // for unique_ptr, shared_ptr
#include <optional> // for optional
#include <vector>   // for vector

namespace node {
struct NodeContext;
} // namespace node

class BlockValidationState;
class CScript;

namespace interfaces {

//! Block template interface
class BlockTemplate
{
public:
    virtual ~BlockTemplate() = default;

    virtual CBlockHeader getBlockHeader() = 0;
    virtual CBlock getBlock() = 0;

    virtual std::vector<CAmount> getTxFees() = 0;
    virtual std::vector<int64_t> getTxSigops() = 0;

    virtual CTransactionRef getCoinbaseTx() = 0;
    virtual std::vector<unsigned char> getCoinbaseCommitment() = 0;
    virtual int getWitnessCommitmentIndex() = 0;

    /**
     * Compute merkle path to the coinbase transaction
     *
     * @return merkle path ordered from the deepest
     */
    virtual std::vector<uint256> getCoinbaseMerklePath() = 0;

    /**
     * Construct and broadcast the block.
     *
     * @returns if the block was processed, independent of block validity
     */
    virtual bool submitSolution(uint32_t version, uint32_t timestamp, uint32_t nonce, CMutableTransaction coinbase) = 0;
};

//! Interface giving clients (RPC, Stratum v2 Template Provider in the future)
//! ability to create block templates.
class Mining
{
public:
    virtual ~Mining() = default;

    //! If this chain is exclusively used for testing
    virtual bool isTestChain() = 0;

    //! Returns whether IBD is still in progress.
    virtual bool isInitialBlockDownload() = 0;

    //! Returns the hash and height for the tip of this chain
    virtual std::optional<BlockRef> getTip() = 0;

    /**
     * Waits for the connected tip to change. If the tip was not connected on
     * startup, this will wait.
     *
     * @param[in] current_tip block hash of the current chain tip. Function waits
     *                        for the chain tip to differ from this.
     * @param[in] timeout     how long to wait for a new tip
     * @returns               Hash and height of the current chain tip after this call.
     */
    virtual BlockRef waitTipChanged(uint256 current_tip, MillisecondsDouble timeout = MillisecondsDouble::max()) = 0;

   /**
     * Construct a new block template
     *
     * @param[in] script_pub_key the coinbase output
     * @param[in] options options for creating the block
     * @returns a block template
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const CScript& script_pub_key, const node::BlockCreateOptions& options = {}) = 0;

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
