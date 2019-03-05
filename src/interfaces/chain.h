// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CHAIN_H
#define BITCOIN_INTERFACES_CHAIN_H

#include <optional.h>               // For Optional and nullopt
#include <primitives/transaction.h> // For CTransactionRef

#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class CBlock;
class CFeeRate;
class CScheduler;
class CValidationState;
class uint256;
enum class RBFTransactionState;
struct CBlockLocator;
struct FeeCalculation;

namespace interfaces {

class Wallet;

//! Interface giving clients (wallet processes, maybe other analysis tools in
//! the future) ability to access to the chain state, receive notifications,
//! estimate fees, and submit transactions.
//!
//! TODO: Current chain methods are too low level, exposing too much of the
//! internal workings of the bitcoin node, and not being very convenient to use.
//! Chain methods should be cleaned up and simplified over time. Examples:
//!
//! * The Chain::lock() method, which lets clients delay chain tip updates
//!   should be removed when clients are able to respond to updates
//!   asynchronously
//!   (https://github.com/bitcoin/bitcoin/pull/10973#issuecomment-380101269).
//!
//! * The relayTransactions() and submitToMemoryPool() methods could be replaced
//!   with a higher-level broadcastTransaction method
//!   (https://github.com/bitcoin/bitcoin/pull/14978#issuecomment-459373984).
//!
//! * The initMessages() and loadWallet() methods which the wallet uses to send
//!   notifications to the GUI should go away when GUI and wallet can directly
//!   communicate with each other without going through the node
//!   (https://github.com/bitcoin/bitcoin/pull/15288#discussion_r253321096).
class Chain
{
public:
    virtual ~Chain() {}

    //! Interface for querying locked chain state, used by legacy code that
    //! assumes state won't change between calls. New code should avoid using
    //! the Lock interface and instead call higher-level Chain methods
    //! that return more information so the chain doesn't need to stay locked
    //! between calls.
    class Lock
    {
    public:
        virtual ~Lock() {}

        //! Get current chain height, not including genesis block (returns 0 if
        //! chain only contains genesis block, nullopt if chain does not contain
        //! any blocks).
        virtual Optional<int> getHeight() = 0;

        //! Get block height above genesis block. Returns 0 for genesis block,
        //! 1 for following block, and so on. Returns nullopt for a block not
        //! included in the current chain.
        virtual Optional<int> getBlockHeight(const uint256& hash) = 0;

        //! Get block depth. Returns 1 for chain tip, 2 for preceding block, and
        //! so on. Returns 0 for a block not included in the current chain.
        virtual int getBlockDepth(const uint256& hash) = 0;

        //! Get block hash. Height must be valid or this function will abort.
        virtual uint256 getBlockHash(int height) = 0;

        //! Get block time. Height must be valid or this function will abort.
        virtual int64_t getBlockTime(int height) = 0;

        //! Get block median time past. Height must be valid or this function
        //! will abort.
        virtual int64_t getBlockMedianTimePast(int height) = 0;

        //! Check that the block is available on disk (i.e. has not been
        //! pruned), and contains transactions.
        virtual bool haveBlockOnDisk(int height) = 0;

        //! Return height of the first block in the chain with timestamp equal
        //! or greater than the given time, or nullopt if there is no block with
        //! a high enough timestamp. Also return the block hash as an optional
        //! output parameter (to avoid the cost of a second lookup in case this
        //! information is needed.)
        virtual Optional<int> findFirstBlockWithTime(int64_t time, uint256* hash) = 0;

        //! Return height of the first block in the chain with timestamp equal
        //! or greater than the given time and height equal or greater than the
        //! given height, or nullopt if there is no such block.
        //!
        //! Calling this with height 0 is equivalent to calling
        //! findFirstBlockWithTime, but less efficient because it requires a
        //! linear instead of a binary search.
        virtual Optional<int> findFirstBlockWithTimeAndHeight(int64_t time, int height) = 0;

        //! Return height of last block in the specified range which is pruned, or
        //! nullopt if no block in the range is pruned. Range is inclusive.
        virtual Optional<int> findPruned(int start_height = 0, Optional<int> stop_height = nullopt) = 0;

        //! Return height of the highest block on the chain that is an ancestor
        //! of the specified block, or nullopt if no common ancestor is found.
        //! Also return the height of the specified block as an optional output
        //! parameter (to avoid the cost of a second hash lookup in case this
        //! information is desired).
        virtual Optional<int> findFork(const uint256& hash, Optional<int>* height) = 0;

        //! Return true if block hash points to the current chain tip, or to a
        //! possible descendant of the current chain tip that isn't currently
        //! connected.
        virtual bool isPotentialTip(const uint256& hash) = 0;

        //! Get locator for the current chain tip.
        virtual CBlockLocator getTipLocator() = 0;

        //! Return height of the latest block common to locator and chain, which
        //! is guaranteed to be an ancestor of the block used to create the
        //! locator.
        virtual Optional<int> findLocatorFork(const CBlockLocator& locator) = 0;

        //! Check if transaction will be final given chain height current time.
        virtual bool checkFinalTx(const CTransaction& tx) = 0;

        //! Add transaction to memory pool if the transaction fee is below the
        //! amount specified by absurd_fee. Returns false if the transaction
        //! could not be added due to the fee or for another reason.
        virtual bool submitToMemoryPool(const CTransactionRef& tx, CAmount absurd_fee, CValidationState& state) = 0;
    };

    //! Return Lock interface. Chain is locked when this is called, and
    //! unlocked when the returned interface is freed.
    virtual std::unique_ptr<Lock> lock(bool try_lock = false) = 0;

    //! Return Lock interface assuming chain is already locked. This
    //! method is temporary and is only used in a few places to avoid changing
    //! behavior while code is transitioned to use the Chain::Lock interface.
    virtual std::unique_ptr<Lock> assumeLocked() = 0;

    //! Return whether node has the block and optionally return block metadata
    //! or contents.
    //!
    //! If a block pointer is provided to retrieve the block contents, and the
    //! block exists but doesn't have data (for example due to pruning), the
    //! block will be empty and all fields set to null.
    virtual bool findBlock(const uint256& hash,
        CBlock* block = nullptr,
        int64_t* time = nullptr,
        int64_t* max_time = nullptr) = 0;

    //! Estimate fraction of total transactions verified if blocks up to
    //! the specified block hash are verified.
    virtual double guessVerificationProgress(const uint256& block_hash) = 0;

    //! Check if transaction is RBF opt in.
    virtual RBFTransactionState isRBFOptIn(const CTransaction& tx) = 0;

    //! Check if transaction has descendants in mempool.
    virtual bool hasDescendantsInMempool(const uint256& txid) = 0;

    //! Relay transaction.
    virtual void relayTransaction(const uint256& txid) = 0;

    //! Calculate mempool ancestor and descendant counts for the given transaction.
    virtual void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants) = 0;

    //! Check if transaction will pass the mempool's chain limits.
    virtual bool checkChainLimits(const CTransactionRef& tx) = 0;

    //! Estimate smart fee.
    virtual CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc = nullptr) = 0;

    //! Fee estimator max target.
    virtual unsigned int estimateMaxBlocks() = 0;

    //! Mempool minimum fee.
    virtual CFeeRate mempoolMinFee() = 0;

    //! Node max tx fee setting (-maxtxfee).
    //! This could be replaced by a per-wallet max fee, as proposed at
    //! https://github.com/bitcoin/bitcoin/issues/15355
    //! But for the time being, wallets call this to access the node setting.
    virtual CAmount maxTxFee() = 0;

    //! Check if pruning is enabled.
    virtual bool getPruneMode() = 0;

    //! Check if p2p enabled.
    virtual bool p2pEnabled() = 0;

    // Check if in IBD.
    virtual bool isInitialBlockDownload() = 0;

    //! Get adjusted time.
    virtual int64_t getAdjustedTime() = 0;

    //! Send init message.
    virtual void initMessage(const std::string& message) = 0;

    //! Send init warning.
    virtual void initWarning(const std::string& message) = 0;

    //! Send init error.
    virtual void initError(const std::string& message) = 0;

    //! Send wallet load notification to the GUI.
    virtual void loadWallet(std::unique_ptr<Wallet> wallet) = 0;
};

//! Interface to let node manage chain clients (wallets, or maybe tools for
//! monitoring and analysis in the future).
class ChainClient
{
public:
    virtual ~ChainClient() {}

    //! Register rpcs.
    virtual void registerRpcs() = 0;

    //! Check for errors before loading.
    virtual bool verify() = 0;

    //! Load saved state.
    virtual bool load() = 0;

    //! Start client execution and provide a scheduler.
    virtual void start(CScheduler& scheduler) = 0;

    //! Save state to disk.
    virtual void flush() = 0;

    //! Shut down client.
    virtual void stop() = 0;
};

//! Return implementation of Chain interface.
std::unique_ptr<Chain> MakeChain();

//! Return implementation of ChainClient interface for a wallet client. This
//! function will be undefined in builds where ENABLE_WALLET is false.
//!
//! Currently, wallets are the only chain clients. But in the future, other
//! types of chain clients could be added, such as tools for monitoring,
//! analysis, or fee estimation. These clients need to expose their own
//! MakeXXXClient functions returning their implementations of the ChainClient
//! interface.
std::unique_ptr<ChainClient> MakeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_CHAIN_H
