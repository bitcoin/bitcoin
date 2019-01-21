// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>

#include <addrdb.h>
#include <amount.h>
#include <banman.h>
#include <chain.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <init.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <llmq/quorums_instantsend.h>
#include <masternode/masternode-sync.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <rpc/server.h>
#include <scheduler.h>
#include <shutdown.h>
#include <sync.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util/system.h>
#include <validation.h>
#include <warnings.h>

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif
#ifdef ENABLE_WALLET
#include <coinjoin/coinjoin-client-options.h>
#include <wallet/fees.h>
#include <wallet/wallet.h>
#define CHECK_WALLET(x) x
#else
#define CHECK_WALLET(x) throw std::logic_error("Wallet function called in non-wallet build.")
#endif

#include <atomic>
#include <boost/thread/thread.hpp>
#include <univalue.h>

namespace interfaces {
namespace {

class EVOImpl : public EVO
{
public:
    CDeterministicMNList getListAtChainTip() override
    {
        return deterministicMNManager->GetListAtChainTip();
    }
};

class LLMQImpl : public LLMQ
{
public:
    size_t getInstantSentLockCount() override
    {
        if (!llmq::quorumInstantSendManager) {
            return 0;
        }
        return llmq::quorumInstantSendManager->GetInstantSendLockCount();
    }
};

class MasternodeSyncImpl : public Masternode::Sync
{
public:
    bool isSynced()
    {
        return masternodeSync.IsSynced();
    }
    bool isBlockchainSynced()
    {
        return masternodeSync.IsBlockchainSynced();
    }
    std::string getSyncStatus()
    {
        return masternodeSync.GetSyncStatus();
    }
};

#ifdef ENABLE_WALLET
class CoinJoinOptionsImpl : public CoinJoin::Options
{
public:
    int getRounds() override
    {
        return CCoinJoinClientOptions::GetRounds();
    }
    int getAmount() override
    {
        return CCoinJoinClientOptions::GetAmount();
    }
    void setEnabled(bool fEnabled) override
    {
        return CCoinJoinClientOptions::SetEnabled(fEnabled);
    }
    void setMultiSessionEnabled(bool fEnabled) override
    {
        CCoinJoinClientOptions::SetMultiSessionEnabled(fEnabled);
    }
    void setRounds(int nRounds) override
    {
        CCoinJoinClientOptions::SetRounds(nRounds);
    }
    void setAmount(CAmount amount) override
    {
        CCoinJoinClientOptions::SetAmount(amount);
    }
    bool isEnabled() override
    {
        return CCoinJoinClientOptions::IsEnabled();
    }
    bool isMultiSessionEnabled() override
    {
        return CCoinJoinClientOptions::IsMultiSessionEnabled();
    }
    bool isCollateralAmount(CAmount nAmount) override
    {
        return CCoinJoin::IsCollateralAmount(nAmount);
    }
    CAmount getMinCollateralAmount() override
    {
        return CCoinJoin::GetCollateralAmount();
    }
    CAmount getMaxCollateralAmount() override
    {
        return CCoinJoin::GetMaxCollateralAmount();
    }
    CAmount getSmallestDenomination() override
    {
        return CCoinJoin::GetSmallestDenomination();
    }
    bool isDenominated(CAmount nAmount) override
    {
        return CCoinJoin::IsDenominatedAmount(nAmount);
    }
    std::vector<CAmount> getStandardDenominations() override
    {
        return CCoinJoin::GetStandardDenominations();
    }
};
#endif

class NodeImpl : public Node
{
    EVOImpl m_evo;
    LLMQImpl m_llmq;
    MasternodeSyncImpl m_masternodeSync;
#ifdef ENABLE_WALLET
    CoinJoinOptionsImpl m_coinjoin;
#endif

    bool parseParameters(int argc, const char* const argv[], std::string& error) override
    {
        return gArgs.ParseParameters(argc, argv, error);
    }
    bool readConfigFiles(std::string& error) override { return gArgs.ReadConfigFiles(error, true); }
    bool softSetArg(const std::string& arg, const std::string& value) override { return gArgs.SoftSetArg(arg, value); }
    bool softSetBoolArg(const std::string& arg, bool value) override { return gArgs.SoftSetBoolArg(arg, value); }
    void selectParams(const std::string& network) override { SelectParams(network); }
    std::string getNetwork() override { return Params().NetworkIDString(); }
    void initLogging() override { InitLogging(); }
    void initParameterInteraction() override { InitParameterInteraction(); }
    std::string getWarnings(const std::string& type) override { return GetWarnings(type); }
    uint64_t getLogCategories() override { return g_logger->GetCategoryMask(); }
    bool baseInitialize() override
    {
        return AppInitBasicSetup() && AppInitParameterInteraction() && AppInitSanityChecks() &&
               AppInitLockDataDirectory();
    }
    bool appInitMain() override { return AppInitMain(); }
    void appShutdown() override
    {
        Interrupt();
        Shutdown();
    }
    void appPrepareShutdown() override
    {
        Interrupt();
        StartRestart();
        PrepareShutdown();
    }
    void startShutdown() override { StartShutdown(); }
    bool shutdownRequested() override { return ShutdownRequested(); }
    void mapPort(bool use_upnp) override
    {
        if (use_upnp) {
            StartMapPort();
        } else {
            InterruptMapPort();
            StopMapPort();
        }
    }
    void setupServerArgs() override { return SetupServerArgs(); }
    bool getProxy(Network net, proxyType& proxy_info) override { return GetProxy(net, proxy_info); }
    size_t getNodeCount(CConnman::NumConnections flags) override
    {
        return g_connman ? g_connman->GetNodeCount(flags) : 0;
    }
    bool getNodesStats(NodesStats& stats) override
    {
        stats.clear();

        if (g_connman) {
            std::vector<CNodeStats> stats_temp;
            g_connman->GetNodeStats(stats_temp);

            stats.reserve(stats_temp.size());
            for (auto& node_stats_temp : stats_temp) {
                stats.emplace_back(std::move(node_stats_temp), false, CNodeStateStats());
            }

            // Try to retrieve the CNodeStateStats for each node.
            TRY_LOCK(::cs_main, lockMain);
            if (lockMain) {
                for (auto& node_stats : stats) {
                    std::get<1>(node_stats) =
                        GetNodeStateStats(std::get<0>(node_stats).nodeid, std::get<2>(node_stats));
                }
            }
            return true;
        }
        return false;
    }
    bool getBanned(banmap_t& banmap) override
    {
        if (g_banman) {
            g_banman->GetBanned(banmap);
            return true;
        }
        return false;
    }
    bool ban(const CNetAddr& net_addr, BanReason reason, int64_t ban_time_offset) override
    {
        if (g_banman) {
            g_banman->Ban(net_addr, reason, ban_time_offset);
            return true;
        }
        return false;
    }
    bool unban(const CSubNet& ip) override
    {
        if (g_banman) {
            g_banman->Unban(ip);
            return true;
        }
        return false;
    }
    bool disconnect(const CNetAddr& net_addr) override
    {
        if (g_connman) {
            return g_connman->DisconnectNode(net_addr);
        }
        return false;
    }
    bool disconnect(NodeId id) override
    {
        if (g_connman) {
            return g_connman->DisconnectNode(id);
        }
        return false;
    }
    int64_t getTotalBytesRecv() override { return g_connman ? g_connman->GetTotalBytesRecv() : 0; }
    int64_t getTotalBytesSent() override { return g_connman ? g_connman->GetTotalBytesSent() : 0; }
    size_t getMempoolSize() override { return ::mempool.size(); }
    size_t getMempoolDynamicUsage() override { return ::mempool.DynamicMemoryUsage(); }
    bool getHeaderTip(int& height, int64_t& block_time) override
    {
        LOCK(::cs_main);
        if (::pindexBestHeader) {
            height = ::pindexBestHeader->nHeight;
            block_time = ::pindexBestHeader->GetBlockTime();
            return true;
        }
        return false;
    }
    int getNumBlocks() override
    {
        LOCK(::cs_main);
        return ::chainActive.Height();
    }
    int64_t getLastBlockTime() override
    {
        LOCK(::cs_main);
        if (::chainActive.Tip()) {
            return ::chainActive.Tip()->GetBlockTime();
        }
        return Params().GenesisBlock().GetBlockTime(); // Genesis block's time of current network
    }
    std::string getLastBlockHash() override
    {
        LOCK(::cs_main);
        if (::chainActive.Tip()) {
            return ::chainActive.Tip()->GetBlockHash().ToString();
        }
        return Params().GenesisBlock().GetHash().ToString(); // Genesis block's hash of current network
    }
    double getVerificationProgress() override
    {
        const CBlockIndex* tip;
        {
            LOCK(::cs_main);
            tip = ::chainActive.Tip();
        }
        return GuessVerificationProgress(Params().TxData(), tip);
    }
    bool isInitialBlockDownload() override { return IsInitialBlockDownload(); }
    bool getReindex() override { return ::fReindex; }
    bool getImporting() override { return ::fImporting; }
    void setNetworkActive(bool active) override
    {
        if (g_connman) {
            g_connman->SetNetworkActive(active);
        }
    }
    bool getNetworkActive() override { return g_connman && g_connman->GetNetworkActive(); }
    CAmount getMaxTxFee() override { return ::maxTxFee; }
    CFeeRate estimateSmartFee(int num_blocks, bool conservative, int* returned_target = nullptr) override
    {
        FeeCalculation fee_calc;
        CFeeRate result = ::feeEstimator.estimateSmartFee(num_blocks, &fee_calc, conservative);
        if (returned_target) {
            *returned_target = fee_calc.returnedTarget;
        }
        return result;
    }
    CFeeRate getDustRelayFee() override { return ::dustRelayFee; }
    UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) override
    {
        JSONRPCRequest req;
        req.params = params;
        req.strMethod = command;
        req.URI = uri;
        return ::tableRPC.execute(req);
    }
    std::vector<std::string> listRpcCommands() override { return ::tableRPC.listCommands(); }
    void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) override { RPCSetTimerInterfaceIfUnset(iface); }
    void rpcUnsetTimerInterface(RPCTimerInterface* iface) override { RPCUnsetTimerInterface(iface); }
    bool getUnspentOutput(const COutPoint& output, Coin& coin) override
    {
        LOCK(::cs_main);
        return ::pcoinsTip->GetCoin(output, coin);
    }
    std::vector<std::unique_ptr<Wallet>> getWallets() override
    {
#ifdef ENABLE_WALLET
        std::vector<std::unique_ptr<Wallet>> wallets;
        for (const std::shared_ptr<CWallet>& wallet : GetWallets()) {
            wallets.emplace_back(MakeWallet(wallet));
        }
        return wallets;
#else
        throw std::logic_error("Node::getWallets() called in non-wallet build.");
#endif
    }
    EVO& evo() override { return m_evo; }
    LLMQ& llmq() override { return m_llmq; }
    Masternode::Sync& masternodeSync() override { return m_masternodeSync; }
#ifdef ENABLE_WALLET
    CoinJoin::Options& coinJoinOptions() override { return m_coinjoin; }
#endif

    std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) override
    {
        return MakeHandler(::uiInterface.InitMessage_connect(fn));
    }
    std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeMessageBox_connect(fn));
    }
    std::unique_ptr<Handler> handleQuestion(QuestionFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeQuestion_connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeHandler(::uiInterface.ShowProgress_connect(fn));
    }
    std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) override
    {
        CHECK_WALLET(
            return MakeHandler(::uiInterface.LoadWallet_connect([fn](std::shared_ptr<CWallet> wallet) { fn(MakeWallet(wallet)); })));
    }
    std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyNumConnectionsChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyNetworkActiveChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyAlertChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) override
    {
        return MakeHandler(::uiInterface.BannedListChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyBlockTip_connect([fn](bool initial_download, const CBlockIndex* block) {
            fn(initial_download, block->nHeight, block->GetBlockTime(), block->GetBlockHash().ToString(),
                GuessVerificationProgress(Params().TxData(), block));
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](bool initial_download, const CBlockIndex* block) {
                fn(initial_download, block->nHeight, block->GetBlockTime(), block->GetBlockHash().ToString(),
                    GuessVerificationProgress(Params().TxData(), block));
            }));
    }
    std::unique_ptr<Handler> handleNotifyMasternodeListChanged(NotifyMasternodeListChangedFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyMasternodeListChanged_connect([fn](const CDeterministicMNList& newList) {
                fn(newList);
            }));
    }
    std::unique_ptr<Handler> handleNotifyAdditionalDataSyncProgressChanged(NotifyAdditionalDataSyncProgressChangedFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyAdditionalDataSyncProgressChanged_connect([fn](double nSyncProgress) {
                fn(nSyncProgress);
            }));
    }
};

} // namespace

std::unique_ptr<Node> MakeNode() { return MakeUnique<NodeImpl>(); }

} // namespace interfaces
