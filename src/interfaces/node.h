// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_NODE_H
#define BITCOIN_INTERFACES_NODE_H

#include <consensus/amount.h> // For CAmount
#include <net.h>        // For NodeId
#include <net_types.h>  // For banmap_t
#include <netaddress.h> // For Network
#include <netbase.h>    // For ConnectionDirection
#include <support/allocators/secure.h> // For SecureString
#include <uint256.h>
#include <util/translation.h>

#include <functional>
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <tuple>
#include <vector>

class BanMan;
class CBlockIndex;
class CCoinControl;
class CDeterministicMNList;
class CFeeRate;
class CGovernanceObject;
class CNodeStats;
class Coin;
class RPCTimerInterface;
class UniValue;
class Proxy;
enum class SynchronizationState;
enum class TransactionError;
enum vote_signal_enum_t : int;
struct bilingual_str;
struct CNodeStateStats;
namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {
class Handler;
class WalletLoader;
namespace CoinJoin {
class Loader;
} // namespace CoinJoin
struct BlockTip;

//! Interface for the src/evo part of a dash node (dashd process).
class EVO
{
public:
    virtual ~EVO() {}
    virtual std::pair<CDeterministicMNList, const CBlockIndex*> getListAtChainTip() = 0;
    virtual void setContext(node::NodeContext* context) {}
};

//! Interface for the src/governance part of a dash node (dashd process).
class GOV
{
public:
    virtual ~GOV() {}
    virtual void getAllNewerThan(std::vector<CGovernanceObject> &objs, int64_t nMoreThanTime) = 0;
    virtual int32_t getObjAbsYesCount(const CGovernanceObject& obj, vote_signal_enum_t vote_signal) = 0;
    virtual bool getObjLocalValidity(const CGovernanceObject& obj, std::string& error, bool check_collateral) = 0;
    virtual bool isEnabled() = 0;
    virtual void setContext(node::NodeContext* context) {}
};

//! Interface for the src/llmq part of a dash node (dashd process).
class LLMQ
{
public:
    virtual ~LLMQ() {}
    virtual size_t getInstantSentLockCount() = 0;
    virtual void setContext(node::NodeContext* context) {}
};

//! Interface for the src/masternode part of a dash node (dashd process).
namespace Masternode
{
class Sync
{
public:
    virtual ~Sync() {}
    virtual bool isBlockchainSynced() = 0;
    virtual bool isSynced() = 0;
    virtual std::string getSyncStatus() =  0;
    virtual void setContext(node::NodeContext* context) {}
};
}

namespace CoinJoin {
//! Interface for the global coinjoin options in src/coinjoin
class Options
{
public:
    virtual int getSessions() = 0;
    virtual int getRounds() = 0;
    virtual int getAmount() = 0;
    virtual int getDenomsGoal() = 0;
    virtual int getDenomsHardCap() = 0;

    virtual void setEnabled(bool fEnabled) = 0;
    virtual void setMultiSessionEnabled(bool fEnabled) = 0;
    virtual void setSessions(int sessions) = 0;
    virtual void setRounds(int nRounds) = 0;
    virtual void setAmount(CAmount amount) = 0;
    virtual void setDenomsGoal(int denoms_goal) = 0;
    virtual void setDenomsHardCap(int denoms_hardcap) = 0;

    virtual bool isMultiSessionEnabled() = 0;
    virtual bool isEnabled() = 0;
    // Static helpers
    virtual bool isCollateralAmount(CAmount nAmount) = 0;
    virtual CAmount getMinCollateralAmount() = 0;
    virtual CAmount getMaxCollateralAmount() = 0;
    virtual CAmount getSmallestDenomination() = 0;
    virtual bool isDenominated(CAmount nAmount) = 0;
    virtual std::array<CAmount, 5> getStandardDenominations() = 0;
};
}

//! Block and header tip information
struct BlockAndHeaderTipInfo
{
    int block_height;
    int64_t block_time;
    uint256 block_hash;
    int header_height;
    int64_t header_time;
    double verification_progress;
};

//! Top-level interface for a dash node (dashd process).
class Node
{
public:
    virtual ~Node() {}

    //! Init logging.
    virtual void initLogging() = 0;

    //! Init parameter interaction.
    virtual void initParameterInteraction() = 0;

    //! Get warnings.
    virtual bilingual_str getWarnings() = 0;

    // Get log flags.
    virtual uint64_t getLogCategories() = 0;

    //! Initialize app dependencies.
    virtual bool baseInitialize() = 0;

    //! Start node.
    virtual bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info = nullptr) = 0;

    //! Stop node.
    virtual void appShutdown() = 0;

    //! Prepare shutdown.
    virtual void appPrepareShutdown() = 0;

    //! Start shutdown.
    virtual void startShutdown() = 0;

    //! Return whether shutdown was requested.
    virtual bool shutdownRequested() = 0;

    //! Map port.
    virtual void mapPort(bool use_upnp, bool use_natpmp) = 0;

    //! Get proxy.
    virtual bool getProxy(Network net, Proxy& proxy_info) = 0;

    //! Get number of connections.
    virtual size_t getNodeCount(ConnectionDirection flags) = 0;

    //! Get stats for connected nodes.
    using NodesStats = std::vector<std::tuple<CNodeStats, bool, CNodeStateStats>>;
    virtual bool getNodesStats(NodesStats& stats) = 0;

    //! Get ban map entries.
    virtual bool getBanned(banmap_t& banmap) = 0;

    //! Ban node.
    virtual bool ban(const CNetAddr& net_addr, int64_t ban_time_offset) = 0;

    //! Unban node.
    virtual bool unban(const CSubNet& ip) = 0;

    //! Disconnect node by address.
    virtual bool disconnectByAddress(const CNetAddr& net_addr) = 0;

    //! Disconnect node by id.
    virtual bool disconnectById(NodeId id) = 0;

    //! Get total bytes recv.
    virtual int64_t getTotalBytesRecv() = 0;

    //! Get total bytes sent.
    virtual int64_t getTotalBytesSent() = 0;

    //! Get mempool size.
    virtual size_t getMempoolSize() = 0;

    //! Get mempool dynamic usage.
    virtual size_t getMempoolDynamicUsage() = 0;

    //! Get header tip height and time.
    virtual bool getHeaderTip(int& height, int64_t& block_time) = 0;

    //! Get num blocks.
    virtual int getNumBlocks() = 0;

    //! Get best block hash.
    virtual uint256 getBestBlockHash() = 0;

    //! Get last block time.
    virtual int64_t getLastBlockTime() = 0;

    //! Get last block hash.
    virtual std::string getLastBlockHash() = 0;

    //! Get verification progress.
    virtual double getVerificationProgress() = 0;

    //! Is initial block download.
    virtual bool isInitialBlockDownload() = 0;

    //! Is masternode.
    virtual bool isMasternode() = 0;

    //! Is loading blocks.
    virtual bool isLoadingBlocks() = 0;

    //! Set network active.
    virtual void setNetworkActive(bool active) = 0;

    //! Get network active.
    virtual bool getNetworkActive() = 0;

    //! Get dust relay fee.
    virtual CFeeRate getDustRelayFee() = 0;

    //! Execute rpc command.
    virtual UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) = 0;

    //! List rpc commands.
    virtual std::vector<std::string> listRpcCommands() = 0;

    //! Set RPC timer interface if unset.
    virtual void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) = 0;

    //! Unset RPC timer interface.
    virtual void rpcUnsetTimerInterface(RPCTimerInterface* iface) = 0;

    //! Get unspent outputs associated with a transaction.
    virtual bool getUnspentOutput(const COutPoint& output, Coin& coin) = 0;

    //! Broadcast transaction.
    virtual TransactionError broadcastTransaction(CTransactionRef tx, CAmount max_tx_fee, bilingual_str& err_string) = 0;

    //! Get wallet loader.
    virtual WalletLoader& walletLoader() = 0;

    //! Return interface for accessing evo related handler.
    virtual EVO& evo() = 0;

    //! Return interface for accessing governance related handler.
    virtual GOV& gov() = 0;

    //! Return interface for accessing llmq related handler.
    virtual LLMQ& llmq() = 0;

    //! Return interface for accessing masternode related handler.
    virtual Masternode::Sync& masternodeSync() = 0;

    //! Return interface for accessing coinjoin options related handler.
    virtual CoinJoin::Options& coinJoinOptions() = 0;

    //! Return interface for accessing coinjoin loader handler.
    virtual std::unique_ptr<interfaces::CoinJoin::Loader>& coinJoinLoader() = 0;

    //! Register handler for init messages.
    using InitMessageFn = std::function<void(const std::string& message)>;
    virtual std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) = 0;

    //! Register handler for message box messages.
    using MessageBoxFn =
        std::function<bool(const bilingual_str& message, const std::string& caption, unsigned int style)>;
    virtual std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) = 0;

    //! Register handler for question messages.
    using QuestionFn = std::function<bool(const bilingual_str& message,
        const std::string& non_interactive_message,
        const std::string& caption,
        unsigned int style)>;
    virtual std::unique_ptr<Handler> handleQuestion(QuestionFn fn) = 0;

    //! Register handler for progress messages.
    using ShowProgressFn = std::function<void(const std::string& title, int progress, bool resume_possible)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for wallet client constructed messages.
    using InitWalletFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) = 0;

    //! Register handler for number of connections changed messages.
    using NotifyNumConnectionsChangedFn = std::function<void(int new_num_connections)>;
    virtual std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) = 0;

    //! Register handler for network active messages.
    using NotifyNetworkActiveChangedFn = std::function<void(bool network_active)>;
    virtual std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) = 0;

    //! Register handler for notify alert messages.
    using NotifyAlertChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) = 0;

    //! Register handler for ban list messages.
    using BannedListChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) = 0;

    //! Register handler for block tip messages.
    using NotifyBlockTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip, double verification_progress)>;
    virtual std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) = 0;

    //! Register handler for chainlock messages.
    using NotifyChainLockFn =
    std::function<void(const std::string& bestChainLockedHash, int32_t bestChainLockedHeight)>;
    virtual std::unique_ptr<Handler> handleNotifyChainLock(NotifyChainLockFn fn) = 0;

    //! Register handler for header tip messages.
    using NotifyHeaderTipFn =
        std::function<void(SynchronizationState, interfaces::BlockTip tip, double verification_progress)>;
    virtual std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) = 0;

    //! Register handler for masternode list update messages.
    using NotifyMasternodeListChangedFn =
        std::function<void(const CDeterministicMNList& newList,
                const CBlockIndex* pindex)>;
    virtual std::unique_ptr<Handler> handleNotifyMasternodeListChanged(NotifyMasternodeListChangedFn fn) = 0;

    //! Register handler for additional data sync progress update messages.
    using NotifyAdditionalDataSyncProgressChangedFn =
        std::function<void(double nSyncProgress)>;
    virtual std::unique_ptr<Handler> handleNotifyAdditionalDataSyncProgressChanged(NotifyAdditionalDataSyncProgressChangedFn fn) = 0;

    //! Get and set internal node context. Useful for testing, but not
    //! accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
    virtual void setContext(node::NodeContext* context) { }
};

//! Return implementation of Node interface.
std::unique_ptr<Node> MakeNode(node::NodeContext& context);

//! Block tip (could be a header or not, depends on the subscribed signal).
struct BlockTip {
    int block_height;
    int64_t block_time;
    uint256 block_hash;
};
} // namespace interfaces

#endif // BITCOIN_INTERFACES_NODE_H
