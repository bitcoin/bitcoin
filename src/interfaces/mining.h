// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <consensus/amount.h>
#include <interfaces/types.h>
#include <node/types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

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
    // Block contains a dummy coinbase transaction that should not be used.
    virtual CBlock getBlock() = 0;

    // Fees per transaction, not including coinbase transaction.
    virtual std::vector<CAmount> getTxFees() = 0;
    // Sigop cost per transaction, not including coinbase transaction.
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
     * Construct and broadcast the block. Modifies the template in place,
     * updating the fields listed below as well as the merkle root.
     *
     * @param[in] version version block header field
     * @param[in] timestamp time block header field (unix timestamp)
     * @param[in] nonce nonce block header field
     * @param[in] coinbase complete coinbase transaction (including witness)
     *
     * @note unlike the submitblock RPC, this method does NOT add the
     *       coinbase witness automatically.
     *
     * @returns if the block was processed, does not necessarily indicate validity.
     *
     * @note Returns true if the block is already known, which can happen if
     *       the solved block is constructed and broadcast by multiple nodes
     *       (e.g. both the miner who constructed the template and the pool).
     */
    virtual bool submitSolution(uint32_t version, uint32_t timestamp, uint32_t nonce, CTransactionRef coinbase) = 0;

    /**
     * Waits for fees in the next block to rise, a new tip or the timeout.
     *
     * @param[in] options   Control the timeout (default forever) and by how much total fees
     *                      for the next block should rise (default infinite).
     *
     * @returns a new BlockTemplate or nothing if the timeout occurs.
     *
     * On testnet this will additionally return a template with difficulty 1 if
     * the tip is more than 20 minutes old.
     */
    virtual std::unique_ptr<BlockTemplate> waitNext(const node::BlockWaitOptions options = {}) = 0;

    /**
     * Interrupts the current wait for the next block template.
    */
    virtual void interruptWait() = 0;
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
     * @param[in] timeout     how long to wait for a new tip (default is forever)
     *
     * @retval BlockRef hash and height of the current chain tip after this call.
     * @retval std::nullopt if the node is shut down.
     */
    virtual std::optional<BlockRef> waitTipChanged(uint256 current_tip, MillisecondsDouble timeout = MillisecondsDouble::max()) = 0;

   /**
     * Construct a new block template.
     *
     * During node initialization, this will wait until the tip is connected.
     *
     * @param[in] options options for creating the block
     * @retval BlockTemplate a block template.
     * @retval std::nullptr if the node is shut down.
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const node::BlockCreateOptions& options = {}) = 0;

    /**
     * Checks if a given block is valid.
     *
     * @param[in] block       the block to check
     * @param[in] options     verification options: the proof-of-work check can be
     *                        skipped in order to verify a template generated by
     *                        external software.
     * @param[out] reason     failure reason (BIP22)
     * @param[out] debug      more detailed rejection reason
     * @returns               whether the block is valid
     *
     * For signets the challenge verification is skipped when check_pow is false.
     */
    virtual bool checkBlock(const CBlock& block, const node::BlockCheckOptions& options, std::string& reason, std::string& debug) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
std::unique_ptr<Mining> MakeMining(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
