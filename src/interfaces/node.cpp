// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>

#include <addrdb.h>
#include <banman.h>
#include <chain.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <llmq/instantsend.h>
#include <mapport.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <node/context.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <rpc/server.h>
#include <shutdown.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util/check.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <warnings.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <coinjoin/coinjoin.h>
#include <coinjoin/options.h>

#include <univalue.h>

namespace interfaces {
namespace {

class EVOImpl : public EVO
{
public:
    CDeterministicMNList getListAtChainTip() override
    {
        return deterministicMNManager == nullptr ? CDeterministicMNList() : deterministicMNManager->GetListAtChainTip();
    }
};

class GOVImpl : public GOV
{
public:
    void getAllNewerThan(std::vector<CGovernanceObject> &objs, int64_t nMoreThanTime) override
    {
        if (governance == nullptr) return;
        governance->GetAllNewerThan(objs, nMoreThanTime);
    }
};

class LLMQImpl : public LLMQ
{
public:
    size_t getInstantSentLockCount() override
    {
        return llmq::quorumInstantSendManager == nullptr ? 0 : llmq::quorumInstantSendManager->GetInstantSendLockCount();
    }
};

class MasternodeSyncImpl : public Masternode::Sync
{
public:
    bool isSynced() override
    {
        return masternodeSync == nullptr ? false : masternodeSync->IsSynced();
    }
    bool isBlockchainSynced() override
    {
        return masternodeSync == nullptr ? false : masternodeSync->IsBlockchainSynced();
    }
    std::string getSyncStatus() override
    {
        return masternodeSync == nullptr ? "" : masternodeSync->GetSyncStatus();
    }
};

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
    std::array<CAmount, 5> getStandardDenominations() override
    {
        return CCoinJoin::GetStandardDenominations();
    }
};

class NodeImpl : public Node
{
public:
    EVOImpl m_evo;
    GOVImpl m_gov;
    LLMQImpl m_llmq;
    MasternodeSyncImpl m_masternodeSync;
    CoinJoinOptionsImpl m_coinjoin;

    NodeImpl(NodeContext* context) { setContext(context); }
    void initError(const bilingual_str& message) override { InitError(message); }
    bool parseParameters(int argc, const char* const argv[], std::string& error) override
    {
        return gArgs.ParseParameters(argc, argv, error);
    }
    bool readConfigFiles(std::string& error) override { return gArgs.ReadConfigFiles(error, true); }
    bool softSetArg(const std::string& arg, const std::string& value) override { return gArgs.SoftSetArg(arg, value); }
    bool softSetBoolArg(const std::string& arg, bool value) override { return gArgs.SoftSetBoolArg(arg, value); }
    void selectParams(const std::string& network) override { SelectParams(network); }
    bool initSettings(std::string& error) override { return gArgs.InitSettings(error); }
    uint64_t getAssumedBlockchainSize() override { return Params().AssumedBlockchainSize(); }
    uint64_t getAssumedChainStateSize() override { return Params().AssumedChainStateSize(); }
    std::string getNetwork() override { return Params().NetworkIDString(); }
    void initLogging() override { InitLogging(gArgs); }
    void initParameterInteraction() override { InitParameterInteraction(gArgs); }
    std::string getWarnings() override { return GetWarnings(true); }
    uint64_t getLogCategories() override { return LogInstance().GetCategoryMask(); }
    bool baseInitialize() override
    {
        return AppInitBasicSetup(gArgs) && AppInitParameterInteraction(gArgs) && AppInitSanityChecks() &&
               AppInitLockDataDirectory() && AppInitInterfaces(*m_context);
    }
    bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info) override
    {
        return AppInitMain(m_context_ref, *m_context, tip_info);
    }
    void appShutdown() override
    {
        Interrupt(*m_context);
        Shutdown(*m_context);
    }
    void appPrepareShutdown() override
    {
        Interrupt(*m_context);
        StartRestart();
        PrepareShutdown(*m_context);
    }
    void startShutdown() override { StartShutdown(); }
    bool shutdownRequested() override { return ShutdownRequested(); }
    void mapPort(bool use_upnp, bool use_natpmp) override { StartMapPort(use_upnp, use_natpmp); }
    void setupServerArgs() override { return SetupServerArgs(*m_context); }
    bool getProxy(Network net, proxyType& proxy_info) override { return GetProxy(net, proxy_info); }
    size_t getNodeCount(CConnman::NumConnections flags) override
    {
        return m_context->connman ? m_context->connman->GetNodeCount(flags) : 0;
    }
    bool getNodesStats(NodesStats& stats) override
    {
        stats.clear();

        if (m_context->connman) {
            std::vector<CNodeStats> stats_temp;
            m_context->connman->GetNodeStats(stats_temp);

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
        if (m_context->banman) {
            m_context->banman->GetBanned(banmap);
            return true;
        }
        return false;
    }
    bool ban(const CNetAddr& net_addr, int64_t ban_time_offset) override
    {
        if (m_context->banman) {
            m_context->banman->Ban(net_addr, ban_time_offset);
            return true;
        }
        return false;
    }
    bool unban(const CSubNet& ip) override
    {
        if (m_context->banman) {
            m_context->banman->Unban(ip);
            return true;
        }
        return false;
    }
    bool disconnect(const CNetAddr& net_addr) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(net_addr);
        }
        return false;
    }
    bool disconnect(NodeId id) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(id);
        }
        return false;
    }
    int64_t getTotalBytesRecv() override { return m_context->connman ? m_context->connman->GetTotalBytesRecv() : 0; }
    int64_t getTotalBytesSent() override { return m_context->connman ? m_context->connman->GetTotalBytesSent() : 0; }
    size_t getMempoolSize() override { return m_context->mempool ? m_context->mempool->size() : 0; }
    size_t getMempoolDynamicUsage() override { return m_context->mempool ? m_context->mempool->DynamicMemoryUsage() : 0; }
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
        return ::ChainActive().Height();
    }
    int64_t getLastBlockTime() override
    {
        LOCK(::cs_main);
        if (::ChainActive().Tip()) {
            return ::ChainActive().Tip()->GetBlockTime();
        }
        return Params().GenesisBlock().GetBlockTime(); // Genesis block's time of current network
    }
    std::string getLastBlockHash() override
    {
        LOCK(::cs_main);
        if (::ChainActive().Tip()) {
            return ::ChainActive().Tip()->GetBlockHash().ToString();
        }
        return Params().GenesisBlock().GetHash().ToString(); // Genesis block's hash of current network
    }
    double getVerificationProgress() override
    {
        const CBlockIndex* tip;
        {
            LOCK(::cs_main);
            tip = ::ChainActive().Tip();
        }
        return GuessVerificationProgress(Params().TxData(), tip);
    }
    bool isInitialBlockDownload() override { return ::ChainstateActive().IsInitialBlockDownload(); }
    bool getReindex() override { return ::fReindex; }
    bool getImporting() override { return ::fImporting; }
    void setNetworkActive(bool active) override
    {
        if (m_context->connman) {
            m_context->connman->SetNetworkActive(active);
        }
    }
    bool getNetworkActive() override { return m_context->connman && m_context->connman->GetNetworkActive(); }
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
        JSONRPCRequest req(m_context_ref);
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
        return ::ChainstateActive().CoinsTip().GetCoin(output, coin);
    }
    WalletClient& walletClient() override
    {
        return *Assert(m_context->wallet_client);
    }

    EVO& evo() override { return m_evo; }
    GOV& gov() override { return m_gov; }
    LLMQ& llmq() override { return m_llmq; }
    Masternode::Sync& masternodeSync() override { return m_masternodeSync; }
    CoinJoin::Options& coinJoinOptions() override { return m_coinjoin; }

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
    std::unique_ptr<Handler> handleNotifyChainLock(NotifyChainLockFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyChainLock_connect([fn](const std::string& bestChainLockHash, int bestChainLockHeight) {
            fn(bestChainLockHash, bestChainLockHeight);
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](bool initial_download, const CBlockIndex* block) {
                fn(initial_download, block->nHeight, block->GetBlockTime(), block->GetBlockHash().ToString(),
                    /* verification progress is unused when a header was received */ 0);
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
    NodeContext* context() override { return m_context; }
    void setContext(NodeContext* context) override
    {
        m_context = context;
        if (context) {
            m_context_ref = *context;
        } else {
            m_context_ref = std::nullopt;
        }
    }
    NodeContext* m_context{nullptr};
    CoreContext m_context_ref{std::nullopt};
};

} // namespace

std::unique_ptr<Node> MakeNode(NodeContext* context) { return std::make_unique<NodeImpl>(context); }

} // namespace interfaces
