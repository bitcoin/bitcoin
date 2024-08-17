// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_NODE_H
#define BITCOIN_INTERFACES_NODE_H

#include <common/settings.h>
#include <consensus/amount.h>          // For CAmount
#include <net.h>                       // For NodeId
#include <net_types.h>                 // For banmap_t
#include <netaddress.h>                // For Network
#include <netbase.h>                   // For ConnectionDirection
#include <support/allocators/secure.h> // For SecureString
#include <util/translation.h>

#include <functional>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <tuple>
#include <vector>

class BanMan;
class CFeeRate;
class CNodeStats;
class Coin;
class RPCTimerInterface;
class UniValue;
class Proxy;
enum class SynchronizationState;
struct CNodeStateStats;
struct bilingual_str;
namespace node {
enum class TransactionError;
struct NodeContext;
} // namespace node
namespace wallet {
class CCoinControl;
} // namespace wallet

namespace interfaces {
class Handler;
class WalletLoader;
struct BlockTip;

//! Information about block and header tips
struct BlockAndHeaderTipInfo
{
    int block_height;
    int64_t block_time;
    int header_height;
    int64_t header_time;
    double verification_progress;
};

//! Interface for external signers used by the GUI.
class ExternalSigner
{
public:
    virtual ~ExternalSigner() = default;

    //! Retrieve signer display name
    virtual std::string getName() = 0;
};

//! Primary interface for a Bitcoin node (bitcoind process).
class Node
{
public:
    virtual ~Node() = default;

    //! Initialize logging.
    virtual void initLogging() = 0;

    //! Initialize parameter interaction.
    virtual void initParameterInteraction() = 0;

    //! Fetch warnings.
    virtual bilingual_str getWarnings() = 0;

    //! Fetch exit status.
    virtual int getExitStatus() = 0;

    // Fetch log flags.
    virtual uint32_t getLogCategories() = 0;

    //! Initialize application dependencies.
    virtual bool baseInitialize() = 0;

    //! Start the node.
    virtual bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info = nullptr) = 0;

    //! Stop the node.
    virtual void appShutdown() = 0;

    //! Initiate shutdown.
    virtual void startShutdown() = 0;

    //! Determine if shutdown was requested.
    virtual bool shutdownRequested() = 0;

    //! Determine if a specific setting in <datadir>/settings.json is ignored
    //! due to being specified in the command line.
    virtual bool isSettingIgnored(const std::string& name) = 0;

    //! Fetch a setting value from <datadir>/settings.json or bitcoin.conf.
    virtual common::SettingsValue getPersistentSetting(const std::string& name) = 0;

    //! Update a setting in <datadir>/settings.json.
    virtual void updateRwSetting(const std::string& name, const common::SettingsValue& value) = 0;

    //! Override a setting value, not persistent, but effective immediately.
    virtual void forceSetting(const std::string& name, const common::SettingsValue& value) = 0;

    //! Clear all settings in <datadir>/settings.json, backing up old settings in <datadir>/settings.json.bak.
    virtual void resetSettings() = 0;

    //! Map port using UPnP or NAT-PMP.
    virtual void mapPort(bool use_upnp, bool use_natpmp) = 0;

    //! Fetch proxy information.
    virtual bool getProxy(Network net, Proxy& proxy_info) = 0;

    //! Fetch the number of connections.
    virtual size_t getNodeCount(ConnectionDirection flags) = 0;

    //! Retrieve stats for connected nodes.
    using NodesStats = std::vector<std::tuple<CNodeStats, bool, CNodeStateStats>>;
    virtual bool getNodesStats(NodesStats& stats) = 0;

    //! Fetch ban map entries.
    virtual bool getBanned(banmap_t& banmap) = 0;

    //! Ban a node.
    virtual bool ban(const CNetAddr& net_addr, int64_t ban_time_offset) = 0;

    //! Unban a node.
    virtual bool unban(const CSubNet& ip) = 0;

    //! Disconnect a node by address.
    virtual bool disconnectByAddress(const CNetAddr& net_addr) = 0;

    //! Disconnect a node by ID.
    virtual bool disconnectById(NodeId id) = 0;

    //! List external signers (devices capable of signing transactions).
    virtual std::vector<std::unique_ptr<ExternalSigner>> listExternalSigners() = 0;

    //! Fetch total bytes received.
    virtual int64_t getTotalBytesRecv() = 0;

    //! Fetch total bytes sent.
    virtual int64_t getTotalBytesSent() = 0;

    //! Fetch mempool size.
    virtual size_t getMempoolSize() = 0;

    //! Fetch mempool dynamic usage.
    virtual size_t getMempoolDynamicUsage() = 0;

    //! Fetch mempool maximum memory usage.
    virtual size_t getMempoolMaxUsage() = 0;

    //! Fetch header tip height and time.
    virtual bool getHeaderTip(int& height, int64_t& block_time) = 0;

    //! Fetch the number of blocks.
    virtual int getNumBlocks() = 0;

    //! Fetch network local addresses.
    virtual std::map<CNetAddr, LocalServiceInfo> getNetLocalAddresses() = 0;

    //! Fetch the best block hash.
    virtual uint256 getBestBlockHash() = 0;

    //! Fetch the last block time.
    virtual int64_t getLastBlockTime() = 0;

    //! Fetch verification progress.
    virtual double getVerificationProgress() = 0;

    //! Check if initial block download is in progress.
    virtual bool isInitialBlockDownload() = 0;

    //! Check if blocks are being loaded.
    virtual bool isLoadingBlocks() = 0;

    //! Set network activity.
    virtual void setNetworkActive(bool active) = 0;

    //! Fetch network activity status.
    virtual bool getNetworkActive() = 0;

    //! Fetch dust relay fee.
    virtual CFeeRate getDustRelayFee() = 0;

    //! Execute an RPC command.
    virtual UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) = 0;

    //! List available RPC commands.
    virtual std::vector<std::string> listRpcCommands() = 0;

    //! Set RPC timer interface if not already set.
    virtual void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) = 0;

    //! Unset the RPC timer interface.
    virtual void rpcUnsetTimerInterface(RPCTimerInterface* iface) = 0;

    //! Fetch unspent output associated with a transaction.
    virtual std::optional<Coin> getUnspentOutput(const COutPoint& output) = 0;

    //! Broadcast a transaction.
    virtual node::TransactionError broadcastTransaction(CTransactionRef tx, CAmount max_tx_fee, std::string& err_string) = 0;

    //! Fetch the wallet loader.
    virtual WalletLoader& walletLoader() = 0;

    //! Register handler for initialization messages.
    using InitMessageFn = std::function<void(const std::string& message)>;
    virtual std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) = 0;

    //! Register handler for message box events.
    using MessageBoxFn = std::function<bool(const bilingual_str& message, const std::string& caption, unsigned int style)>;
    virtual std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) = 0;

    //! Register handler for question events.
    using QuestionFn = std::function<bool(const bilingual_str& message,
        const std::string& non_interactive_message,
        const std::string& caption,
        unsigned int style)>;
    virtual std::unique_ptr<Handler> handleQuestion(QuestionFn fn) = 0;

    //! Register handler for progress events.
    using ShowProgressFn = std::function<void(const std::string& title, int progress, bool resume_possible)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for wallet initialization events.
    using InitWalletFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) = 0;

    //! Register handler for connection count change events.
    using NotifyNumConnectionsChangedFn = std::function<void(int new_num_connections)>;
    virtual std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) = 0;

    //! Register handler for network activity change events.
    using NotifyNetworkActiveChangedFn = std::function<void(bool network_active)>;
    virtual std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) = 0;

    //! Register handler for alert change events.
    using NotifyAlertChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) = 0;

    //! Register handler for banned list change events.
    using BannedListChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) = 0;

    //! Register handler for block tip change events.
    using NotifyBlockTipFn = std::function<void(SynchronizationState state, int height, int64_t block_time, double verification_progress)>;
    virtual std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) = 0;

    //! Register handler for header tip change events.
    using NotifyHeaderTipFn = std::function<void(int height, int64_t block_time, double verification_progress)>;
    virtual std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) = 0;

    //! Register handler for transaction added to mempool events.
    using NotifyTransactionAddedToMempoolFn = std::function<void(const CTransactionRef& tx)>;
    virtual std::unique_ptr<Handler> handleNotifyTransactionAddedToMempool(NotifyTransactionAddedToMempoolFn fn) = 0;

    //! Register handler for transaction removed from mempool events.
    using NotifyTransactionRemovedFromMempoolFn = std::function<void(const CTransactionRef& tx, MemPoolRemovalReason reason)>;
    virtual std::unique_ptr<Handler> handleNotifyTransactionRemovedFromMempool(NotifyTransactionRemovedFromMempoolFn fn) = 0;

    //! Register handler for transaction confirmed events.
    using NotifyTransactionConfirmedFn = std::function<void(const CTransactionRef& tx, int blocks)>;
    virtual std::unique_ptr<Handler> handleNotifyTransactionConfirmed(NotifyTransactionConfirmedFn fn) = 0;

    //! Register handler for active chain tip change events.
    using NotifyActiveChainTipFn = std::function<void(int height, int64_t block_time)>;
    virtual std::unique_ptr<Handler> handleNotifyActiveChainTip(NotifyActiveChainTipFn fn) = 0;

    //! Register handler for keypool depletion warning events.
    using NotifyKeypoolDrainedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleNotifyKeypoolDrained(NotifyKeypoolDrainedFn fn) = 0;

    //! Register handler for block connected events.
    using NotifyBlockConnectedFn = std::function<void(const std::shared_ptr<const CBlock>& block)>;
    virtual std::unique_ptr<Handler> handleNotifyBlockConnected(NotifyBlockConnectedFn fn) = 0;

    //! Register handler for block disconnected events.
    using NotifyBlockDisconnectedFn = std::function<void(const std::shared_ptr<const CBlock>& block)>;
    virtual std::unique_ptr<Handler> handleNotifyBlockDisconnected(NotifyBlockDisconnectedFn fn) = 0;
};

//! Return implementation of Node interface.
std::unique_ptr<Node> MakeNode(NodeContext& context);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_NODE_H
