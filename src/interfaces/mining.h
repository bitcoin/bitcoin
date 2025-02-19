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
    virtual bool submitSolution(uint32_t version, uint32_t timestamp, uint32_t nonce, CTransactionRef coinbase) = 0;
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
     * Waits for the connected tip to change. During node initialization, this will
     * wait until the tip is connected (regardless of `timeout`).
     *
     * @param[in] current_tip block hash of the current chain tip. Function waits
     *                        for the chain tip to differ from this.
     * @param[in] timeout     how long to wait for a new tip
     * @retval BlockRef hash and height of the current chain tip after this call.
     * @retval std::nullopt if the node is shut down.
     */
    virtual std::optional<BlockRef> waitTipChanged(uint256 current_tip, MillisecondsDouble timeout = MillisecondsDouble::max()) = 0;

   /**
     * Construct a new block template
     *
     * @param[in] options options for creating the block
     * @returns a block template
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const node::BlockCreateOptions& options = {}) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
std::unique_ptr<Mining> MakeMining(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
