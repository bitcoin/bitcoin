// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <init.h>

#include <kernel/checks.h>

#include <addrman.h>
#include <banman.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <consensus/amount.h>
#include <deploymentstatus.h>
#include <hash.h>
#include <httprpc.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/coinstatsindex.h>
#include <index/txindex.h>
#include <init/common.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <interfaces/mining.h>
#include <interfaces/node.h>
#include <kernel/context.h>
#include <key.h>
#include <logging.h>
#include <mapport.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <netbase.h>
#include <netgroup.h>
#include <node/blockmanager_args.h>
#include <node/blockstorage.h>
#include <node/caches.h>
#include <node/chainstate.h>
#include <node/chainstatemanager_args.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <node/kernel_notifications.h>
#include <node/mempool_args.h>
#include <node/mempool_persist.h>
#include <node/mempool_persist_args.h>
#include <node/miner.h>
#include <node/peerman_args.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/fees_args.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <protocol.h>
#include <rpc/blockchain.h>
#include <rpc/register.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <scheduler.h>
#include <script/sigcache.h>
#include <sync.h>
#include <torcontrol.h>
#include <txdb.h>
#include <txmempool.h>
#include <util/asmap.h>
#include <util/batchpriority.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/syserror.h>
#include <util/thread.h>
#include <util/threadnames.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <walletinitinterface.h>

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#ifndef WIN32
#include <cerrno>
#include <signal.h>
#include <sys/stat.h>
#endif

#include <boost/signals2/signal.hpp>

#ifdef ENABLE_ZMQ
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqrpc.h>
#endif

using common::AmountErrMsg;
using common::InvalidPortErrMsg;
using common::ResolveErrMsg;

using node::ApplyArgsManOptions;
using node::BlockManager;
using node::CacheSizes;
using node::CalculateCacheSizes;
using node::DEFAULT_PERSIST_MEMPOOL;
using node::DEFAULT_PRINT_MODIFIED_FEE;
using node::DEFAULT_STOPATHEIGHT;
using node::DumpMempool;
using node::LoadMempool;
using node::KernelNotifications;
using node::LoadChainstate;
using node::MempoolPath;
using node::NodeContext;
using node::ShouldPersistMempool;
using node::ImportBlocks;
using node::VerifyLoadedChainstate;
using util::Join;
using util::ReplaceAll;
using util::ToString;

static constexpr bool DEFAULT_PROXYRANDOMIZE{true};
static constexpr bool DEFAULT_REST_ENABLE{false};
static constexpr bool DEFAULT_I2P_ACCEPT_INCOMING{true};
static constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};

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
static const char* BITCOIN_PID_FILENAME = "bitcoind.pid";
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

static std::optional<util::SignalInterrupt> g_shutdown;

void InitContext(NodeContext& node)
{
    assert(!g_shutdown);
    g_shutdown.emplace();

    node.args = &gArgs;
    node.shutdown = &*g_shutdown;
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
// A clean exit happens when the SignalInterrupt object is triggered, which
// makes the main thread's SignalInterrupt::wait() call return, and join all
// other ongoing threads in the thread group to the main thread.
// Shutdown() is then called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//
// Shutdown for Qt is very similar, only it uses a QTimer to detect
// ShutdownRequested() getting set, and then does the normal Qt
// shutdown thing.
//

bool ShutdownRequested(node::NodeContext& node)
{
    return bool{*Assert(node.shutdown)};
}

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
    InterruptMapPort();
    if (node.connman)
        node.connman->Interrupt();
    for (auto* index : node.indexes) {
        index->Interrupt();
    }
}

void Shutdown(NodeContext& node)
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
    for (const auto& client : node.chain_clients) {
        client->flush();
    }
    StopMapPort();

    // Because these depend on each-other, we make sure that neither can be
    // using the other before destroying them.
    if (node.peerman && node.validation_signals) node.validation_signals->UnregisterValidationInterface(node.peerman.get());
    if (node.connman) node.connman->Stop();

    StopTorControl();

    if (node.chainman && node.chainman->m_thread_load.joinable()) node.chainman->m_thread_load.join();
    // After everything has been shut down, but before things get flushed, stop the
    // the scheduler. After this point, SyncWithValidationInterfaceQueue() should not be called anymore
    // as this would prevent the shutdown from completing.
    if (node.scheduler) node.scheduler->stop();

    // After the threads that potentially access these pointers have been stopped,
    // destruct and reset all to nullptr.
    node.peerman.reset();
    node.connman.reset();
    node.banman.reset();
    node.addrman.reset();
    node.netgroupman.reset();

    if (node.mempool && node.mempool->GetLoadTried() && ShouldPersistMempool(*node.args)) {
        DumpMempool(*node.mempool, MempoolPath(*node.args));
    }

    // Drop transactions we were still watching, record fee estimations and unregister
    // fee estimator from validation interface.
    if (node.fee_estimator) {
        node.fee_estimator->Flush();
        if (node.validation_signals) {
            node.validation_signals->UnregisterValidationInterface(node.fee_estimator.get());
        }
    }

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
    if (node.validation_signals) node.validation_signals->FlushBackgroundCallbacks();

    // Stop and delete all indexes only after flushing background callbacks.
    for (auto* index : node.indexes) index->Stop();
    if (g_txindex) g_txindex.reset();
    if (g_coin_stats_index) g_coin_stats_index.reset();
    DestroyAllBlockFilterIndexes();
    node.indexes.clear(); // all instances are nullptr now

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
    }
    for (const auto& client : node.chain_clients) {
        client->stop();
    }

#ifdef ENABLE_ZMQ
    if (g_zmq_notification_interface) {
        if (node.validation_signals) node.validation_signals->UnregisterValidationInterface(g_zmq_notification_interface.get());
        g_zmq_notification_interface.reset();
    }
#endif

    node.chain_clients.clear();
    if (node.validation_signals) {
        node.validation_signals->UnregisterAllValidationInterfaces();
    }
    node.mempool.reset();
    node.fee_estimator.reset();
    node.chainman.reset();
    node.validation_signals.reset();
    node.scheduler.reset();
    node.ecc_context.reset();
    node.kernel.reset();

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
    // Return value is intentionally ignored because there is not a better way
    // of handling this failure in a signal handler.
    (void)(*Assert(g_shutdown))();
}

static void HandleSIGHUP(int)
{
    LogInstance().m_reopen_file = true;
}
#else
static BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType)
{
    if (!(*Assert(g_shutdown))()) {
        LogError("Failed to send shutdown signal on Ctrl-C\n");
        return false;
    }
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
    argsman.AddArg("-help-debug", "Print help message with debugging options and exit", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST); // server-only for now

    init::AddLoggingArgs(argsman);

    const auto defaultBaseParams = CreateBaseChainParams(ChainType::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(ChainType::TESTNET);
    const auto signetBaseParams = CreateBaseChainParams(ChainType::SIGNET);
    const auto regtestBaseParams = CreateBaseChainParams(ChainType::REGTEST);
    const auto defaultChainParams = CreateChainParams(argsman, ChainType::MAIN);
    const auto testnetChainParams = CreateChainParams(argsman, ChainType::TESTNET);
    const auto signetChainParams = CreateChainParams(argsman, ChainType::SIGNET);
    const auto regtestChainParams = CreateChainParams(argsman, ChainType::REGTEST);

    // Hidden Options
    std::vector<std::string> hidden_args = {
        "-dbcrashratio", "-forcecompactdb",
        // GUI args. These will be overwritten by SetupUIArgs for the GUI
        "-choosedatadir", "-lang=<lang>", "-min", "-resetguisettings", "-splash", "-uiplatform"};

    argsman.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-alertnotify=<cmd>", "Execute command when an alert is raised (%s in cmd is replaced by message)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-assumevalid=<hex>", strprintf("If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet: %s, signet: %s)", defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex(), signetChainParams->GetConsensus().defaultAssumeValid.GetHex()), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksdir=<dir>", "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksxor",
                   strprintf("Whether an XOR-key applies to blocksdir *.dat files. "
                             "The created XOR-key will be zeros for an existing blocksdir or when `-blocksxor=0` is "
                             "set, and random for a freshly initialized blocksdir. "
                             "(default: %u)",
                             kernel::DEFAULT_XOR_BLOCKSDIR),
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-fastprune", "Use smaller block files and lower minimum prune height for testing purposes", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
#if HAVE_SYSTEM
    argsman.AddArg("-blocknotify=<cmd>", "Execute command when the best block changes (%s in cmd is replaced by block hash)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-blockreconstructionextratxn=<n>", strprintf("Extra transactions to keep in memory for compact block reconstructions (default: %u)", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blocksonly", strprintf("Whether to reject transactions from network peers. Disables automatic broadcast and rebroadcast of transactions, unless the source peer has the 'forcerelay' permission. RPC transactions are not affected. (default: %u)", DEFAULT_BLOCKSONLY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-coinstatsindex", strprintf("Maintain coinstats index used by the gettxoutsetinfo RPC (default: %u)", DEFAULT_COINSTATSINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-conf=<file>", strprintf("Specify path to read-only configuration file. Relative paths will be prefixed by datadir location (only useable from command line, not configuration file) (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbbatchsize", strprintf("Maximum database write batch size in bytes (default: %u)", nDefaultDbBatchSize), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-dbcache=<n>", strprintf("Maximum database cache size <n> MiB (%d to %d, default: %d). In addition, unused mempool memory is shared for this cache (see -maxmempool).", nMinDbCache, nMaxDbCache, nDefaultDbCache), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-includeconf=<file>", "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-allowignoredconf", strprintf("For backwards compatibility, treat an unused %s file in the datadir as a warning, not an error.", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-loadblock=<file>", "Imports blocks from external file on startup", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxmempool=<n>", strprintf("Keep the transaction memory pool below <n> megabytes (default: %u)", DEFAULT_MAX_MEMPOOL_SIZE_MB), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-maxorphantx=<n>", strprintf("Keep at most <n> unconnectable transactions in memory (default: %u)", DEFAULT_MAX_ORPHAN_TRANSACTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-mempoolexpiry=<n>", strprintf("Do not keep transactions in the mempool longer than <n> hours (default: %u)", DEFAULT_MEMPOOL_EXPIRY_HOURS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-minimumchainwork=<hex>", strprintf("Minimum work assumed to exist on a valid chain in hex (default: %s, testnet: %s, signet: %s)", defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex(), signetChainParams->GetConsensus().nMinimumChainWork.GetHex()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    argsman.AddArg("-par=<n>", strprintf("Set the number of script verification threads (0 = auto, up to %d, <0 = leave that many cores free, default: %d)",
        MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-persistmempool", strprintf("Whether to save the mempool on shutdown and load on restart (default: %u)", DEFAULT_PERSIST_MEMPOOL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-persistmempoolv1",
                   strprintf("Whether a mempool.dat file created by -persistmempool or the savemempool RPC will be written in the legacy format "
                             "(version 1) or the current format (version 2). This temporary option will be removed in the future. (default: %u)",
                             DEFAULT_PERSIST_V1_DAT),
                   ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-pid=<file>", strprintf("Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)", BITCOIN_PID_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-prune=<n>", strprintf("Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >=%u = automatically prune block files to stay under the specified target size in MiB)", MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-reindex", "If enabled, wipe chain state and block index, and rebuild them from blk*.dat files on disk. Also wipe and rebuild other optional indexes that are active. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-reindex-chainstate", "If enabled, wipe chain state, and rebuild it from blk*.dat files on disk. If an assumeutxo snapshot was loaded, its chainstate will be wiped as well. The snapshot can then be reloaded via RPC.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-settings=<file>", strprintf("Specify path to dynamic settings data file. Can be disabled with -nosettings. File is written at runtime and not meant to be edited by users (use %s instead for custom settings). Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME, BITCOIN_SETTINGS_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    argsman.AddArg("-startupnotify=<cmd>", "Execute command on startup.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-shutdownnotify=<cmd>", "Execute command immediately before beginning shutdown. The need for shutdown may be urgent, so be careful not to delay it long (if the command doesn't require interaction with the server, consider having it fork into the background).", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    argsman.AddArg("-txindex", strprintf("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)", DEFAULT_TXINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    argsman.AddArg("-blockfilterindex=<type>",
                 strprintf("Maintain an index of compact filters by block (default: %s, values: %s).", DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()) +
                 " If <type> is not supplied or if <type> = 1, indexes for all known types are enabled.",
                 ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    argsman.AddArg("-addnode=<ip>", strprintf("Add a node to connect to and attempt to keep the connection open (see the addnode RPC help for more info). This option can be specified multiple times to add multiple nodes; connections are limited to %u at a time and are counted separately from the -maxconnections limit.", MAX_ADDNODE_CONNECTIONS), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-asmap=<file>", strprintf("Specify asn mapping used for bucketing of the peers (default: %s). Relative paths will be prefixed by the net-specific datadir location.", DEFAULT_ASMAP_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bantime=<n>", strprintf("Default duration (in seconds) of manually configured bans (default: %u)", DEFAULT_MISBEHAVING_BANTIME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-bind=<addr>[:<port>][=onion]", strprintf("Bind to given address and always listen on it (default: 0.0.0.0). Use [host]:port notation for IPv6. Append =onion to tag any incoming connections to that address and port as incoming Tor connections (default: 127.0.0.1:%u=onion, testnet: 127.0.0.1:%u=onion, signet: 127.0.0.1:%u=onion, regtest: 127.0.0.1:%u=onion)", defaultBaseParams->OnionServiceTargetPort(), testnetBaseParams->OnionServiceTargetPort(), signetBaseParams->OnionServiceTargetPort(), regtestBaseParams->OnionServiceTargetPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-cjdnsreachable", "If set, then this host is configured for CJDNS (connecting to fc00::/8 addresses would lead us to the CJDNS network, see doc/cjdns.md) (default: 0)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-connect=<ip>", "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-discover", "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dns", strprintf("Allow DNS lookups for -addnode, -seednode and -connect (default: %u)", DEFAULT_NAME_LOOKUP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-dnsseed", strprintf("Query for peer addresses via DNS lookup, if low on addresses (default: %u unless -connect used or -maxconnections=0)", DEFAULT_DNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-externalip=<ip>", "Specify your own public address", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-fixedseeds", strprintf("Allow fixed seeds if DNS seeds don't provide peers (default: %u)", DEFAULT_FIXEDSEEDS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-forcednsseed", strprintf("Always query for peer addresses via DNS lookup (default: %u)", DEFAULT_FORCEDNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listen", strprintf("Accept connections from outside (default: %u if no -proxy, -connect or -maxconnections=0)", DEFAULT_LISTEN), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-listenonion", strprintf("Automatically create Tor onion service (default: %d)", DEFAULT_LISTEN_ONION), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxconnections=<n>", strprintf("Maintain at most <n> automatic connections to peers (default: %u). This limit does not apply to connections manually added via -addnode or the addnode RPC, which have a separate limit of %u.", DEFAULT_MAX_PEER_CONNECTIONS, MAX_ADDNODE_CONNECTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxreceivebuffer=<n>", strprintf("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXRECEIVEBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxsendbuffer=<n>", strprintf("Maximum per-connection memory usage for the send buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXSENDBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-maxuploadtarget=<n>", strprintf("Tries to keep outbound traffic under the given target per 24h. Limit does not apply to peers with 'download' permission or blocks created within past week. 0 = no limit (default: %s). Optional suffix units [k|K|m|M|g|G|t|T] (default: M). Lowercase is 1000 base while uppercase is 1024 base", DEFAULT_MAX_UPLOAD_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#ifdef HAVE_SOCKADDR_UN
    argsman.AddArg("-onion=<ip:port|path>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy). May be a local file path prefixed with 'unix:'.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    argsman.AddArg("-onion=<ip:port>", "Use separate SOCKS5 proxy to reach peers via Tor onion services, set -noonion to disable (default: -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#endif
    argsman.AddArg("-i2psam=<ip:port>", "I2P SAM proxy to reach I2P peers and accept I2P connections (default: none)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-i2pacceptincoming", strprintf("Whether to accept inbound I2P connections (default: %i). Ignored if -i2psam is not set. Listening for inbound I2P connections is done through the SAM proxy, not by binding to a local address and port.", DEFAULT_I2P_ACCEPT_INCOMING), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-onlynet=<net>", "Make automatic outbound connections only to network <net> (" + Join(GetNetworkNames(), ", ") + "). Inbound and manual connections are not affected by this option. It can be specified multiple times to allow multiple networks.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-v2transport", strprintf("Support v2 transport (default: %u)", DEFAULT_V2_TRANSPORT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerbloomfilters", strprintf("Support filtering of blocks and transaction with bloom filters (default: %u)", DEFAULT_PEERBLOOMFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peerblockfilters", strprintf("Serve compact block filters to peers per BIP 157 (default: %u)", DEFAULT_PEERBLOCKFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-txreconciliation", strprintf("Enable transaction reconciliations per BIP 330 (default: %d)", DEFAULT_TXRECONCILIATION_ENABLE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
    argsman.AddArg("-port=<port>", strprintf("Listen for connections on <port> (default: %u, testnet: %u, signet: %u, regtest: %u). Not relevant for I2P (see doc/i2p.md).", defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), signetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
#ifdef HAVE_SOCKADDR_UN
    argsman.AddArg("-proxy=<ip:port|path>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled). May be a local file path prefixed with 'unix:' if the proxy supports it.", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_ELISION, OptionsCategory::CONNECTION);
#else
    argsman.AddArg("-proxy=<ip:port>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled)", ArgsManager::ALLOW_ANY | ArgsManager::DISALLOW_ELISION, OptionsCategory::CONNECTION);
#endif
    argsman.AddArg("-proxyrandomize", strprintf("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)", DEFAULT_PROXYRANDOMIZE), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-seednode=<ip>", "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes. During startup, seednodes will be tried before dnsseeds.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-networkactive", "Enable all P2P network activity (default: 1). Can be changed by the setnetworkactive RPC command", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-timeout=<n>", strprintf("Specify socket connection timeout in milliseconds. If an initial attempt to connect is unsuccessful after this amount of time, drop it (minimum: 1, default: %d)", DEFAULT_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    argsman.AddArg("-peertimeout=<n>", strprintf("Specify a p2p connection timeout delay in seconds. After connecting to a peer, wait this amount of time before considering disconnection based on inactivity (minimum: 1, default: %d)", DEFAULT_PEER_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
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

#ifdef ENABLE_ZMQ
    argsman.AddArg("-zmqpubhashblock=<address>", "Enable publish hash block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtx=<address>", "Enable publish hash transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblock=<address>", "Enable publish raw block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtx=<address>", "Enable publish raw transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequence=<address>", "Enable publish hash block and tx sequence in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashblockhwm=<n>", strprintf("Set publish hash block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubhashtxhwm=<n>", strprintf("Set publish hash transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawblockhwm=<n>", strprintf("Set publish raw block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubrawtxhwm=<n>", strprintf("Set publish raw transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    argsman.AddArg("-zmqpubsequencehwm=<n>", strprintf("Set publish hash sequence message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
#else
    hidden_args.emplace_back("-zmqpubhashblock=<address>");
    hidden_args.emplace_back("-zmqpubhashtx=<address>");
    hidden_args.emplace_back("-zmqpubrawblock=<address>");
    hidden_args.emplace_back("-zmqpubrawtx=<address>");
    hidden_args.emplace_back("-zmqpubsequence=<n>");
    hidden_args.emplace_back("-zmqpubhashblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubsequencehwm=<n>");
#endif

    argsman.AddArg("-checkblocks=<n>", strprintf("How many blocks to check at startup (default: %u, 0 = all)", DEFAULT_CHECKBLOCKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checklevel=<n>", strprintf("How thorough the block verification of -checkblocks is: %s (0-4, default: %u)", Join(CHECKLEVEL_DOC, ", "), DEFAULT_CHECKLEVEL), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkblockindex", strprintf("Do a consistency check for the block tree, chainstate, and other validation data structures every <n> operations. Use 0 to disable. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkaddrman=<n>", strprintf("Run addrman consistency checks every <n> operations. Use 0 to disable. (default: %u)", DEFAULT_ADDRMAN_CONSISTENCY_CHECKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkmempool=<n>", strprintf("Run mempool consistency checks every <n> transactions. Use 0 to disable. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-checkpoints", strprintf("Enable rejection of any forks from the known historical chain until block %s (default: %u)", defaultChainParams->Checkpoints().GetHeight(), DEFAULT_CHECKPOINTS_ENABLED), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-deprecatedrpc=<method>", "Allows deprecated RPC method(s) to be used", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopafterblockimport", strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-stopatheight", strprintf("Stop running after reaching the given height in the main chain (default: %u)", DEFAULT_STOPATHEIGHT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitancestorcount=<n>", strprintf("Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)", DEFAULT_ANCESTOR_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitancestorsize=<n>", strprintf("Do not accept transactions whose size with all in-mempool ancestors exceeds <n> kilobytes (default: %u)", DEFAULT_ANCESTOR_SIZE_LIMIT_KVB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitdescendantcount=<n>", strprintf("Do not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u)", DEFAULT_DESCENDANT_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-limitdescendantsize=<n>", strprintf("Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool descendants (default: %u).", DEFAULT_DESCENDANT_SIZE_LIMIT_KVB), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-test=<option>", "Pass a test-only option. Options include : " + Join(TEST_OPTIONS_DOC, ", ") + ".", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-capturemessages", "Capture all P2P messages to disk", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mocktime=<n>", "Replace actual time with " + UNIX_EPOCH_TIME + " (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxsigcachesize=<n>", strprintf("Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)", DEFAULT_VALIDATION_CACHE_BYTES >> 20), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-maxtipage=<n>",
                   strprintf("Maximum tip age in seconds to consider node in initial block download (default: %u)",
                             Ticks<std::chrono::seconds>(DEFAULT_MAX_TIP_AGE)),
                   ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-printpriority", strprintf("Log transaction fee rate in " + CURRENCY_UNIT + "/kvB when mining blocks (default: %u)", DEFAULT_PRINT_MODIFIED_FEE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-uacomment=<cmt>", "Append comment to the user agent string", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    SetupChainParamsBaseOptions(argsman);

    argsman.AddArg("-acceptnonstdtxn", strprintf("Relay and mine \"non-standard\" transactions (test networks only; default: %u)", DEFAULT_ACCEPT_NON_STD_TXN), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-incrementalrelayfee=<amt>", strprintf("Fee rate (in %s/kvB) used to define cost of relay, used for mempool limiting and replacement policy. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-dustrelayfee=<amt>", strprintf("Fee rate (in %s/kvB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)", CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-acceptstalefeeestimates", strprintf("Read fee estimates even if they are stale (%sdefault: %u) fee estimates are considered stale if they are %s hours old", "regtest only; ", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES, Ticks<std::chrono::hours>(MAX_FILE_AGE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-bytespersigop", strprintf("Equivalent bytes per sigop in transactions for relay and mining (default: %u)", DEFAULT_BYTES_PER_SIGOP), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarrier", strprintf("Relay and mine data carrier transactions (default: %u)", DEFAULT_ACCEPT_DATACARRIER), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-datacarriersize",
                   strprintf("Relay and mine transactions whose data-carrying raw scriptPubKey "
                             "is of this size or less (default: %u)",
                             MAX_OP_RETURN_RELAY),
                   ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-mempoolfullrbf", strprintf("Accept transaction replace-by-fee without requiring replaceability signaling (default: %u)", DEFAULT_MEMPOOL_FULL_RBF), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-permitbaremultisig", strprintf("Relay transactions creating non-P2SH multisig outputs (default: %u)", DEFAULT_PERMIT_BAREMULTISIG), ArgsManager::ALLOW_ANY,
                   OptionsCategory::NODE_RELAY);
    argsman.AddArg("-minrelaytxfee=<amt>", strprintf("Fees (in %s/kvB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistforcerelay", strprintf("Add 'forcerelay' permission to whitelisted peers with default permissions. This will relay transactions even if the transactions were already in the mempool. (default: %d)", DEFAULT_WHITELISTFORCERELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    argsman.AddArg("-whitelistrelay", strprintf("Add 'relay' permission to whitelisted peers with default permissions. This will accept relayed transactions even when not relaying transactions (default: %d)", DEFAULT_WHITELISTRELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);


    argsman.AddArg("-blockmaxweight=<n>", strprintf("Set maximum BIP141 block weight (default: %d)", DEFAULT_BLOCK_MAX_WEIGHT), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockmintxfee=<amt>", strprintf("Set lowest fee rate (in %s/kvB) for transactions to be included in block creation. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    argsman.AddArg("-blockversion=<n>", "Override block version to test forking scenarios", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::BLOCK_CREATION);

    argsman.AddArg("-rest", strprintf("Accept public REST requests (default: %u)", DEFAULT_REST_ENABLE), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcallowip=<ip>", "Allow JSON-RPC connections from specified source. Valid values for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0), a network/CIDR (e.g. 1.2.3.4/24), all ipv4 (0.0.0.0/0), or all ipv6 (::/0). This option can be specified multiple times", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcauth=<userpw>", "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcauth. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcbind=<addr>[:port]", "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcdoccheck", strprintf("Throw a non-fatal error at runtime if the documentation for an RPC is incorrect (default: %u)", DEFAULT_RPC_DOC_CHECK), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpccookieperms=<readable-by>", strprintf("Set permissions on the RPC auth cookie file so that it is readable by [owner|group|all] (default: owner [via umask 0077])"), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcport=<port>", strprintf("Listen for JSON-RPC connections on <port> (default: %u, testnet: %u, signet: %u, regtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), signetBaseParams->RPCPort(), regtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcservertimeout=<n>", strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-rpcthreads=<n>", strprintf("Set the number of threads to service RPC calls (default: %d)", DEFAULT_HTTP_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY | ArgsManager::SENSITIVE, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelist=<whitelist>", "Set a whitelist to filter incoming RPC calls for a specific user. The field <whitelist> comes in the format: <USERNAME>:<rpc 1>,<rpc 2>,...,<rpc n>. If multiple whitelists are set for a given user, they are set-intersected. See -rpcwhitelistdefault documentation for information on default whitelist behavior.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcwhitelistdefault", "Sets default behavior for rpc whitelisting. Unless rpcwhitelistdefault is set to 0, if any -rpcwhitelist is set, the rpc server acts as if all rpc users are subject to empty-unless-otherwise-specified whitelists. If rpcwhitelistdefault is set to 1 and no -rpcwhitelist is set, rpc server acts as if all rpc users are subject to empty whitelists.", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    argsman.AddArg("-rpcworkqueue=<n>", strprintf("Set the depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    argsman.AddArg("-server", "Accept command line and JSON-RPC commands", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);

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

static bool AppInitServers(NodeContext& node)
{
    const ArgsManager& args = *Assert(node.args);
    RPCServer::OnStarted(&OnRPCStarted);
    RPCServer::OnStopped(&OnRPCStopped);
    if (!InitHTTPServer(*Assert(node.shutdown))) {
        return false;
    }
    StartRPC();
    node.rpc_interruption_point = RpcInterruptionPoint;
    if (!StartHTTPRPC(&node))
        return false;
    if (args.GetBoolArg("-rest", DEFAULT_REST_ENABLE)) StartREST(&node);
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
            LogInfo("parameter interaction: -bind set -> setting -listen=1\n");
    }
    if (args.IsArgSet("-whitebind")) {
        if (args.SoftSetBoolArg("-listen", true))
            LogInfo("parameter interaction: -whitebind set -> setting -listen=1\n");
    }

    if (args.IsArgSet("-connect") || args.GetIntArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS) <= 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (args.SoftSetBoolArg("-dnsseed", false))
            LogInfo("parameter interaction: -connect or -maxconnections=0 set -> setting -dnsseed=0\n");
        if (args.SoftSetBoolArg("-listen", false))
            LogInfo("parameter interaction: -connect or -maxconnections=0 set -> setting -listen=0\n");
    }

    std::string proxy_arg = args.GetArg("-proxy", "");
    if (proxy_arg != "" && proxy_arg != "0") {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (args.SoftSetBoolArg("-listen", false))
            LogInfo("parameter interaction: -proxy set -> setting -listen=0\n");
        // to protect privacy, do not map ports when a proxy is set. The user may still specify -listen=1
        // to listen locally, so don't rely on this happening through -listen below.
        if (args.SoftSetBoolArg("-upnp", false))
            LogInfo("parameter interaction: -proxy set -> setting -upnp=0\n");
        if (args.SoftSetBoolArg("-natpmp", false)) {
            LogInfo("parameter interaction: -proxy set -> setting -natpmp=0\n");
        }
        // to protect privacy, do not discover addresses by default
        if (args.SoftSetBoolArg("-discover", false))
            LogInfo("parameter interaction: -proxy set -> setting -discover=0\n");
    }

    if (!args.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (args.SoftSetBoolArg("-upnp", false))
            LogInfo("parameter interaction: -listen=0 -> setting -upnp=0\n");
        if (args.SoftSetBoolArg("-natpmp", false)) {
            LogInfo("parameter interaction: -listen=0 -> setting -natpmp=0\n");
        }
        if (args.SoftSetBoolArg("-discover", false))
            LogInfo("parameter interaction: -listen=0 -> setting -discover=0\n");
        if (args.SoftSetBoolArg("-listenonion", false))
            LogInfo("parameter interaction: -listen=0 -> setting -listenonion=0\n");
        if (args.SoftSetBoolArg("-i2pacceptincoming", false)) {
            LogInfo("parameter interaction: -listen=0 -> setting -i2pacceptincoming=0\n");
        }
    }

    if (args.IsArgSet("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (args.SoftSetBoolArg("-discover", false))
            LogInfo("parameter interaction: -externalip set -> setting -discover=0\n");
    }

    if (args.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)) {
        // disable whitelistrelay in blocksonly mode
        if (args.SoftSetBoolArg("-whitelistrelay", false))
            LogInfo("parameter interaction: -blocksonly=1 -> setting -whitelistrelay=0\n");
        // Reduce default mempool size in blocksonly mode to avoid unexpected resource usage
        if (args.SoftSetArg("-maxmempool", ToString(DEFAULT_BLOCKSONLY_MAX_MEMPOOL_SIZE_MB)))
            LogInfo("parameter interaction: -blocksonly=1 -> setting -maxmempool=%d\n", DEFAULT_BLOCKSONLY_MAX_MEMPOOL_SIZE_MB);
    }

    // Forcing relay from whitelisted hosts implies we will accept relays from them in the first place.
    if (args.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY)) {
        if (args.SoftSetBoolArg("-whitelistrelay", true))
            LogInfo("parameter interaction: -whitelistforcerelay=1 -> setting -whitelistrelay=1\n");
    }
    if (args.IsArgSet("-onlynet")) {
        const auto onlynets = args.GetArgs("-onlynet");
        bool clearnet_reachable = std::any_of(onlynets.begin(), onlynets.end(), [](const auto& net) {
            const auto n = ParseNetwork(net);
            return n == NET_IPV4 || n == NET_IPV6;
        });
        if (!clearnet_reachable && args.SoftSetBoolArg("-dnsseed", false)) {
            LogInfo("parameter interaction: -onlynet excludes IPv4 and IPv6 -> setting -dnsseed=0\n");
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
ServiceFlags nLocalServices = ServiceFlags(NODE_NETWORK_LIMITED | NODE_WITNESS);
int64_t peer_connect_timeout;
std::set<BlockFilterType> g_enabled_filter_types;

} // namespace

[[noreturn]] static void new_handler_terminate()
{
    // Rather than throwing std::bad-alloc if allocation fails, terminate
    // immediately to (try to) avoid chain corruption.
    // Since logging may itself allocate memory, set the handler directly
    // to terminate first.
    std::set_new_handler(std::terminate);
    LogError("Out of memory. Terminating.\n");

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
    // on the command line or in this chain's section of the config file.
    ChainType chain = args.GetChainType();
    if (chain == ChainType::SIGNET) {
        LogPrintf("Signet derived magic (message start): %s\n", HexStr(chainparams.MessageStart()));
    }
    bilingual_str errors;
    for (const auto& arg : args.GetUnsuitableSectionOnlyArgs()) {
        errors += strprintf(_("Config setting for %s only applied on %s network when in [%s] section.") + Untranslated("\n"), arg, ChainTypeToString(chain), ChainTypeToString(chain));
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

    if (!fs::is_directory(args.GetBlocksDirPath())) {
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
        if (g_enabled_filter_types.count(BlockFilterType::BASIC) != 1) {
            return InitError(_("Cannot set -peerblockfilters without -blockfilterindex."));
        }

        nLocalServices = ServiceFlags(nLocalServices | NODE_COMPACT_FILTERS);
    }

    if (args.GetIntArg("-prune", 0)) {
        if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX))
            return InitError(_("Prune mode is incompatible with -txindex."));
        if (args.GetBoolArg("-reindex-chainstate", false)) {
            return InitError(_("Prune mode is incompatible with -reindex-chainstate. Use full -reindex instead."));
        }
    }

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
    if (auto result{init::SetLoggingCategories(args)}; !result) return InitError(util::ErrorString(result));
    if (auto result{init::SetLoggingLevel(args)}; !result) return InitError(util::ErrorString(result));

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

    if (args.IsArgSet("-test")) {
        if (chainparams.GetChainType() != ChainType::REGTEST) {
            return InitError(Untranslated("-test=<option> can only be used with regtest"));
        }
        const std::vector<std::string> options = args.GetArgs("-test");
        for (const std::string& option : options) {
            auto it = std::find_if(TEST_OPTIONS_DOC.begin(), TEST_OPTIONS_DOC.end(), [&option](const std::string& doc_option) {
                size_t pos = doc_option.find(" (");
                return (pos != std::string::npos) && (doc_option.substr(0, pos) == option);
            });
            if (it == TEST_OPTIONS_DOC.end()) {
                InitWarning(strprintf(_("Unrecognised option \"%s\" provided in -test=<option>."), option));
            }
        }
    }

    // Also report errors from parsing before daemonization
    {
        kernel::Notifications notifications{};
        ChainstateManager::Options chainman_opts_dummy{
            .chainparams = chainparams,
            .datadir = args.GetDataDirNet(),
            .notifications = notifications,
        };
        auto chainman_result{ApplyArgsManOptions(args, chainman_opts_dummy)};
        if (!chainman_result) {
            return InitError(util::ErrorString(chainman_result));
        }
        BlockManager::Options blockman_opts_dummy{
            .chainparams = chainman_opts_dummy.chainparams,
            .blocks_dir = args.GetBlocksDirPath(),
            .notifications = chainman_opts_dummy.notifications,
        };
        auto blockman_result{ApplyArgsManOptions(args, blockman_opts_dummy)};
        if (!blockman_result) {
            return InitError(util::ErrorString(blockman_result));
        }
    }

    return true;
}

static bool LockDataDirectory(bool probeOnly)
{
    // Make sure only a single Bitcoin process is using the data directory.
    const fs::path& datadir = gArgs.GetDataDirNet();
    switch (util::LockDirectory(datadir, ".lock", probeOnly)) {
    case util::LockResult::ErrorWrite:
        return InitError(strprintf(_("Cannot write to data directory '%s'; check permissions."), fs::PathToString(datadir)));
    case util::LockResult::ErrorLock:
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. %s is probably already running."), fs::PathToString(datadir), PACKAGE_NAME));
    case util::LockResult::Success: return true;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

bool AppInitSanityChecks(const kernel::Context& kernel)
{
    // ********************************************************* Step 4: sanity checks
    auto result{kernel::SanityChecks(kernel)};
    if (!result) {
        InitError(util::ErrorString(result));
        return InitError(strprintf(_("Initialization sanity check failed. %s is shutting down."), PACKAGE_NAME));
    }

    if (!ECC_InitSanityCheck()) {
        return InitError(strprintf(_("Elliptic curve cryptography sanity check failure. %s is shutting down."), PACKAGE_NAME));
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
    node.mining = node.init->makeMining();
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
        LogPrintf("Warning: relative datadir option '%s' specified, which will be interpreted relative to the "
                  "current working directory '%s'. This is fragile, because if bitcoin is started in the future "
                  "from a different location, it will be unable to locate the current data files. There could "
                  "also be data loss if bitcoin is started while in a temporary directory.\n",
                  args.GetArg("-datadir", ""), fs::PathToString(fs::current_path()));
    }

    assert(!node.scheduler);
    node.scheduler = std::make_unique<CScheduler>();
    auto& scheduler = *node.scheduler;

    // Start the lightweight task scheduler thread
    scheduler.m_service_thread = std::thread(util::TraceThread, "scheduler", [&] { scheduler.serviceQueue(); });

    // Gather some entropy once per minute.
    scheduler.scheduleEvery([]{
        RandAddPeriodic();
    }, std::chrono::minutes{1});

    // Check disk space every 5 minutes to avoid db corruption.
    scheduler.scheduleEvery([&args, &node]{
        constexpr uint64_t min_disk_space = 50 << 20; // 50 MB
        if (!CheckDiskSpace(args.GetBlocksDirPath(), min_disk_space)) {
            LogError("Shutting down due to lack of disk space!\n");
            if (!(*Assert(node.shutdown))()) {
                LogError("Failed to send shutdown signal after disk space check\n");
            }
        }
    }, std::chrono::minutes{5});

    assert(!node.validation_signals);
    node.validation_signals = std::make_unique<ValidationSignals>(std::make_unique<SerialTaskRunner>(scheduler));
    auto& validation_signals = *node.validation_signals;

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
#ifdef ENABLE_ZMQ
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

    PeerManager::Options peerman_opts{};
    ApplyArgsManOptions(args, peerman_opts);

    {

        // Read asmap file if configured
        std::vector<bool> asmap;
        if (args.IsArgSet("-asmap")) {
            fs::path asmap_path = args.GetPathArg("-asmap", DEFAULT_ASMAP_FILENAME);
            if (!asmap_path.is_absolute()) {
                asmap_path = args.GetDataDirNet() / asmap_path;
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

    FastRandomContext rng;
    assert(!node.banman);
    node.banman = std::make_unique<BanMan>(args.GetDataDirNet() / "banlist", &uiInterface, args.GetIntArg("-bantime", DEFAULT_MISBEHAVING_BANTIME));
    assert(!node.connman);
    node.connman = std::make_unique<CConnman>(rng.rand64(),
                                              rng.rand64(),
                                              *node.addrman, *node.netgroupman, chainparams, args.GetBoolArg("-networkactive", true));

    assert(!node.fee_estimator);
    // Don't initialize fee estimation with old data if we don't relay transactions,
    // as they would never get updated.
    if (!peerman_opts.ignore_incoming_txs) {
        bool read_stale_estimates = args.GetBoolArg("-acceptstalefeeestimates", DEFAULT_ACCEPT_STALE_FEE_ESTIMATES);
        if (read_stale_estimates && (chainparams.GetChainType() != ChainType::REGTEST)) {
            return InitError(strprintf(_("acceptstalefeeestimates is not supported on %s chain."), chainparams.GetChainTypeString()));
        }
        node.fee_estimator = std::make_unique<CBlockPolicyEstimator>(FeeestPath(args), read_stale_estimates);

        // Flush estimates to disk periodically
        CBlockPolicyEstimator* fee_estimator = node.fee_estimator.get();
        scheduler.scheduleEvery([fee_estimator] { fee_estimator->FlushFeeEstimates(); }, FEE_FLUSH_INTERVAL);
        validation_signals.RegisterValidationInterface(fee_estimator);
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
        // arg name            UNIX socket support
        {"-i2psam",                 false},
        {"-onion",                  true},
        {"-proxy",                  true},
        {"-rpcbind",                false},
        {"-torcontrol",             false},
        {"-whitebind",              false},
        {"-zmqpubhashblock",        true},
        {"-zmqpubhashtx",           true},
        {"-zmqpubrawblock",         true},
        {"-zmqpubrawtx",            true},
        {"-zmqpubsequence",         true},
    }) {
        for (const std::string& socket_addr : args.GetArgs(arg)) {
            std::string host_out;
            uint16_t port_out{0};
            if (!SplitHostPort(socket_addr, port_out, host_out)) {
#ifdef HAVE_SOCKADDR_UN
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
    if (args.GetBoolArg("-dnsseed") == true && !g_reachable_nets.Contains(NET_IPV4) && !g_reachable_nets.Contains(NET_IPV6)) {
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

#ifdef ENABLE_ZMQ
    g_zmq_notification_interface = CZMQNotificationInterface::Create(
        [&chainman = node.chainman](std::vector<uint8_t>& block, const CBlockIndex& index) {
            assert(chainman);
            return chainman->m_blockman.ReadRawBlockFromDisk(block, WITH_LOCK(cs_main, return index.GetBlockPos()));
        });

    if (g_zmq_notification_interface) {
        validation_signals.RegisterValidationInterface(g_zmq_notification_interface.get());
    }
#endif

    // ********************************************************* Step 7: load block chain

    node.notifications = std::make_unique<KernelNotifications>(*Assert(node.shutdown), node.exit_status, *Assert(node.warnings));
    ReadNotificationArgs(args, *node.notifications);
    ChainstateManager::Options chainman_opts{
        .chainparams = chainparams,
        .datadir = args.GetDataDirNet(),
        .notifications = *node.notifications,
        .signals = &validation_signals,
    };
    Assert(ApplyArgsManOptions(args, chainman_opts)); // no error can happen, already checked in AppInitParameterInteraction

    BlockManager::Options blockman_opts{
        .chainparams = chainman_opts.chainparams,
        .blocks_dir = args.GetBlocksDirPath(),
        .notifications = chainman_opts.notifications,
    };
    Assert(ApplyArgsManOptions(args, blockman_opts)); // no error can happen, already checked in AppInitParameterInteraction

    // cache size calculations
    CacheSizes cache_sizes = CalculateCacheSizes(args, g_enabled_filter_types.size());

    LogPrintf("Cache configuration:\n");
    LogPrintf("* Using %.1f MiB for block index database\n", cache_sizes.block_tree_db * (1.0 / 1024 / 1024));
    if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        LogPrintf("* Using %.1f MiB for transaction index database\n", cache_sizes.tx_index * (1.0 / 1024 / 1024));
    }
    for (BlockFilterType filter_type : g_enabled_filter_types) {
        LogPrintf("* Using %.1f MiB for %s block filter index database\n",
                  cache_sizes.filter_index * (1.0 / 1024 / 1024), BlockFilterTypeName(filter_type));
    }
    LogPrintf("* Using %.1f MiB for chain state database\n", cache_sizes.coins_db * (1.0 / 1024 / 1024));

    assert(!node.mempool);
    assert(!node.chainman);

    CTxMemPool::Options mempool_opts{
        .check_ratio = chainparams.DefaultConsistencyChecks() ? 1 : 0,
        .signals = &validation_signals,
    };
    auto result{ApplyArgsManOptions(args, chainparams, mempool_opts)};
    if (!result) {
        return InitError(util::ErrorString(result));
    }

    bool do_reindex{args.GetBoolArg("-reindex", false)};
    const bool do_reindex_chainstate{args.GetBoolArg("-reindex-chainstate", false)};

    for (bool fLoaded = false; !fLoaded && !ShutdownRequested(node);) {
        bilingual_str mempool_error;
        node.mempool = std::make_unique<CTxMemPool>(mempool_opts, mempool_error);
        if (!mempool_error.empty()) {
            return InitError(mempool_error);
        }
        LogPrintf("* Using %.1f MiB for in-memory UTXO set (plus up to %.1f MiB of unused mempool space)\n", cache_sizes.coins * (1.0 / 1024 / 1024), mempool_opts.max_size_bytes * (1.0 / 1024 / 1024));

        try {
            node.chainman = std::make_unique<ChainstateManager>(*Assert(node.shutdown), chainman_opts, blockman_opts);
        } catch (std::exception& e) {
            return InitError(strprintf(Untranslated("Failed to initialize ChainstateManager: %s"), e.what()));
        }
        ChainstateManager& chainman = *node.chainman;

        // This is defined and set here instead of inline in validation.h to avoid a hard
        // dependency between validation and index/base, since the latter is not in
        // libbitcoinkernel.
        chainman.restart_indexes = [&node]() {
            LogPrintf("[snapshot] restarting indexes\n");

            // Drain the validation interface queue to ensure that the old indexes
            // don't have any pending work.
            Assert(node.validation_signals)->SyncWithValidationInterfaceQueue();

            for (auto* index : node.indexes) {
                index->Interrupt();
                index->Stop();
                if (!(index->Init() && index->StartBackgroundSync())) {
                    LogPrintf("[snapshot] WARNING failed to restart index %s on snapshot chain\n", index->GetName());
                }
            }
        };

        node::ChainstateLoadOptions options;
        options.mempool = Assert(node.mempool.get());
        options.wipe_block_tree_db = do_reindex;
        options.wipe_chainstate_db = do_reindex || do_reindex_chainstate;
        options.prune = chainman.m_blockman.IsPruneMode();
        options.check_blocks = args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS);
        options.check_level = args.GetIntArg("-checklevel", DEFAULT_CHECKLEVEL);
        options.require_full_verification = args.IsArgSet("-checkblocks") || args.IsArgSet("-checklevel");
        options.coins_error_cb = [] {
            uiInterface.ThreadSafeMessageBox(
                _("Error reading from database, shutting down."),
                "", CClientUIInterface::MSG_ERROR);
        };

        uiInterface.InitMessage(_("Loading block index…").translated);
        const auto load_block_index_start_time{SteadyClock::now()};
        auto catch_exceptions = [](auto&& f) {
            try {
                return f();
            } catch (const std::exception& e) {
                LogError("%s\n", e.what());
                return std::make_tuple(node::ChainstateLoadStatus::FAILURE, _("Error opening block database"));
            }
        };
        auto [status, error] = catch_exceptions([&]{ return LoadChainstate(chainman, cache_sizes, options); });
        if (status == node::ChainstateLoadStatus::SUCCESS) {
            uiInterface.InitMessage(_("Verifying blocks…").translated);
            if (chainman.m_blockman.m_have_pruned && options.check_blocks > MIN_BLOCKS_TO_KEEP) {
                LogWarning("pruned datadir may not have more than %d blocks; only checking available blocks\n",
                                  MIN_BLOCKS_TO_KEEP);
            }
            std::tie(status, error) = catch_exceptions([&]{ return VerifyLoadedChainstate(chainman, options);});
            if (status == node::ChainstateLoadStatus::SUCCESS) {
                fLoaded = true;
                LogPrintf(" block index %15dms\n", Ticks<std::chrono::milliseconds>(SteadyClock::now() - load_block_index_start_time));
            }
        }

        if (status == node::ChainstateLoadStatus::FAILURE_FATAL || status == node::ChainstateLoadStatus::FAILURE_INCOMPATIBLE_DB || status == node::ChainstateLoadStatus::FAILURE_INSUFFICIENT_DBCACHE) {
            return InitError(error);
        }

        if (!fLoaded && !ShutdownRequested(node)) {
            // first suggest a reindex
            if (!do_reindex) {
                bool fRet = uiInterface.ThreadSafeQuestion(
                    error + Untranslated(".\n\n") + _("Do you want to rebuild the block database now?"),
                    error.original + ".\nPlease restart with -reindex or -reindex-chainstate to recover.",
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    do_reindex = true;
                    if (!Assert(node.shutdown)->reset()) {
                        LogError("Internal error: failed to reset shutdown signal.\n");
                    }
                } else {
                    LogError("Aborted block database rebuild. Exiting.\n");
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
    if (ShutdownRequested(node)) {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    ChainstateManager& chainman = *Assert(node.chainman);

    assert(!node.peerman);
    node.peerman = PeerManager::make(*node.connman, *node.addrman,
                                     node.banman.get(), chainman,
                                     *node.mempool, *node.warnings,
                                     peerman_opts);
    validation_signals.RegisterValidationInterface(node.peerman.get());

    // ********************************************************* Step 8: start indexers

    if (args.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        g_txindex = std::make_unique<TxIndex>(interfaces::MakeChain(node), cache_sizes.tx_index, false, do_reindex);
        node.indexes.emplace_back(g_txindex.get());
    }

    for (const auto& filter_type : g_enabled_filter_types) {
        InitBlockFilterIndex([&]{ return interfaces::MakeChain(node); }, filter_type, cache_sizes.filter_index, false, do_reindex);
        node.indexes.emplace_back(GetBlockFilterIndex(filter_type));
    }

    if (args.GetBoolArg("-coinstatsindex", DEFAULT_COINSTATSINDEX)) {
        g_coin_stats_index = std::make_unique<CoinStatsIndex>(interfaces::MakeChain(node), /*cache_size=*/0, false, do_reindex);
        node.indexes.emplace_back(g_coin_stats_index.get());
    }

    // Init indexes
    for (auto index : node.indexes) if (!index->Init()) return false;

    // ********************************************************* Step 9: load wallet
    for (const auto& client : node.chain_clients) {
        if (!client->load()) {
            return false;
        }
    }

    // ********************************************************* Step 10: data directory maintenance

    // if pruning, perform the initial blockstore prune
    // after any wallet rescanning has taken place.
    if (chainman.m_blockman.IsPruneMode()) {
        if (chainman.m_blockman.m_blockfiles_indexed) {
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

    // ********************************************************* Step 11: import blocks

    if (!CheckDiskSpace(args.GetDataDirNet())) {
        InitError(strprintf(_("Error: Disk space is low for %s"), fs::quoted(fs::PathToString(args.GetDataDirNet()))));
        return false;
    }
    if (!CheckDiskSpace(args.GetBlocksDirPath())) {
        InitError(strprintf(_("Error: Disk space is low for %s"), fs::quoted(fs::PathToString(args.GetBlocksDirPath()))));
        return false;
    }

    int chain_active_height = WITH_LOCK(cs_main, return chainman.ActiveChain().Height());

    // On first startup, warn on low block storage space
    if (!do_reindex && !do_reindex_chainstate && chain_active_height <= 1) {
        uint64_t assumed_chain_bytes{chainparams.AssumedBlockchainSize() * 1024 * 1024 * 1024};
        uint64_t additional_bytes_needed{
            chainman.m_blockman.IsPruneMode() ?
                std::min(chainman.m_blockman.GetPruneTarget(), assumed_chain_bytes) :
                assumed_chain_bytes};

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
#endif

    std::vector<fs::path> vImportFiles;
    for (const std::string& strFile : args.GetArgs("-loadblock")) {
        vImportFiles.push_back(fs::PathFromString(strFile));
    }

    chainman.m_thread_load = std::thread(&util::TraceThread, "initload", [=, &chainman, &args, &node] {
        ScheduleBatchPriority();
        // Import blocks
        ImportBlocks(chainman, vImportFiles);
        if (args.GetBoolArg("-stopafterblockimport", DEFAULT_STOPAFTERBLOCKIMPORT)) {
            LogPrintf("Stopping after block import\n");
            if (!(*Assert(node.shutdown))()) {
                LogError("Failed to send shutdown signal after finishing block import\n");
            }
            return;
        }

        // Start indexes initial sync
        if (!StartIndexBackgroundSync(node)) {
            bilingual_str err_str = _("Failed to start indexes, shutting down..");
            chainman.GetNotifications().fatalError(err_str);
            return;
        }
        // Load mempool from disk
        if (auto* pool{chainman.ActiveChainstate().GetMempool()}) {
            LoadMempool(*pool, ShouldPersistMempool(args) ? MempoolPath(args) : fs::path{}, chainman.ActiveChainstate(), {});
            pool->SetLoadTried(!chainman.m_interrupt);
        }
    });

    // Wait for genesis block to be processed
    {
        WAIT_LOCK(g_genesis_wait_mutex, lock);
        // We previously could hang here if shutdown was requested prior to
        // ImportBlocks getting started, so instead we just wait on a timer to
        // check ShutdownRequested() regularly.
        while (!fHaveGenesis && !ShutdownRequested(node)) {
            g_genesis_wait_cv.wait_for(lock, std::chrono::milliseconds(500));
        }
        block_notify_genesis_wait_connection.disconnect();
    }

    if (ShutdownRequested(node)) {
        return false;
    }

    // ********************************************************* Step 12: start node

    //// debug print
    int64_t best_block_time{};
    {
        LOCK(cs_main);
        LogPrintf("block tree size = %u\n", chainman.BlockIndex().size());
        chain_active_height = chainman.ActiveChain().Height();
        best_block_time = chainman.ActiveChain().Tip() ? chainman.ActiveChain().Tip()->GetBlockTime() : chainman.GetParams().GenesisBlock().GetBlockTime();
        if (tip_info) {
            tip_info->block_height = chain_active_height;
            tip_info->block_time = best_block_time;
            tip_info->verification_progress = GuessVerificationProgress(chainman.GetParams().TxData(), chainman.ActiveChain().Tip());
        }
        if (tip_info && chainman.m_best_header) {
            tip_info->header_height = chainman.m_best_header->nHeight;
            tip_info->header_time = chainman.m_best_header->GetBlockTime();
        }
    }
    LogPrintf("nBestHeight = %d\n", chain_active_height);
    if (node.peerman) node.peerman->SetBestBlock(chain_active_height, std::chrono::seconds{best_block_time});

    // Map ports with UPnP or NAT-PMP.
    StartMapPort(args.GetBoolArg("-upnp", DEFAULT_UPNP), args.GetBoolArg("-natpmp", DEFAULT_NATPMP));

    CConnman::Options connOptions;
    connOptions.nLocalServices = nLocalServices;
    connOptions.m_max_automatic_connections = nMaxConnections;
    connOptions.uiInterface = &uiInterface;
    connOptions.m_banman = node.banman.get();
    connOptions.m_msgproc = node.peerman.get();
    connOptions.nSendBufferMaxSize = 1000 * args.GetIntArg("-maxsendbuffer", DEFAULT_MAXSENDBUFFER);
    connOptions.nReceiveFloodSize = 1000 * args.GetIntArg("-maxreceivebuffer", DEFAULT_MAXRECEIVEBUFFER);
    connOptions.m_added_nodes = args.GetArgs("-addnode");
    connOptions.nMaxOutboundLimit = *opt_max_upload;
    connOptions.m_peer_connect_timeout = peer_connect_timeout;
    connOptions.whitelist_forcerelay = args.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY);
    connOptions.whitelist_relay = args.GetBoolArg("-whitelistrelay", DEFAULT_WHITELISTRELAY);

    // Port to bind to if `-bind=addr` is provided without a `:port` suffix.
    const uint16_t default_bind_port =
        static_cast<uint16_t>(args.GetIntArg("-port", Params().GetDefaultPort()));

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

    CService onion_service_target;
    if (!connOptions.onion_binds.empty()) {
        onion_service_target = connOptions.onion_binds.front();
    } else if (!connOptions.vBinds.empty()) {
        onion_service_target = connOptions.vBinds.front();
    } else {
        onion_service_target = DefaultOnionServiceTarget();
        connOptions.onion_binds.push_back(onion_service_target);
    }

    if (args.GetBoolArg("-listenonion", DEFAULT_LISTEN_ONION)) {
        if (connOptions.onion_binds.size() > 1) {
            InitWarning(strprintf(_("More than one onion bind address is provided. Using %s "
                                    "for the automatically created Tor onion service."),
                                  onion_service_target.ToStringAddrPort()));
        }
        StartTorControl(onion_service_target);
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

    if (!node.connman->Start(scheduler, connOptions)) {
        return false;
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
        client->start(scheduler);
    }

    BanMan* banman = node.banman.get();
    scheduler.scheduleEvery([banman]{
        banman->DumpBanlist();
    }, DUMP_BANS_INTERVAL);

    if (node.peerman) node.peerman->StartScheduledTasks(scheduler);

#if HAVE_SYSTEM
    StartupNotify(args);
#endif

    return true;
}

bool StartIndexBackgroundSync(NodeContext& node)
{
    // Find the oldest block among all indexes.
    // This block is used to verify that we have the required blocks' data stored on disk,
    // starting from that point up to the current tip.
    // indexes_start_block='nullptr' means "start from height 0".
    std::optional<const CBlockIndex*> indexes_start_block;
    std::string older_index_name;
    ChainstateManager& chainman = *Assert(node.chainman);
    const Chainstate& chainstate = WITH_LOCK(::cs_main, return chainman.GetChainstateForIndexing());
    const CChain& index_chain = chainstate.m_chain;

    for (auto index : node.indexes) {
        const IndexSummary& summary = index->GetSummary();
        if (summary.synced) continue;

        // Get the last common block between the index best block and the active chain
        LOCK(::cs_main);
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(summary.best_block_hash);
        if (!index_chain.Contains(pindex)) {
            pindex = index_chain.FindFork(pindex);
        }

        if (!indexes_start_block || !pindex || pindex->nHeight < indexes_start_block.value()->nHeight) {
            indexes_start_block = pindex;
            older_index_name = summary.name;
            if (!pindex) break; // Starting from genesis so no need to look for earlier block.
        }
    };

    // Verify all blocks needed to sync to current tip are present.
    if (indexes_start_block) {
        LOCK(::cs_main);
        const CBlockIndex* start_block = *indexes_start_block;
        if (!start_block) start_block = chainman.ActiveChain().Genesis();
        if (!chainman.m_blockman.CheckBlockDataAvailability(*index_chain.Tip(), *Assert(start_block))) {
            return InitError(strprintf(Untranslated("%s best block of the index goes beyond pruned data. Please disable the index or reindex (which will download the whole blockchain again)"), older_index_name));
        }
    }

    // Start threads
    for (auto index : node.indexes) if (!index->StartBackgroundSync()) return false;
    return true;
}
