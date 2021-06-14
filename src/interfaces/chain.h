// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CHAIN_H
#define BITCOIN_INTERFACES_CHAIN_H

#include <primitives/transaction.h> // For CTransactionRef
#include <util/settings.h>          // For util::SettingsValue

#include <functional>
#include <memory>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class ArgsManager;
class CBlock;
class CFeeRate;
class CRPCCommand;
class CScheduler;
class Coin;
class uint256;
enum class MemPoolRemovalReason;
enum class RBFTransactionState;
struct bilingual_str;
struct CBlockLocator;
struct FeeCalculation;
struct NodeContext;

namespace interfaces {

class Handler;
class Wallet;

//! Helper for findBlock to selectively return pieces of block data. If block is
//! found, data will be returned by setting specified output variables. If block
//! is not found, output variables will keep their previous values.
class FoundBlock
{
public:
    FoundBlock& hash(uint256& hash) { m_hash = &hash; return *this; }
    FoundBlock& height(int& height) { m_height = &height; return *this; }
    FoundBlock& time(int64_t& time) { m_time = &time; return *this; }
    FoundBlock& maxTime(int64_t& max_time) { m_max_time = &max_time; return *this; }
    FoundBlock& mtpTime(int64_t& mtp_time) { m_mtp_time = &mtp_time; return *this; }
    //! Return whether block is in the active (most-work) chain.
    FoundBlock& inActiveChain(bool& in_active_chain) { m_in_active_chain = &in_active_chain; return *this; }
    //! Return next block in the active chain if current block is in the active chain.
    FoundBlock& nextBlock(const FoundBlock& next_block) { m_next_block = &next_block; return *this; }
    //! Read block data from disk. If the block exists but doesn't have data
    //! (for example due to pruning), the CBlock variable will be set to null.
    FoundBlock& data(CBlock& data) { m_data = &data; return *this; }

    uint256* m_hash = nullptr;
    int* m_height = nullptr;
    int64_t* m_time = nullptr;
    int64_t* m_max_time = nullptr;
    int64_t* m_mtp_time = nullptr;
    bool* m_in_active_chain = nullptr;
    const FoundBlock* m_next_block = nullptr;
    CBlock* m_data = nullptr;
    mutable bool found = false;
};

//! Interface giving clients (wallet processes, maybe other analysis tools in
//! the future) ability to access to the chain state, receive notifications,
//! estimate fees, and submit transactions.
//!
//! TODO: Current chain methods are too low level, exposing too much of the
//! internal workings of the bitcoin node, and not being very convenient to use.
//! Chain methods should be cleaned up and simplified over time. Examples:
//!
//! * The initMessages() and showProgress() methods which the wallet uses to send
//!   notifications to the GUI should go away when GUI and wallet can directly
//!   communicate with each other without going through the node
//!   (https://github.com/bitcoin/bitcoin/pull/15288#discussion_r253321096).
//!
//! * The handleRpc, registerRpcs, rpcEnableDeprecated methods and other RPC
//!   methods can go away if wallets listen for HTTP requests on their own
//!   ports instead of registering to handle requests on the node HTTP port.
//!
//! * Move fee estimation queries to an asynchronous interface and let the
//!   wallet cache it, fee estimation being driven by node mempool, wallet
//!   should be the consumer.
//!
//! * `guessVerificationProgress` and similar methods can go away if rescan
//!   logic moves out of the wallet, and the wallet just requests scans from the
//!   node (https://github.com/bitcoin/bitcoin/issues/11756)
class Chain
{
public:
    virtual ~Chain() {}

    //! Get current chain height, not including genesis block (returns 0 if
    //! chain only contains genesis block, nullopt if chain does not contain
    //! any blocks)
    virtual std::optional<int> getHeight() = 0;

    //! Get block hash. Height must be valid or this function will abort.
    virtual uint256 getBlockHash(int height) = 0;

    //! Check that the block is available on disk (i.e. has not been
    //! pruned), and contains transactions.
    virtual bool haveBlockOnDisk(int height) = 0;

    //! Get locator for the current chain tip.
    virtual CBlockLocator getTipLocator() = 0;

    //! Return height of the highest block on chain in common with the locator,
    //! which will either be the original block used to create the locator,
    //! or one of its ancestors.
    virtual std::optional<int> findLocatorFork(const CBlockLocator& locator) = 0;

    //! Check if transaction will be final given chain height current time.
    virtual bool checkFinalTx(const CTransaction& tx) = 0;

    //! Return whether node has the block and optionally return block metadata
    //! or contents.
    virtual bool findBlock(const uint256& hash, const FoundBlock& block={}) = 0;

    //! Find first block in the chain with timestamp >= the given time
    //! and height >= than the given height, return false if there is no block
    //! with a high enough timestamp and height. Optionally return block
    //! information.
    virtual bool findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, const FoundBlock& block={}) = 0;

    //! Find ancestor of block at specified height and optionally return
    //! ancestor information.
    virtual bool findAncestorByHeight(const uint256& block_hash, int ancestor_height, const FoundBlock& ancestor_out={}) = 0;

    //! Return whether block descends from a specified ancestor, and
    //! optionally return ancestor information.
    virtual bool findAncestorByHash(const uint256& block_hash,
        const uint256& ancestor_hash,
        const FoundBlock& ancestor_out={}) = 0;

    //! Find most recent common ancestor between two blocks and optionally
    //! return block information.
    virtual bool findCommonAncestor(const uint256& block_hash1,
        const uint256& block_hash2,
        const FoundBlock& ancestor_out={},
        const FoundBlock& block1_out={},
        const FoundBlock& block2_out={}) = 0;

    //! Look up unspent output information. Returns coins in the mempool and in
    //! the current chain UTXO set. Iterates through all the keys in the map and
    //! populates the values.
    virtual void findCoins(std::map<COutPoint, Coin>& coins) = 0;

    //! Estimate fraction of total transactions verified if blocks up to
    //! the specified block hash are verified.
    virtual double guessVerificationProgress(const uint256& block_hash) = 0;

    //! Return true if data is available for all blocks in the specified range
    //! of blocks. This checks all blocks that are ancestors of block_hash in
    //! the height range from min_height to max_height, inclusive.
    virtual bool hasBlocks(const uint256& block_hash, int min_height = 0, std::optional<int> max_height = {}) = 0;

    //! Check if transaction is RBF opt in.
    virtual RBFTransactionState isRBFOptIn(const CTransaction& tx) = 0;

    //! Check if transaction is in mempool.
    virtual bool isInMempool(const uint256& txid) = 0;

    //! Check if transaction has descendants in mempool.
    virtual bool hasDescendantsInMempool(const uint256& txid) = 0;

    //! Transaction is added to memory pool, if the transaction fee is below the
    //! amount specified by max_tx_fee, and broadcast to all peers if relay is set to true.
    //! Return false if the transaction could not be added due to the fee or for another reason.
    virtual bool broadcastTransaction(const CTransactionRef& tx,
        const CAmount& max_tx_fee,
        bool relay,
        std::string& err_string) = 0;

    //! Calculate mempool ancestor and descendant counts for the given transaction.
    virtual void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants) = 0;

    //! Get the node's package limits.
    //! Currently only returns the ancestor and descendant count limits, but could be enhanced to
    //! return more policy settings.
    virtual void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) = 0;

    //! Check if transaction will pass the mempool's chain limits.
    virtual bool checkChainLimits(const CTransactionRef& tx) = 0;

    //! Estimate smart fee.
    virtual CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc = nullptr) = 0;

    //! Fee estimator max target.
    virtual unsigned int estimateMaxBlocks() = 0;

    //! Mempool minimum fee.
    virtual CFeeRate mempoolMinFee() = 0;

    //! Relay current minimum fee (from -minrelaytxfee and -incrementalrelayfee settings).
    virtual CFeeRate relayMinFee() = 0;

    //! Relay incremental fee setting (-incrementalrelayfee), reflecting cost of relay.
    virtual CFeeRate relayIncrementalFee() = 0;

    //! Relay dust fee setting (-dustrelayfee), reflecting lowest rate it's economical to spend.
    virtual CFeeRate relayDustFee() = 0;

    //! Check if any block has been pruned.
    virtual bool havePruned() = 0;

    //! Check if the node is ready to broadcast transactions.
    virtual bool isReadyToBroadcast() = 0;

    //! Check if in IBD.
    virtual bool isInitialBlockDownload() = 0;

    //! Check if shutdown requested.
    virtual bool shutdownRequested() = 0;

    //! Get adjusted time.
    virtual int64_t getAdjustedTime() = 0;

    //! Send init message.
    virtual void initMessage(const std::string& message) = 0;

    //! Send init warning.
    virtual void initWarning(const bilingual_str& message) = 0;

    //! Send init error.
    virtual void initError(const bilingual_str& message) = 0;

    //! Send progress indicator.
    virtual void showProgress(const std::string& title, int progress, bool resume_possible) = 0;

    //! Chain notifications.
    class Notifications
    {
    public:
        virtual ~Notifications() {}
        virtual void transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) {}
        virtual void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) {}
        virtual void blockConnected(const CBlock& block, int height) {}
        virtual void blockDisconnected(const CBlock& block, int height) {}
        virtual void updatedBlockTip() {}
        virtual void chainStateFlushed(const CBlockLocator& locator) {}
    };

    //! Register handler for notifications.
    virtual std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications> notifications) = 0;

    //! Wait for pending notifications to be processed unless block hash points to the current
    //! chain tip.
    virtual void waitForNotificationsIfTipChanged(const uint256& old_tip) = 0;

    //! Register handler for RPC. Command is not copied, so reference
    //! needs to remain valid until Handler is disconnected.
    virtual std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) = 0;

    //! Check if deprecated RPC is enabled.
    virtual bool rpcEnableDeprecated(const std::string& method) = 0;

    //! Run function after given number of seconds. Cancel any previous calls with same name.
    virtual void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) = 0;

    //! Current RPC serialization flags.
    virtual int rpcSerializationFlags() = 0;

    //! Get settings value.
    virtual util::SettingsValue getSetting(const std::string& arg) = 0;

    //! Get list of settings values.
    virtual std::vector<util::SettingsValue> getSettingsList(const std::string& arg) = 0;

    //! Return <datadir>/settings.json setting value.
    virtual util::SettingsValue getRwSetting(const std::string& name) = 0;

    //! Write a setting to <datadir>/settings.json. Optionally just update the
    //! setting in memory and do not write the file.
    virtual bool updateRwSetting(const std::string& name, const util::SettingsValue& value, bool write=true) = 0;

    //! Synchronously send transactionAddedToMempool notifications about all
    //! current mempool transactions to the specified handler and return after
    //! the last one is sent. These notifications aren't coordinated with async
    //! notifications sent by handleNotifications, so out of date async
    //! notifications from handleNotifications can arrive during and after
    //! synchronous notifications from requestMempoolTransactions. Clients need
    //! to be prepared to handle this by ignoring notifications about unknown
    //! removed transactions and already added new transactions.
    virtual void requestMempoolTransactions(Notifications& notifications) = 0;

    //! Check if Taproot has activated
    virtual bool isTaprootActive() = 0;
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

    //! Set mock time.
    virtual void setMockTime(int64_t time) = 0;
};

//! Return implementation of Chain interface.
std::unique_ptr<Chain> MakeChain(NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_CHAIN_H
