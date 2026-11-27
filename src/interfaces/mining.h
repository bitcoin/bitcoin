// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <consensus/amount.h>
#include <interfaces/types.h>
#include <node/mining_types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {

//! Handle to a block.
//!
//! The handle keeps the block and its transactions in memory until it is
//! destroyed, also after the transactions leave the mempool and after any
//! template it came from is destroyed. Clients should destroy handles they
//! no longer need.
class Block
{
public:
    virtual ~Block() = default;

    //! Return the block header.
    virtual CBlockHeader header() = 0;

    //! Return the full block.
    virtual CBlock data() = 0;

    /**
     * Compute merkle path to the coinbase transaction
     *
     * @return merkle path ordered from the deepest
     */
    virtual std::vector<uint256> coinbaseMerklePath() = 0;
};

//! Block template interface
class BlockTemplate
{
public:
    virtual ~BlockTemplate() = default;

    /**
     * Return the template block. It contains a dummy coinbase transaction
     * that should not be used and may not match a transaction constructed from
     * getCoinbaseTx(). The template is unchanged by submitSolution().
     *
     * The handle remains valid after the template is destroyed.
     */
    virtual std::unique_ptr<Block> block() = 0;

    //! Deprecated: no longer supported, returns an error.
    virtual CBlockHeader getBlockHeaderOld1() { throw std::runtime_error("Old getBlockHeader (@1) not supported. Please update your client!"); }

    //! Deprecated: no longer supported, returns an error.
    virtual CBlock getBlockOld2() { throw std::runtime_error("Old getBlock (@2) not supported. Please update your client!"); }

    // Fees per transaction, not including coinbase transaction.
    virtual std::vector<CAmount> getTxFees() = 0;
    // Sigop cost per transaction, not including coinbase transaction.
    virtual std::vector<int64_t> getTxSigops() = 0;

    /** Return fields needed to construct a coinbase transaction */
    virtual node::CoinbaseTx getCoinbaseTx() = 0;

    //! Deprecated: no longer supported, returns an error.
    virtual std::vector<uint256> getCoinbaseMerklePathOld6() { throw std::runtime_error("Old getCoinbaseMerklePath (@6) not supported. Please update your client!"); }

    /**
     * Construct and broadcast a block from a copy of the template,
     * updating the fields listed below as well as the merkle root.
     * The template itself is unchanged.
     *
     * @param[in] version version block header field
     * @param[in] timestamp time block header field (unix timestamp)
     * @param[in] nonce nonce block header field
     * @param[in] coinbase complete coinbase transaction (including witness)
     * @param[out] accepted true if accepted as a new block
     * @param[out] reason failure reason (BIP22)
     * @param[out] debug  more detailed rejection reason
     *
     * @note Unlike the submitblock RPC, this method does not call
     *       UpdateUncommittedBlockStructures to add a missing coinbase witness
     *       reserved value. Callers must provide a complete coinbase transaction,
     *       including the witness when a witness commitment is present.
     *
     * @note for heights <= 16, the BIP34 height push in getCoinbaseTx().script_sig_prefix
     *       is only one byte long, so the coinbase scriptSig needs at least
     *       one additional byte of data to avoid bad-cb-length.
     *
     * @returns a handle to the submitted block, including on rejection
     * @throws std::invalid_argument if coinbase is null
     *
     * The handle is returned regardless of the validation result, so callers
     * can preserve rejected blocks for investigation.
     */
    virtual std::unique_ptr<Block> submitSolution(uint32_t version, uint32_t timestamp, uint32_t nonce, CTransactionRef coinbase, bool& accepted, std::string& reason, std::string& debug) = 0;

    //! Deprecated older method preserved to return an explicit error for IPC
    //! clients using mining.capnp @7.
    virtual bool submitSolutionOld7(uint32_t, uint32_t, uint32_t, CTransactionRef)
    {
        throw std::runtime_error("Old submitSolution (@7) not supported. Please update your client!");
    }

    //! Deprecated: no longer supported, returns an error.
    virtual bool submitSolutionOld10(uint32_t, uint32_t, uint32_t, CTransactionRef, std::string&, std::string&) { throw std::runtime_error("Old submitSolution (@10) not supported. Please update your client!"); }

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
    virtual std::unique_ptr<BlockTemplate> waitNext(node::BlockWaitOptions options = {}) = 0;

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
     * @retval std::nullopt if the node is shut down or interrupt() is called.
     */
    virtual std::optional<BlockRef> waitTipChanged(uint256 current_tip, MillisecondsDouble timeout = MillisecondsDouble::max()) = 0;

   /**
     * Construct a new block template.
     *
     * @param[in] options options for creating the block
     * @param[in] cooldown wait for tip to be connected and IBD to complete.
     *                     If the best header is ahead of the tip, wait for the
     *                     tip to catch up. It's recommended to disable this on
     *                     regtest and signets with only one miner, as these
     *                     could stall.
     * @retval BlockTemplate a block template.
     * @retval std::nullptr if the node is shut down or interrupt() is called.
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const node::BlockCreateOptions& options = {}, bool cooldown = true) = 0;

    /**
     * Interrupts createNewBlock and waitTipChanged.
     */
    virtual void interrupt() = 0;

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

    /**
     * Process a fully assembled block.
     *
     * Similar to the submitblock RPC. Accepts a complete block, validates
     * it, and if accepted as new, processes it into chainstate. Accepted
     * blocks may then be announced to peers through normal validation signals.
     *
     * @param[in]  block  the complete block to submit
     * @param[out] accepted true if the block was accepted as a new block. Set
     *                    to false, with reason set, if the block is a duplicate
     *                    or the validation result is inconclusive.
     * @param[out] reason failure reason (BIP22)
     * @param[out] debug  more detailed rejection reason
     * @returns           a handle to the submitted block, including on rejection
     *
     * @note Unlike the submitblock RPC, this method does not call
     *       UpdateUncommittedBlockStructures to add a missing coinbase witness
     *       reserved value. Callers must submit a fully formed block, including
     *       the coinbase witness when a witness commitment is present.
     */
    virtual std::unique_ptr<Block> submitBlock(const CBlock& block, bool& accepted, std::string& reason, std::string& debug) = 0;

    //! Deprecated: no longer supported, returns an error.
    virtual bool submitBlockOld7(const CBlock&, std::string&, std::string&) { throw std::runtime_error("Old submitBlock (@7) not supported. Please update your client!"); }

    /**
     * Fetch raw transactions from the mempool by txid.
     *
     * @param[in] txids   transaction ids to look up
     * @returns           one entry per requested txid containing the
     *                    transaction if found, otherwise nullptr
     */
    virtual std::vector<CTransactionRef> getTransactionsByTxID(const std::vector<Txid>& txids) = 0;

    /**
     * Fetch raw transactions from the mempool by wtxid.
     *
     * @param[in] wtxids   witness transaction ids to look up
     * @returns            one entry per requested wtxid containing the
     *                     transaction if found, otherwise nullptr
     */
    virtual std::vector<CTransactionRef> getTransactionsByWitnessID(const std::vector<Wtxid>& wtxids) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual const node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
//!
//! @param[in] wait_loaded waits for chainstate data to be loaded and a
//!                        chain tip to be connected before
//!                        returning. Used to prevent external clients from
//!                        being able to crash the node during startup.
std::unique_ptr<Mining> MakeMining(const node::NodeContext& node, bool wait_loaded=true);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
