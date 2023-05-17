// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Copyright (c) 2014-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <init.h>

#include <kernel/checks.h>
#include <kernel/mempool_options.h>
#include <kernel/mempool_persist.h>
#include <kernel/validation_cache_sizes.h>

#include <addrman.h>
#include <banman.h>
#include <base58.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <context.h>
#include <consensus/amount.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <httpserver.h>
#include <httprpc.h>
#include <chainlock/chainlock.h>
#include <chainlock/handler.h>
#include <init/common.h>
#include <interfaces/chain.h>
#include <index/addressindex.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/spentindex.h>
#include <index/timestampindex.h>
#include <index/txindex.h>
#include <interfaces/init.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <kernel/coinstats.h>
#include <mapport.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <netbase.h>
#include <netgroup.h>
#include <node/blockstorage.h>
#include <node/caches.h>
#include <node/chainstate.h>
#include <node/chainstatemanager_args.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <node/mempool_args.h>
#include <node/mempool_persist_args.h>
#include <node/miner.h>
#include <node/sync_manager.h>
#include <node/txreconciliation.h>
#include <node/validation_cache_args.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/fees_args.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <rpc/blockchain.h>
#include <rpc/register.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <scheduler.h>
#include <script/sigcache.h>
#include <script/standard.h>
#include <shutdown.h>
#include <sync.h>
#include <timedata.h>
#include <torcontrol.h>
#include <txdb.h>
#include <txmempool.h>
#include <util/asmap.h>
#include <util/error.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/syserror.h>
#include <util/system.h>
#include <util/thread.h>
#include <util/threadnames.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <walletinitinterface.h>

#include <active/context.h>
#include <active/masternode.h>
#include <bls/bls.h>
#include <coinjoin/coinjoin.h>
#include <coinjoin/server.h>
#include <coinjoin/walletman.h>
#include <dsnotificationinterface.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/specialtxman.h>
#include <flat-database.h>
#include <governance/governance.h>
#include <governance/net_governance.h>
#include <instantsend/instantsend.h>
#include <instantsend/net_instantsend.h>
#include <llmq/context.h>
#include <llmq/dkgsessionmgr.h>
#include <llmq/signing.h>
#include <llmq/net_dkg.h>
#include <llmq/net_quorum.h>
#include <llmq/net_signing.h>
#include <llmq/observer.h>
#include <llmq/options.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <masternode/utils.h>
#include <messagesigner.h>
#include <netfulfilledman.h>
#include <spork.h>
#include <stats/client.h>

#ifdef ENABLE_WALLET
#include <coinjoin/client.h>
#include <coinjoin/options.h>
#endif // ENABLE_WALLET

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <set>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32
#include <cerrno>
#include <signal.h>
#include <sys/stat.h>
#endif

#include <boost/signals2/signal.hpp>

#if ENABLE_ZMQ
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqrpc.h>
#endif

using kernel::CoinStatsHashType;
using kernel::DumpMempool;
using kernel::ValidationCacheSizes;

using node::ApplyArgsManOptions;
using node::CacheSizes;
using node::CalculateCacheSizes;
using node::DEFAULT_PERSIST_MEMPOOL;
using node::DEFAULT_PRINTPRIORITY;
using node::DEFAULT_STOPAFTERBLOCKIMPORT;
using node::g_indexes_ready_to_sync;
using node::LoadChainstate;
using node::MempoolPath;
using node::ShouldPersistMempool;
using node::NodeContext;
using node::ThreadImport;
using node::VerifyLoadedChainstate;
using node::fPruneMode;
using node::fReindex;
using node::nPruneTarget;
#ifdef ENABLE_WALLET
using wallet::DEFAULT_DISABLE_WALLET;
#endif // ENABLE_WALLET

static constexpr bool DEFAULT_PROXYRANDOMIZE{true};
static constexpr bool DEFAULT_REST_ENABLE{false};
static constexpr bool DEFAULT_I2P_ACCEPT_INCOMING{true};

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

static const char* DEFAULT_ASMAP_FILENAME="ip_asn.map";
/**
 * The PID file facilities.
 */
static const char* BITCOIN_PID_FILENAME = "dashd.pid";
/**
 * True if this process has created a PID file.
 * Used to determine whether we should remove the PID file on shutdown.
 */
static bool g_generated_pid{false};

static fs::path GetPidFile(const ArgsManager& args)
{
    return AbsPathForConfigVal(args, args.GetPathArg("-pid", BITCOIN_PID_FILENAME));
}

[[nodiscard]] static bool CreatePidFile(const ArgsManager& args)
{
    std::ofstream file{GetPidFile(args)};
    if (file) {
#ifdef WIN32
        tfm::format(file, "%d\n", GetCurrentProcessId());
#else
        tfm::format(file, "%d\n", getpid());
#endif
        g_generated_pid = true;
        return true;
    } else {
        return InitError(strprintf(_("Unable to create the PID file '%s': %s"), fs::PathToString(GetPidFile(args)), SysErrorString(errno)));
    }
}

static void RemovePidFile(const ArgsManager& args)
{
    if (!g_generated_pid) return;
    const auto pid_path{GetPidFile(args)};
    if (std::error_code error; !fs::remove(pid_path, error)) {
        std::string msg{error ? error.message() : "File does not exist"};
        LogPrintf("Unable to remove PID file (%s): %s\n", fs::PathToString(pid_path), msg);
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

//
// Thread management and startup/shutdown:
//
// The network-processing threads are all part of a thread group
// created by AppInit() or the Qt main() function.
//
// A clean exit happens when StartShutdown() or the SIGTERM
// signal handler sets ShutdownRequested(), which makes main thread's
// WaitForShutdown() interrupts the thread group.
// And then, WaitForShutdown() makes all other on-going threads
// in the thread group join the main thread.
// Shutdown() is then called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//
// Shutdown for Qt is very similar, only it uses a QTimer to detect
// ShutdownRequested() getting set, and then does the normal Qt
// shutdown thing.
//

#if HAVE_SYSTEM
static void ShutdownNotify(const ArgsManager& args)
{
    std::vector<std::thread> threads;
    for (const auto& cmd : args.GetArgs("-shutdownnotify")) {
        threads.emplace_back(runCommand, cmd);
    }
    for (auto& t : threads) {
        t.join();
    }
}
#endif

void Interrupt(NodeContext& node)
{
#if HAVE_SYSTEM
    ShutdownNotify(*node.args);
#endif
    InterruptHTTPServer();
    InterruptHTTPRPC();
    InterruptRPC();
    InterruptREST();
    InterruptTorControl();
    if (node.peerman) {
        node.peerman->InterruptHandlers();
    }
    InterruptMapPort();
    if (node.connman)
        node.connman->Interrupt();
    if (g_txindex) {
        g_txindex->Interrupt();
    }
    if (node.address_index) {
        node.address_index->Interrupt();
    }
    if (node.timestamp_index) {
        node.timestamp_index->Interrupt();
    }
    if (node.spent_index) {
        node.spent_index->Interrupt();
    }
    ForEachBlockFilterIndex([](BlockFilterIndex& index) { index.Interrupt(); });
    if (g_coin_stats_index) {
        g_coin_stats_index->Interrupt();
    }
}

/** Preparing steps before shutting down or restarting the wallet */
void PrepareShutdown(NodeContext& node)
{
    static Mutex g_shutdown_mutex;
    TRY_LOCK(g_shutdown_mutex, lock_shutdown);
    if (!lock_shutdown) return;
    LogPrintf("%s: In progress...\n", __func__);
    Assert(node.args);

    /// Note: Shutdown() must be able to handle cases in which initialization failed part of the way,
    /// for example if the data directory was found to be locked.
    /// Be sure that anything that writes files or flushes caches only does this if the respective
    /// module was initialized.
    util::ThreadRename("shutoff");
    if (node.mempool) node.mempool->AddTransactionsUpdated(1);

    StopHTTPRPC();
    StopREST();
    StopRPC();
    StopHTTPServer();

    if (node.active_ctx) node.active_ctx->Stop();
    if (node.clhandler) node.clhandler->Stop();
    if (node.peerman) node.peerman->StopHandlers();


    for (const auto& client : node.chain_clients) {
        client->flush();
    }
    StopMapPort();

    // Because these depend on each-other, we make sure that neither can be
    // using the other before destroying them.
    if (node.clhandler) UnregisterValidationInterface(node.clhandler.get());
    if (node.peerman) UnregisterValidationInterface(node.peerman.get());
    if (node.connman) node.connman->Stop();

    StopTorControl();

    // After everything has been shut down, but before things get flushed, stop the
    // CScheduler/checkqueue, threadGroup and load block thread.
    if (node.scheduler) node.scheduler->stop();
    if (node.chainman && node.chainman->m_load_block.joinable()) node.chainman->m_load_block.join();
    StopScriptCheckWorkerThreads();

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    // After the threads that potentially access these pointers have been stopped,
    // destruct and reset all to nullptr.
    node.peerman.reset();
    node.connman.reset();
    node.banman.reset();
    node.addrman.reset();
    node.netgroupman.reset();
    ::g_stats_client.reset();

    if (node.mempool && node.mempool->GetLoadTried() && ShouldPersistMempool(*node.args)) {
        DumpMempool(*node.mempool, MempoolPath(*node.args));
    }

    // Drop transactions we were still watching, and record fee estimations.
    if (node.fee_estimator) node.fee_estimator->Flush();

    // FlushStateToDisk generates a ChainStateFlushed callback, which we should avoid missing
    if (node.chainman) {
        LOCK(cs_main);
        for (Chainstate* chainstate : node.chainman->GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
            }
        }
    }

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    if (node.observer_ctx) {
        UnregisterValidationInterface(node.observer_ctx.get());
    }

    if (node.active_ctx) {
        UnregisterValidationInterface(node.active_ctx.get());
    }

    if (node.cj_walletman) {
        UnregisterValidationInterface(node.cj_walletman.get());
    }

    if (node.ds_notification_interface) {
        UnregisterValidationInterface(node.ds_notification_interface.get());
        node.ds_notification_interface.reset();
    }

    // After all scheduled tasks have been flushed, destroy pointers
    // and reset all to nullptr.
    node.observer_ctx.reset();
    node.active_ctx.reset();
    node.govman.reset();
    node.mn_sync.reset();
    node.sporkman.reset();
    node.netfulfilledman.reset();
    node.mn_metaman.reset();

    // Stop and delete all indexes only after flushing background callbacks.
    if (g_txindex) {
        g_txindex->Stop();
        g_txindex.reset();
    }
    if (node.address_index) {
        node.address_index->Stop();
        node.address_index.reset();
    }
    if (node.timestamp_index) {
        node.timestamp_index->Stop();
        node.timestamp_index.reset();
    }
    if (node.spent_index) {
        node.spent_index->Stop();
        node.spent_index.reset();
    }
    if (g_coin_stats_index) {
        g_coin_stats_index->Stop();
        g_coin_stats_index.reset();
    }
    ForEachBlockFilterIndex([](BlockFilterIndex& index) { index.Stop(); });
    DestroyAllBlockFilterIndexes();

    // Any future callbacks will be dropped. This should absolutely be safe - if
    // missing a callback results in an unrecoverable situation, unclean shutdown
    // would too. The only reason to do the above flushes is to let the wallet catch
    // up with our current chain to avoid any strange pruning edge cases and make
    // next startup faster by avoiding rescan.

    if (node.chainman) {
        LOCK(cs_main);
        for (Chainstate* chainstate : node.chainman->GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
                chainstate->ResetCoinsViews();
            }
        }
        // The mempool holds raw pointers to dmnman and isman, so it must be
        // destroyed before either manager.
        node.mempool.reset();
        node.chain_helper.reset();
        node.llmq_ctx.reset();
        node.isman.reset();
        node.dmnman.reset();
        node.evodb.reset();
    }
    for (const auto& client : node.chain_clients) {
        client->stop();
    }

#if ENABLE_ZMQ
    if (g_zmq_notification_interface) {
        UnregisterValidationInterface(g_zmq_notification_interface.get());
        g_zmq_notification_interface.reset();
    }
#endif

    node.chain_clients.clear();

    // After all wallets are removed, destroy all CoinJoin objects
    // and reset them to nullptr
    node.cj_walletman.reset();
    node.dstxman.reset();

    UnregisterAllValidationInterfaces();
    GetMainSignals().UnregisterBackgroundSignalScheduler();

    // We need to manually release our directory locks if we are expected to restart
    // because the replacement instance will start before this instance stops and the
    // global context won't be torn down in time to release the locks automatically.
    if (RestartRequested()) {
        ReleaseDirectoryLocks();
    }
}

/**
* Shutdown is split into 2 parts:
* Part 1: shut down everything but the main wallet instance (done in PrepareShutdown() )
* Part 2: delete wallet instance
*
* In case of a restart PrepareShutdown() was already called before, but this method here gets
* called implicitly when the parent object is deleted. In this case we have to skip the
* PrepareShutdown() part because it was already executed and just delete the wallet instance.
*/
void Shutdown(NodeContext& node)
{
    // Shutdown part 1: prepare shutdown
    if(!RestartRequested()) {
        PrepareShutdown(node);
    }
    // Shutdown part 2: delete wallet instance
    node.kernel.reset();
    node.mempool.reset();
    node.fee_estimator.reset();
    node.chainman.reset();
    node.scheduler.reset();

    RemovePidFile(*node.args);

    LogPrintf("%s: done\n", __func__);
}

/**
 * Signal handlers are very limited in what they are allowed to do.
 * The execution context the handler is invoked in is not guaranteed,
 * so we restrict handler operations to just touching variables:
 */
#ifndef WIN32
static void HandleSIGTERM(int)
{
    StartShutdown();
}

static void HandleSIGHUP(int)
{
    LogInstance().m_reopen_file = true;
}
#else
static BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType)
{
    StartShutdown();
    Sleep(INFINITE);
    return true;
}
#endif

#ifndef WIN32
static void registerSignalHandler(int signal, void(*handler)(int))
{
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signal, &sa, nullptr);
}
#endif

static boost::signals2::connection rpc_notify_block_change_connection;
static void OnRPCStarted()
{
    rpc_notify_block_change_connection = uiInterface.NotifyBlockTip_connect(std::bind(RPCNotifyBlockChange, std::placeholders::_2));
}

static void OnRPCStopped()
{
    rpc_notify_block_change_connection.disconnect();
    RPCNotifyBlockChange(nullptr);
    g_best_block_cv.notify_all();
    LogPrint(BCLog::RPC, "RPC stopped.\n");
}

void SetupServerArgs(ArgsManager& argsman)
{
    SetupHelpOptions(argsman);
    argsman.AddArg("-help-debug", "Print help message with debugging options and exit", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    init::AddLoggingArgs(argsman);

    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    const auto devnetBaseParams = CreateBaseChainParams(CBaseChainParams::DEVNET);
    const auto regtestBaseParams = CreateBaseChainParams(CBaseChainParams::REGTEST);
    const auto defaultChainParams = CreateChainParams(argsman, CBaseChainParams::MAIN);
    const auto testnetChainParams = CreateChainParams(argsman, CBaseChainParams::TESTNET);
    const auto devnetChainParams = CreateChainParams(argsman, CBaseChainParams::DEVNET);
    const auto regtestChainParams = CreateChainParams(argsman, CBaseChainParams::REGTEST);

    // Hidden Options
    std::vector<std::string> hidden_args = {"-dbcrashratio", "-forcecompactdb", "-printcrashinfo",
        // GUI args. These will be overwritten by SetupUIArgs for the GUI
        "-choosedatadir", "-lang=<lang>", "-min", "-resetguisettings", "-splash", "-uiplatform"};


    // Set all of the args and their help
    // When adding new options to the categories, please keep and ensure alphabetical ordering.
#if HAVE_SYSTEM
    argsman.AddArg("-alertnotify=<cmd>", "Execute command when an alert is raised (%s in cmd is replaced by message)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-assumevalid=<hex>", strprintf("If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet: %s, devnet: %s)", defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex(), devnetChainParams->GetConsensus().defaultAssumeValid.GetHex()), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksdir=<dir>", "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-fastprune", "Use smaller block files and lower minimum prune height for testing purposes", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-tinyblk", "Use smaller block files for testing purposes", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
#if HAVE_SYSTEM
    argsman.AddArg("-blocknotify=<cmd>", "Execute command when the best block changes (%s in cmd is replaced by block hash)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-blockreconstructionextratxn=<n>", strprintf("Extra transactions to keep in memory for compact block reconstructions (default: %u)", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksonly", strprintf("Whether to reject transactions from network peers. Disables automatic broadcast and rebroadcast of transactions, unless the source peer has the 'forcerelay' permission. RPC transactions are not affected. (default: %u)", DEFAULT_BLOCKSONLY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-chainlocknotify=<cmd>", "Execute command when the best chainlock changes (%s in cmd is replaced by chainlocked block hash)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-coinstatsindex", strprintf("Maintain coinstats index used by the gettxoutsetinfo RPC (default: %u)", DEFAULT_COINSTATSINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-conf=<file>", strprintf("Specify path to read-only configuration file. Relative paths will be prefixed by datadir location (only useable from command line, not configuration file) (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbbatchsize", strprintf("Maximum database write batch size in bytes (default: %u)", nDefaultDbBatchSize), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbcache=<n>", strprintf("Maximum database cache size <n> MiB (%d to %d, default: %d). In addition, unused mempool memory is shared for this cache (see -maxmempool).", nMinDbCache, nMaxDbCache, nDefaultDbCache), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-includeconf=<file>", "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-allowignoredconf", strprintf("For backwards compatibility, treat an unused %s file in the datadir as a warning, not an error.", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-loadblock=<file>", "Imports blocks from external file on startup", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxmempool=<n>", strprintf("Keep the transaction memory pool below <n> megabytes (default: %u)", DEFAULT_MAX_MEMPOOL_SIZE_MB), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxorphantxsize=<n>", strprintf("Maximum total size of all orphan transactions in megabytes (default: %u)", DEFAULT_MAX_ORPHAN_TRANSACTIONS_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxrecsigsage=<n>", strprintf("Number of seconds to keep LLMQ recovery sigs (default: %u)", llmq::DEFAULT_MAX_RECOVERED_SIGS_AGE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-mempoolexpiry=<n>", strprintf("Do not keep transactions in the mempool longer than <n> hours (default: %u)", DEFAULT_MEMPOOL_EXPIRY_HOURS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-minimumchainwork=<hex>", strprintf("Minimum work assumed to exist on a valid chain in hex (default: %s, testnet: %s, devnet: %s)", defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex(), devnetChainParams->GetConsensus().nMinimumChainWork.GetHex()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-par=<n>", strprintf("Set the number of script verification threads (0 = auto, <0 = leave that many cores free, max: %d, default: %d)",
        MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-parbls=<n>", strprintf("Set the number of BLS verification threads (0 = auto, <0 = leave that many cores free, max: %d, default: %d)",
        llmq::MAX_BLSCHECK_THREADS, llmq::DEFAULT_BLSCHECK_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-persistmempool", strprintf("Whether to save the mempool on shutdown and load on restart (default: %u)", DEFAULT_PERSIST_MEMPOOL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-pid=<file>", strprintf("Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)", BITCOIN_PID_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-prune=<n>", strprintf("Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks, and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex, -addressindex, -spentindex and -disablegovernance=false. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >%u = automatically prune block files to stay under the specified target size in MiB)", MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-settings=<file>", strprintf("Specify path to dynamic settings data file. Can be disabled with -nosettings. File is written at runtime and not meant to be edited by users (use %s instead for custom settings). Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME, BITCOIN_SETTINGS_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-syncmempool", strprintf("Sync mempool from other nodes on start (default: %u)", DEFAULT_SYNC_MEMPOOL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-startupnotify=<cmd>", "Execute command on startup.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-shutdownnotify=<cmd>", "Execute command immediately before beginning shutdown. The need for shutdown may be urgent, so be careful not to delay it long (if the command doesn't require interaction with the server, consider having it fork into the background).", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    argsman.AddArg("-addressindex", strprintf("Maintain a full address index, used to query for the balance, txids and unspent outputs for addresses (default: %u)", DEFAULT_ADDRESSINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-reindex", "Rebuild chain state and block index from the blk*.dat files on disk. This will also rebuild active optional indexes.", ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-reindex-chainstate", "Rebuild chain state from the currently indexed blocks. When in pruning mode or if blocks on disk might be corrupted, use full -reindex instead.", ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-spentindex", strprintf("Maintain a full spent index, used to query the spending txid and input index for an outpoint (default: %u)", DEFAULT_SPENTINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-timestampindex", strprintf("Maintain a timestamp index for block hashes, used to query blocks hashes by a range of timestamps (default: %u)", DEFAULT_TIMESTAMPINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-txindex", strprintf("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)", DEFAULT_TXINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::INDEXING);
    argsman.AddArg("-blockfilterindex=<type>",
                 strprintf("Maintain an index of compact filters by block (default: %s, values: %s).", DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()) +
                 " If <type> is not supplied or if <type> = 1, indexes for all known types are enabled." +
                 " Automatically enabled for masternodes with value 'basic'.",
                 ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    argsman.AddArg("-asmap=<file>", strprintf("Specify asn mapping used for bucketing of the peers (default: %s). Relative paths will be prefixed by the net-specific datadir location.", DEFAULT_ASMAP_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-addnode=<ip>", strprintf("Add a node to connect to and attempt to keep the connection open (see the addnode RPC help for more info). This option can be specified multiple times to add multiple nodes; connections are limited to %u at a time and are counted separately from the -maxconnections limit.", MAX_ADDNODE_CONNECTIONS), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-allowprivatenet", strprintf("Allow RFC1918 addresses to be relayed and connected to (default: %u)", DEFAULT_ALLOWPRIVATENET), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bantime=<n>", strprintf("Default duration (in seconds) of manually configured bans (default: %u)", DEFAULT_MISBEHAVING_BANTIME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bind=<addr>[:<port>][=onion]", strprintf("Bind to given address and always listen on it (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections (default: 127.0.0.1:%u=onion, testnet: 127.0.0.1:%u=onion, devnet: 127.0.0.1:%u=onion, regtest: 127.0.0.1:%u=onion). "
                                                      "Without an =onion bind, the default onion bind is added unless -bind is given and -listenonion is disabled; failing to bind it stops startup only with an explicit -listenonion=1, and otherwise no onion service is created. While -listenonion is enabled, a wildcard =onion bind is an error.", defaultBaseParams->OnionServiceTargetPort(), testnetBaseParams->OnionServiceTargetPort(), devnetBaseParams->OnionServiceTargetPort(), regtestBaseParams->OnionServiceTargetPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-cjdnsreachable", "If set, then this host is configured for CJDNS (connecting to fc00::/8 addresses would lead us to the CJDNS network, see doc/cjdns.md) (default: 0)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-connect=<ip>", "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-discover", "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dns", strprintf("Allow DNS lookups for -addnode, -seednode and -connect (default: %u)", DEFAULT_NAME_LOOKUP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dnsseed", strprintf("Query for peer addresses via DNS lookup, if low on addresses (default: %u unless -connect used or -maxconnections=0)", DEFAULT_DNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-externalip=<ip>", "Specify your own public address", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-fixedseeds", strprintf("Allow fixed seeds if DNS seeds don't provide peers (default: %u)", DEFAULT_FIXEDSEEDS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-forcednsseed", strprintf("Always query for peer addresses via DNS lookup (default: %u)", DEFAULT_FORCEDNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listen", strprintf("Accept connections from outside (default: %u if no -proxy, -connect or -maxconnections=0)", DEFAULT_LISTEN), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listenonion", strprintf("Automatically create Tor onion service if Tor is available and the onion bind could be bound (see -bind). If set to 1 explicitly, fail to start if the onion bind cannot be bound (default: %d)", DEFAULT_LISTEN_ONION), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxconnections=<n>", strprintf("Maintain at most <n> connections to peers (temporary service connections excluded) (default: %u). This limit does not apply to connections manually added via -addnode or the addnode RPC, which have a separate limit of %u.", DEFAULT_MAX_PEER_CONNECTIONS, MAX_ADDNODE_CONNECTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxreceivebuffer=<n>", strprintf("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXRECEIVEBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxsendbuffer=<n>", strprintf("Maximum per-connection memory usage for the send buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXSENDBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxtimeadjustment", strprintf("Maximum allowed median peer time offset adjustment. Local perspective of time may be influenced by outbound peers forward or backward by this amount (default: %u seconds).", DEFAULT_MAX_TIME_ADJUSTMENT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxuploadtarget=<n>", strprintf("Tries to keep outbound traffic under the given target per 24h. Limit does not apply to peers with 'download' permission or blocks created within past week. 0 = no limit (default: %s). Optional suffix units [k|K|m|M|g|G|t|T] (default: M). Lowercase is 1000 base while uppercase is 1024 base", DEFAULT_MAX_UPLOAD_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#if HAVE_SOCKADDR_UN
    argsman.AddArg("-onion=<ip:port|path>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy). May be a local file path prefixed with 'unix:'.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    argsman.AddArg("-onion=<ip:port>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#endif
    argsman.AddArg("-i2psam=<ip:port>", "I2P SAM proxy to reach I2P peers and accept I2P connections (default: none)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-i2pacceptincoming", strprintf("Whether to accept inbound I2P connections (default: %i). Ignored if -i2psam is not set. Listening for inbound I2P connections is done through the SAM proxy, not by binding to a local address and port.", DEFAULT_I2P_ACCEPT_INCOMING), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-onlynet=<net>", "Make automatic outbound connections only to network <net> (" + Join(GetNetworkNames(), ", ") + "). Inbound and manual connections are not affected by this option. It can be specified multiple times to allow multiple networks.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-v2transport", strprintf("Support v2 transport (default: %u)", DEFAULT_V2_TRANSPORT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerblockfilters", strprintf("Serve compact block filters to peers per BIP 157 (default: %u, automatically enabled for masternodes)", DEFAULT_PEERBLOCKFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerbloomfilters", strprintf("Support filtering of blocks and transaction with bloom filters (default: %u)", DEFAULT_PEERBLOOMFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-txreconciliation", strprintf("Enable transaction reconciliations per BIP 330 (default: %d)", DEFAULT_TXRECONCILIATION_ENABLE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peertimeout=<n>", strprintf("Specify a p2p connection timeout delay in seconds. After connecting to a peer, wait this amount of time before considering disconnection based on inactivity (minimum: 1, default: %d)", DEFAULT_PEER_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    // TODO: remove the sentence "Nodes not using ... incoming connections." once the changes from
    // https://github.com/bitcoin/bitcoin/pull/23542 have become widespread.
    argsman.AddArg("-port=<port>", strprintf("Listen for connections on <port>. Nodes not using the default ports (default: %u, testnet: %u, devnet: %u, regtest: %u) are unlikely to get incoming connections. Not relevant for I2P (see doc/i2p.md).", defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), devnetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
#if HAVE_SOCKADDR_UN
    argsman.AddArg("-proxy=<ip:port|path>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled). May be a local file path prefixed with 'unix:' if the proxy supports it.", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_ELISION, OptionsCategory::CONNECTION);
#else
    argsman.AddArg("-proxy=<ip:port>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled)", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_ELISION, OptionsCategory::CONNECTION);
#endif
    argsman.AddArg("-proxyrandomize", strprintf("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)", DEFAULT_PROXYRANDOMIZE), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-seednode=<ip>", "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-socketevents=<mode>", "Socket events mode, which must be one of 'select', 'poll', 'epoll' or 'kqueue', depending on your system (default: Linux - 'epoll', FreeBSD/Apple - 'kqueue', Windows - 'select')", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-networkactive", "Enable all P2P network activity (default: 1). Can be changed by the setnetworkactive RPC command", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-timeout=<n>", strprintf("Specify socket connection timeout in milliseconds. If an initial attempt to connect is unsuccessful after this amount of time, drop it (minimum: 1, default: %d)", DEFAULT_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-torcontrol=<ip>:<port>", strprintf("Tor control host and port to use if onion listening enabled (default: %s). If no port is specified, the default port of %i will be used.", DEFAULT_TOR_CONTROL, DEFAULT_TOR_CONTROL_PORT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-torpassword=<pass>", "Tor control port password (default: empty)", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::CONNECTION);
#ifdef USE_UPNP
    argsman.AddArg("-upnp", strprintf("Use UPnP to map the listening port (default: %u)", DEFAULT_UPNP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    hidden_args.emplace_back("-upnp");
#endif
#ifdef USE_NATPMP
    argsman.AddArg("-natpmp", strprintf("Use NAT-PMP to map the listening port (default: %u)", DEFAULT_NATPMP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    hidden_args.emplace_back("-natpmp");
#endif // USE_NATPMP
    argsman.AddArg("-whitebind=<[permissions@]addr>", "Bind to the given address and add permission flags to the peers connecting to it. "
        "Use [host]:port notation for IPv6. Allowed permissions: " + Join(NET_PERMISSIONS_DOC, ", ") + ". "
        "Specify multiple permissions separated by commas (default: download,noban,mempool,relay). Can be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    argsman.AddArg("-whitelist=<[permissions@]IP address or network>", "Add permission flags to the peers using the given IP address (e.g. 1.2.3.4) or "
        "CIDR-notated network (e.g. 1.2.3.0/24). Uses the same permissions as "
        "-whitebind. "
        "Additional flags \"in\" and \"out\" control whether permissions apply to incoming connections and/or manual (default: incoming only). "
        "Can be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    g_wallet_init_interface.AddWalletOptions(argsman);

#if ENABLE_ZMQ
    argsman.AddArg("-zmqpubhashblock=<address>", "Enable publish hash block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashchainlock=<address>", "Enable publish hash block (locked via ChainLocks) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashgovernanceobject=<address>", "Enable publish hash of governance objects (like proposals) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashgovernancevote=<address>", "Enable publish hash of governance votes in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashinstantsenddoublespend=<address>", "Enable publish transaction hashes of attempted InstantSend double spend in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashrecoveredsig=<address>", "Enable publish message hash of recovered signatures (recovered by LLMQs) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtx=<address>", "Enable publish hash transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtxlock=<address>", "Enable publish hash transaction (locked via InstantSend) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblock=<address>", "Enable publish raw block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawchainlock=<address>", "Enable publish raw block (locked via ChainLocks) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawchainlocksig=<address>", "Enable publish raw block (locked via ChainLocks) and CLSIG message in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawgovernancevote=<address>", "Enable publish raw governance objects (like proposals) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawgovernanceobject=<address>", "Enable publish raw governance votes in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawinstantsenddoublespend=<address>", "Enable publish raw transactions of attempted InstantSend double spend in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawrecoveredsig=<address>", "Enable publish raw recovered signatures (recovered by LLMQs) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtx=<address>", "Enable publish raw transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxlock=<address>", "Enable publish raw transaction (locked via InstantSend) in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxlocksig=<address>", "Enable publish raw transaction (locked via InstantSend) and ISLOCK in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequence=<address>", "Enable publish hash block and tx sequence in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashblockhwm=<n>", strprintf("Set publish hash block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashchainlockhwm=<n>", strprintf("Set publish hash chain lock outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashgovernanceobjecthwm=<n>", strprintf("Set publish hash governance object outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashgovernancevotehwm=<n>", strprintf("Set publish hash governance vote outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashinstantsenddoublespendhwm=<n>", strprintf("Set publish hash InstantSend double spend outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashrecoveredsighwm=<n>", strprintf("Set publish hash recovered signature outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtxhwm=<n>", strprintf("Set publish hash transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtxlockhwm=<n>", strprintf("Set publish hash transaction lock outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblockhwm=<n>", strprintf("Set publish raw block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawchainlockhwm=<n>", strprintf("Set publish raw chain lock outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawchainlocksighwm=<n>", strprintf("Set publish raw chain lock signature outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawgovernanceobjecthwm=<n>", strprintf("Set publish raw governance object outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawgovernancevotehwm=<n>", strprintf("Set publish raw governance vote outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawinstantsenddoublespendhwm=<n>", strprintf("Set publish raw InstantSend double spend outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawrecoveredsighwm=<n>", strprintf("Set publish raw recovered signature outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxhwm=<n>", strprintf("Set publish raw transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxlockhwm=<n>", strprintf("Set publish raw transaction lock outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxlocksighwm=<n>", strprintf("Set publish raw transaction lock signature outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequencehwm=<n>", strprintf("Set publish hash sequence message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
#else
    hidden_args.emplace_back("-zmqpubhashblock=<address>");
    hidden_args.emplace_back("-zmqpubhashchainlock=<address>");
    hidden_args.emplace_back("-zmqpubhashgovernanceobject=<address>");
    hidden_args.emplace_back("-zmqpubhashgovernancevote=<address>");
    hidden_args.emplace_back("-zmqpubhashinstantsenddoublespend=<address>");
    hidden_args.emplace_back("-zmqpubhashrecoveredsig=<address>");
    hidden_args.emplace_back("-zmqpubhashtx=<address>");
    hidden_args.emplace_back("-zmqpubhashtxlock=<address>");
    hidden_args.emplace_back("-zmqpubrawblock=<address>");
    hidden_args.emplace_back("-zmqpubrawchainlock=<address>");
    hidden_args.emplace_back("-zmqpubrawchainlocksig=<address>");
    hidden_args.emplace_back("-zmqpubrawgovernancevote=<address>");
    hidden_args.emplace_back("-zmqpubrawgovernanceobject=<address>");
    hidden_args.emplace_back("-zmqpubrawinstantsenddoublespend=<address>");
    hidden_args.emplace_back("-zmqpubrawrecoveredsig=<address>");
    hidden_args.emplace_back("-zmqpubrawtx=<address>");
    hidden_args.emplace_back("-zmqpubrawtxlock=<address>");
    hidden_args.emplace_back("-zmqpubrawtxlocksig=<address>");
    hidden_args.emplace_back("-zmqpubsequence=<n>");
    hidden_args.emplace_back("-zmqpubhashblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashchainlockhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashgovernanceobjecthwm=<n>");
    hidden_args.emplace_back("-zmqpubhashgovernancevotehwm=<n>");
    hidden_args.emplace_back("-zmqpubhashinstantsenddoublespendhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashrecoveredsighwm=<n>");
    hidden_args.emplace_back("-zmqpubhashtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashtxlockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawchainlockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawchainlocksighwm=<n>");
    hidden_args.emplace_back("-zmqpubrawgovernanceobjecthwm=<n>");
    hidden_args.emplace_back("-zmqpubrawgovernancevotehwm=<n>");
    hidden_args.emplace_back("-zmqpubrawinstantsenddoublespendhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawrecoveredsighwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxlockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxlocksighwm=<n>");
    hidden_args.emplace_back("-zmqpubsequencehwm=<n>");
#endif

    argsman.AddArg("-checkblockindex", strprintf("Do a consistency check for the block tree, and  occasionally. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkblocks=<n>", strprintf("How many blocks to check at startup (default: %u, 0 = all)", DEFAULT_CHECKBLOCKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checklevel=<n>", strprintf("How thorough the block verification of -checkblocks is: %s (0-4, default: %u)", Join(CHECKLEVEL_DOC, ", "), DEFAULT_CHECKLEVEL), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkaddrman=<n>", strprintf("Run addrman consistency checks every <n> operations. Use 0 to disable. (default: %u)", DEFAULT_ADDRMAN_CONSISTENCY_CHECKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkmempool=<n>", strprintf("Run mempool consistency checks every <n> transactions. Use 0 to disable. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkpoints", strprintf("Enable rejection of any forks from the known historical chain until block %s (default: %u)", defaultChainParams->Checkpoints().GetHeight(), DEFAULT_CHECKPOINTS_ENABLED), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-deprecatedrpc=<method>", "Allows deprecated RPC method(s) to be used", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-forceevodbrepair", "Force evodb masternode list diff verification and repair on startup, even if already repaired (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitancestorcount=<n>", strprintf("Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)", DEFAULT_ANCESTOR_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitancestorsize=<n>", strprintf("Do not accept transactions whose size with all in-mempool ancestors exceeds <n> kilobytes (default: %u)", DEFAULT_ANCESTOR_SIZE_LIMIT_KVB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitdescendantcount=<n>", strprintf("Do not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u)", DEFAULT_DESCENDANT_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitdescendantsize=<n>", strprintf("Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool descendants (default: %u).", DEFAULT_DESCENDANT_SIZE_LIMIT_KVB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopafterblockimport", strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopatheight", strprintf("Stop running after reaching the given height in the main chain (default: %u)", DEFAULT_STOPATHEIGHT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-watchquorums=<n>", strprintf("Watch and validate quorum communication (default: %u)", llmq::DEFAULT_WATCH_QUORUMS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-capturemessages", "Capture all P2P messages to disk", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-disablegovernance", strprintf("Disable governance validation (0-1, default: %u)", 0), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxsigcachesize=<n>", strprintf("Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)", DEFAULT_MAX_SIG_CACHE_BYTES >> 20), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxtipage=<n>",
                   strprintf("Maximum tip age in seconds to consider node in initial block download (default: %u)",
                             Ticks<std::chrono::seconds>(DEFAULT_MAX_TIP_AGE)),
                   ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mocktime=<n>", "Replace actual time with " + UNIX_EPOCH_TIME + " (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printpriority", strprintf("Log transaction fee per kB when mining blocks (default: %u)", DEFAULT_PRINTPRIORITY), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-pushversion", "Protocol version to report to other nodes", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-sporkaddr=<dashaddress>", "Override spork address. Only useful for regtest and devnet. Using this on mainnet or testnet will ban you.", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-sporkkey=<privatekey>", "Set the private key to be used for signing spork messages.", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-uacomment=<cmt>", "Append comment to the user agent string", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    SetupChainParamsOptions(argsman);

    argsman.AddArg("-llmq-data-recovery=<n>", strprintf("Enable automated quorum data recovery (default: %u)", llmq::DEFAULT_ENABLE_QUORUM_DATA_RECOVERY), ArgsManager::ALLOW_ANY, OptionsCategory::MASTERNODE);
    argsman.AddArg("-llmq-qvvec-sync=<quorum_name>:<mode>", strprintf("Defines from which LLMQ type the masternode should sync quorum verification vectors. Can be used multiple times with different LLMQ types. <mode>: %d (sync always from all quorums of the type defined by <quorum_name>), %d (sync from all quorums of the type defined by <quorum_name> if a member of any of the quorums)", (int32_t)llmq::QvvecSyncMode::Always, (int32_t)llmq::QvvecSyncMode::OnlyIfTypeMember), ArgsManager::ALLOW_ANY, OptionsCategory::MASTERNODE);
    argsman.AddArg("-masternodeblsprivkey=<hex>", "Set the masternode BLS private key and enable the client to act as a masternode", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::MASTERNODE);
    argsman.AddArg("-deprecated-platform-user=<user>", "Set the username for the \"platform user\", a restricted user intended to be used by Dash Platform, to the specified username.", ArgsManager::ALLOW_ANY, OptionsCategory::MASTERNODE);

    argsman.AddArg("-acceptnonstdtxn", strprintf("Relay and mine \"non-standard\" transactions (test networks only; default: %u)", DEFAULT_ACCEPT_NON_STD_TXN), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-dustrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)", CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-incrementalrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to define cost of relay, used for mempool limiting. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-acceptstalefeeestimates", strprintf("Read fee estimates even if they are stale (%sdefault: %u) fee estimates are considered stale if they are %s hours old", "regtest only; ", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES, Ticks<std::chrono::hours>(MAX_FILE_AGE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-bytespersigop", strprintf("Equivalent bytes per sigop in transactions for relay and mining (default: %u)", DEFAULT_BYTES_PER_SIGOP), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarrier", strprintf("Relay and mine data carrier transactions (default: %u)", DEFAULT_ACCEPT_DATACARRIER), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarriersize",
                   strprintf("Relay and mine transactions whose data-carrying raw scriptPubKey "
                             "is of this size or less (default: %u)",
                             MAX_OP_RETURN_RELAY),
                   ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-permitbaremultisig", strprintf("Relay non-P2SH multisig (default: %u)", DEFAULT_PERMIT_BAREMULTISIG), ArgsManager::ALLOW_ANY,
                   OptionsCategory::NODE_RELAY);
    argsman.AddArg("-minrelaytxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistforcerelay", strprintf("Add 'forcerelay' permission to whitelisted peers with default permissions. This will relay transactions even if the transactions were already in the mempool. (default: %d)", DEFAULT_WHITELISTFORCERELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistrelay", strprintf("Add 'relay' permission to whitelisted peers with default permissions. This will accept relayed transactions even when not relaying transactions (default: %d)", DEFAULT_WHITELISTRELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);

    argsman.AddArg("-blockmaxsize=<n>", strprintf("Set maximum block size in bytes (default: %d)", DEFAULT_BLOCK_MAX_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockmintxfee=<amt>", strprintf("Set lowest fee rate (in %s/kB) for transactions to be included in block creation. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockversion=<n>", "Override block version to test forking scenarios", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::BLOCK_CREATION);

    argsman.AddArg("-rest", strprintf("Accept public REST requests (default: %u)", DEFAULT_REST_ENABLE), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcallowip=<ip>", "Allow JSON-RPC connections from specified source. Valid values for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0), a network/CIDR (e.g. 1.2.3.4/24), all ipv4 (0.0.0.0/0), or all ipv6 (::/0). This option can be specified multiple times", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcauth=<userpw>", "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcuser. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcbind=<addr>[:port]", "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost, or if -rpcallowip has been specified, 0.0.0.0 and :: i.e., all addresses)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcdoccheck", strprintf("Throw a non-fatal error at runtime if the documentation for an RPC is incorrect (default: %u)", DEFAULT_RPC_DOC_CHECK), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcexternaluser=<users>", "List of comma-separated usernames for JSON-RPC external connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcexternalworkqueue=<n>", strprintf("Set the depth of the work queue to service external RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcport=<port>", strprintf("Listen for JSON-RPC connections on <port> (default: %u, testnet: %u, devnet: %u, regtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), devnetBaseParams->RPCPort(), regtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcservertimeout=<n>", strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcthreads=<n>", strprintf("Set the number of threads to service RPC calls (default: %d)", DEFAULT_HTTP_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelist=<whitelist>", "Set a whitelist to filter incoming RPC calls for a specific user. The field <whitelist> comes in the format: <USERNAME>:<rpc 1>,<rpc 2>,...,<rpc n>. If multiple whitelists are set for a given user, they are set-intersected. See -rpcwhitelistdefault documentation for information on default whitelist behavior.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelistdefault", "Sets default behavior for rpc whitelisting. Unless rpcwhitelistdefault is set to 0, if any -rpcwhitelist is set, the rpc server acts as if all rpc users are subject to empty-unless-otherwise-specified whitelists. If rpcwhitelistdefault is set to 1 and no -rpcwhitelist is set, rpc server acts as if all rpc users are subject to empty whitelists.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcworkqueue=<n>", strprintf("Set the depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-server", "Accept command line and JSON-RPC commands", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);

    argsman.AddArg("-statsbatchsize=<bytes>", strprintf("Specify the size of each batch of stats messages (default: %d)", DEFAULT_STATSD_BATCH_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
    argsman.AddArg("-statsduration=<ms>", strprintf("Specify the number of milliseconds between stats messages (default: %d)", DEFAULT_STATSD_DURATION), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
    argsman.AddArg("-statshost=<ip>", strprintf("Specify statsd host (default: %s)", DEFAULT_STATSD_HOST), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
    hidden_args.emplace_back("-statsport");
    argsman.AddArg("-statsperiod=<seconds>", strprintf("Specify the number of seconds between periodic measurements (default: %d)", DEFAULT_STATSD_PERIOD), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
    argsman.AddArg("-statsprefix=<string>", strprintf("Specify an optional string prepended to every stats key (default: %s)", DEFAULT_STATSD_PREFIX), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
    argsman.AddArg("-statssuffix=<string>", strprintf("Specify an optional string appended to every stats key (default: %s)", DEFAULT_STATSD_SUFFIX), ArgsManager::ALLOW_ANY, OptionsCategory::STATSD);
#if HAVE_DECL_FORK
    argsman.AddArg("-daemon", strprintf("Run in the background as a daemon and accept commands (default: %d)", DEFAULT_DAEMON), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-daemonwait", strprintf("Wait for initialization to be finished before exiting. This implies -daemon (default: %d)", DEFAULT_DAEMONWAIT), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-daemon");
    hidden_args.emplace_back("-daemonwait");
#endif

    // Add the hidden options
    argsman.AddHiddenArgs(hidden_args);
}

static bool fHaveGenesis = false;
static GlobalMutex g_genesis_wait_mutex;
static std::condition_variable g_genesis_wait_cv;

static void BlockNotifyGenesisWait(const CBlockIndex* pBlockIndex)
{
    if (pBlockIndex != nullptr) {
        {
            LOCK(g_genesis_wait_mutex);
            fHaveGenesis = true;
        }
        g_genesis_wait_cv.notify_all();
    }
}

#if HAVE_SYSTEM
static void StartupNotify(const ArgsManager& args)
{
    std::string cmd = args.GetArg("-startupnotify", "");
    if (!cmd.empty()) {
        std::thread t(runCommand, cmd);
        t.detach(); // thread runs free
    }
}
#endif

static void PeriodicStats(NodeContext& node)
{
    assert(::g_stats_client->active());
    ChainstateManager& chainman = *Assert(node.chainman);
    const CTxMemPool& mempool = *Assert(node.mempool);
    const llmq::CInstantSendManager& isman = *Assert(node.isman);
    chainman.ActiveChainstate().ForceFlushStateToDisk();
    const auto maybe_stats = WITH_LOCK(::cs_main, return GetUTXOStats(&chainman.ActiveChainstate().CoinsDB(), chainman.m_blockman, /*hash_type=*/CoinStatsHashType::NONE, node.rpc_interruption_point, chainman.ActiveChain().Tip(), /*index_requested=*/true));
    if (maybe_stats.has_value()) {
        ::g_stats_client->gauge("utxoset.tx", maybe_stats->nTransactions, 1.0f);
        ::g_stats_client->gauge("utxoset.txOutputs", maybe_stats->nTransactionOutputs, 1.0f);
        ::g_stats_client->gauge("utxoset.dbSizeBytes", maybe_stats->nDiskSize, 1.0f);
        ::g_stats_client->gauge("utxoset.blockHeight", maybe_stats->nHeight, 1.0f);
        if (maybe_stats->total_amount.has_value()) {
            ::g_stats_client->gauge("utxoset.totalAmount", (double)maybe_stats->total_amount.value() / (double)COIN, 1.0f);
        }
    } else {
        // something went wrong
        LogPrintf("%s: GetUTXOStats failed\n", __func__);
    }

    CBlockIndex *tip = WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip());
    double nNetworkHashPS = [&]() {
        // Short version of GetNetworkHashPS(120, -1);
        CBlockIndex *pindex = tip;
        int64_t minTime = pindex->GetBlockTime();
        int64_t maxTime = minTime;
        for (int i = 0; i < 120 && pindex->pprev != nullptr; i++) {
            pindex = pindex->pprev;
            int64_t time = pindex->GetBlockTime();
            minTime = std::min(time, minTime);
            maxTime = std::max(time, maxTime);
        }
        if (minTime == maxTime) return 0.0;
        arith_uint256 workDiff = tip->nChainWork - pindex->nChainWork;
        int64_t timeDiff = maxTime - minTime;
        return workDiff.getdouble() / timeDiff;
    }();

    if (nNetworkHashPS > 0.0) {
        ::g_stats_client->gaugeDouble("network.hashesPerSecond", nNetworkHashPS);
        ::g_stats_client->gaugeDouble("network.terahashesPerSecond", nNetworkHashPS / 1e12);
        ::g_stats_client->gaugeDouble("network.petahashesPerSecond", nNetworkHashPS / 1e15);
        ::g_stats_client->gaugeDouble("network.exahashesPerSecond", nNetworkHashPS / 1e18);
    }
    // No need for cs_main, we never use null tip here
    ::g_stats_client->gaugeDouble("network.difficulty", (double)GetDifficulty(tip));

    ::g_stats_client->gauge("transactions.txCacheSize", WITH_LOCK(cs_main, return chainman.ActiveChainstate().CoinsTip().GetCacheSize()), 1.0f);
    ::g_stats_client->gauge("transactions.totalTransactions", tip->nChainTx, 1.0f);

    {
        LOCK(mempool.cs);
        ::g_stats_client->gauge("transactions.mempool.totalTransactions", mempool.size(), 1.0f);
        ::g_stats_client->gauge("transactions.mempool.totalTxBytes", (int64_t) mempool.GetTotalTxSize(), 1.0f);
        ::g_stats_client->gauge("transactions.mempool.memoryUsageBytes", (int64_t) mempool.DynamicMemoryUsage(), 1.0f);
        ::g_stats_client->gauge("transactions.mempool.minFeePerKb", mempool.GetMinFee().GetFeePerK(), 1.0f);
    }
    ::g_stats_client->gauge("transactions.mempool.lockedTransactions", isman.GetInstantSendLockCount(), 1.0f);
}

static bool AppInitServers(NodeContext& node)
{
    const ArgsManager& args = *Assert(node.args);
    RPCServer::OnStarted(&OnRPCStarted);
    RPCServer::OnStopped(&OnRPCStopped);
    if (!InitHTTPServer())
        return false;
    StartRPC();
    node.rpc_interruption_point = RpcInterruptionPoint;
    if (!StartHTTPRPC(node))
        return false;
    if (args.GetBoolArg("-rest", DEFAULT_REST_ENABLE)) StartREST(node);
    StartHTTPServer();
    return true;
}

// Parameter interaction based on rules
void InitParameterInteraction(ArgsManager& args)
{
    // when specifying an explicit binding address, you want to listen on it
    // even when -connect or -proxy is specified
    if (args.IsArgSet("-bind")) {
        if (args.SoftSetBoolArg("-listen", true))
            LogPrintf("%s: parameter interaction: -bind set -> setting -listen=1\n", __func__);
    }
    if (args.IsArgSet("-whitebind")) {
        if (args.SoftSetBoolArg("-listen", true))
            LogPrintf("%s: parameter interaction: -whitebind set -> setting -listen=1\n", __func__);
    }

    if (args.IsArgSet("-connect") || args.GetIntArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS) <= 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (args.SoftSetBoolArg("-dnsseed", false))
            LogPrintf("%s: parameter interaction: -connect or -maxconnections=0 set -> setting -dnsseed=0\n", __func__);
        if (args.SoftSetBoolArg("-listen", false))
            LogPrintf("%s: parameter interaction: -connect or -maxconnections=0 set -> setting -listen=0\n", __func__);
    }

    std::string proxy_arg = args.GetArg("-proxy", "");
    if (proxy_arg != "" && proxy_arg != "0") {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (args.SoftSetBoolArg("-listen", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -listen=0\n", __func__);
        // to protect privacy, do not map ports when a proxy is set. The user may still specify -listen=1
        // to listen locally, so don't rely on this happening through -listen below.
        if (args.SoftSetBoolArg("-upnp", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -upnp=0\n", __func__);
        if (args.SoftSetBoolArg("-natpmp", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -natpmp=0\n", __func__);
        // to protect privacy, do not discover addresses by default
        if (args.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -discover=0\n", __func__);
    }

    if (!args.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (args.SoftSetBoolArg("-upnp", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -upnp=0\n", __func__);
        if (args.SoftSetBoolArg("-natpmp", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -natpmp=0\n", __func__);
        if (args.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -discover=0\n", __func__);
        if (args.SoftSetBoolArg("-listenonion", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -listenonion=0\n", __func__);
        if (args.SoftSetBoolArg("-i2pacceptincoming", false)) {
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -i2pacceptincoming=0\n", __func__);
        }
    }

    if (args.IsArgSet("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (args.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -externalip set -> setting -discover=0\n", __func__);
    }

    if (args.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)) {
        // disable whitelistrelay in blocksonly mode
        if (args.SoftSetBoolArg("-whitelistrelay", false))
            LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -whitelistrelay=0\n", __func__);
        // Reduce default mempool size in blocksonly mode to avoid unexpected resource usage
        if (args.SoftSetArg("-maxmempool", ToString(DEFAULT_BLOCKSONLY_MAX_MEMPOOL_SIZE_MB)))
            LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -maxmempool=%d\n", __func__, DEFAULT_BLOCKSONLY_MAX_MEMPOOL_SIZE_MB);
    }

    // Forcing relay from whitelisted hosts implies we will accept relays from them in the first place.
    if (args.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY)) {
        if (args.SoftSetBoolArg("-whitelistrelay", true))
            LogPrintf("%s: parameter interaction: -whitelistforcerelay=1 -> setting -whitelistrelay=1\n", __func__);
    }

    if (args.IsArgSet("-onlynet")) {
        const auto onlynets = args.GetArgs("-onlynet");
        bool clearnet_reachable = std::any_of(onlynets.begin(), onlynets.end(), [](const auto& net) {
            const auto n = ParseNetwork(net);
            return n == NET_IPV4 || n == NET_IPV6;
        });
        if (!clearnet_reachable && args.SoftSetBoolArg("-dnsseed", false)) {
            LogPrintf("%s: parameter interaction: -onlynet excludes IPv4 and IPv6 -> setting -dnsseed=0\n", __func__);
        }
    }

    int64_t nPruneArg = args.GetIntArg("-prune", 0);
    if (nPruneArg > 0) {
        if (args.SoftSetBoolArg("-disablegovernance", true)) {
            LogPrintf("%s: parameter interaction: -prune=%d -> setting -disablegovernance=true\n", __func__, nPruneArg);
        }
        if (args.SoftSetBoolArg("-txindex", false)) {
            LogPrintf("%s: parameter interaction: -prune=%d -> setting -txindex=false\n", __func__, nPruneArg);
        }
    }

    if (args.IsArgSet("-masternodeblsprivkey")) {
        if (args.SoftSetBoolArg("-disablewallet", true)) {
            LogPrintf("%s: parameter interaction: -masternodeblsprivkey set -> setting -disablewallet=1\n", __func__);
        }
        // Enable block filters for masternodes to improve network services
        if (args.SoftSetBoolArg("-peerblockfilters", true)) {
            LogPrintf("%s: parameter interaction: -masternodeblsprivkey set -> setting -peerblockfilters=1\n", __func__);
        }
        if (args.SoftSetArg("-blockfilterindex", "basic")) {
            LogPrintf("%s: parameter interaction: -masternodeblsprivkey set -> setting -blockfilterindex=basic\n", __func__);
        }
    }
}

/**
 * Initialize global loggers.
 *
 * Note that this is called very early in the process lifetime, so you should be
 * careful about what global state you rely on here.
 */
void InitLogging(const ArgsManager& args)
{
    init::SetLoggingOptions(args);
    init::LogPackageVersion();
}

namespace { // Variables internal to initialization process only

int nMaxConnections;
int nUserMaxConnections;
int nFD;
ServiceFlags nLocalServices = ServiceFlags(NODE_NETWORK_LIMITED | NODE_HEADERS_COMPRESSED);
int64_t peer_connect_timeout;
std::set<BlockFilterType> g_enabled_filter_types;

} // namespace

[[noreturn]] static void new_handler_terminate()
{
    // Rather than throwing std::bad-alloc if allocation fails, terminate
    // immediately to (try to) avoid chain corruption.
    // Since LogPrintf may itself allocate memory, set the handler directly
    // to terminate first.
    std::set_new_handler(std::terminate);
    LogPrintf("Error: Out of memory. Terminating.\n");

    // The log was successful, terminate now.
    std::terminate();
};

bool AppInitBasicSetup(const ArgsManager& args, std::atomic<int>& exit_status)
{
    // ********************************************************* Step 1: setup
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, 0));
    // Disable confusing "helpful" text message on abort, Ctrl-C
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef WIN32
    // Enable heap terminate-on-corruption
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif
    if (!InitShutdownState(exit_status)) {
        return InitError(Untranslated("Initializing wait-for-shutdown state failed."));
    }

    if (!SetupNetworking()) {
        return InitError(Untranslated("Initializing networking failed."));
    }

#ifndef WIN32
    // Clean shutdown on SIGTERM
    registerSignalHandler(SIGTERM, HandleSIGTERM);
    registerSignalHandler(SIGINT, HandleSIGTERM);

    // Reopen debug.log on SIGHUP
    registerSignalHandler(SIGHUP, HandleSIGHUP);

    // Ignore SIGPIPE, otherwise it will bring the daemon down if the client closes unexpectedly
    signal(SIGPIPE, SIG_IGN);
#else
    SetConsoleCtrlHandler(consoleCtrlHandler, true);
#endif

    std::set_new_handler(new_handler_terminate);

    return true;
}

bool AppInitParameterInteraction(const ArgsManager& args)
{
    const CChainParams& chainparams = Params();
    // ********************************************************* Step 2: parameter interactions

    // also see: InitParameterInteraction()

    // Error if network-specific options (-addnode, -connect, etc) are
    // specified in default section of config file, but not overridden
    // on the command line or in this network's section of the config file.
    std::string network = args.GetChainName();
    bilingual_str errors;
    for (const auto& arg : args.GetUnsuitableSectionOnlyArgs()) {
        errors += strprintf(_("Config setting for %s only applied on %s network when in [%s] section.") + Untranslated("\n"), arg, network, network);
    }

    if (!errors.empty()) {
        return InitError(errors);
    }

    // Warn if unrecognized section name are present in the config file.
    bilingual_str warnings;
    for (const auto& section : args.GetUnrecognizedSections()) {
        warnings += strprintf(Untranslated("%s:%i ") + _("Section [%s] is not recognized.") + Untranslated("\n"), section.m_file, section.m_line, section.m_name);
    }

    if (!warnings.empty()) {
        InitWarning(warnings);
    }

    if (!fs::is_directory(gArgs.GetBlocksDirPath())) {
        return InitError(strprintf(_("Specified blocks directory \"%s\" does not exist."), args.GetArg("-blocksdir", "")));
    }

    // parse and validate enabled filter types
    std::string blockfilterindex_value = args.GetArg("-blockfilterindex", DEFAULT_BLOCKFILTERINDEX);
    if (blockfilterindex_value == "" || blockfilterindex_value == "1") {
        g_enabled_filter_types = AllBlockFilterTypes();
    } else if (blockfilterindex_value != "0") {
        const std::vector<std::string> names = args.GetArgs("-blockfilterindex");
        for (const auto& name : names) {
            BlockFilterType filter_type;
            if (!BlockFilterTypeByName(name, filter_type)) {
                return InitError(strprintf(_("Unknown -blockfilterindex value %s."), name));
            }
            g_enabled_filter_types.insert(filter_type);
        }
    }

    // Signal NODE_P2P_V2 if BIP324 v2 transport is enabled.
    if (args.GetBoolArg("-v2transport", DEFAULT_V2_TRANSPORT)) {
        nLocalServices = ServiceFlags(nLocalServices | NODE_P2P_V2);
    }

    // Signal NODE_COMPACT_FILTERS if peerblockfilters and basic filters index are both enabled.
    if (args.GetBoolArg("-peerblockfilters", DEFAULT_PEERBLOCKFILTERS)) {
        if (g_enabled_filter_types.count(BlockFilterType::BASIC_FILTER) != 1) {
            return InitError(_("Cannot set -peerblockfilters without -blockfilterindex."));
        }

        nLocalServices = ServiceFlags(nLocalServices | NODE_COMPACT_FILTERS);
    }

    if (args.GetIntArg("-prune", 0)) {
        if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX))
            return InitError(_("Prune mode is incompatible with -txindex."));
        if (args.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX))
            return InitError(_("Prune mode is incompatible with -addressindex."));
        if (args.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX))
            return InitError(_("Prune mode is incompatible with -spentindex."));
        if (args.GetBoolArg("-reindex-chainstate", false)) {
            return InitError(_("Prune mode is incompatible with -reindex-chainstate. Use full -reindex instead."));
        }
        if (!args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE)) {
            return InitError(_("Prune mode is incompatible with -disablegovernance=false."));
        }
    }

    if (args.IsArgSet("-devnet")) {
        // Require setting of ports when running devnet
        if (args.GetBoolArg("-listen", DEFAULT_LISTEN) && !args.IsArgSet("-port")) {
            return InitError(_("-port must be specified when -devnet and -listen are specified"));
        }
        if (args.GetBoolArg("-server", false) && !args.IsArgSet("-rpcport")) {
            return InitError(_("-rpcport must be specified when -devnet and -server are specified"));
        }
        if (args.GetArgs("-devnet").size() > 1) {
            return InitError(_("-devnet can only be specified once"));
        }
    }

    fAllowPrivateNet = args.GetBoolArg("-allowprivatenet", DEFAULT_ALLOWPRIVATENET);

    // If -forcednsseed is set to true, ensure -dnsseed has not been set to false
    if (args.GetBoolArg("-forcednsseed", DEFAULT_FORCEDNSSEED) && !args.GetBoolArg("-dnsseed", DEFAULT_DNSSEED)){
        return InitError(_("Cannot set -forcednsseed to true when setting -dnsseed to false."));
    }

    // -bind and -whitebind can't be set when not listening
    size_t nUserBind = args.GetArgs("-bind").size() + args.GetArgs("-whitebind").size();
    if (nUserBind != 0 && !args.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        return InitError(Untranslated("Cannot set -bind or -whitebind together with -listen=0"));
    }

    // if listen=0, then disallow listenonion=1
    if (!args.GetBoolArg("-listen", DEFAULT_LISTEN) && args.GetBoolArg("-listenonion", DEFAULT_LISTEN_ONION)) {
        return InitError(Untranslated("Cannot set -listen=0 together with -listenonion=1"));
    }

    // Make sure enough file descriptors are available
    int nBind = std::max(nUserBind, size_t(1));
    nUserMaxConnections = args.GetIntArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS);
    nMaxConnections = std::max(nUserMaxConnections, 0);

    nFD = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS + MAX_ADDNODE_CONNECTIONS + nBind + NUM_FDS_MESSAGE_CAPTURE);

#ifdef USE_POLL
    int fd_max = nFD;
#else
    int fd_max = FD_SETSIZE;
#endif
    // Trim requested connection counts, to fit into system limitations
    // <int> in std::min<int>(...) to work around FreeBSD compilation issue described in #2695
    nMaxConnections = std::max(std::min<int>(nMaxConnections, fd_max - nBind - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS - NUM_FDS_MESSAGE_CAPTURE), 0);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available."));
    nMaxConnections = std::min(nFD - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS - NUM_FDS_MESSAGE_CAPTURE, nMaxConnections);

    if (nMaxConnections < nUserMaxConnections)
        InitWarning(strprintf(_("Reducing -maxconnections from %d to %d, because of system limitations."), nUserMaxConnections, nMaxConnections));

    // ********************************************************* Step 3: parameter-to-internal-flags
    auto result = init::SetLoggingCategories(args);
    if (!result) return InitError(util::ErrorString(result));
    result = init::SetLoggingLevel(args);
    if (!result) return InitError(util::ErrorString(result));

    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg = args.GetIntArg("-prune", 0);
    if (nPruneArg < 0) {
        return InitError(_("Prune cannot be configured with a negative value."));
    }
    nPruneTarget = (uint64_t) nPruneArg * 1024 * 1024;
    if (nPruneArg == 1) {  // manual pruning: -prune=1
        nPruneTarget = std::numeric_limits<uint64_t>::max();
        fPruneMode = true;
    } else if (nPruneTarget) {
        if (args.GetBoolArg("-regtest", false)) {
            // we use 1MB blocks to test this on regtest
            if (nPruneTarget < 550 * 1024 * 1024) {
                return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), 550));
            }
        } else {
            if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
                return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024));
            }
        }
        fPruneMode = true;
    }

    nConnectTimeout = args.GetIntArg("-timeout", DEFAULT_CONNECT_TIMEOUT);
    if (nConnectTimeout <= 0) {
        nConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
    }

    peer_connect_timeout = args.GetIntArg("-peertimeout", DEFAULT_PEER_CONNECT_TIMEOUT);
    if (peer_connect_timeout <= 0) {
        return InitError(Untranslated("peertimeout must be a positive integer."));
    }

    // Sanity check argument for min fee for including tx in block
    // TODO: Harmonize which arguments need sanity checking and where that happens
    if (args.IsArgSet("-blockmintxfee")) {
        if (!ParseMoney(args.GetArg("-blockmintxfee", ""))) {
            return InitError(AmountErrMsg("blockmintxfee", args.GetArg("-blockmintxfee", "")));
        }
    }

    nBytesPerSigOp = args.GetIntArg("-bytespersigop", nBytesPerSigOp);

    if (!g_wallet_init_interface.ParameterInteraction()) return false;

    // Option to startup with mocktime set (used for regression testing):
    SetMockTime(args.GetIntArg("-mocktime", 0)); // SetMockTime(0) is a no-op

    if (args.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS))
        nLocalServices = ServiceFlags(nLocalServices | NODE_BLOOM);

    try {
        const bool fQuorumVvecRequestsEnabled{llmq::GetEnabledQuorumVvecSyncEntries(args).size() > 0};
        if (!args.GetBoolArg("-llmq-data-recovery", llmq::DEFAULT_ENABLE_QUORUM_DATA_RECOVERY) && fQuorumVvecRequestsEnabled) {
            InitWarning(Untranslated("-llmq-qvvec-sync set but recovery is disabled due to -llmq-data-recovery=0"));
        }
    } catch (const std::invalid_argument& e) {
        return InitError(Untranslated(e.what()));
    }

    if (args.IsArgSet("-masternodeblsprivkey")) {
        if (!args.GetBoolArg("-listen", DEFAULT_LISTEN) && Params().RequireRoutableExternalIP()) {
            return InitError(Untranslated("Masternode must accept connections from outside, set -listen=1"));
        }
        if (!args.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
            return InitError(Untranslated("Masternode must have transaction index enabled, set -txindex=1"));
        }
        if (!args.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS)) {
            return InitError(Untranslated("Masternode must have bloom filters enabled, set -peerbloomfilters=1"));
        }
        if (args.GetIntArg("-prune", 0) > 0) {
            return InitError(Untranslated("Masternode must have no pruning enabled, set -prune=0"));
        }
        if (args.GetIntArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS) < DEFAULT_MAX_PEER_CONNECTIONS) {
            return InitError(strprintf(Untranslated("Masternode must be able to handle at least %d connections, set -maxconnections=%d"), DEFAULT_MAX_PEER_CONNECTIONS, DEFAULT_MAX_PEER_CONNECTIONS));
        }
        if (args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE)) {
            return InitError(_("You can not disable governance validation on a masternode."));
        }
        if (args.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)) {
            return InitError(Untranslated("Masternode must relay transactions, set -blocksonly=0"));
        }
        if (!args.GetBoolArg("-peerblockfilters", DEFAULT_PEERBLOCKFILTERS)) {
            return InitError(Untranslated("Masternode must serve compact block filters, set -peerblockfilters=1"));
        }
        // Once the daily budget minus a reserve of up to one max-size block per 10 minutes is used
        // up, the node refuses filtered blocks, historical blocks and BIP35 mempool requests while
        // still advertising NODE_BLOOM. A cap that never triggers is equivalent to no cap.
        if (ParseByteUnits(args.GetArg("-maxuploadtarget", DEFAULT_MAX_UPLOAD_TARGET), ByteUnit::M).value_or(0) != 0) {
            return InitError(Untranslated("Masternode must serve blocks and SPV clients without an upload limit, set "
                                          "-maxuploadtarget=0"));
        }
        kernel::MemPoolOptions mempool_opts{};
        if (const auto error{ApplyArgsManOptions(args, chainparams, mempool_opts)}) {
            return InitError(*error);
        }
        // Regression tests deliberately give individual masternodes a stricter policy to create
        // mempool inconsistencies (see feature_llmq_is_retroactive.py).
        auto stricter{GetStricterThanDefaultRelayPolicy(mempool_opts)};
        if (nBytesPerSigOp > DEFAULT_BYTES_PER_SIGOP) stricter.emplace_back("-bytespersigop");
        if (!stricter.empty() && chainparams.NetworkIDString() != CBaseChainParams::REGTEST) {
            return InitError(
                strprintf(Untranslated("Masternode must not use a stricter transaction relay policy than the default, "
                                       "which would make it skip InstantSend signing. Remove or restore: %s"),
                          Join(stricter, ", ")));
        }
    }

    // Governance needs the transaction index to look up proposal collateral transactions
    if (!args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE) &&
        !args.GetBoolArg("-txindex", DEFAULT_TXINDEX) && chainparams.NetworkIDString() != CBaseChainParams::REGTEST) {
        return InitError(_("Transaction index can't be disabled with governance validation enabled. Either start with "
                           "-disablegovernance command line switch or enable transaction index."));
    }

    if (args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE)) {
        InitWarning(_("You are starting with governance validation disabled.") +
            (fPruneMode ?
                Untranslated(" ") + _("This is expected because you are running a pruned node.") :
                Untranslated("")));
    }

    // Also report errors from parsing before daemonization
    {
        ChainstateManager::Options chainman_opts_dummy{
            .chainparams = chainparams,
        };
        if (const auto error{ApplyArgsManOptions(args, chainman_opts_dummy)}) {
            return InitError(*error);
        }
    }

    return true;
}

static bool LockDataDirectory(bool probeOnly)
{
    // Make sure only a single Dash Core process is using the data directory.
    const fs::path& datadir = gArgs.GetDataDirNet();
    if (!DirIsWritable(datadir)) {
        return InitError(strprintf(_("Cannot write to data directory '%s'; check permissions."), fs::PathToString(datadir)));
    }
    if (!LockDirectory(datadir, ".lock", probeOnly)) {
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. %s is probably already running."), fs::PathToString(datadir), PACKAGE_NAME));
    }
    return true;
}

bool AppInitSanityChecks(const kernel::Context& kernel)
{
    // ********************************************************* Step 4: sanity checks
    if (auto error = kernel::SanityChecks(kernel)) {
        InitError(*error);
        return InitError(strprintf(_("Initialization sanity check failed. %s is shutting down."), PACKAGE_NAME));
    }

    // Probe the data directory lock to give an early error message, if possible
    // We cannot hold the data directory lock here, as the forking for daemon() hasn't yet happened,
    // and a fork will cause weird behavior to it.
    return LockDataDirectory(true);
}

bool AppInitLockDataDirectory()
{
    // After daemonization get the data directory lock again and hold on to it until exit
    // This creates a slight window for a race condition to happen, however this condition is harmless: it
    // will at most make us exit without printing a message to console.
    if (!LockDataDirectory(false)) {
        // Detailed error printed inside LockDataDirectory
        return false;
    }
    return true;
}

bool AppInitInterfaces(NodeContext& node)
{
    node.chain = node.init->makeChain();
    return true;
}

bool AppInitMain(NodeContext& node, interfaces::BlockAndHeaderTipInfo* tip_info)
{
    const ArgsManager& args = *Assert(node.args);
    const CChainParams& chainparams = Params();

    auto opt_max_upload = ParseByteUnits(args.GetArg("-maxuploadtarget", DEFAULT_MAX_UPLOAD_TARGET), ByteUnit::M);
    if (!opt_max_upload) {
        return InitError(strprintf(_("Unable to parse -maxuploadtarget: '%s'"), args.GetArg("-maxuploadtarget", "")));
    }

    // ********************************************************* Step 4a: application initialization
    if (!CreatePidFile(args)) {
        // Detailed error printed inside CreatePidFile().
        return false;
    }
    if (!init::StartLogging(args)) {
        // Detailed error printed inside StartLogging().
        return false;
    }

    LogPrintf("Using at most %i automatic connections (%i file descriptors available)\n", nMaxConnections, nFD);

    // Warn about relative -datadir path.
    if (args.IsArgSet("-datadir") && !args.GetPathArg("-datadir").is_absolute()) {
        LogPrintf("Warning: relative datadir option '%s' specified, which will be interpreted relative to the " /* Continued */
                  "current working directory '%s'. This is fragile, because if Dash Core is started in the future "
                  "from a different location, it will be unable to locate the current data files. There could "
                  "also be data loss if Dash Core is started while in a temporary directory.\n",
                  args.GetArg("-datadir", ""), fs::PathToString(fs::current_path()));
    }

    ValidationCacheSizes validation_cache_sizes{};
    ApplyArgsManOptions(args, validation_cache_sizes);
    if (!InitSignatureCache(validation_cache_sizes.signature_cache_bytes)
        || !InitScriptExecutionCache(validation_cache_sizes.script_execution_cache_bytes))
    {
        return InitError(strprintf(_("Unable to allocate memory for -maxsigcachesize: '%s' MiB"), args.GetIntArg("-maxsigcachesize", DEFAULT_MAX_SIG_CACHE_BYTES >> 20)));
    }

    int script_threads = args.GetIntArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
    if (script_threads <= 0) {
        // -par=0 means autodetect (number of cores - 1 script threads)
        // -par=-n means "leave n cores free" (number of cores - n - 1 script threads)
        script_threads += GetNumCores();
    }

    // Subtract 1 because the main thread counts towards the par threads
    script_threads = std::max(script_threads - 1, 0);

    // Number of script-checking threads <= MAX_SCRIPTCHECK_THREADS
    script_threads = std::min(script_threads, MAX_SCRIPTCHECK_THREADS);

    LogPrintf("Script verification uses %d additional threads\n", script_threads);
    if (script_threads >= 1) {
        StartScriptCheckWorkerThreads(script_threads);
    }

    assert(!node.scheduler);
    node.scheduler = std::make_unique<CScheduler>();

    // Start the lightweight task scheduler thread
    node.scheduler->m_service_thread = std::thread(util::TraceThread, "scheduler", [&] { node.scheduler->serviceQueue(); });

    // Gather some entropy once per minute.
    node.scheduler->scheduleEvery([]{
        RandAddPeriodic();
    }, std::chrono::minutes{1});

    // Check disk space every 5 minutes to avoid db corruption.
    node.scheduler->scheduleEvery([&args]{
        constexpr uint64_t min_disk_space = 50 << 20; // 50 MB
        if (!CheckDiskSpace(args.GetBlocksDirPath(), min_disk_space)) {
            LogPrintf("Shutting down due to lack of disk space!\n");
            StartShutdown();
        }
    }, std::chrono::minutes{5});

    GetMainSignals().RegisterBackgroundSignalScheduler(*node.scheduler);

    // Create client interfaces for wallets that are supposed to be loaded
    // according to -wallet and -disablewallet options. This only constructs
    // the interfaces, it doesn't load wallet data. Wallets actually get loaded
    // when load() and start() interface methods are called below.
    g_wallet_init_interface.Construct(node);
    uiInterface.InitWallet();

    /* Register RPC commands regardless of -server setting so they will be
     * available in the GUI RPC console even if external calls are disabled.
     */
    RegisterAllCoreRPCCommands(tableRPC);
    for (const auto& client : node.chain_clients) {
        client->registerRpcs();
    }
#ifdef ENABLE_WALLET
    // Register non-core wallet-only RPC commands. These are commands that
    // aren't a part of the wallet library but heavily rely on wallet logic.
    // TODO: Move them to chain client interfaces so they can be called
    //       with registerRpcs()
    if (!args.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const auto& commands : {
            GetWalletCoinJoinRPCCommands(),
            GetWalletEvoRPCCommands(),
            GetWalletGovernanceRPCCommands(),
            GetWalletMasternodeRPCCommands(),
        }) {
            node.wallet_loader->registerOtherRpcs(commands);
        }
    }
#endif // ENABLE_WALLET

#if ENABLE_ZMQ
    RegisterZMQRPCCommands(tableRPC);
#endif

    /* Start the RPC server already.  It will be started in "warmup" mode
     * and not really process calls already (but it will signify connections
     * that the server is there and will be ready later).  Warmup mode will
     * be disabled when initialisation is finished.
     */
    if (args.GetBoolArg("-server", false)) {
        uiInterface.InitMessage_connect(SetRPCWarmupStatus);
        if (!AppInitServers(node))
            return InitError(_("Unable to start HTTP server. See debug log for details."));
    }

    // ********************************************************* Step 5: verify wallet database integrity

    g_wallet_init_interface.InitAutoBackup();
    for (const auto& client : node.chain_clients) {
        if (!client->verify()) {
            return false;
        }
    }

    // ********************************************************* Step 6: network initialization
    // Note that we absolutely cannot open any actual connections
    // until the very end ("start node") as the UTXO/block state
    // is not yet setup and may end up being set up twice if we
    // need to reindex later.

    fListen = args.GetBoolArg("-listen", DEFAULT_LISTEN);
    fDiscover = args.GetBoolArg("-discover", true);
    const bool ignores_incoming_txs{args.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)};

    {

        // Read asmap file if configured
        std::vector<bool> asmap;
        if (args.IsArgSet("-asmap")) {
            fs::path asmap_path = args.GetPathArg("-asmap", DEFAULT_ASMAP_FILENAME);
            if (!asmap_path.is_absolute()) {
                asmap_path = gArgs.GetDataDirNet() / asmap_path;
            }
            if (!fs::exists(asmap_path)) {
                InitError(strprintf(_("Could not find asmap file %s"), fs::quoted(fs::PathToString(asmap_path))));
                return false;
            }
            asmap = DecodeAsmap(asmap_path);
            if (asmap.size() == 0) {
                InitError(strprintf(_("Could not parse asmap file %s"), fs::quoted(fs::PathToString(asmap_path))));
                return false;
            }
            const uint256 asmap_version = (HashWriter{} << asmap).GetHash();
            LogPrintf("Using asmap version %s for IP bucketing\n", asmap_version.ToString());
        } else {
            LogPrintf("Using /16 prefix for IP bucketing\n");
        }

        // Initialize netgroup manager
        assert(!node.netgroupman);
        node.netgroupman = std::make_unique<NetGroupManager>(std::move(asmap));

        // Initialize addrman
        assert(!node.addrman);
        uiInterface.InitMessage(_("Loading P2P addresses…").translated);
        auto addrman{LoadAddrman(*node.netgroupman, args)};
        if (!addrman) return InitError(util::ErrorString(addrman));
        node.addrman = std::move(*addrman);
    }

    std::string sem_str = args.GetArg("-socketevents", DEFAULT_SOCKETEVENTS);
    ::g_socket_events_mode = SEMFromString(sem_str);
    if (::g_socket_events_mode == SocketEventsMode::Unknown) {
        return InitError(strprintf(_("Invalid -socketevents ('%s') specified. Only these modes are supported: %s"), sem_str, GetSupportedSocketEventsStr()));
    }

    // We need to initialize g_stats_client early as currently, g_stats_client is called
    // regardless of whether transmitting stats are desirable or not and if
    // g_stats_client isn't present when that attempt is made, the client will crash.
    {
        auto stats_client = StatsdClient::make(args);
        if (!stats_client) {
            return InitError(_("Cannot init Statsd client") + Untranslated(" (") + util::ErrorString(stats_client) + Untranslated(")"));
        }
        ::g_stats_client = std::move(*stats_client);
    }

    assert(!node.banman);
    node.banman = std::make_unique<BanMan>(gArgs.GetDataDirNet() / "banlist", &uiInterface, args.GetIntArg("-bantime", DEFAULT_MISBEHAVING_BANTIME));
    assert(!node.connman);
    node.connman = std::make_unique<CConnman>(GetRand<uint64_t>(),
                                              GetRand<uint64_t>(),
                                              *node.addrman, *node.netgroupman, args.GetBoolArg("-networkactive", true));

    assert(!node.fee_estimator);
    // Don't initialize fee estimation with old data if we don't relay transactions,
    // as they would never get updated.
    if (!ignores_incoming_txs) {
        bool read_stale_estimates = args.GetBoolArg("-acceptstalefeeestimates", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES);
        if (read_stale_estimates && (chainparams.NetworkIDString() != CBaseChainParams::REGTEST)) {
            return InitError(strprintf(_("acceptstalefeeestimates is not supported on %s chain."), chainparams.NetworkIDString()));
        }
        node.fee_estimator = std::make_unique<CBlockPolicyEstimator>(FeeestPath(args), read_stale_estimates);

        // Flush estimates to disk periodically
        CBlockPolicyEstimator* fee_estimator = node.fee_estimator.get();
        node.scheduler->scheduleEvery([fee_estimator] { fee_estimator->FlushFeeEstimates(); }, FEE_FLUSH_INTERVAL);
    }

    assert(!node.mn_metaman);
    node.mn_metaman = std::make_unique<CMasternodeMetaMan>();

    assert(!node.netfulfilledman);
    node.netfulfilledman = std::make_unique<CNetFulfilledRequestManager>();

    const bool is_governance_enabled{!args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE)};

    assert(!node.sporkman);
    node.sporkman = std::make_unique<CSporkManager>();
    node.chainlocks = std::make_unique<chainlock::Chainlocks>(*node.sporkman);

    const std::string spork_address{args.GetArg("-sporkaddr", chainparams.SporkAddress())};
    if (args.GetArgs("-sporkaddr").size() > 1) {
        InitWarning(strprintf(_("More than one spork address is provided. Using %s."), spork_address));
    }
    if (!node.sporkman->SetSporkAddress(spork_address)) {
        return InitError(_("Invalid spork address specified with -sporkaddr"));
    }

    if (args.IsArgSet("-sporkkey")) { // spork priv key
        if (!node.sporkman->SetPrivKey(args.GetArg("-sporkkey", ""))) {
            return InitError(_("Unable to sign spork message, wrong key?"));
        }
    }

    // Check port numbers
    for (const std::string port_option : {
        "-port",
        "-rpcport",
    }) {
        if (args.IsArgSet(port_option)) {
            const std::string port = args.GetArg(port_option, "");
            uint16_t n;
            if (!ParseUInt16(port, &n) || n == 0) {
                return InitError(InvalidPortErrMsg(port_option, port));
            }
        }
    }

    for ([[maybe_unused]] const auto& [arg, unix] : std::vector<std::pair<std::string, bool>>{
        // arg name                 UNIX socket support
        {"-i2psam",                             false},
        {"-onion",                               true},
        {"-proxy",                               true},
        {"-rpcbind",                            false},
        {"-torcontrol",                         false},
        {"-whitebind",                          false},
        {"-zmqpubhashblock",                     true},
        {"-zmqpubhashchainlock",                 true},
        {"-zmqpubhashgovernanceobject",          true},
        {"-zmqpubhashgovernancevote",            true},
        {"-zmqpubhashinstantsenddoublespend",    true},
        {"-zmqpubhashrecoveredsig",              true},
        {"-zmqpubhashtx",                        true},
        {"-zmqpubhashtxlock",                    true},
        {"-zmqpubrawblock",                      true},
        {"-zmqpubrawchainlock",                  true},
        {"-zmqpubrawchainlocksig",               true},
        {"-zmqpubrawgovernancevote",             true},
        {"-zmqpubrawgovernanceobject",           true},
        {"-zmqpubrawinstantsenddoublespend",     true},
        {"-zmqpubrawrecoveredsig",               true},
        {"-zmqpubrawtx",                         true},
        {"-zmqpubrawtxlock",                     true},
        {"-zmqpubrawtxlocksig",                  true},
        {"-zmqpubsequence",                      true},
    }) {
        for (const std::string& socket_addr : args.GetArgs(arg)) {
            std::string host_out;
            uint16_t port_out{0};
            if (!SplitHostPort(socket_addr, port_out, host_out)) {
#if HAVE_SOCKADDR_UN
                // Allow unix domain sockets for some options e.g. unix:/some/file/path
                if (!unix || socket_addr.find(ADDR_PREFIX_UNIX) != 0) {
                    return InitError(InvalidPortErrMsg(arg, socket_addr));
                }
#else
                return InitError(InvalidPortErrMsg(arg, socket_addr));
#endif
            }
        }
    }

    for (const std::string& socket_addr : args.GetArgs("-bind")) {
        std::string host_out;
        uint16_t port_out{0};
        std::string bind_socket_addr = socket_addr.substr(0, socket_addr.rfind('='));
        if (!SplitHostPort(bind_socket_addr, port_out, host_out)) {
            return InitError(InvalidPortErrMsg("-bind", socket_addr));
        }
    }

    // sanitize comments per BIP-0014, format user agent and check total size
    std::vector<std::string> uacomments;

    if (chainparams.NetworkIDString() == CBaseChainParams::DEVNET) {
        // Add devnet name to user agent. This allows to disconnect nodes immediately if they don't belong to our own devnet
        uacomments.push_back(strprintf("devnet.%s", args.GetDevNetName()));
    }

    for (const std::string& cmt : args.GetArgs("-uacomment")) {
        if (cmt != SanitizeString(cmt, SAFE_CHARS_UA_COMMENT))
            return InitError(strprintf(_("User Agent comment (%s) contains unsafe characters."), cmt));
        uacomments.push_back(cmt);
    }
    strSubVersion = FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, uacomments);
    if (strSubVersion.size() > MAX_SUBVERSION_LENGTH) {
        return InitError(strprintf(_("Total length of network version string (%i) exceeds maximum length (%i). Reduce the number or size of uacomments."),
            strSubVersion.size(), MAX_SUBVERSION_LENGTH));
    }

    if (args.IsArgSet("-onlynet")) {
        g_reachable_nets.RemoveAll();
        for (const std::string& snet : args.GetArgs("-onlynet")) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(strprintf(_("Unknown network specified in -onlynet: '%s'"), snet));
            g_reachable_nets.Add(net);
        }
    }

    if (!args.IsArgSet("-cjdnsreachable")) {
        if (args.IsArgSet("-onlynet") && g_reachable_nets.Contains(NET_CJDNS)) {
            return InitError(
                _("Outbound connections restricted to CJDNS (-onlynet=cjdns) but "
                  "-cjdnsreachable is not provided"));
        }
        g_reachable_nets.Remove(NET_CJDNS);
    }
    // Now g_reachable_nets.Contains(NET_CJDNS) is true if:
    // 1. -cjdnsreachable is given and
    // 2.1. -onlynet is not given or
    // 2.2. -onlynet=cjdns is given

    // Requesting DNS seeds entails connecting to IPv4/IPv6, which -onlynet options may prohibit:
    // If -dnsseed=1 is explicitly specified, abort. If it's left unspecified by the user, we skip
    // the DNS seeds by adjusting -dnsseed in InitParameterInteraction.
    if (args.GetBoolArg("-dnsseed", DEFAULT_DNSSEED) == true && !g_reachable_nets.Contains(NET_IPV4) && !g_reachable_nets.Contains(NET_IPV6)) {
        return InitError(strprintf(_("Incompatible options: -dnsseed=1 was explicitly specified, but -onlynet forbids connections to IPv4/IPv6")));
    };

    // Check for host lookup allowed before parsing any network related parameters
    fNameLookup = args.GetBoolArg("-dns", DEFAULT_NAME_LOOKUP);

    Proxy onion_proxy;

    bool proxyRandomize = args.GetBoolArg("-proxyrandomize", DEFAULT_PROXYRANDOMIZE);
    // -proxy sets a proxy for all outgoing network traffic
    // -noproxy (or -proxy=0) as well as the empty string can be used to not set a proxy, this is the default
    std::string proxyArg = args.GetArg("-proxy", "");
    if (proxyArg != "" && proxyArg != "0") {
        Proxy addrProxy;
        if (IsUnixSocketPath(proxyArg)) {
            addrProxy = Proxy(proxyArg, proxyRandomize);
        } else {
            const std::optional<CService> proxyAddr{Lookup(proxyArg, 9050, fNameLookup)};
            if (!proxyAddr.has_value()) {
                return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'"), proxyArg));
            }

            addrProxy = Proxy(proxyAddr.value(), proxyRandomize);
        }

        if (!addrProxy.IsValid())
            return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'"), proxyArg));

        SetProxy(NET_IPV4, addrProxy);
        SetProxy(NET_IPV6, addrProxy);
        SetProxy(NET_CJDNS, addrProxy);
        SetNameProxy(addrProxy);
        onion_proxy = addrProxy;
    }

    const bool onlynet_used_with_onion{args.IsArgSet("-onlynet") && g_reachable_nets.Contains(NET_ONION)};

    // -onion can be used to set only a proxy for .onion, or override normal proxy for .onion addresses
    // -noonion (or -onion=0) disables connecting to .onion entirely
    // An empty string is used to not override the onion proxy (in which case it defaults to -proxy set above, or none)
    std::string onionArg = args.GetArg("-onion", "");
    if (onionArg != "") {
        if (onionArg == "0") { // Handle -noonion/-onion=0
            onion_proxy = Proxy{};
            if (onlynet_used_with_onion) {
                return InitError(
                    _("Outbound connections restricted to Tor (-onlynet=onion) but the proxy for "
                      "reaching the Tor network is explicitly forbidden: -onion=0"));
            }
        } else {
            if (IsUnixSocketPath(onionArg)) {
                onion_proxy = Proxy(onionArg, proxyRandomize);
            } else {
                const std::optional<CService> addr{Lookup(onionArg, 9050, fNameLookup)};
                if (!addr.has_value() || !addr->IsValid()) {
                    return InitError(strprintf(_("Invalid -onion address or hostname: '%s'"), onionArg));
                }

                onion_proxy = Proxy(addr.value(), proxyRandomize);
            }
        }
    }

    if (onion_proxy.IsValid()) {
        SetProxy(NET_ONION, onion_proxy);
    } else {
        // If -listenonion is set, then we will (try to) connect to the Tor control port
        // later from the torcontrol thread and may retrieve the onion proxy from there.
        const bool listenonion_disabled{!args.GetBoolArg("-listenonion", DEFAULT_LISTEN_ONION)};
        if (onlynet_used_with_onion && listenonion_disabled) {
            return InitError(
                _("Outbound connections restricted to Tor (-onlynet=onion) but the proxy for "
                  "reaching the Tor network is not provided: none of -proxy, -onion or "
                  "-listenonion is given"));
        }
        g_reachable_nets.Remove(NET_ONION);
    }

    for (const std::string& strAddr : args.GetArgs("-externalip")) {
        const std::optional<CService> addrLocal{Lookup(strAddr, GetListenPort(), fNameLookup)};
        if (addrLocal.has_value() && addrLocal->IsValid())
            AddLocal(addrLocal.value(), LOCAL_MANUAL);
        else
            return InitError(ResolveErrMsg("externalip", strAddr));
    }

#if ENABLE_ZMQ
    g_zmq_notification_interface = CZMQNotificationInterface::Create();

    if (g_zmq_notification_interface) {
        RegisterValidationInterface(g_zmq_notification_interface.get());
    }
#endif

    // ********************************************************* Step 7a: Load sporks

    if (!node.sporkman->LoadCache()) {
        auto file_path = fs::PathToString(gArgs.GetDataDirNet() / "sporks.dat");
        return InitError(strprintf(_("Failed to load sporks cache from %s"), file_path));
    }

    // ********************************************************* Step 7b: load block chain

    fReindex = args.GetBoolArg("-reindex", false);
    bool fReindexChainState = args.GetBoolArg("-reindex-chainstate", false);
    ChainstateManager::Options chainman_opts{
        .chainparams = chainparams,
    };
    Assert(!ApplyArgsManOptions(args, chainman_opts)); // no error can happen, already checked in AppInitParameterInteraction

    // cache size calculations
    CacheSizes cache_sizes = CalculateCacheSizes(args, g_enabled_filter_types.size());

    LogPrintf("Cache configuration:\n");
    LogPrintf("* Using %.1f MiB for block index database\n", cache_sizes.block_tree_db * (1.0 / 1024 / 1024));
    if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        LogPrintf("* Using %.1f MiB for transaction index database\n", cache_sizes.tx_index * (1.0 / 1024 / 1024));
    }
    if (args.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX)) {
        LogPrintf("* Using %.1f MiB for address index database\n", cache_sizes.address_index * (1.0 / 1024 / 1024));
    }
    if (args.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX)) {
        LogPrintf("* Using %.1f MiB for timestamp index database\n", cache_sizes.timestamp_index * (1.0 / 1024 / 1024));
    }
    if (args.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX)) {
        LogPrintf("* Using %.1f MiB for spent index database\n", cache_sizes.spent_index * (1.0 / 1024 / 1024));
    }
    for (BlockFilterType filter_type : g_enabled_filter_types) {
        LogPrintf("* Using %.1f MiB for %s block filter index database\n",
                  cache_sizes.filter_index * (1.0 / 1024 / 1024), BlockFilterTypeName(filter_type));
    }
    LogPrintf("* Using %.1f MiB for chain state database\n", cache_sizes.coins_db * (1.0 / 1024 / 1024));

    assert(!node.mempool);
    assert(!node.chainman);
    assert(!node.mn_sync);

    CTxMemPool::Options mempool_opts{
        .estimator = node.fee_estimator.get(),
        .check_ratio = chainparams.DefaultConsistencyChecks() ? 1 : 0,
    };
    if (const auto err{ApplyArgsManOptions(args, chainparams, mempool_opts)}) {
        return InitError(*err);
    }
    mempool_opts.check_ratio = std::clamp<int>(mempool_opts.check_ratio, 0, 1'000'000);

    int64_t descendant_limit_bytes = mempool_opts.limits.descendant_size_vbytes * 40;
    if (mempool_opts.max_size_bytes < 0 || mempool_opts.max_size_bytes < descendant_limit_bytes) {
        return InitError(strprintf(_("-maxmempool must be at least %d MB"), std::ceil(descendant_limit_bytes / 1'000'000.0)));
    }
    LogPrintf("* Using %.1f MiB for in-memory UTXO set (plus up to %.1f MiB of unused mempool space)\n", cache_sizes.coins * (1.0 / 1024 / 1024), mempool_opts.max_size_bytes * (1.0 / 1024 / 1024));

    for (bool fLoaded = false; !fLoaded && !ShutdownRequested();) {
        // On a retry iteration the previous instances still hold the on-disk
        // database locks, so release them before opening the databases again.
        node.mempool.reset();
        node.isman.reset();
        node.dmnman.reset();
        node.evodb.reset();
        auto catch_exceptions = [](auto&& f) {
            try {
                return f();
            } catch (const std::exception& e) {
                LogPrintf("%s\n", e.what());
                return std::make_tuple(node::ChainstateLoadStatus::FAILURE, _("Error opening block database"));
            }
        };
        auto [status, error] = catch_exceptions([&]() -> node::ChainstateLoadResult {
            node.evodb = std::make_unique<CEvoDB>(util::DbWrapperParams{.path = args.GetDataDirNet(), .memory = false, .wipe = node::fReindex || fReindexChainState});
            node.dmnman = std::make_unique<CDeterministicMNManager>(*node.evodb, *node.mn_metaman);
            node.isman = std::make_unique<llmq::CInstantSendManager>(*node.sporkman, util::DbWrapperParams{.path = args.GetDataDirNet(), .memory = false, .wipe = node::fReindex || fReindexChainState});

            mempool_opts.dmnman = node.dmnman.get();
            mempool_opts.isman = node.isman.get();
            node.mempool = std::make_unique<CTxMemPool>(mempool_opts);
            return {node::ChainstateLoadStatus::SUCCESS, {}};
        });

        node.chainman = std::make_unique<ChainstateManager>(chainman_opts);
        ChainstateManager& chainman = *node.chainman;

        /**
         * The manager needs to be constructed regardless of whether governance
         * validation is needed or not.
         *
         * Instead, we decide whether to initialize its database based on whether we
         * need it or not further down and then query if the database is initialized
         * to check if validation is enabled.
         */
        node.mn_sync = std::make_unique<CMasternodeSync>(std::make_unique<NodeSyncNotifierImpl>(*node.connman, *node.netfulfilledman));

        node::ChainstateLoadOptions options;
        options.chainlocks = Assert(node.chainlocks.get());
        options.mn_sync = Assert(node.mn_sync.get());
        options.data_dir = args.GetDataDirNet();
        options.reindex = node::fReindex;
        options.reindex_chainstate = fReindexChainState;
        options.prune = chainman.m_blockman.IsPruneMode();
        options.bls_threads = [&args]() -> int8_t {
            int64_t threads = args.GetIntArg("-parbls", llmq::DEFAULT_BLSCHECK_THREADS);
            if (threads <= 0) {
                // -parbls=0 means autodetect (number of cores - 1 validator threads)
                // -parbls=-n means "leave n cores free" (number of cores - n - 1 validator threads)
                threads += GetNumCores();
            }
            // Subtract 1 because the main thread counts towards the par threads
            const int64_t adjusted_threads = std::clamp<int64_t>(threads, 1, int64_t{llmq::MAX_BLSCHECK_THREADS} + 1) - 1;
            return static_cast<int8_t>(adjusted_threads);
        }();
        options.max_recsigs_age = args.GetIntArg("-maxrecsigsage", llmq::DEFAULT_MAX_RECOVERED_SIGS_AGE);
        options.check_blocks = args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS);
        options.check_level = args.GetIntArg("-checklevel", DEFAULT_CHECKLEVEL);
        options.require_full_verification = args.IsArgSet("-checkblocks") || args.IsArgSet("-checklevel");
        options.check_interrupt = ShutdownRequested;
        options.coins_error_cb = [] {
            uiInterface.ThreadSafeMessageBox(
                _("Error reading from database, shutting down."),
                "", CClientUIInterface::MSG_ERROR);
        };

        uiInterface.InitMessage(_("Loading block index…").translated);
        const auto load_block_index_start_time{SteadyClock::now()};
        if (status == node::ChainstateLoadStatus::SUCCESS) {
            options.mempool = Assert(node.mempool.get());
            options.isman = Assert(node.isman.get());
            std::tie(status, error) = catch_exceptions([&]{ return LoadChainstate(chainman, cache_sizes, options, *node.evodb, *node.dmnman, node.llmq_ctx, node.chain_helper); });
        }
        if (status == node::ChainstateLoadStatus::SUCCESS) {
            uiInterface.InitMessage(_("Verifying blocks…").translated);
            if (chainman.m_blockman.m_have_pruned && options.check_blocks > chainman.GetParams().MinBlocksToKeep()) {
                LogWarning("pruned datadir may not have more than %d blocks; only checking available blocks\n",
                           chainman.GetParams().MinBlocksToKeep());
            }
            std::tie(status, error) = catch_exceptions([&]{ return VerifyLoadedChainstate(chainman, options, *Assert(node.evodb), [](bool bls_state) {
                        LogPrintf("AppInitMain: bls_legacy_scheme=%d\n", bls_state);
                        });});
            if (status == node::ChainstateLoadStatus::SUCCESS) {
                fLoaded = true;
                LogPrintf(" block index %15dms\n", Ticks<std::chrono::milliseconds>(SteadyClock::now() - load_block_index_start_time));
            }
        }

        if (status == node::ChainstateLoadStatus::FAILURE_FATAL || status == node::ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB || status == node::ChainstateLoadStatus::FAILURE_INSUFFICIENT_DBCACHE) {
            return InitError(error);
        }

        if (!fLoaded && !ShutdownRequested()) {
            // first suggest a reindex
            if (!options.reindex) {
                bool fRet = uiInterface.ThreadSafeQuestion(
                    error + Untranslated(".\n\n") + _("Do you want to rebuild the block database now?"),
                    error.original + ".\nPlease restart with -reindex or -reindex-chainstate to recover.",
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    fReindex = true;
                    AbortShutdown();
                } else {
                    LogPrintf("Aborted block database rebuild. Exiting.\n");
                    return false;
                }
            } else {
                return InitError(error);
            }
        }
    }

    // As LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (ShutdownRequested()) {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    ChainstateManager& chainman = *Assert(node.chainman);

    assert(!node.dstxman);
    node.dstxman = std::make_unique<CDSTXManager>(*node.chainlocks);

    node.clhandler = std::make_unique<chainlock::ChainlockHandler>(*node.chainlocks, chainman, *node.mempool, *node.mn_sync);
    RegisterValidationInterface(node.clhandler.get());

    assert(!node.govman);
    node.govman = std::make_unique<CGovernanceManager>(*node.mn_metaman, *node.chainman, *node.chain_helper->superblocks, *node.dmnman, *node.mn_sync);

    // ********************************************************* Step 7c: Setup masternode mode or watch-only mode
    assert(!node.active_ctx);
    assert(!node.observer_ctx);

    const bool quorums_recovery = args.GetBoolArg("-llmq-data-recovery", llmq::DEFAULT_ENABLE_QUORUM_DATA_RECOVERY);
    const bool quorums_watch = args.GetBoolArg("-watchquorums", llmq::DEFAULT_WATCH_QUORUMS);
    const llmq::QvvecSyncModeMap sync_map{llmq::GetEnabledQuorumVvecSyncEntries(args)};
    const util::DbWrapperParams dash_db_params{.path = args.GetDataDirNet(), .memory = false, .wipe = (fReindex || fReindexChainState)};
    if (const auto operator_sk_str = args.GetArg("-masternodeblsprivkey", ""); !operator_sk_str.empty()) {
        const CBLSSecretKey operator_sk{ParseHex(operator_sk_str)};
        if (!operator_sk.IsValid()) {
            return InitError(_("Invalid masternodeblsprivkey. Please see documentation."));
        }
        // Will init later in ThreadImport
        node.active_ctx = std::make_unique<ActiveContext>(*node.llmq_ctx->bls_worker, chainman, *node.connman, *node.dmnman,
                                                          *node.govman, *node.chain_helper->superblocks,
                                                          *node.sporkman, *node.chainlocks, *node.mempool, *node.clhandler, *node.isman,
                                                          *node.llmq_ctx->qman, *node.llmq_ctx->qsnapman, *node.llmq_ctx->sigman,
                                                          *node.mn_sync, operator_sk, dash_db_params, quorums_watch);
        RegisterValidationInterface(node.active_ctx.get());
    } else if (quorums_watch) {
        node.observer_ctx = std::make_unique<llmq::ObserverContext>(*node.dmnman, *node.llmq_ctx->qman, *node.llmq_ctx->qsnapman,
                                                                    chainman, *node.sporkman, dash_db_params);
        RegisterValidationInterface(node.observer_ctx.get());
    }

    assert(!node.cj_walletman);
#ifdef ENABLE_WALLET
    if (!node.active_ctx) {
        // Only constructed in wallet-enabled builds; stays null otherwise, must check before use
        node.cj_walletman = CJWalletManager::make(chainman, *node.dmnman, *node.mn_metaman, *node.mempool, *node.mn_sync,
                                                  *node.isman, !ignores_incoming_txs);
    }
#endif

    assert(!node.peerman);
    node.peerman = PeerManager::make(*node.connman, *node.addrman, node.banman.get(), *node.dstxman,
                                     chainman, *node.mempool, *node.mn_metaman, *node.mn_sync,
                                     *node.sporkman, *node.chainlocks, *node.clhandler,
                                     node.active_ctx ? node.active_ctx->nodeman.get() : nullptr,
                                     *node.dmnman, node.cj_walletman.get(), *node.isman, *node.llmq_ctx, ignores_incoming_txs);
    RegisterValidationInterface(node.peerman.get());

    node.ds_notification_interface = std::make_unique<CDSNotificationInterface>(
        *node.connman, *node.dstxman, *node.mn_sync, *node.govman, chainman, *node.dmnman
    );
    RegisterValidationInterface(node.ds_notification_interface.get());

    // ********************************************************* Step 7d: Setup other Dash services

    node.peerman->AddExtraHandler(std::make_unique<NetInstantSend>(node.peerman.get(), *node.isman, node.active_ctx ? node.active_ctx->is_signer.get() : nullptr, *node.llmq_ctx->sigman, *node.llmq_ctx->qman, *node.chainlocks, chainman, *node.mempool, *node.mn_sync));
    node.peerman->AddExtraHandler(std::make_unique<llmq::NetSigning>(node.peerman.get(), *node.llmq_ctx->sigman, node.active_ctx ? node.active_ctx->shareman.get() : nullptr, *node.sporkman));

    {
        llmq::QuorumRole* quorum_role = node.active_ctx ? static_cast<llmq::QuorumRole*>(node.active_ctx.get())
                                                        : static_cast<llmq::QuorumRole*>(node.observer_ctx.get());
        auto net_quorum = std::make_unique<llmq::NetQuorum>(
            node.peerman.get(), *node.llmq_ctx->bls_worker, *node.connman, *node.dmnman,
            *node.llmq_ctx->qman, *node.llmq_ctx->qsnapman, chainman,
            *node.mn_sync, *node.sporkman, quorum_role,
            node.active_ctx ? node.active_ctx->nodeman.get() : nullptr,
            llmq::DEFAULT_WORKER_COUNT, sync_map, quorums_recovery);
        node.peerman->AddExtraHandler(std::move(net_quorum));
    }

    if (node.active_ctx) {
        node.peerman->AddExtraHandler(std::make_unique<llmq::NetDKG>(
            node.peerman.get(), *node.sporkman, *node.active_ctx->qdkgsman, chainman, quorums_watch,
            *node.llmq_ctx->qman, *node.active_ctx,
            *node.llmq_ctx->bls_worker, *node.dmnman, *node.mn_metaman,
            *node.active_ctx->dkgdbgman, *node.llmq_ctx->quorum_block_processor, *node.llmq_ctx->qsnapman,
            *node.active_ctx->nodeman, *node.connman));
    } else if (node.observer_ctx) {
        node.peerman->AddExtraHandler(std::make_unique<llmq::NetDKG>(
            node.peerman.get(), *node.sporkman, *node.observer_ctx->qdkgsman, chainman,
            *node.llmq_ctx->qman, *node.observer_ctx));
    } else {
        node.peerman->AddExtraHandler(std::make_unique<llmq::NetDKGStub>(node.peerman.get()));
    }

    if (node.active_ctx) {
        auto cj_server = std::make_unique<CCoinJoinServer>(node.peerman.get(), chainman, *node.connman, *node.dmnman, *node.dstxman, *node.mn_metaman,
                                                           *node.mempool, *node.active_ctx->nodeman, *node.mn_sync, *node.isman);
        node.active_ctx->SetCJServer(cj_server.get());
        node.peerman->AddExtraHandler(std::move(cj_server));
    }

    if (node.cj_walletman) {
        RegisterValidationInterface(node.cj_walletman.get());
    }

    bool fLoadCacheFiles = !(fReindex || fReindexChainState) && WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip() != nullptr);

    if (!node.netfulfilledman->LoadCache(fLoadCacheFiles)) {
        auto file_path = fs::PathToString(gArgs.GetDataDirNet() / "netfulfilled.dat");
        if (fLoadCacheFiles) {
            return InitError(strprintf(_("Failed to load fulfilled requests cache from %s"), file_path));
        }
        return InitError(strprintf(_("Failed to clear fulfilled requests cache at %s"), file_path));
    }

    if (!node.mn_metaman->LoadCache(fLoadCacheFiles)) {
        auto file_path = fs::PathToString(gArgs.GetDataDirNet() / "mncache.dat");
        if (fLoadCacheFiles) {
            return InitError(strprintf(_("Failed to load masternode cache from %s"), file_path));
        }
        return InitError(strprintf(_("Failed to clear masternode cache at %s"), file_path));
    }

    if (is_governance_enabled) {
        if (!node.govman->LoadCache(fLoadCacheFiles)) {
            auto file_path = fs::PathToString(gArgs.GetDataDirNet() / "governance.dat");
            if (fLoadCacheFiles) {
                return InitError(strprintf(_("Failed to load governance cache from %s"), file_path));
            }
            return InitError(strprintf(_("Failed to clear governance cache at %s"), file_path));
        }
    }
    // Always register NetGovernance so it can suppress governance inv items in AlreadyHave()
    // even when -disablegovernance is set. The handler's ProcessMessage/Schedule paths
    // early-return on !IsValid(), and AlreadyHave() short-circuits to true so we don't
    // track governance inventory that cannot be processed.
    node.peerman->AddExtraHandler(std::make_unique<NetGovernance>(node.peerman.get(), *node.govman, *node.mn_sync, *node.netfulfilledman, *node.connman));
    node.peerman->AddExtraHandler(std::make_unique<SyncManager>(node.peerman.get(), *node.govman, *node.mn_sync, *node.connman, *node.netfulfilledman));

    // ********************************************************* Step 8: start indexers

    // If reindex-chainstate was specified, delay syncing indexes until ThreadImport has reindexed the chain
    if (!fReindexChainState) g_indexes_ready_to_sync = true;
    if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        g_txindex = std::make_unique<TxIndex>(interfaces::MakeChain(node), cache_sizes.tx_index, false, fReindex);
        if (!g_txindex->Start()) {
            return false;
        }
    }

    if (args.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX)) {
        node.address_index = std::make_unique<AddressIndex>(interfaces::MakeChain(node), cache_sizes.address_index, false, fReindex);
        if (!node.address_index->Start()) {
            return false;
        }
    }

    if (args.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX)) {
        node.timestamp_index = std::make_unique<TimestampIndex>(interfaces::MakeChain(node), cache_sizes.timestamp_index, false, fReindex);
        if (!node.timestamp_index->Start()) {
            return false;
        }
    }

    if (args.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX)) {
        node.spent_index = std::make_unique<SpentIndex>(interfaces::MakeChain(node), cache_sizes.spent_index, false, fReindex);
        if (!node.spent_index->Start()) {
            return false;
        }
    }

    for (const auto& filter_type : g_enabled_filter_types) {
        InitBlockFilterIndex([&]{ return interfaces::MakeChain(node); }, filter_type, cache_sizes.filter_index, false, fReindex);
        if (!GetBlockFilterIndex(filter_type)->Start()) {
            return false;
        }
    }

    if (args.GetBoolArg("-coinstatsindex", DEFAULT_COINSTATSINDEX)) {
        g_coin_stats_index = std::make_unique<CoinStatsIndex>(interfaces::MakeChain(node), /*cache_size=*/0, false, fReindex);
        if (!g_coin_stats_index->Start()) {
            return false;
        }
    }

    // ********************************************************* Step 9: load wallet
    for (const auto& client : node.chain_clients) {
        if (!client->load()) {
            return false;
        }
    }

    // As InitLoadWallet can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    if (ShutdownRequested())
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }
    // ********************************************************* Step 10: data directory maintenance

    // if pruning, perform the initial blockstore prune
    // after any wallet rescanning has taken place.
    if (chainman.m_blockman.IsPruneMode()) {
        if (!fReindex) {
            LOCK(cs_main);
            for (Chainstate* chainstate : chainman.GetAll()) {
                uiInterface.InitMessage(_("Pruning blockstore…").translated);
                chainstate->PruneAndFlush();
            }
        }
    } else {
        LogPrintf("Setting NODE_NETWORK on non-prune mode\n");
        nLocalServices = ServiceFlags(nLocalServices | NODE_NETWORK);
    }

    // As PruneAndFlush can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    if (ShutdownRequested())
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    // ********************************************************* Step 10a: schedule Dash-specific tasks

    node.peerman->StartHandlers();
    node.clhandler->Start();

    node.scheduler->scheduleEvery(std::bind(&CNetFulfilledRequestManager::DoMaintenance, std::ref(*node.netfulfilledman)), std::chrono::minutes{1});
    node.scheduler->scheduleEvery(std::bind(&CMasternodeUtils::DoMaintenance, std::ref(*node.connman), std::ref(*node.dmnman), std::ref(*node.mn_sync), node.cj_walletman.get()), std::chrono::minutes{1});
    node.peerman->ScheduleHandlers(*node.scheduler);

    if (node.active_ctx) {
        node.active_ctx->Start();
        node.scheduler->scheduleEvery(std::bind(&llmq::CDKGSessionManager::CleanupOldContributions, std::ref(*node.active_ctx->qdkgsman)), std::chrono::hours{1});
    }

    if (node.cj_walletman) {
        node.cj_walletman->Schedule(*node.connman, *node.scheduler);
    }

    if (::g_stats_client->active()) {
        int nStatsPeriod = std::min(std::max((int)args.GetIntArg("-statsperiod", DEFAULT_STATSD_PERIOD), MIN_STATSD_PERIOD), MAX_STATSD_PERIOD);
        node.scheduler->scheduleEvery(std::bind(&PeriodicStats, std::ref(node)), std::chrono::seconds{nStatsPeriod});
    }

    // ********************************************************* Step 11: import blocks

    if (!CheckDiskSpace(gArgs.GetDataDirNet())) {
        InitError(strprintf(_("Error: Disk space is low for %s"), fs::quoted(fs::PathToString(gArgs.GetDataDirNet()))));
        return false;
    }
    if (!CheckDiskSpace(gArgs.GetBlocksDirPath())) {
        InitError(strprintf(_("Error: Disk space is low for %s"), fs::quoted(fs::PathToString(gArgs.GetBlocksDirPath()))));
        return false;
    }

    int chain_active_height = WITH_LOCK(cs_main, return chainman.ActiveChain().Height());

    // On first startup, warn on low block storage space
    if (!fReindex && !fReindexChainState && chain_active_height <= 1) {
        uint64_t additional_bytes_needed{
            chainman.m_blockman.IsPruneMode() ?
                chainman.m_blockman.GetPruneTarget() :
                chainparams.AssumedBlockchainSize() * 1024 * 1024 * 1024};

        if (!CheckDiskSpace(args.GetBlocksDirPath(), additional_bytes_needed)) {
            InitWarning(strprintf(_(
                    "Disk space for %s may not accommodate the block files. " \
                    "Approximately %u GB of data will be stored in this directory."
                ),
                fs::quoted(fs::PathToString(args.GetBlocksDirPath())),
                chainparams.AssumedBlockchainSize()
            ));
        }
    }

    // Either install a handler to notify us when genesis activates, or set fHaveGenesis directly.
    // No locking, as this happens before any background thread is started.
    boost::signals2::connection block_notify_genesis_wait_connection;
    if (WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip() == nullptr)) {
        block_notify_genesis_wait_connection = uiInterface.NotifyBlockTip_connect(std::bind(BlockNotifyGenesisWait, std::placeholders::_2));
    } else {
        fHaveGenesis = true;
    }

#if HAVE_SYSTEM
    const std::string block_notify = args.GetArg("-blocknotify", "");
    if (!block_notify.empty()) {
        uiInterface.NotifyBlockTip_connect([block_notify](SynchronizationState sync_state, const CBlockIndex* pBlockIndex) {
            if (sync_state != SynchronizationState::POST_INIT || !pBlockIndex) return;
            std::string command = block_notify;
            ReplaceAll(command, "%s", pBlockIndex->GetBlockHash().GetHex());
            std::thread t(runCommand, command);
            t.detach(); // thread runs free
        });
    }
    const std::string chainlock_notify = args.GetArg("-chainlocknotify", "");
    if (!chainlock_notify.empty()) {
        uiInterface.NotifyChainLock_connect([chainlock_notify](const std::string& bestChainLockHash, int bestChainLockHeight) {
            std::string command = chainlock_notify;
            ReplaceAll(command, "%s", bestChainLockHash);
            std::thread t(runCommand, command);
            t.detach(); // thread runs free
        });
    }
#endif

    std::vector<fs::path> vImportFiles;
    for (const std::string& strFile : args.GetArgs("-loadblock")) {
        vImportFiles.push_back(fs::PathFromString(strFile));
    }

    chainman.m_load_block = std::thread(&util::TraceThread, "loadblk", [=, &args, &chainman, &node] {
        // ThreadImport can switch fReindex from true to false, fetch its original state here to use later
        bool skip_evodb_repair_on_reindex = fReindex || fReindexChainState;
        ThreadImport(chainman, vImportFiles, args, ShouldPersistMempool(args) ? MempoolPath(args) : fs::path{});

        {
            const CBlockIndex* tip = WITH_LOCK(::cs_main, return chainman.ActiveTip());
            const bool ibd = chainman.ActiveChainstate().IsInitialBlockDownload();
            if (node.active_ctx) {
                // On masternodes, defer the full broadcast until after
                // nodeman->Init() so that GetProTxHash() is available
                // for quorum connection setup and skShare derivation.
                // Only kick CDSNotificationInterface here (cached block
                // height for DS/MN payments/budgets).
                node.ds_notification_interface->InitializeCurrentBlockTip(tip, ibd);
            } else {
                // Non-masternode nodes (including observer-only): broadcast
                // to all subscribers now; no proTxHash dependency.
                GetMainSignals().InitializeCurrentBlockTip(tip, ibd);
            }
        }

        // Seed InstantSend tip-height cache; NetInstantSend receives future
        // updates via CValidationInterface but misses InitializeCurrentBlockTip.
        // TODO: move cache updates from NetInstantSend to g_ds_notification due to specific of Tip's processing
        node.isman->CacheTipHeight(WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()));

        {
            // Get all UTXOs for each MN collateral in one go so that we can fill coin cache early
            // and reduce further locking overhead for cs_main in other parts of code including GUI
            LogPrintf("Filling coin cache with masternode UTXOs...\n");
            LOCK(cs_main);
            const auto start{SteadyClock::now()};
            const auto mnList{node.dmnman->GetListAtChainTip()};
            mnList.ForEachMN(/*onlyValid=*/false, [&](const auto& dmn) {
                Coin coin;
                GetUTXOCoin(chainman.ActiveChainstate(), dmn.collateralOutpoint, coin);
            });
            LogPrintf("Filling coin cache with masternode UTXOs: done in %dms\n", Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
        }

        if (skip_evodb_repair_on_reindex) {
            LogPrintf("Skipping evodb repair during reindex\n");
            WITH_LOCK(::cs_main, node.dmnman->CompleteRepair());  // Mark as repaired since we're rebuilding fresh
        } else if (node.dmnman->IsRepaired() && !args.GetBoolArg("-forceevodbrepair", false)) {
            LogPrintf("Masternode list diffs are already repaired\n");
        } else {
            const CBlockIndex* start_index;
            const CBlockIndex* stop_index;
            {
                LOCK(cs_main);
                const auto& consensus_params = Params().GetConsensus();
                start_index = chainman.ActiveChain()[consensus_params.DIP0003Height];
                stop_index = chainman.ActiveChain().Tip();
            }

            if (start_index && stop_index && start_index->nHeight < stop_index->nHeight) {
                LogPrintf("Verifying and repairing masternode list diffs...\n");
                const auto start{SteadyClock::now()};
                // Create a callback that wraps CSpecialTxProcessor::BuildNewListFromBlock
                auto build_list_func =
                    [&node, &chainman](const CBlock& block, const CBlockIndex* const pindexPrev,
                                       const CDeterministicMNList& prevList, const CCoinsViewCache& view, bool debugLogs,
                                       BlockValidationState& state, CDeterministicMNList& mnListRet) -> bool {
                    const bool is_v24_active{DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_V24)};
                    return node.chain_helper->special_tx->RebuildListFromBlock(block, pindexPrev, is_v24_active, prevList,
                                                                               view, debugLogs, state, mnListRet);
                };
                auto result = node.dmnman->RecalculateAndRepairDiffs(start_index, stop_index, build_list_func, true);

                if (!result.verification_errors.empty()) {
                    LogPrintf("WARNING: Verification errors:\n%s\n", Join(result.verification_errors, "\n"));
                }

                if (!result.repair_errors.empty()) {
                    // Critical errors occurred - reindex required
                    LogPrintf("Failed to repair masternode list diffs. Database corruption detected. " /* Continued */
                              "Please restart with -reindex to rebuild the database.\n"
                              "Errors:\n%s\n",
                              Join(result.repair_errors, "\n"));
                    StartShutdown();
                    return;
                }
                WITH_LOCK(::cs_main, node.dmnman->CompleteRepair());
                LogPrintf("Successfully repaired %d masternode list diffs, verified %d snapshots in %ds\n",
                          result.diffs_recalculated, result.snapshots_verified,
                          Ticks<std::chrono::seconds>(SteadyClock::now() - start));
            }
        }

        if (node.active_ctx) {
            node.active_ctx->nodeman->Init(WITH_LOCK(::cs_main, return chainman.ActiveTip()));
            // Now that nodeman->Init has set proTxHash, fan out the
            // startup tip to all CValidationInterface subscribers.
            // The earlier call only kicked CDSNotificationInterface
            // directly because the full broadcast (NetQuorum,
            // ActiveContext) depends on a valid proTxHash.
            const CBlockIndex* tip = WITH_LOCK(::cs_main, return chainman.ActiveTip());
            const bool ibd = chainman.ActiveChainstate().IsInitialBlockDownload();
            GetMainSignals().InitializeCurrentBlockTip(tip, ibd);
        }
    });
#ifdef ENABLE_WALLET
    if (!args.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        g_wallet_init_interface.AutoLockMasternodeCollaterals(*node.wallet_loader);
    }
#endif // ENABLE_WALLET

    // Wait for genesis block to be processed
    {
        WAIT_LOCK(g_genesis_wait_mutex, lock);
        // We previously could hang here if StartShutdown() is called prior to
        // ThreadImport getting started, so instead we just wait on a timer to
        // check ShutdownRequested() regularly.
        while (!fHaveGenesis && !ShutdownRequested()) {
            g_genesis_wait_cv.wait_for(lock, std::chrono::milliseconds(500));
        }
        block_notify_genesis_wait_connection.disconnect();
    }

    // As importing blocks can take several minutes, it's possible the user
    // requested to kill the GUI during one of the last operations. If so, exit.
    if (ShutdownRequested()) {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    // ********************************************************* Step 12: start node

    //// debug print
    {
        LOCK(cs_main);
        LogPrintf("block tree size = %u\n", chainman.BlockIndex().size());
        chain_active_height = chainman.ActiveChain().Height();
        if (tip_info) {
            tip_info->block_height = chain_active_height;
            tip_info->block_time = chainman.ActiveChain().Tip() ? chainman.ActiveChain().Tip()->GetBlockTime() : chainman.GetParams().GenesisBlock().GetBlockTime();
            tip_info->block_hash = chainman.ActiveChain().Tip() ? chainman.ActiveChain().Tip()->GetBlockHash() : chainman.GetParams().GenesisBlock().GetHash();
            tip_info->verification_progress = GuessVerificationProgress(chainman.GetParams().TxData(), chainman.ActiveChain().Tip());
        }
        if (tip_info && chainman.m_best_header) {
            tip_info->header_height = chainman.m_best_header->nHeight;
            tip_info->header_time = chainman.m_best_header->GetBlockTime();
        }
    }
    LogPrintf("nBestHeight = %d\n", chain_active_height);
    if (node.peerman) node.peerman->SetBestHeight(chain_active_height);

    // Map ports with UPnP or NAT-PMP.
    StartMapPort(args.GetBoolArg("-upnp", DEFAULT_UPNP), args.GetBoolArg("-natpmp", DEFAULT_NATPMP));

    CConnman::Options connOptions;
    connOptions.nLocalServices = nLocalServices;
    connOptions.nMaxConnections = nMaxConnections;
    connOptions.m_max_outbound_full_relay = std::min(MAX_OUTBOUND_FULL_RELAY_CONNECTIONS, connOptions.nMaxConnections);
    connOptions.m_max_outbound_block_relay = std::min(MAX_BLOCK_RELAY_ONLY_CONNECTIONS, connOptions.nMaxConnections-connOptions.m_max_outbound_full_relay);
    connOptions.m_max_outbound_onion = std::min(MAX_DESIRED_ONION_CONNECTIONS, connOptions.nMaxConnections / 2);
    connOptions.nMaxAddnode = MAX_ADDNODE_CONNECTIONS;
    connOptions.nMaxFeeler = MAX_FEELER_CONNECTIONS;
    connOptions.uiInterface = &uiInterface;
    connOptions.m_banman = node.banman.get();
    connOptions.m_msgproc = node.peerman.get();
    connOptions.nSendBufferMaxSize = 1000 * args.GetIntArg("-maxsendbuffer", DEFAULT_MAXSENDBUFFER);
    connOptions.nReceiveFloodSize = 1000 * args.GetIntArg("-maxreceivebuffer", DEFAULT_MAXRECEIVEBUFFER);
    connOptions.m_added_nodes = args.GetArgs("-addnode");
    connOptions.nMaxOutboundLimit = *opt_max_upload;
    connOptions.m_peer_connect_timeout = peer_connect_timeout;
    connOptions.socketEventsMode = ::g_socket_events_mode;
    connOptions.m_active_masternode = node.active_ctx != nullptr;
    // wallets may mix with CoinJoin, which connects to the mixing masternode
    connOptions.m_masternode_connections = node.active_ctx != nullptr || quorums_watch ||
                                           node.wallet_loader != nullptr;
    connOptions.whitelist_forcerelay = args.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY);
    connOptions.whitelist_relay = args.GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY);

    // Port to bind to if `-bind=addr` is provided without a `:port` suffix.
    const uint16_t default_bind_port =
        static_cast<uint16_t>(args.GetIntArg("-port", chainman.GetParams().GetDefaultPort()));

    const auto BadPortWarning = [](const char* prefix, uint16_t port) {
        return strprintf(_("%s request to listen on port %u. This port is considered \"bad\" and "
                           "thus it is unlikely that any peer will connect to it. See "
                           "doc/p2p-bad-ports.md for details and a full list."),
                         prefix,
                         port);
    };

    for (const std::string& bind_arg : args.GetArgs("-bind")) {
        std::optional<CService> bind_addr;
        const size_t index = bind_arg.rfind('=');
        if (index == std::string::npos) {
            bind_addr = Lookup(bind_arg, default_bind_port, /*fAllowLookup=*/false);
            if (bind_addr.has_value()) {
                connOptions.vBinds.push_back(bind_addr.value());
                if (IsBadPort(bind_addr.value().GetPort())) {
                    InitWarning(BadPortWarning("-bind", bind_addr.value().GetPort()));
                }
                continue;
            }
        } else {
            const std::string network_type = bind_arg.substr(index + 1);
            if (network_type == "onion") {
                const std::string truncated_bind_arg = bind_arg.substr(0, index);
                bind_addr = Lookup(truncated_bind_arg, BaseParams().OnionServiceTargetPort(), false);
                if (bind_addr.has_value()) {
                    connOptions.onion_binds.push_back(bind_addr.value());
                    continue;
                }
            }
        }
        return InitError(ResolveErrMsg("bind", bind_arg));
    }

    for (const std::string& strBind : args.GetArgs("-whitebind")) {
        NetWhitebindPermissions whitebind;
        bilingual_str error;
        if (!NetWhitebindPermissions::TryParse(strBind, whitebind, error)) return InitError(error);
        connOptions.vWhiteBinds.push_back(whitebind);
    }

    // If the user did not specify -bind= or -whitebind= then we bind
    // on any address - 0.0.0.0 (IPv4) and :: (IPv6).
    connOptions.bind_on_any = args.GetArgs("-bind").empty() && args.GetArgs("-whitebind").empty();

    // Emit a warning if a bad port is given to -port= but only if -bind and -whitebind are not
    // given, because if they are, then -port= is ignored.
    if (connOptions.bind_on_any && args.IsArgSet("-port")) {
        const uint16_t port_arg = args.GetIntArg("-port", 0);
        if (IsBadPort(port_arg)) {
            InitWarning(BadPortWarning("-port", port_arg));
        }
    }

    const bool listen_onion{args.GetBoolArg("-listenonion", DEFAULT_LISTEN_ONION)};
    const bool listen_onion_explicit{listen_onion && args.IsArgSet("-listenonion")};
    if (connOptions.onion_binds.empty() && (connOptions.vBinds.empty() || listen_onion)) {
        connOptions.onion_binds.push_back(DefaultOnionServiceTarget());
        connOptions.onion_bind_optional = !listen_onion_explicit;
    }

    std::optional<CService> onion_service_target;
    if (listen_onion) {
        // A wildcard listener can't tag the peers Tor forwards to it as onion.
        if (std::ranges::any_of(connOptions.onion_binds, [](auto& b) { return b.IsBindAny(); })) {
            return InitError(_("The automatic Tor onion service cannot use a wildcard onion bind because the Tor daemon wouldn't be able to forward incoming connections to us. Use a specific address such as -bind=127.0.0.1:<port>=onion, or disable the service with -listenonion=0."));
        }
        onion_service_target = connOptions.onion_binds.front();
        if (connOptions.onion_binds.size() > 1) {
            InitWarning(strprintf(_("More than one onion bind address is provided. Using %s "
                                    "for the automatically created Tor onion service."),
                                  onion_service_target->ToStringAddrPort()));
        }
    }

    if (connOptions.bind_on_any) {
        // Only add all IP addresses of the machine if we would be listening on
        // any address - 0.0.0.0 (IPv4) and :: (IPv6).
        Discover();
    }

    for (const auto& net : args.GetArgs("-whitelist")) {
        NetWhitelistPermissions subnet;
        ConnectionDirection connection_direction;
        bilingual_str error;
        if (!NetWhitelistPermissions::TryParse(net, subnet, connection_direction, error)) return InitError(error);
        if (connection_direction & ConnectionDirection::In) {
            connOptions.vWhitelistedRangeIncoming.push_back(subnet);
        }
        if (connection_direction & ConnectionDirection::Out) {
            connOptions.vWhitelistedRangeOutgoing.push_back(subnet);
        }
    }

    connOptions.vSeedNodes = args.GetArgs("-seednode");

    // Initiate outbound connections unless connect=0
    connOptions.m_use_addrman_outgoing = !args.IsArgSet("-connect");
    if (!connOptions.m_use_addrman_outgoing) {
        const auto connect = args.GetArgs("-connect");
        if (connect.size() != 1 || connect[0] != "0") {
            connOptions.m_specified_outgoing = connect;
        }
        if (!connOptions.m_specified_outgoing.empty() && !connOptions.vSeedNodes.empty()) {
            LogPrintf("-seednode is ignored when -connect is used\n");
        }

        if (args.IsArgSet("-dnsseed") && args.GetBoolArg("-dnsseed", DEFAULT_DNSSEED) && args.IsArgSet("-proxy")) {
            LogPrintf("-dnsseed is ignored when -connect is used and -proxy is specified\n");
        }
    }

    const std::string& i2psam_arg = args.GetArg("-i2psam", "");
    if (!i2psam_arg.empty()) {
        const std::optional<CService> addr{Lookup(i2psam_arg, 7656, fNameLookup)};
        if (!addr.has_value() || !addr->IsValid()) {
            return InitError(strprintf(_("Invalid -i2psam address or hostname: '%s'"), i2psam_arg));
        }
        SetProxy(NET_I2P, Proxy{addr.value()});
    } else {
        if (args.IsArgSet("-onlynet") && g_reachable_nets.Contains(NET_I2P)) {
            return InitError(
                _("Outbound connections restricted to i2p (-onlynet=i2p) but "
                  "-i2psam is not provided"));
        }
        g_reachable_nets.Remove(NET_I2P);
    }

    connOptions.m_i2p_accept_incoming = args.GetBoolArg("-i2pacceptincoming", DEFAULT_I2P_ACCEPT_INCOMING);

    if (!node.connman->Start(*node.dmnman, *node.mn_metaman, *node.mn_sync, *node.scheduler, connOptions)) {
        return false;
    }

    if (listen_onion) {
        // The optional default onion bind may have failed to bind.
        if (onion_service_target && !node.connman->HasOnionBind(*onion_service_target)) {
            onion_service_target.reset();
        }
        StartTorControl(onion_service_target);
    }

    // ********************************************************* Step 13: finished

    // At this point, the RPC is "started", but still in warmup, which means it
    // cannot yet be called. Before we make it callable, we need to make sure
    // that the RPC's view of the best block is valid and consistent with
    // ChainstateManager's active tip.
    //
    // If we do not do this, RPC's view of the best block will be height=0 and
    // hash=0x0. This will lead to erroroneous responses for things like
    // waitforblockheight.
    RPCNotifyBlockChange(WITH_LOCK(chainman.GetMutex(), return chainman.ActiveTip()));
    SetRPCWarmupFinished();

    uiInterface.InitMessage(_("Done loading").translated);

    for (const auto& client : node.chain_clients) {
        client->start(*node.scheduler);
    }

    BanMan* banman = node.banman.get();
    node.scheduler->scheduleEvery([banman]{
        banman->DumpBanlist();
    }, DUMP_BANS_INTERVAL);

    if (node.peerman) node.peerman->StartScheduledTasks(*node.scheduler);

#if HAVE_SYSTEM
    StartupNotify(args);
#endif

    return true;
}
