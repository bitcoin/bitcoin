// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CHAIN_H
#define BITCOIN_INTERFACES_CHAIN_H

#include <blockfilter.h>
#include <common/settings.h>
#include <node/types.h>
#include <primitives/transaction.h>
#include <util/result.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ArgsManager;
class CBlock;
class CBlockUndo;
class CFeeRate;
class CRPCCommand;
class CScheduler;
class Coin;
class uint256;
enum class MemPoolRemovalReason;
enum class RBFTransactionState;
enum class ChainstateRole;
struct bilingual_str;
struct CBlockLocator;
struct FeeCalculation;
namespace node {
struct NodeContext;
} // namespace node

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
    //! Return locator if block is in the active chain.
    FoundBlock& locator(CBlockLocator& locator) { m_locator = &locator; return *this; }
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
    CBlockLocator* m_locator = nullptr;
    const FoundBlock* m_next_block = nullptr;
    CBlock* m_data = nullptr;
    mutable bool found = false;
};

//! Block data sent with blockConnected, blockDisconnected notifications.
struct BlockInfo {
    const uint256& hash;
    const uint256* prev_hash = nullptr;
    int height = -1;
    int file_number = -1;
    unsigned data_pos = 0;
    const CBlock* data = nullptr;
    const CBlockUndo* undo_data = nullptr;
    // The maximum time in the chain up to and including this block.
    // A timestamp that can only move forward.
    unsigned int chain_time_max{0};

    BlockInfo(const uint256& hash LIFETIMEBOUND) : hash(hash) {}
};

//! The action to be taken after updating a settings value.
//! WRITE indicates that the updated value must be written to disk,
//! while SKIP_WRITE indicates that the change will be kept in memory-only
//! without persisting it.
enum class SettingsAction {
    WRITE,
    SKIP_WRITE
};

using SettingsUpdate = std::function<std::optional<interfaces::SettingsAction>(common::SettingsValue&)>;

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
    virtual ~Chain() = default;

    //! Get current chain height, not including genesis block (returns 0 if
    //! chain only contains genesis block, nullopt if chain does not contain
    //! any blocks)
    virtual std::optional<int> getHeight() = 0;

    //! Get block hash. Height must be valid or this function will abort.
    virtual uint256 getBlockHash(int height) = 0;

    //! Check that the block is available on disk (i.e. has not been
    //! pruned), and contains transactions.
    virtual bool haveBlockOnDisk(int height) = 0;

    //! Return height of the highest block on chain in common with the locator,
    //! which will either be the original block used to create the locator,
    //! or one of its ancestors.
    virtual std::optional<int> findLocatorFork(const CBlockLocator& locator) = 0;

    //! Returns whether a block filter index is available.
    virtual bool hasBlockFilterIndex(BlockFilterType filter_type) = 0;

    //! Returns whether any of the elements match the block via a BIP 157 block filter
    //! or std::nullopt if the block filter for this block couldn't be found.
    virtual std::optional<bool> blockFilterMatchesAny(BlockFilterType filter_type, const uint256& block_hash, const GCSFilter::ElementSet& filter_set) = 0;

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
    virtual bool isInMempool(const Txid& txid) = 0;

    //! Check if transaction has descendants in mempool.
    virtual bool hasDescendantsInMempool(const Txid& txid) = 0;

    //! Process a local transaction, optionally adding it to the mempool and
    //! optionally broadcasting it to the network.
    //! @param[in] tx Transaction to process.
    //! @param[in] max_tx_fee Don't add the transaction to the mempool or
    //! broadcast it if its fee is higher than this.
    //! @param[in] broadcast_method Whether to add the transaction to the
    //! mempool and how/whether to broadcast it.
    //! @param[out] err_string Set if an error occurs.
    //! @return False if the transaction could not be added due to the fee or for another reason.
    virtual bool broadcastTransaction(const CTransactionRef& tx,
                                      const CAmount& max_tx_fee,
                                      node::TxBroadcast broadcast_method,
                                      std::string& err_string) = 0;

    //! Calculate mempool ancestor and cluster counts for the given transaction.
    virtual void getTransactionAncestry(const Txid& txid, size_t& ancestors, size_t& cluster_count, size_t* ancestorsize = nullptr, CAmount* ancestorfees = nullptr) = 0;

    //! For each outpoint, calculate the fee-bumping cost to spend this outpoint at the specified
    //  feerate, including bumping its ancestors. For example, if the target feerate is 10sat/vbyte
    //  and this outpoint refers to a mempool transaction at 3sat/vbyte, the bump fee includes the
    //  cost to bump the mempool transaction to 10sat/vbyte (i.e. 7 * mempooltx.vsize). If that
    //  transaction also has, say, an unconfirmed parent with a feerate of 1sat/vbyte, the bump fee
    //  includes the cost to bump the parent (i.e. 9 * parentmempooltx.vsize).
    //
    //  If the outpoint comes from an unconfirmed transaction that is already above the target
    //  feerate or bumped by its descendant(s) already, it does not need to be bumped. Its bump fee
    //  is 0. Likewise, if any of the transaction's ancestors are already bumped by a transaction
    //  in our mempool, they are not included in the transaction's bump fee.
    //
    //  Also supported is bump-fee calculation in the case of replacements. If an outpoint
    //  conflicts with another transaction in the mempool, it is assumed that the goal is to replace
    //  that transaction. As such, the calculation will exclude the to-be-replaced transaction, but
    //  will include the fee-bumping cost. If bump fees of descendants of the to-be-replaced
    //  transaction are requested, the value will be 0. Fee-related RBF rules are not included as
    //  they are logically distinct.
    //
    //  Any outpoints that are otherwise unavailable from the mempool (e.g. UTXOs from confirmed
    //  transactions or transactions not yet broadcast by the wallet) are given a bump fee of 0.
    //
    //  If multiple outpoints come from the same transaction (which would be very rare because
    //  it means that one transaction has multiple change outputs or paid the same wallet using multiple
    //  outputs in the same transaction) or have shared ancestry, the bump fees are calculated
    //  independently, i.e. as if only one of them is spent. This may result in double-fee-bumping. This
    //  caveat can be rectified per use of the sister-function CalculateCombinedBumpFee(…).
    virtual std::map<COutPoint, CAmount> calculateIndividualBumpFees(const std::vector<COutPoint>& outpoints, const CFeeRate& target_feerate) = 0;

    //! Calculate the combined bump fee for an input set per the same strategy
    //  as in CalculateIndividualBumpFees(…).
    //  Unlike CalculateIndividualBumpFees(…), this does not return individual
    //  bump fees per outpoint, but a single bump fee for the shared ancestry.
    //  The combined bump fee may be used to correct overestimation due to
    //  shared ancestry by multiple UTXOs after coin selection.
    virtual std::optional<CAmount> calculateCombinedBumpFee(const std::vector<COutPoint>& outpoints, const CFeeRate& target_feerate) = 0;

    //! Get the node's package limits.
    //! Currently only returns the ancestor and descendant count limits, but could be enhanced to
    //! return more policy settings.
    virtual void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) = 0;

    //! Check if transaction will pass the mempool's chain limits.
    virtual util::Result<void> checkChainLimits(const CTransactionRef& tx) = 0;

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

    //! Get the current prune height.
    virtual std::optional<int> getPruneHeight() = 0;

    //! Check if the node is ready to broadcast transactions.
    virtual bool isReadyToBroadcast() = 0;

    //! Check if in IBD.
    virtual bool isInitialBlockDownload() = 0;

    //! Check if shutdown requested.
    virtual bool shutdownRequested() = 0;

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
        virtual ~Notifications() = default;
        virtual void transactionAddedToMempool(const CTransactionRef& tx) {}
        virtual void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason) {}
        virtual void blockConnected(ChainstateRole role, const BlockInfo& block) {}
        virtual void blockDisconnected(const BlockInfo& block) {}
        virtual void updatedBlockTip() {}
        virtual void chainStateFlushed(ChainstateRole role, const CBlockLocator& locator) {}
    };

    //! Options specifying which chain notifications are required.
    struct NotifyOptions
    {
        //! Include undo data with block connected notifications.
        bool connect_undo_data = false;
        //! Include block data with block disconnected notifications.
        bool disconnect_data = false;
        //! Include undo data with block disconnected notifications.
        bool disconnect_undo_data = false;
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

    //! Get settings value.
    virtual common::SettingsValue getSetting(const std::string& arg) = 0;

    //! Get list of settings values.
    virtual std::vector<common::SettingsValue> getSettingsList(const std::string& arg) = 0;

    //! Return <datadir>/settings.json setting value.
    virtual common::SettingsValue getRwSetting(const std::string& name) = 0;

    //! Updates a setting in <datadir>/settings.json.
    //! Null can be passed to erase the setting. There is intentionally no
    //! support for writing null values to settings.json.
    //! Depending on the action returned by the update function, this will either
    //! update the setting in memory or write the updated settings to disk.
    virtual bool updateRwSetting(const std::string& name, const SettingsUpdate& update_function) = 0;

    //! Replace a setting in <datadir>/settings.json with a new value.
    //! Null can be passed to erase the setting.
    //! This method provides a simpler alternative to updateRwSetting when
    //! atomically reading and updating the setting is not required.
    virtual bool overwriteRwSetting(const std::string& name, common::SettingsValue value, SettingsAction action = SettingsAction::WRITE) = 0;

    //! Delete a given setting in <datadir>/settings.json.
    //! This method provides a simpler alternative to overwriteRwSetting when
    //! erasing a setting, for ease of use and readability.
    virtual bool deleteRwSettings(const std::string& name, SettingsAction action = SettingsAction::WRITE) = 0;

    //! Synchronously send transactionAddedToMempool notifications about all
    //! current mempool transactions to the specified handler and return after
    //! the last one is sent. These notifications aren't coordinated with async
    //! notifications sent by handleNotifications, so out of date async
    //! notifications from handleNotifications can arrive during and after
    //! synchronous notifications from requestMempoolTransactions. Clients need
    //! to be prepared to handle this by ignoring notifications about unknown
    //! removed transactions and already added new transactions.
    virtual void requestMempoolTransactions(Notifications& notifications) = 0;

    //! Return true if an assumed-valid chain is in use.
    virtual bool hasAssumedValidChain() = 0;

    //! Get internal node context. Useful for testing, but not
    //! accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Interface to let node manage chain clients (wallets, or maybe tools for
//! monitoring and analysis in the future).
class ChainClient
{
public:
    virtual ~ChainClient() = default;

    //! Register rpcs.
    virtual void registerRpcs() = 0;

    //! Check for errors before loading.
    virtual bool verify() = 0;

    //! Load saved state.
    virtual bool load() = 0;

    //! Start client execution and provide a scheduler.
    virtual void start(CScheduler& scheduler) = 0;

    //! Shut down client.
    virtual void stop() = 0;

    //! Set mock time.
    virtual void setMockTime(int64_t time) = 0;

    //! Mock the scheduler to fast forward in time.
    virtual void schedulerMockForward(std::chrono::seconds delta_seconds) = 0;
};

//! Return implementation of Chain interface.
std::unique_ptr<Chain> MakeChain(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_CHAIN_H
