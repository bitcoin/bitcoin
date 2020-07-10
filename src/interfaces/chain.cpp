// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/chain.h>

#include <chain.h>
#include <chainparams.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <net_processing.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/transaction.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <shutdown.h>
#include <sync.h>
#include <timedata.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <uint256.h>
#include <univalue.h>
#include <util/system.h>
#include <validation.h>
#include <validationinterface.h>

#include <memory>
#include <utility>

namespace interfaces {
namespace {

class LockImpl : public Chain::Lock, public UniqueLock<RecursiveMutex>
{
    Optional<int> getHeight() override
    {
        LockAssertion lock(::cs_main);
        int height = ::ChainActive().Height();
        if (height >= 0) {
            return height;
        }
        return nullopt;
    }
    Optional<int> getBlockHeight(const uint256& hash) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = LookupBlockIndex(hash);
        if (block && ::ChainActive().Contains(block)) {
            return block->nHeight;
        }
        return nullopt;
    }
    uint256 getBlockHash(int height) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = ::ChainActive()[height];
        assert(block != nullptr);
        return block->GetBlockHash();
    }
    int64_t getBlockTime(int height) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = ::ChainActive()[height];
        assert(block != nullptr);
        return block->GetBlockTime();
    }
    int64_t getBlockMedianTimePast(int height) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = ::ChainActive()[height];
        assert(block != nullptr);
        return block->GetMedianTimePast();
    }
    bool haveBlockOnDisk(int height) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = ::ChainActive()[height];
        return block && ((block->nStatus & BLOCK_HAVE_DATA) != 0) && block->nTx > 0;
    }
    Optional<int> findFirstBlockWithTimeAndHeight(int64_t time, int height, uint256* hash) override
    {
        LockAssertion lock(::cs_main);
        CBlockIndex* block = ::ChainActive().FindEarliestAtLeast(time, height);
        if (block) {
            if (hash) *hash = block->GetBlockHash();
            return block->nHeight;
        }
        return nullopt;
    }
    Optional<int> findPruned(int start_height, Optional<int> stop_height) override
    {
        LockAssertion lock(::cs_main);
        if (::fPruneMode) {
            CBlockIndex* block = stop_height ? ::ChainActive()[*stop_height] : ::ChainActive().Tip();
            while (block && block->nHeight >= start_height) {
                if ((block->nStatus & BLOCK_HAVE_DATA) == 0) {
                    return block->nHeight;
                }
                block = block->pprev;
            }
        }
        return nullopt;
    }
    Optional<int> findFork(const uint256& hash, Optional<int>* height) override
    {
        LockAssertion lock(::cs_main);
        const CBlockIndex* block = LookupBlockIndex(hash);
        const CBlockIndex* fork = block ? ::ChainActive().FindFork(block) : nullptr;
        if (height) {
            if (block) {
                *height = block->nHeight;
            } else {
                height->reset();
            }
        }
        if (fork) {
            return fork->nHeight;
        }
        return nullopt;
    }
    CBlockLocator getTipLocator() override
    {
        LockAssertion lock(::cs_main);
        return ::ChainActive().GetLocator();
    }
    Optional<int> findLocatorFork(const CBlockLocator& locator) override
    {
        LockAssertion lock(::cs_main);
        if (CBlockIndex* fork = FindForkInGlobalIndex(::ChainActive(), locator)) {
            return fork->nHeight;
        }
        return nullopt;
    }
    bool checkFinalTx(const CTransaction& tx) override
    {
        LockAssertion lock(::cs_main);
        return CheckFinalTx(tx);
    }

    using UniqueLock::UniqueLock;
};

class NotificationsProxy : public CValidationInterface
{
public:
    explicit NotificationsProxy(std::shared_ptr<Chain::Notifications> notifications)
        : m_notifications(std::move(notifications)) {}
    virtual ~NotificationsProxy() = default;
    void TransactionAddedToMempool(const CTransactionRef& tx) override
    {
        m_notifications->transactionAddedToMempool(tx);
    }
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason) override
    {
        m_notifications->transactionRemovedFromMempool(tx, reason);
    }
    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockConnected(*block, index->nHeight);
    }
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockDisconnected(*block, index->nHeight);
    }
    void UpdatedBlockTip(const CBlockIndex* index, const CBlockIndex* fork_index, bool is_ibd) override
    {
        m_notifications->updatedBlockTip();
    }
    void ChainStateFlushed(const CBlockLocator& locator) override { m_notifications->chainStateFlushed(locator); }
    std::shared_ptr<Chain::Notifications> m_notifications;
};

class NotificationsHandlerImpl : public Handler
{
public:
    explicit NotificationsHandlerImpl(std::shared_ptr<Chain::Notifications> notifications)
        : m_proxy(std::make_shared<NotificationsProxy>(std::move(notifications)))
    {
        RegisterSharedValidationInterface(m_proxy);
    }
    ~NotificationsHandlerImpl() override { disconnect(); }
    void disconnect() override
    {
        if (m_proxy) {
            UnregisterSharedValidationInterface(m_proxy);
            m_proxy.reset();
        }
    }
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
                    if (code.isNum() && code.get_int() == RPC_WALLET_NOT_FOUND) {
                        return false;
                    }
                }
                throw;
            }
        };
        ::tableRPC.appendCommand(m_command.name, &m_command);
    }

    void disconnect() override final
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
    std::unique_ptr<Chain::Lock> lock(bool try_lock) override
    {
        auto lock = MakeUnique<LockImpl>(::cs_main, "cs_main", __FILE__, __LINE__, try_lock);
        if (try_lock && lock && !*lock) return {};
        std::unique_ptr<Chain::Lock> result = std::move(lock); // Temporary to avoid CWG 1579
        return result;
    }
    bool findBlock(const uint256& hash, CBlock* block, int64_t* time, int64_t* time_max) override
    {
        CBlockIndex* index;
        {
            LOCK(cs_main);
            index = LookupBlockIndex(hash);
            if (!index) {
                return false;
            }
            if (time) {
                *time = index->GetBlockTime();
            }
            if (time_max) {
                *time_max = index->GetBlockTimeMax();
            }
        }
        if (block && !ReadBlockFromDisk(*block, index, Params().GetConsensus())) {
            block->SetNull();
        }
        return true;
    }
    void findCoins(std::map<COutPoint, Coin>& coins) override { return FindCoins(m_node, coins); }
    double guessVerificationProgress(const uint256& block_hash) override
    {
        LOCK(cs_main);
        return GuessVerificationProgress(Params().TxData(), LookupBlockIndex(block_hash));
    }
    RBFTransactionState isRBFOptIn(const CTransaction& tx) override
    {
        LOCK(::mempool.cs);
        return IsRBFOptIn(tx, ::mempool);
    }
    bool hasDescendantsInMempool(const uint256& txid) override
    {
        LOCK(::mempool.cs);
        auto it = ::mempool.GetIter(txid);
        return it && (*it)->GetCountWithDescendants() > 1;
    }
    bool broadcastTransaction(const CTransactionRef& tx,
        const CAmount& max_tx_fee,
        bool relay,
        std::string& err_string) override
    {
        const TransactionError err = BroadcastTransaction(m_node, tx, err_string, max_tx_fee, relay, /*wait_callback*/ false);
        // Chain clients only care about failures to accept the tx to the mempool. Disregard non-mempool related failures.
        // Note: this will need to be updated if BroadcastTransactions() is updated to return other non-mempool failures
        // that Chain clients do not need to know about.
        return TransactionError::OK == err;
    }
    void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants) override
    {
        ::mempool.GetTransactionAncestry(txid, ancestors, descendants);
    }
    void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) override
    {
        limit_ancestor_count = gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        limit_descendant_count = gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
    }
    bool checkChainLimits(const CTransactionRef& tx) override
    {
        LockPoints lp;
        CTxMemPoolEntry entry(tx, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries ancestors;
        auto limit_ancestor_count = gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        auto limit_ancestor_size = gArgs.GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
        auto limit_descendant_count = gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        auto limit_descendant_size = gArgs.GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
        std::string unused_error_string;
        LOCK(::mempool.cs);
        return ::mempool.CalculateMemPoolAncestors(entry, ancestors, limit_ancestor_count, limit_ancestor_size,
            limit_descendant_count, limit_descendant_size, unused_error_string);
    }
    CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc) override
    {
        return ::feeEstimator.estimateSmartFee(num_blocks, calc, conservative);
    }
    unsigned int estimateMaxBlocks() override
    {
        return ::feeEstimator.HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    }
    CFeeRate mempoolMinFee() override
    {
        return ::mempool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000);
    }
    CFeeRate relayMinFee() override { return ::minRelayTxFee; }
    CFeeRate relayIncrementalFee() override { return ::incrementalRelayFee; }
    CFeeRate relayDustFee() override { return ::dustRelayFee; }
    bool havePruned() override
    {
        LOCK(cs_main);
        return ::fHavePruned;
    }
    bool isReadyToBroadcast() override { return !::fImporting && !::fReindex && !isInitialBlockDownload(); }
    bool isInitialBlockDownload() override { return ::ChainstateActive().IsInitialBlockDownload(); }
    bool shutdownRequested() override { return ShutdownRequested(); }
    int64_t getAdjustedTime() override { return GetAdjustedTime(); }
    void initMessage(const std::string& message) override { ::uiInterface.InitMessage(message); }
    void initWarning(const std::string& message) override { InitWarning(message); }
    void initError(const std::string& message) override { InitError(message); }
    void showProgress(const std::string& title, int progress, bool resume_possible) override
    {
        ::uiInterface.ShowProgress(title, progress, resume_possible);
    }
    std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications> notifications) override
    {
        return MakeUnique<NotificationsHandlerImpl>(std::move(notifications));
    }
    void waitForNotificationsIfTipChanged(const uint256& old_tip) override
    {
        if (!old_tip.IsNull()) {
            LOCK(::cs_main);
            if (old_tip == ::ChainActive().Tip()->GetBlockHash()) return;
        }
        SyncWithValidationInterfaceQueue();
    }
    std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) override
    {
        return MakeUnique<RpcHandlerImpl>(command);
    }
    bool rpcEnableDeprecated(const std::string& method) override { return IsDeprecatedRPCEnabled(method); }
    void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) override
    {
        RPCRunLater(name, std::move(fn), seconds);
    }
    int rpcSerializationFlags() override { return RPCSerializationFlags(); }
    void requestMempoolTransactions(Notifications& notifications) override
    {
        LOCK2(::cs_main, ::mempool.cs);
        for (const CTxMemPoolEntry& entry : ::mempool.mapTx) {
            notifications.transactionAddedToMempool(entry.GetSharedTx());
        }
    }
    NodeContext& m_node;
};
} // namespace

std::unique_ptr<Chain> MakeChain(NodeContext& node) { return MakeUnique<ChainImpl>(node); }

} // namespace interfaces
