// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>

#include <cachedb.h>
#include <amount.h>
#include <chain.h>
#include <chainparams.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <modules/masternode/masternode_sync.h>
#include <modules/masternode/masternode_config.h>
#include <modules/masternode/masternode_man.h>
#include <modules/platform/funding.h>
#include <modules/privatesend/privatesend.h>
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
#include <util/strencodings.h>
#include <util/system.h>
#include <validation.h>
#include <warnings.h>

#if defined(HAVE_CONFIG_H)
#include <config/chaincoin-config.h>
#endif

#include <atomic>
#include <univalue.h>

class CWallet;
fs::path GetWalletDir();
std::vector<fs::path> ListWalletDir();
std::vector<std::shared_ptr<CWallet>> GetWallets();

namespace interfaces {

class Wallet;

namespace {

Proposal MakeProposal(const CGovernanceObject& pGovObj)
{
    Proposal result;
    if (pGovObj.GetObjectType() == GOVERNANCE_OBJECT_PROPOSAL) {
        UniValue objResult(UniValue::VOBJ);
        UniValue dataObj(UniValue::VOBJ);
        objResult.read(pGovObj.GetDataAsPlainString());

        std::vector<UniValue> arr1 = objResult.getValues();
        std::vector<UniValue> arr2 = arr1.at( 0 ).getValues();
        dataObj = arr2.at( 1 );

        result.hash = pGovObj.GetHash();
        result.start = dataObj["start_epoch"].get_int64();
        result.end = dataObj["end_epoch"].get_int64();
        result.yes = pGovObj.GetYesCount(VOTE_SIGNAL_FUNDING);
        result.no = pGovObj.GetNoCount(VOTE_SIGNAL_FUNDING);
        result.abs_yes = pGovObj.GetAbsoluteYesCount(VOTE_SIGNAL_FUNDING);
        result.amount = dataObj["payment_amount"].get_int64();
        result.name = dataObj["name"].get_str();
        result.url = dataObj["url"].get_str();

        return result;
    }
    return {};
}

class NodeImpl : public Node
{
public:
    NodeImpl() { m_interfaces.chain = MakeChain(); }
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
    uint32_t getLogCategories() override { return g_logger->GetCategoryMask(); }
    bool baseInitialize() override
    {
        return AppInitBasicSetup() && AppInitParameterInteraction() && AppInitSanityChecks() &&
               AppInitLockDataDirectory();
    }
    bool appInitMain() override { return AppInitMain(m_interfaces); }
    void appShutdown() override
    {
        Interrupt();
        Shutdown(m_interfaces);
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
        if (g_connman) {
            g_connman->GetBanned(banmap);
            return true;
        }
        return false;
    }
    bool ban(const CNetAddr& net_addr, BanReason reason, int64_t ban_time_offset) override
    {
        if (g_connman) {
            g_connman->Ban(net_addr, reason, ban_time_offset);
            return true;
        }
        return false;
    }
    bool unban(const CSubNet& ip) override
    {
        if (g_connman) {
            g_connman->Unban(ip);
            return true;
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
    double getVerificationProgress() override
    {
        const CBlockIndex* tip;
        {
            LOCK(::cs_main);
            tip = ::chainActive.Tip();
        }
        if (!::masternodeSync.IsBlockchainSynced())
            return GuessVerificationProgress(Params().TxData(), tip);
        else
            return ::masternodeSync.getMNSyncStatus();
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
    //! Module signals

    std::string getMNSyncStatus() override { return ::masternodeSync.GetSyncStatus(); }
    bool IsMasternodelistSynced() override { return ::masternodeSync.IsMasternodeListSynced(); }
    bool MNIsBlockchainsynced() override { return ::masternodeSync.IsBlockchainSynced(); }
    bool MNIsSynced() override { return ::masternodeSync.IsSynced(); }
    int MNConfigCount() override { return ::masternodeConfig.getCount(); }
    std::vector<CMasternodeConfig::CMasternodeEntry>& MNgetEntries() override { return ::masternodeConfig.getEntries(); }

    bool MNStartAlias(std::string strAlias, std::string strErrorRet) override
    {
        for (const auto& mne : ::masternodeConfig.getEntries()) {
            if(mne.getAlias() == strAlias) {
                CMasternodeBroadcast mnb;
                if (!CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strErrorRet, mnb)) {
                    return false;
                }

                int nDoS = 0;
                if (!g_connman || !::mnodeman.CheckMnbAndUpdateMasternodeList(nullptr, mnb, nDoS, g_connman.get())) {
                    strErrorRet = "Failed to verify MNB";
                    return false;
                }
                ::mnodeman.NotifyMasternodeUpdates(g_connman.get());
                return true;
            }
        }
        strErrorRet = "Masternode not found";
        return false;
    }

    bool MNStartAll(std::string strCommand, std::string strErrorRet, int nCountSuccessful, int nCountFailed) override
    {
        for (const auto& mne : ::masternodeConfig.getEntries()) {
            std::string strError;
            CMasternodeBroadcast mnb;

            int32_t nOutputIndex = 0;
            if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
                continue;
            }

            COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), nOutputIndex);

            if(strCommand == "start-missing" && ::mnodeman.Has(outpoint)) continue;

            bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            int nDoS = 0;
            if (fSuccess && (!g_connman || !::mnodeman.CheckMnbAndUpdateMasternodeList(nullptr, mnb, nDoS, g_connman.get()))) {
                strError = "Failed to verify MNB";
                fSuccess = false;
            }
            if(fSuccess) {
                nCountSuccessful++;
                ::mnodeman.NotifyMasternodeUpdates(g_connman.get());
            } else {
                nCountFailed++;
                strErrorRet += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
            }
        }
        return true;
    }

    MasterNodeCount getMNcount() override
    {
        MasterNodeCount result;
        result.size = ::mnodeman.size();
        result.compatible = ::mnodeman.CountMasternodes(MIN_PRIVATESEND_PEER_PROTO_VERSION);
        result.enabled = ::mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION);
        result.countIPv4 = ::mnodeman.CountByIP(NET_IPV4);
        result.countIPv6 = ::mnodeman.CountByIP(NET_IPV6);
        result.countTOR = ::mnodeman.CountByIP(NET_ONION);
        return result;
    }
    Proposal getProposal(const uint256& hash) override
    {
        LOCK(governance.cs);
        CGovernanceObject* pGovObj = ::governance.FindGovernanceObject(hash);
        return MakeProposal(*pGovObj);
    }
    std::vector<Proposal> getProposals() override
    {
        std::vector<Proposal> result;
        std::vector<const CGovernanceObject*> objs = ::governance.GetAllNewerThan(0);
        for (const auto& pGovObj : objs)
        {
            if(pGovObj->GetObjectType() != GOVERNANCE_OBJECT_PROPOSAL) continue;
            result.emplace_back(MakeProposal(*pGovObj));
        }
        return result;
    }
    bool sendVoting(const uint256& hash, const std::string strVoteSignal, int& nSuccessRet, int& nFailedRet) override
    {
        return g_connman ? ::governance.VoteWithAll(hash, strVoteSignal, nSuccessRet, nFailedRet, g_connman.get()) : false;
    }
    std::string getWalletDir() override
    {
        return GetWalletDir().string();
    }
    std::vector<std::string> listWalletDir() override
    {
        std::vector<std::string> paths;
        for (auto& path : ListWalletDir()) {
            paths.push_back(path.string());
        }
        return paths;
    }
    std::vector<std::unique_ptr<Wallet>> getWallets() override
    {
        std::vector<std::unique_ptr<Wallet>> wallets;
        for (const std::shared_ptr<CWallet>& wallet : GetWallets()) {
            wallets.emplace_back(MakeWallet(wallet));
        }
        return wallets;
    }
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
        return MakeHandler(::uiInterface.LoadWallet_connect([fn](std::shared_ptr<CWallet> wallet) { fn(MakeWallet(wallet)); }));
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
            fn(initial_download, block->nHeight, block->GetBlockTime(),
                GuessVerificationProgress(Params().TxData(), block));
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](bool initial_download, const CBlockIndex* block) {
                fn(initial_download, block->nHeight, block->GetBlockTime(),
                    GuessVerificationProgress(Params().TxData(), block));
            }));
    }
    std::unique_ptr<Handler> handleMasternodeChanged(MasternodeChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyMasternodeChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleProposalChanged(ProposalChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyProposalChanged_connect(fn));
    }
    InitInterfaces m_interfaces;
};

} // namespace

std::unique_ptr<Node> MakeNode() { return MakeUnique<NodeImpl>(); }

} // namespace interfaces
