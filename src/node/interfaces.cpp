// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <banman.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <deploymentstatus.h>
#include <external_signer.h>
#include <index/blockfilterindex.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <kernel/chain.h>
#include <kernel/context.h>
#include <kernel/mempool_entry.h>
#include <logging.h>
#include <mapport.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <node/blockstorage.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <node/mini_miner.h>
#include <node/transaction.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#include <univalue.h>
#include <util/check.h>
#include <util/result.h>
#include <util/signalinterrupt.h>
#include <util/string.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <warnings.h>

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <any>
#include <memory>
#include <optional>
#include <utility>

#include <boost/signals2/signal.hpp>

using interfaces::BlockTip;
using interfaces::Chain;
using interfaces::FoundBlock;
using interfaces::Handler;
using interfaces::MakeSignalHandler;
using interfaces::Node;
using interfaces::WalletLoader;

namespace node {
// All members of the classes in this namespace are intentionally public, as the
// classes themselves are private.
namespace {
#ifdef ENABLE_EXTERNAL_SIGNER
class ExternalSignerImpl : public interfaces::ExternalSigner
{
public:
    ExternalSignerImpl(::ExternalSigner signer) : m_signer(std::move(signer)) {}
    std::string getName() override { return m_signer.m_name; }
    ::ExternalSigner m_signer;
};
#endif

class NodeImpl : public Node
{
public:
    explicit NodeImpl(NodeContext& context) { setContext(&context); }
    void initLogging() override { InitLogging(args()); }
    void initParameterInteraction() override { InitParameterInteraction(args()); }
    bilingual_str getWarnings() override { return Join(GetWarnings(), Untranslated("<hr />")); }
    int getExitStatus() override { return Assert(m_context)->exit_status.load(); }
    uint32_t getLogCategories() override { return LogInstance().GetCategoryMask(); }
    bool baseInitialize() override
    {
        if (!AppInitBasicSetup(args(), Assert(context())->exit_status)) return false;
        if (!AppInitParameterInteraction(args())) return false;

        m_context->kernel = std::make_unique<kernel::Context>();
        m_context->ecc_context = std::make_unique<ECC_Context>();
        if (!AppInitSanityChecks(*m_context->kernel)) return false;

        if (!AppInitLockDataDirectory()) return false;
        if (!AppInitInterfaces(*m_context)) return false;

        return true;
    }
    bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info) override
    {
        if (AppInitMain(*m_context, tip_info)) return true;
        // Error during initialization, set exit status before continue
        m_context->exit_status.store(EXIT_FAILURE);
        return false;
    }
    void appShutdown() override
    {
        Interrupt(*m_context);
        Shutdown(*m_context);
    }
    void startShutdown() override
    {
        if (!(*Assert(Assert(m_context)->shutdown))()) {
            LogPrintf("Error: failed to send shutdown signal\n");
        }
        // Stop RPC for clean shutdown if any of waitfor* commands is executed.
        if (args().GetBoolArg("-server", false)) {
            InterruptRPC();
            StopRPC();
        }
    }
    bool shutdownRequested() override { return ShutdownRequested(*Assert(m_context)); };
    bool isSettingIgnored(const std::string& name) override
    {
        bool ignored = false;
        args().LockSettings([&](common::Settings& settings) {
            if (auto* options = common::FindKey(settings.command_line_options, name)) {
                ignored = !options->empty();
            }
        });
        return ignored;
    }
    common::SettingsValue getPersistentSetting(const std::string& name) override { return args().GetPersistentSetting(name); }
    void updateRwSetting(const std::string& name, const common::SettingsValue& value) override
    {
        args().LockSettings([&](common::Settings& settings) {
            if (value.isNull()) {
                settings.rw_settings.erase(name);
            } else {
                settings.rw_settings[name] = value;
            }
        });
        args().WriteSettingsFile();
    }
    void forceSetting(const std::string& name, const common::SettingsValue& value) override
    {
        args().LockSettings([&](common::Settings& settings) {
            if (value.isNull()) {
                settings.forced_settings.erase(name);
            } else {
                settings.forced_settings[name] = value;
            }
        });
    }
    void resetSettings() override
    {
        args().WriteSettingsFile(/*errors=*/nullptr, /*backup=*/true);
        args().LockSettings([&](common::Settings& settings) {
            settings.rw_settings.clear();
        });
        args().WriteSettingsFile();
    }
    void mapPort(bool use_upnp, bool use_natpmp) override { StartMapPort(use_upnp, use_natpmp); }
    bool getProxy(Network net, Proxy& proxy_info) override { return GetProxy(net, proxy_info); }
    size_t getNodeCount(ConnectionDirection flags) override
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
            if (m_context->peerman) {
                TRY_LOCK(::cs_main, lockMain);
                if (lockMain) {
                    for (auto& node_stats : stats) {
                        std::get<1>(node_stats) =
                            m_context->peerman->GetNodeStateStats(std::get<0>(node_stats).nodeid, std::get<2>(node_stats));
                    }
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
    bool disconnectByAddress(const CNetAddr& net_addr) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(net_addr);
        }
        return false;
    }
    bool disconnectById(NodeId id) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(id);
        }
        return false;
    }
    std::vector<std::unique_ptr<interfaces::ExternalSigner>> listExternalSigners() override
    {
#ifdef ENABLE_EXTERNAL_SIGNER
        std::vector<ExternalSigner> signers = {};
        const std::string command = args().GetArg("-signer", "");
        if (command == "") return {};
        ExternalSigner::Enumerate(command, signers, Params().GetChainTypeString());
        std::vector<std::unique_ptr<interfaces::ExternalSigner>> result;
        result.reserve(signers.size());
        for (auto& signer : signers) {
            result.emplace_back(std::make_unique<ExternalSignerImpl>(std::move(signer)));
        }
        return result;
#else
        // This result is indistinguishable from a successful call that returns
        // no signers. For the current GUI this doesn't matter, because the wallet
        // creation dialog disables the external signer checkbox in both
        // cases. The return type could be changed to std::optional<std::vector>
        // (or something that also includes error messages) if this distinction
        // becomes important.
        return {};
#endif // ENABLE_EXTERNAL_SIGNER
    }
    int64_t getTotalBytesRecv() override { return m_context->connman ? m_context->connman->GetTotalBytesRecv() : 0; }
    int64_t getTotalBytesSent() override { return m_context->connman ? m_context->connman->GetTotalBytesSent() : 0; }
    size_t getMempoolSize() override { return m_context->mempool ? m_context->mempool->size() : 0; }
    size_t getMempoolDynamicUsage() override { return m_context->mempool ? m_context->mempool->DynamicMemoryUsage() : 0; }
    bool getHeaderTip(int& height, int64_t& block_time) override
    {
        LOCK(::cs_main);
        auto best_header = chainman().m_best_header;
        if (best_header) {
            height = best_header->nHeight;
            block_time = best_header->GetBlockTime();
            return true;
        }
        return false;
    }
    int getNumBlocks() override
    {
        LOCK(::cs_main);
        return chainman().ActiveChain().Height();
    }
    uint256 getBestBlockHash() override
    {
        const CBlockIndex* tip = WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip());
        return tip ? tip->GetBlockHash() : chainman().GetParams().GenesisBlock().GetHash();
    }
    int64_t getLastBlockTime() override
    {
        LOCK(::cs_main);
        if (chainman().ActiveChain().Tip()) {
            return chainman().ActiveChain().Tip()->GetBlockTime();
        }
        return chainman().GetParams().GenesisBlock().GetBlockTime(); // Genesis block's time of current network
    }
    double getVerificationProgress() override
    {
        return GuessVerificationProgress(chainman().GetParams().TxData(), WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip()));
    }
    bool isInitialBlockDownload() override
    {
        return chainman().IsInitialBlockDownload();
    }
    bool isLoadingBlocks() override { return chainman().m_blockman.LoadingBlocks(); }
    void setNetworkActive(bool active) override
    {
        if (m_context->connman) {
            m_context->connman->SetNetworkActive(active);
        }
    }
    bool getNetworkActive() override { return m_context->connman && m_context->connman->GetNetworkActive(); }
    CFeeRate getDustRelayFee() override
    {
        if (!m_context->mempool) return CFeeRate{DUST_RELAY_TX_FEE};
        return m_context->mempool->m_opts.dust_relay_feerate;
    }
    UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) override
    {
        JSONRPCRequest req;
        req.context = m_context;
        req.params = params;
        req.strMethod = command;
        req.URI = uri;
        return ::tableRPC.execute(req);
    }
    std::vector<std::string> listRpcCommands() override { return ::tableRPC.listCommands(); }
    void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) override { RPCSetTimerInterfaceIfUnset(iface); }
    void rpcUnsetTimerInterface(RPCTimerInterface* iface) override { RPCUnsetTimerInterface(iface); }
    std::optional<Coin> getUnspentOutput(const COutPoint& output) override
    {
        LOCK(::cs_main);
        Coin coin;
        if (chainman().ActiveChainstate().CoinsTip().GetCoin(output, coin)) return coin;
        return {};
    }
    TransactionError broadcastTransaction(CTransactionRef tx, CAmount max_tx_fee, std::string& err_string) override
    {
        return BroadcastTransaction(*m_context, std::move(tx), err_string, max_tx_fee, /*relay=*/ true, /*wait_callback=*/ false);
    }
    WalletLoader& walletLoader() override
    {
        return *Assert(m_context->wallet_loader);
    }
    std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) override
    {
        return MakeSignalHandler(::uiInterface.InitMessage_connect(fn));
    }
    std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ThreadSafeMessageBox_connect(fn));
    }
    std::unique_ptr<Handler> handleQuestion(QuestionFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ThreadSafeQuestion_connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ShowProgress_connect(fn));
    }
    std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) override
    {
        return MakeSignalHandler(::uiInterface.InitWallet_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyNumConnectionsChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyNetworkActiveChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyAlertChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.BannedListChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyBlockTip_connect([fn](SynchronizationState sync_state, const CBlockIndex* block) {
            fn(sync_state, BlockTip{block->nHeight, block->GetBlockTime(), block->GetBlockHash()},
                GuessVerificationProgress(Params().TxData(), block));
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeSignalHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](SynchronizationState sync_state, int64_t height, int64_t timestamp, bool presync) {
                fn(sync_state, BlockTip{(int)height, timestamp, uint256{}}, presync);
            }));
    }
    NodeContext* context() override { return m_context; }
    void setContext(NodeContext* context) override
    {
        m_context = context;
    }
    ArgsManager& args() { return *Assert(Assert(m_context)->args); }
    ChainstateManager& chainman() { return *Assert(m_context->chainman); }
    NodeContext* m_context{nullptr};
};

// NOLINTNEXTLINE(misc-no-recursion)
bool FillBlock(const CBlockIndex* index, const FoundBlock& block, UniqueLock<RecursiveMutex>& lock, const CChain& active, const BlockManager& blockman)
{
    if (!index) return false;
    if (block.m_hash) *block.m_hash = index->GetBlockHash();
    if (block.m_height) *block.m_height = index->nHeight;
    if (block.m_time) *block.m_time = index->GetBlockTime();
    if (block.m_max_time) *block.m_max_time = index->GetBlockTimeMax();
    if (block.m_mtp_time) *block.m_mtp_time = index->GetMedianTimePast();
    if (block.m_in_active_chain) *block.m_in_active_chain = active[index->nHeight] == index;
    if (block.m_locator) { *block.m_locator = GetLocator(index); }
    if (block.m_next_block) FillBlock(active[index->nHeight] == index ? active[index->nHeight + 1] : nullptr, *block.m_next_block, lock, active, blockman);
    if (block.m_data) {
        REVERSE_LOCK(lock);
        if (!blockman.ReadBlockFromDisk(*block.m_data, *index)) block.m_data->SetNull();
    }
    block.found = true;
    return true;
}

class NotificationsProxy : public CValidationInterface
{
public:
    explicit NotificationsProxy(std::shared_ptr<Chain::Notifications> notifications)
        : m_notifications(std::move(notifications)) {}
    virtual ~NotificationsProxy() = default;
    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override
    {
        m_notifications->transactionAddedToMempool(tx.info.m_tx);
    }
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        m_notifications->transactionRemovedFromMempool(tx, reason);
    }
    void BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockConnected(role, kernel::MakeBlockInfo(index, block.get()));
    }
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockDisconnected(kernel::MakeBlockInfo(index, block.get()));
    }
    void UpdatedBlockTip(const CBlockIndex* index, const CBlockIndex* fork_index, bool is_ibd) override
    {
        m_notifications->updatedBlockTip();
    }
    void ChainStateFlushed(ChainstateRole role, const CBlockLocator& locator) override {
        m_notifications->chainStateFlushed(role, locator);
    }
    std::shared_ptr<Chain::Notifications> m_notifications;
};

class NotificationsHandlerImpl : public Handler
{
public:
    explicit NotificationsHandlerImpl(ValidationSignals& signals, std::shared_ptr<Chain::Notifications> notifications)
        : m_signals{signals}, m_proxy{std::make_shared<NotificationsProxy>(std::move(notifications))}
    {
        m_signals.RegisterSharedValidationInterface(m_proxy);
    }
    ~NotificationsHandlerImpl() override { disconnect(); }
    void disconnect() override
    {
        if (m_proxy) {
            m_signals.UnregisterSharedValidationInterface(m_proxy);
            m_proxy.reset();
        }
    }
    ValidationSignals& m_signals;
    std::shared_ptr<NotificationsProxy> m_proxy;
};

class RpcHandlerImpl : public Handler
{
public:
    explicit RpcHandlerImpl(const CRPCCommand& command) : m_command(command), m_wrapped_command(&command)
    {
        m_command.actor = [this](const JSONRPCRequest& request, UniValue& result, bool last_handler) {
            if (!m_wrapped_command) return false;
            try {
                return m_wrapped_command->actor(request, result, last_handler);
            } catch (const UniValue& e) {
                // If this is not the last handler and a wallet not found
                // exception was thrown, return false so the next handler can
                // try to handle the request. Otherwise, reraise the exception.
                if (!last_handler) {
                    const UniValue& code = e["code"];
                    if (code.isNum() && code.getInt<int>() == RPC_WALLET_NOT_FOUND) {
                        return false;
                    }
                }
                throw;
            }
        };
        ::tableRPC.appendCommand(m_command.name, &m_command);
    }

    void disconnect() final
    {
        if (m_wrapped_command) {
            m_wrapped_command = nullptr;
            ::tableRPC.removeCommand(m_command.name, &m_command);
        }
    }

    ~RpcHandlerImpl() override { disconnect(); }

    CRPCCommand m_command;
    const CRPCCommand* m_wrapped_command;
};

class ChainImpl : public Chain
{
public:
    explicit ChainImpl(NodeContext& node) : m_node(node) {}
    std::optional<int> getHeight() override
    {
        const int height{WITH_LOCK(::cs_main, return chainman().ActiveChain().Height())};
        return height >= 0 ? std::optional{height} : std::nullopt;
    }
    uint256 getBlockHash(int height) override
    {
        LOCK(::cs_main);
        return Assert(chainman().ActiveChain()[height])->GetBlockHash();
    }
    bool haveBlockOnDisk(int height) override
    {
        LOCK(::cs_main);
        const CBlockIndex* block{chainman().ActiveChain()[height]};
        return block && ((block->nStatus & BLOCK_HAVE_DATA) != 0) && block->nTx > 0;
    }
    CBlockLocator getTipLocator() override
    {
        LOCK(::cs_main);
        return chainman().ActiveChain().GetLocator();
    }
    CBlockLocator getActiveChainLocator(const uint256& block_hash) override
    {
        LOCK(::cs_main);
        const CBlockIndex* index = chainman().m_blockman.LookupBlockIndex(block_hash);
        return GetLocator(index);
    }
    std::optional<int> findLocatorFork(const CBlockLocator& locator) override
    {
        LOCK(::cs_main);
        if (const CBlockIndex* fork = chainman().ActiveChainstate().FindForkInGlobalIndex(locator)) {
            return fork->nHeight;
        }
        return std::nullopt;
    }
    bool hasBlockFilterIndex(BlockFilterType filter_type) override
    {
        return GetBlockFilterIndex(filter_type) != nullptr;
    }
    std::optional<bool> blockFilterMatchesAny(BlockFilterType filter_type, const uint256& block_hash, const GCSFilter::ElementSet& filter_set) override
    {
        const BlockFilterIndex* block_filter_index{GetBlockFilterIndex(filter_type)};
        if (!block_filter_index) return std::nullopt;

        BlockFilter filter;
        const CBlockIndex* index{WITH_LOCK(::cs_main, return chainman().m_blockman.LookupBlockIndex(block_hash))};
        if (index == nullptr || !block_filter_index->LookupFilter(index, filter)) return std::nullopt;
        return filter.GetFilter().MatchAny(filter_set);
    }
    bool findBlock(const uint256& hash, const FoundBlock& block) override
    {
        WAIT_LOCK(cs_main, lock);
        return FillBlock(chainman().m_blockman.LookupBlockIndex(hash), block, lock, chainman().ActiveChain(), chainman().m_blockman);
    }
    bool findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, const FoundBlock& block) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        return FillBlock(active.FindEarliestAtLeast(min_time, min_height), block, lock, active, chainman().m_blockman);
    }
    bool findAncestorByHeight(const uint256& block_hash, int ancestor_height, const FoundBlock& ancestor_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        if (const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash)) {
            if (const CBlockIndex* ancestor = block->GetAncestor(ancestor_height)) {
                return FillBlock(ancestor, ancestor_out, lock, active, chainman().m_blockman);
            }
        }
        return FillBlock(nullptr, ancestor_out, lock, active, chainman().m_blockman);
    }
    bool findAncestorByHash(const uint256& block_hash, const uint256& ancestor_hash, const FoundBlock& ancestor_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash);
        const CBlockIndex* ancestor = chainman().m_blockman.LookupBlockIndex(ancestor_hash);
        if (block && ancestor && block->GetAncestor(ancestor->nHeight) != ancestor) ancestor = nullptr;
        return FillBlock(ancestor, ancestor_out, lock, chainman().ActiveChain(), chainman().m_blockman);
    }
    bool findCommonAncestor(const uint256& block_hash1, const uint256& block_hash2, const FoundBlock& ancestor_out, const FoundBlock& block1_out, const FoundBlock& block2_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        const CBlockIndex* block1 = chainman().m_blockman.LookupBlockIndex(block_hash1);
        const CBlockIndex* block2 = chainman().m_blockman.LookupBlockIndex(block_hash2);
        const CBlockIndex* ancestor = block1 && block2 ? LastCommonAncestor(block1, block2) : nullptr;
        // Using & instead of && below to avoid short circuiting and leaving
        // output uninitialized. Cast bool to int to avoid -Wbitwise-instead-of-logical
        // compiler warnings.
        return int{FillBlock(ancestor, ancestor_out, lock, active, chainman().m_blockman)} &
               int{FillBlock(block1, block1_out, lock, active, chainman().m_blockman)} &
               int{FillBlock(block2, block2_out, lock, active, chainman().m_blockman)};
    }
    void findCoins(std::map<COutPoint, Coin>& coins) override { return FindCoins(m_node, coins); }
    double guessVerificationProgress(const uint256& block_hash) override
    {
        LOCK(::cs_main);
        return GuessVerificationProgress(chainman().GetParams().TxData(), chainman().m_blockman.LookupBlockIndex(block_hash));
    }
    bool hasBlocks(const uint256& block_hash, int min_height, std::optional<int> max_height) override
    {
        // hasBlocks returns true if all ancestors of block_hash in specified
        // range have block data (are not pruned), false if any ancestors in
        // specified range are missing data.
        //
        // For simplicity and robustness, min_height and max_height are only
        // used to limit the range, and passing min_height that's too low or
        // max_height that's too high will not crash or change the result.
        LOCK(::cs_main);
        if (const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash)) {
            if (max_height && block->nHeight >= *max_height) block = block->GetAncestor(*max_height);
            for (; block->nStatus & BLOCK_HAVE_DATA; block = block->pprev) {
                // Check pprev to not segfault if min_height is too low
                if (block->nHeight <= min_height || !block->pprev) return true;
            }
        }
        return false;
    }
    RBFTransactionState isRBFOptIn(const CTransaction& tx) override
    {
        if (!m_node.mempool) return IsRBFOptInEmptyMempool(tx);
        LOCK(m_node.mempool->cs);
        return IsRBFOptIn(tx, *m_node.mempool);
    }
    bool isInMempool(const uint256& txid) override
    {
        if (!m_node.mempool) return false;
        LOCK(m_node.mempool->cs);
        return m_node.mempool->exists(GenTxid::Txid(txid));
    }
    bool hasDescendantsInMempool(const uint256& txid) override
    {
        if (!m_node.mempool) return false;
        LOCK(m_node.mempool->cs);
        const auto entry{m_node.mempool->GetEntry(Txid::FromUint256(txid))};
        if (entry == nullptr) return false;
        return entry->GetCountWithDescendants() > 1;
    }
    bool broadcastTransaction(const CTransactionRef& tx,
        const CAmount& max_tx_fee,
        bool relay,
        std::string& err_string) override
    {
        const TransactionError err = BroadcastTransaction(m_node, tx, err_string, max_tx_fee, relay, /*wait_callback=*/false);
        // Chain clients only care about failures to accept the tx to the mempool. Disregard non-mempool related failures.
        // Note: this will need to be updated if BroadcastTransactions() is updated to return other non-mempool failures
        // that Chain clients do not need to know about.
        return TransactionError::OK == err;
    }
    void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* ancestorsize, CAmount* ancestorfees) override
    {
        ancestors = descendants = 0;
        if (!m_node.mempool) return;
        m_node.mempool->GetTransactionAncestry(txid, ancestors, descendants, ancestorsize, ancestorfees);
    }

    std::map<COutPoint, CAmount> calculateIndividualBumpFees(const std::vector<COutPoint>& outpoints, const CFeeRate& target_feerate) override
    {
        if (!m_node.mempool) {
            std::map<COutPoint, CAmount> bump_fees;
            for (const auto& outpoint : outpoints) {
                bump_fees.emplace(outpoint, 0);
            }
            return bump_fees;
        }
        return MiniMiner(*m_node.mempool, outpoints).CalculateBumpFees(target_feerate);
    }

    std::optional<CAmount> calculateCombinedBumpFee(const std::vector<COutPoint>& outpoints, const CFeeRate& target_feerate) override
    {
        if (!m_node.mempool) {
            return 0;
        }
        return MiniMiner(*m_node.mempool, outpoints).CalculateTotalBumpFees(target_feerate);
    }
    void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) override
    {
        const CTxMemPool::Limits default_limits{};

        const CTxMemPool::Limits& limits{m_node.mempool ? m_node.mempool->m_opts.limits : default_limits};

        limit_ancestor_count = limits.ancestor_count;
        limit_descendant_count = limits.descendant_count;
    }
    util::Result<void> checkChainLimits(const CTransactionRef& tx) override
    {
        if (!m_node.mempool) return {};
        LockPoints lp;
        CTxMemPoolEntry entry(tx, 0, 0, 0, 0, false, 0, lp);
        LOCK(m_node.mempool->cs);
        return m_node.mempool->CheckPackageLimits({tx}, entry.GetTxSize());
    }
    CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc) override
    {
        if (!m_node.fee_estimator) return {};
        return m_node.fee_estimator->estimateSmartFee(num_blocks, calc, conservative);
    }
    unsigned int estimateMaxBlocks() override
    {
        if (!m_node.fee_estimator) return 0;
        return m_node.fee_estimator->HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    }
    CFeeRate mempoolMinFee() override
    {
        if (!m_node.mempool) return {};
        return m_node.mempool->GetMinFee();
    }
    CFeeRate relayMinFee() override
    {
        if (!m_node.mempool) return CFeeRate{DEFAULT_MIN_RELAY_TX_FEE};
        return m_node.mempool->m_opts.min_relay_feerate;
    }
    CFeeRate relayIncrementalFee() override
    {
        if (!m_node.mempool) return CFeeRate{DEFAULT_INCREMENTAL_RELAY_FEE};
        return m_node.mempool->m_opts.incremental_relay_feerate;
    }
    CFeeRate relayDustFee() override
    {
        if (!m_node.mempool) return CFeeRate{DUST_RELAY_TX_FEE};
        return m_node.mempool->m_opts.dust_relay_feerate;
    }
    bool havePruned() override
    {
        LOCK(::cs_main);
        return chainman().m_blockman.m_have_pruned;
    }
    bool isReadyToBroadcast() override { return !chainman().m_blockman.LoadingBlocks() && !isInitialBlockDownload(); }
    bool isInitialBlockDownload() override
    {
        return chainman().IsInitialBlockDownload();
    }
    bool shutdownRequested() override { return ShutdownRequested(m_node); }
    void initMessage(const std::string& message) override { ::uiInterface.InitMessage(message); }
    void initWarning(const bilingual_str& message) override { InitWarning(message); }
    void initError(const bilingual_str& message) override { InitError(message); }
    void showProgress(const std::string& title, int progress, bool resume_possible) override
    {
        ::uiInterface.ShowProgress(title, progress, resume_possible);
    }
    std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications> notifications) override
    {
        return std::make_unique<NotificationsHandlerImpl>(validation_signals(), std::move(notifications));
    }
    void waitForNotificationsIfTipChanged(const uint256& old_tip) override
    {
        if (!old_tip.IsNull() && old_tip == WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip()->GetBlockHash())) return;
        validation_signals().SyncWithValidationInterfaceQueue();
    }
    std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) override
    {
        return std::make_unique<RpcHandlerImpl>(command);
    }
    bool rpcEnableDeprecated(const std::string& method) override { return IsDeprecatedRPCEnabled(method); }
    void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) override
    {
        RPCRunLater(name, std::move(fn), seconds);
    }
    common::SettingsValue getSetting(const std::string& name) override
    {
        return args().GetSetting(name);
    }
    std::vector<common::SettingsValue> getSettingsList(const std::string& name) override
    {
        return args().GetSettingsList(name);
    }
    common::SettingsValue getRwSetting(const std::string& name) override
    {
        common::SettingsValue result;
        args().LockSettings([&](const common::Settings& settings) {
            if (const common::SettingsValue* value = common::FindKey(settings.rw_settings, name)) {
                result = *value;
            }
        });
        return result;
    }
    bool updateRwSetting(const std::string& name, const common::SettingsValue& value, bool write) override
    {
        args().LockSettings([&](common::Settings& settings) {
            if (value.isNull()) {
                settings.rw_settings.erase(name);
            } else {
                settings.rw_settings[name] = value;
            }
        });
        return !write || args().WriteSettingsFile();
    }
    void requestMempoolTransactions(Notifications& notifications) override
    {
        if (!m_node.mempool) return;
        LOCK2(::cs_main, m_node.mempool->cs);
        for (const CTxMemPoolEntry& entry : m_node.mempool->entryAll()) {
            notifications.transactionAddedToMempool(entry.GetSharedTx());
        }
    }
    bool hasAssumedValidChain() override
    {
        return chainman().IsSnapshotActive();
    }

    NodeContext* context() override { return &m_node; }
    ArgsManager& args() { return *Assert(m_node.args); }
    ChainstateManager& chainman() { return *Assert(m_node.chainman); }
    ValidationSignals& validation_signals() { return *Assert(m_node.validation_signals); }
    NodeContext& m_node;
};
} // namespace
} // namespace node

namespace interfaces {
std::unique_ptr<Node> MakeNode(node::NodeContext& context) { return std::make_unique<node::NodeImpl>(context); }
std::unique_ptr<Chain> MakeChain(node::NodeContext& context) { return std::make_unique<node::ChainImpl>(context); }
} // namespace interfaces
