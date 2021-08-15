// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <init.h>

#include <addrman.h>
#include <amount.h>
#include <banman.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <compat/sanity.h>
#include <consensus/validation.h>
#include <fs.h>
#include <httprpc.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <key.h>
#include <miner.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <netbase.h>
#include <poc/poc.h>
#include <policy/feerate.h>
#include <policy/fees.h>
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
#include <timedata.h>
#include <torcontrol.h>
#include <txdb.h>
#include <txmempool.h>
#include <ui_interface.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/time.h>
#include <util/translation.h>
#include <util/validation.h>
#include <validation.h>
#include <validationinterface.h>
#include <walletinitinterface.h>

#ifdef ENABLE_OMNICORE
#include <omnicore_api.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <set>

#ifndef WIN32
#include <attributes.h>
#include <cerrno>
#include <signal.h>
#include <sys/stat.h>
#endif

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread.hpp>

#if ENABLE_ZMQ
#include <zmq/zmqabstractnotifier.h>
#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqrpc.h>
#endif

static bool fFeeEstimatesInitialized = false;
static const bool DEFAULT_PROXYRANDOMIZE = true;
static const bool DEFAULT_REST_ENABLE = false;
static const bool DEFAULT_STOPAFTERBLOCKIMPORT = false;

// Dump addresses to banlist.dat every 15 minutes (900s)
static constexpr int DUMP_BANS_INTERVAL = 60 * 15;

std::unique_ptr<CConnman> g_connman;
std::unique_ptr<PeerLogicValidation> peerLogic;
std::unique_ptr<BanMan> g_banman;

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

static const char* FEE_ESTIMATES_FILENAME="fee_estimates.dat";

/**
 * The PID file facilities.
 */
static const char* BITCOIN_PID_FILENAME = "qitcoind.pid";

static fs::path GetPidFile()
{
    return AbsPathForConfigVal(fs::path(gArgs.GetArg("-pid", BITCOIN_PID_FILENAME)));
}

NODISCARD static bool CreatePidFile()
{
    fsbridge::ofstream file{GetPidFile()};
    if (file) {
#ifdef WIN32
        tfm::format(file, "%d\n", GetCurrentProcessId());
#else
        tfm::format(file, "%d\n", getpid());
#endif
        return true;
    } else {
        return InitError(strprintf(_("Unable to create the PID file '%s': %s").translated, GetPidFile().string(), std::strerror(errno)));
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

static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

static boost::thread_group threadGroup;
static CScheduler scheduler;

void Interrupt()
{
    InterruptPOC();
    InterruptHTTPServer();
    InterruptHTTPRPC();
    InterruptRPC();
    InterruptREST();
    InterruptTorControl();
    InterruptMapPort();
    if (g_connman)
        g_connman->Interrupt();
    if (g_txindex) {
        g_txindex->Interrupt();
    }
    ForEachBlockFilterIndex([](BlockFilterIndex& index) { index.Interrupt(); });
}

void Shutdown(InitInterfaces& interfaces)
{
    LogPrintf("%s: In progress...\n", __func__);
    static CCriticalSection cs_Shutdown;
    TRY_LOCK(cs_Shutdown, lockShutdown);
    if (!lockShutdown)
        return;

    /// Note: Shutdown() must be able to handle cases in which initialization failed part of the way,
    /// for example if the data directory was found to be locked.
    /// Be sure that anything that writes files or flushes caches only does this if the respective
    /// module was initialized.
    util::ThreadRename("shutoff");
    mempool.AddTransactionsUpdated(1);

    StopPOC();
    StopHTTPRPC();
    StopREST();
    StopRPC();
    StopHTTPServer();
    for (const auto& client : interfaces.chain_clients) {
        client->flush();
    }
    StopMapPort();

    // Because these depend on each-other, we make sure that neither can be
    // using the other before destroying them.
    if (peerLogic) UnregisterValidationInterface(peerLogic.get());
    if (g_connman) g_connman->Stop();

    StopTorControl();

    // After everything has been shut down, but before things get flushed, stop the
    // CScheduler/checkqueue threadGroup
    threadGroup.interrupt_all();
    threadGroup.join_all();

    // After the threads that potentially access these pointers have been stopped,
    // destruct and reset all to nullptr.
    peerLogic.reset();
    g_connman.reset();
    g_banman.reset();

    if (::mempool.IsLoaded() && gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        DumpMempool(::mempool);
    }

    if (fFeeEstimatesInitialized)
    {
        ::feeEstimator.FlushUnconfirmed();
        fs::path est_path = GetDataDir() / FEE_ESTIMATES_FILENAME;
        CAutoFile est_fileout(fsbridge::fopen(est_path, "wb"), SER_DISK, CLIENT_VERSION);
        if (!est_fileout.IsNull())
            ::feeEstimator.Write(est_fileout);
        else
            LogPrintf("%s: Failed to write fee estimates to %s\n", __func__, est_path.string());
        fFeeEstimatesInitialized = false;
    }

    // FlushStateToDisk generates a ChainStateFlushed callback, which we should avoid missing
    //
    // g_chainstate is referenced here directly (instead of ::ChainstateActive()) because it
    // may not have been initialized yet.
    {
        LOCK(cs_main);
        if (g_chainstate && g_chainstate->CanFlushToDisk()) {
            g_chainstate->ForceFlushStateToDisk();
        }
    }

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    // Stop and delete all indexes only after flushing background callbacks.
    if (g_txindex) {
        g_txindex->Stop();
        g_txindex.reset();
    }
    ForEachBlockFilterIndex([](BlockFilterIndex& index) { index.Stop(); });
    DestroyAllBlockFilterIndexes();

    // Any future callbacks will be dropped. This should absolutely be safe - if
    // missing a callback results in an unrecoverable situation, unclean shutdown
    // would too. The only reason to do the above flushes is to let the wallet catch
    // up with our current chain to avoid any strange pruning edge cases and make
    // next startup faster by avoiding rescan.

    {
        LOCK(cs_main);
        if (g_chainstate && g_chainstate->CanFlushToDisk()) {
            g_chainstate->ForceFlushStateToDisk();
            g_chainstate->ResetCoinsViews();
        }
        pblocktree.reset();
    }
    for (const auto& client : interfaces.chain_clients) {
        client->stop();
    }

#ifdef ENABLE_OMNICORE
    omnicore_api::Shutdown();
#endif

#if ENABLE_ZMQ
    if (g_zmq_notification_interface) {
        UnregisterValidationInterface(g_zmq_notification_interface);
        delete g_zmq_notification_interface;
        g_zmq_notification_interface = nullptr;
    }
#endif

    try {
        if (!fs::remove(GetPidFile())) {
            LogPrintf("%s: Unable to remove PID file: File does not exist\n", __func__);
        }
    } catch (const fs::filesystem_error& e) {
        LogPrintf("%s: Unable to remove PID file: %s\n", __func__, fsbridge::get_filesystem_error_message(e));
    }
    interfaces.chain_clients.clear();
    UnregisterAllValidationInterfaces();
    GetMainSignals().UnregisterBackgroundSignalScheduler();
    GetMainSignals().UnregisterWithMempoolSignals(mempool);
    globalVerifyHandle.reset();
    ECC_Stop();
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
#ifdef ENABLE_OMNICORE
    omnicore_api::ReopenDebugLog();
#endif
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
    rpc_notify_block_change_connection = uiInterface.NotifyBlockTip_connect(&RPCNotifyBlockChange);
}

static void OnRPCStopped()
{
    rpc_notify_block_change_connection.disconnect();
    RPCNotifyBlockChange(false, nullptr);
    g_best_block_cv.notify_all();
    LogPrint(BCLog::RPC, "RPC stopped.\n");
}

void SetupServerArgs()
{
    SetupHelpOptions(gArgs);
    gArgs.AddArg("-help-debug", "Print help message with debugging options and exit", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST); // server-only for now

    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    const auto regtestBaseParams = CreateBaseChainParams(CBaseChainParams::REGTEST);
    const auto defaultChainParams = CreateChainParams(CBaseChainParams::MAIN);
    const auto testnetChainParams = CreateChainParams(CBaseChainParams::TESTNET);
    const auto regtestChainParams = CreateChainParams(CBaseChainParams::REGTEST);

    // Hidden Options
    std::vector<std::string> hidden_args = {
        "-dbcrashratio", "-forcecompactdb",
        // GUI args. These will be overwritten by SetupUIArgs for the GUI
        "-allowselfsignedrootcertificates", "-choosedatadir", "-lang=<lang>", "-min", "-resetguisettings", "-rootcertificates=<file>", "-splash", "-uiplatform"};

    gArgs.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    gArgs.AddArg("-alertnotify=<cmd>", "Execute command when a relevant alert is received or we see a really long fork (%s in cmd is replaced by message)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    gArgs.AddArg("-assumevalid=<hex>", strprintf("If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet: %s)", defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex()), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blocksdir=<dir>", "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#if HAVE_SYSTEM
    gArgs.AddArg("-blocknotify=<cmd>", "Execute command when the best block changes (%s in cmd is replaced by block hash)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#endif
    gArgs.AddArg("-blockreconstructionextratxn=<n>", strprintf("Extra transactions to keep in memory for compact block reconstructions (default: %u)", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blocksonly", strprintf("Whether to reject transactions from network peers. Transactions from the wallet, RPC and relay whitelisted inbound peers are not affected. (default: %u)", DEFAULT_BLOCKSONLY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-conf=<file>", strprintf("Specify configuration file. Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-dbbatchsize", strprintf("Maximum database write batch size in bytes (default: %u)", nDefaultDbBatchSize), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-dbcache=<n>", strprintf("Maximum database cache size <n> MiB (%d to %d, default: %d). In addition, unused mempool memory is shared for this cache (see -maxmempool).", nMinDbCache, nMaxDbCache, nDefaultDbCache), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-debuglogfile=<file>", strprintf("Specify location of debug log file. Relative paths will be prefixed by a net-specific datadir location. (-nodebuglogfile to disable; default: %s)", DEFAULT_DEBUGLOGFILE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-feefilter", strprintf("Tell other nodes to filter invs to us by our mempool min fee (default: %u)", DEFAULT_FEEFILTER), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-includeconf=<file>", "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-loadblock=<file>", "Imports blocks from external blk000??.dat file on startup", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-maxmempool=<n>", strprintf("Keep the transaction memory pool below <n> megabytes (default: %u)", DEFAULT_MAX_MEMPOOL_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-maxorphantx=<n>", strprintf("Keep at most <n> unconnectable transactions in memory (default: %u)", DEFAULT_MAX_ORPHAN_TRANSACTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-mempoolexpiry=<n>", strprintf("Do not keep transactions in the mempool longer than <n> hours (default: %u)", DEFAULT_MEMPOOL_EXPIRY), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-minimumchainwork=<hex>", strprintf("Minimum work assumed to exist on a valid chain in hex (default: %s, testnet: %s)", defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-par=<n>", strprintf("Set the number of script verification threads (%u to %d, 0 = auto, <0 = leave that many cores free, default: %d)",
        -GetNumCores(), MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-persistmempool", strprintf("Whether to save the mempool on shutdown and load on restart (default: %u)", DEFAULT_PERSIST_MEMPOOL), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-pid=<file>", strprintf("Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)", BITCOIN_PID_FILENAME), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-prune=<n>", strprintf("Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks, and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex and -rescan. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >=%u = automatically prune block files to stay under the specified target size in MiB)", MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-reindex", "Rebuild chain state and block index from the blk*.dat files on disk", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-reindex-chainstate", "Rebuild chain state from the currently indexed blocks. When in pruning mode or if blocks on disk might be corrupted, use full -reindex instead.", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#ifndef WIN32
    gArgs.AddArg("-sysperms", "Create new files with system default permissions, instead of umask 077 (only effective with disabled wallet functionality)", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-sysperms");
#endif
    gArgs.AddArg("-txindex", strprintf("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)", DEFAULT_TXINDEX), ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blockfilterindex=<type>",
                 strprintf("Maintain an index of compact filters by block (default: %s, values: %s).", DEFAULT_BLOCKFILTERINDEX, ListBlockFilterTypes()) +
                 " If <type> is not supplied or if <type> = 1, indexes for all known types are enabled.",
                 ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);

    gArgs.AddArg("-addnode=<ip>", "Add a node to connect to and attempt to keep the connection open (see the `addnode` RPC command help for more info). This option can be specified multiple times to add multiple nodes.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-banscore=<n>", strprintf("Threshold for disconnecting misbehaving peers (default: %u)", DEFAULT_BANSCORE_THRESHOLD), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-bantime=<n>", strprintf("Number of seconds to keep misbehaving peers from reconnecting (default: %u)", DEFAULT_MISBEHAVING_BANTIME), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-bind=<addr>", "Bind to given address and always listen on it. Use [host]:port notation for IPv6", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-connect=<ip>", "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-discover", "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-dns", strprintf("Allow DNS lookups for -addnode, -seednode and -connect (default: %u)", DEFAULT_NAME_LOOKUP), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-dnsseed", "Query for peer addresses via DNS lookup, if low on addresses (default: 1 unless -connect used)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-enablebip61", strprintf("Send reject messages per BIP61 (default: %u)", DEFAULT_ENABLE_BIP61), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-externalip=<ip>", "Specify your own public address", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-forcednsseed", strprintf("Always query for peer addresses via DNS lookup (default: %u)", DEFAULT_FORCEDNSSEED), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-listen", "Accept connections from outside (default: 1 if no -proxy or -connect)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-listenonion", strprintf("Automatically create Tor hidden service (default: %d)", DEFAULT_LISTEN_ONION), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxconnections=<n>", strprintf("Maintain at most <n> connections to peers (default: %u)", DEFAULT_MAX_PEER_CONNECTIONS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxreceivebuffer=<n>", strprintf("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXRECEIVEBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxsendbuffer=<n>", strprintf("Maximum per-connection send buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXSENDBUFFER), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxtimeadjustment", strprintf("Maximum allowed median peer time offset adjustment. Local perspective of time may be influenced by peers forward or backward by this amount. (default: %u seconds)", DEFAULT_MAX_TIME_ADJUSTMENT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxuploadtarget=<n>", strprintf("Tries to keep outbound traffic under the given target (in MiB per 24h), 0 = no limit (default: %d)", DEFAULT_MAX_UPLOAD_TARGET), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-onion=<ip:port>", "Use separate SOCKS5 proxy to reach peers via Tor hidden services, set -noonion to disable (default: -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-onlynet=<net>", "Make outgoing connections only through network <net> (ipv4, ipv6 or onion). Incoming connections are not affected by this option. This option can be specified multiple times to allow multiple networks.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-peerbloomfilters", strprintf("Support filtering of blocks and transaction with bloom filters (default: %u)", DEFAULT_PEERBLOOMFILTERS), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-permitbaremultisig", strprintf("Relay non-P2SH multisig (default: %u)", DEFAULT_PERMIT_BAREMULTISIG), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-port=<port>", strprintf("Listen for connections on <port> (default: %u, testnet: %u, regtest: %u)", defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort(), regtestChainParams->GetDefaultPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-proxy=<ip:port>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-proxyrandomize", strprintf("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)", DEFAULT_PROXYRANDOMIZE), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-seednode=<ip>", "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-timeout=<n>", strprintf("Specify connection timeout in milliseconds (minimum: 1, default: %d)", DEFAULT_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-peertimeout=<n>", strprintf("Specify p2p connection timeout in seconds. This option determines the amount of time a peer may be inactive before the connection to it is dropped. (minimum: 1, default: %d)", DEFAULT_PEER_CONNECT_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-torcontrol=<ip>:<port>", strprintf("Tor control port to use if onion listening enabled (default: %s)", DEFAULT_TOR_CONTROL), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
    gArgs.AddArg("-torpassword=<pass>", "Tor control port password (default: empty)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#ifdef USE_UPNP
#if USE_UPNP
    gArgs.AddArg("-upnp", "Use UPnP to map the listening port (default: 1 when listening and no -proxy)", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#else
    gArgs.AddArg("-upnp", strprintf("Use UPnP to map the listening port (default: %u)", 0), ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);
#endif
#else
    hidden_args.emplace_back("-upnp");
#endif
    gArgs.AddArg("-whitebind=<[permissions@]addr>", "Bind to given address and whitelist peers connecting to it. "
        "Use [host]:port notation for IPv6. Allowed permissions are bloomfilter (allow requesting BIP37 filtered blocks and transactions), "
        "noban (do not ban for misbehavior), "
        "forcerelay (relay even non-standard transactions), "
        "relay (relay even in -blocksonly mode), "
        "and mempool (allow requesting BIP35 mempool contents). "
        "Specify multiple permissions separated by commas (default: noban,mempool,relay). Can be specified multiple times.", ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    gArgs.AddArg("-whitelist=<[permissions@]IP address or network>", "Whitelist peers connecting from the given IP address (e.g. 1.2.3.4) or "
        "CIDR notated network(e.g. 1.2.3.0/24). Uses same permissions as "
        "-whitebind. Can be specified multiple times." , ArgsManager::ALLOW_ANY, OptionsCategory::CONNECTION);

    g_wallet_init_interface.AddWalletOptions();

#if ENABLE_ZMQ
    gArgs.AddArg("-zmqpubhashblock=<address>", "Enable publish hash block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashtx=<address>", "Enable publish hash transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawblock=<address>", "Enable publish raw block in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawtx=<address>", "Enable publish raw transaction in <address>", ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashblockhwm=<n>", strprintf("Set publish hash block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashtxhwm=<n>", strprintf("Set publish hash transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawblockhwm=<n>", strprintf("Set publish raw block outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawtxhwm=<n>", strprintf("Set publish raw transaction outbound message high water mark (default: %d)", CZMQAbstractNotifier::DEFAULT_ZMQ_SNDHWM), ArgsManager::ALLOW_ANY, OptionsCategory::ZMQ);
#else
    hidden_args.emplace_back("-zmqpubhashblock=<address>");
    hidden_args.emplace_back("-zmqpubhashtx=<address>");
    hidden_args.emplace_back("-zmqpubrawblock=<address>");
    hidden_args.emplace_back("-zmqpubrawtx=<address>");
    hidden_args.emplace_back("-zmqpubhashblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubhashtxhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawblockhwm=<n>");
    hidden_args.emplace_back("-zmqpubrawtxhwm=<n>");
#endif

    gArgs.AddArg("-checkblocks=<n>", strprintf("How many blocks to check at startup (default: %u, 0 = all)", DEFAULT_CHECKBLOCKS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checklevel=<n>", strprintf("How thorough the block verification of -checkblocks is: "
        "level 0 reads the blocks from disk, "
        "level 1 verifies block validity, "
        "level 2 verifies undo data, "
        "level 3 checks disconnection of tip blocks, "
        "and level 4 tries to reconnect the blocks, "
        "each level includes the checks of the previous levels "
        "(0-4, default: %u)", DEFAULT_CHECKLEVEL), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkblockindex", strprintf("Do a consistency check for the block tree, chainstate, and other validation data structures occasionally. (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkmempool=<n>", strprintf("Run checks every <n> transactions (default: %u, regtest: %u)", defaultChainParams->DefaultConsistencyChecks(), regtestChainParams->DefaultConsistencyChecks()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkpoints", strprintf("Disable expensive verification for known chain history (default: %u)", DEFAULT_CHECKPOINTS_ENABLED), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-deprecatedrpc=<method>", "Allows deprecated RPC method(s) to be used", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-dropmessagestest=<n>", "Randomly drop 1 of every <n> network messages", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-stopafterblockimport", strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-stopatheight", strprintf("Stop running after reaching the given height in the main chain (default: %u)", DEFAULT_STOPATHEIGHT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitancestorcount=<n>", strprintf("Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)", DEFAULT_ANCESTOR_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitancestorsize=<n>", strprintf("Do not accept transactions whose size with all in-mempool ancestors exceeds <n> kilobytes (default: %u)", DEFAULT_ANCESTOR_SIZE_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitdescendantcount=<n>", strprintf("Do not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u)", DEFAULT_DESCENDANT_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitdescendantsize=<n>", strprintf("Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool descendants (default: %u).", DEFAULT_DESCENDANT_SIZE_LIMIT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-addrmantest", "Allows to test address relay on localhost", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-debug=<category>", "Output debugging information (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> = 1, output all debugging information. <category> can be: " + ListLogCategories() + ".", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-debugexclude=<category>", strprintf("Exclude debugging information for a category. Can be used in conjunction with -debug=1 to output debug logs for all categories except one or more specified categories."), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logips", strprintf("Include IP addresses in debug output (default: %u)", DEFAULT_LOGIPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logtimestamps", strprintf("Prepend debug output with timestamp (default: %u)", DEFAULT_LOGTIMESTAMPS), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logthreadnames", strprintf("Prepend debug output with name of the originating thread (only available on platforms supporting thread_local) (default: %u)", DEFAULT_LOGTHREADNAMES), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logtimemicros", strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-mocktime=<n>", "Replace actual time with <n> seconds since epoch (default: 0)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-maxsigcachesize=<n>", strprintf("Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)", DEFAULT_MAX_SIG_CACHE_SIZE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-maxtipage=<n>", strprintf("Maximum tip age in seconds to consider node in initial block download (default: %u)", DEFAULT_MAX_TIP_AGE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printpriority", strprintf("Log transaction fee per kB when mining blocks (default: %u)", DEFAULT_PRINTPRIORITY), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-shrinkdebugfile", "Shrink debug.log file on client startup (default: 1 when no -debug)", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-uacomment=<cmt>", "Append comment to the user agent string", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);

    SetupChainParamsBaseOptions();

    gArgs.AddArg("-acceptnonstdtxn", strprintf("Relay and mine \"non-standard\" transactions (%sdefault: %u)", "testnet/regtest only; ", !testnetChainParams->RequireStandard()), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-incrementalrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to define cost of relay, used for mempool limiting and BIP 125 replacement. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-dustrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to define dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)", CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-bytespersigop", strprintf("Equivalent bytes per sigop in transactions for relay and mining (default: %u)", DEFAULT_BYTES_PER_SIGOP), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-datacarrier", strprintf("Relay and mine data carrier transactions (default: %u)", DEFAULT_ACCEPT_DATACARRIER), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-datacarriersize", strprintf("Maximum size of data in data carrier transactions we relay and mine (default: %u)", MAX_OP_RETURN_RELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-minrelaytxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-whitelistforcerelay", strprintf("Add 'forcerelay' permission to whitelisted inbound peers with default permissions. This will relay transactions even if the transactions were already in the mempool or violate local relay policy. (default: %d)", DEFAULT_WHITELISTFORCERELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-whitelistrelay", strprintf("Add 'relay' permission to whitelisted inbound peers with default permissions. The will accept relayed transactions even when not relaying transactions (default: %d)", DEFAULT_WHITELISTRELAY), ArgsManager::ALLOW_ANY, OptionsCategory::NODE_RELAY);


    gArgs.AddArg("-blockmaxweight=<n>", strprintf("Set maximum BIP141 block weight (default: %d)", DEFAULT_BLOCK_MAX_WEIGHT), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    gArgs.AddArg("-blockmintxfee=<amt>", strprintf("Set lowest fee rate (in %s/kB) for transactions to be included in block creation. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::BLOCK_CREATION);
    gArgs.AddArg("-blockversion=<n>", "Override block version to test forking scenarios", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::BLOCK_CREATION);

    gArgs.AddArg("-rest", strprintf("Accept public REST requests (default: %u)", DEFAULT_REST_ENABLE), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcallowip=<ip>", "Allow JSON-RPC connections from specified source. Valid for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24). This option can be specified multiple times", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcauth=<userpw>", "Username and HMAC-SHA-256 hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcauth. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcbind=<addr>[:port]", "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost)", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    gArgs.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcport=<port>", strprintf("Listen for JSON-RPC connections on <port> (default: %u, testnet: %u, regtest: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort(), regtestBaseParams->RPCPort()), ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcserialversion", strprintf("Sets the serialization of raw transaction or block hex returned in non-verbose mode, non-segwit(0) or segwit(1) (default: %d)", DEFAULT_RPC_SERIALIZE_VERSION), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcservertimeout=<n>", strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcthreads=<n>", strprintf("Set the number of threads to service RPC calls (default: %d)", DEFAULT_HTTP_THREADS), ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);
    gArgs.AddArg("-rpcworkqueue=<n>", strprintf("Set the depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::RPC);
    gArgs.AddArg("-server", "Accept command line and JSON-RPC commands", ArgsManager::ALLOW_ANY, OptionsCategory::RPC);

    // Qitcoin
    gArgs.AddArg("-forcecheckdeadline", strprintf("Force check every block work (default: %u)", DEFAULT_CHECKWORK_ENABLED), ArgsManager::ALLOW_ANY, OptionsCategory::POC);
    gArgs.AddArg("-signprivkey", "Import private key for block signature", ArgsManager::ALLOW_ANY, OptionsCategory::POC);

#ifdef ENABLE_OMNICORE
    gArgs.AddArg("-omni", strprintf("Enable omnicore (default: %u)", DEFAULT_OMNICORE), ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnistartclean", "Clear all persistence files on startup; triggers reparsing of Omni transactions (default: 0)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnitxcache", "The maximum number of transactions in the input transaction cache (default: 500000)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniprogressfrequency", "Time in seconds after which the initial scanning progress is reported (default: 30)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniseedblockfilter", "Set skipping of blocks without Omni transactions during initial scan (default: 1)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnilogfile", "The path of the log file (default: omnicore.log)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnidebug=<category>", "Enable or disable log categories, can be \"all\" or \"none\"", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniautocommit", "Enable or disable broadcasting of transactions, when creating transactions (default: 1)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnioverrideforcedshutdown", "Overwrite shutdown, triggered by an alert (default: 0)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnialertallowsender", "Whitelist senders of alerts, can be \"any\")", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnialertignoresender", "Ignore senders of alerts", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniactivationignoresender", "Ignore senders of activations", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniactivationallowsender", "Whitelist senders of activations", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omniuiwalletscope", "Max. transactions to show in trade and transaction history (default: 65535)", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
    gArgs.AddArg("-omnishowblockconsensushash", "Calculate and log the consensus hash for the specified block", ArgsManager::ALLOW_ANY, OptionsCategory::OMNI);
#else
    hidden_args.emplace_back("-omni");
    hidden_args.emplace_back("-omnistartclean");
    hidden_args.emplace_back("-omnitxcache");
    hidden_args.emplace_back("-omniprogressfrequency");
    hidden_args.emplace_back("-omniseedblockfilter");
    hidden_args.emplace_back("-omnilogfile");
    hidden_args.emplace_back("-omnidebug");
    hidden_args.emplace_back("-omniautocommit");
    hidden_args.emplace_back("-omnioverrideforcedshutdown");
    hidden_args.emplace_back("-omnialertallowsender");
    hidden_args.emplace_back("-omnialertignoresender");
    hidden_args.emplace_back("-omomniactivationignoresenderni");
    hidden_args.emplace_back("-omniactivationallowsender");
    hidden_args.emplace_back("-omniuiwalletscope");
    hidden_args.emplace_back("-omnishowblockconsensushash");
#endif

#if HAVE_DECL_DAEMON
    gArgs.AddArg("-daemon", "Run in the background as a daemon and accept commands", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-daemon");
#endif

    // Add the hidden options
    gArgs.AddHiddenArgs(hidden_args);
}

std::string LicenseInfo()
{
    const std::string URL_SOURCE_CODE = "<https://github.com/qitcoin/qitcoin>";
    const std::string URL_WEBSITE = "<https://qitchain.org>";

    return CopyrightHolders(_("Copyright (C) %s").translated) + "\n" +
           "\n" +
           strprintf(_("Please contribute if you find %s useful. "
                       "Visit %s for further information about the software.").translated,
               _(PACKAGE_NAME).translated, URL_WEBSITE) +
           "\n" +
           strprintf(_("The source code is available from %s.").translated,
               URL_SOURCE_CODE) +
           "\n" +
           "\n" +
           _("This is experimental software.").translated + "\n" +
           strprintf(_("Distributed under the MIT software license, see the accompanying file %s or %s").translated, "COPYING", "<https://opensource.org/licenses/MIT>") + "\n" +
           "\n" +
           strprintf(_("This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit %s and cryptographic software written by Eric Young and UPnP software written by Thomas Bernard.").translated, "<https://www.openssl.org>") +
           "\n";
}

#if HAVE_SYSTEM
static void BlockNotifyCallback(bool initialSync, const CBlockIndex *pBlockIndex)
{
    if (initialSync || !pBlockIndex)
        return;

    std::string strCmd = gArgs.GetArg("-blocknotify", "");
    if (!strCmd.empty()) {
        boost::replace_all(strCmd, "%s", pBlockIndex->GetBlockHash().GetHex());
        std::thread t(runCommand, strCmd);
        t.detach(); // thread runs free
    }
}
#endif

static bool fHaveGenesis = false;
static Mutex g_genesis_wait_mutex;
static std::condition_variable g_genesis_wait_cv;

static void BlockNotifyGenesisWait(bool, const CBlockIndex *pBlockIndex)
{
    if (pBlockIndex != nullptr) {
        {
            LOCK(g_genesis_wait_mutex);
            fHaveGenesis = true;
        }
        g_genesis_wait_cv.notify_all();
    }
}

struct CImportingNow
{
    CImportingNow() {
        assert(fImporting == false);
        fImporting = true;
    }

    ~CImportingNow() {
        assert(fImporting == true);
        fImporting = false;
    }
};


// If we're using -prune with -reindex, then delete block files that will be ignored by the
// reindex.  Since reindexing works by starting at block file 0 and looping until a blockfile
// is missing, do the same here to delete any later block files after a gap.  Also delete all
// rev files since they'll be rewritten by the reindex anyway.  This ensures that vinfoBlockFile
// is in sync with what's actually on disk by the time we start downloading, so that pruning
// works correctly.
static void CleanupBlockRevFiles()
{
    std::map<std::string, fs::path> mapBlockFiles;

    // Glob all blk?????.dat and rev?????.dat files from the blocks directory.
    // Remove the rev files immediately and insert the blk file paths into an
    // ordered map keyed by block file index.
    LogPrintf("Removing unusable blk?????.dat and rev?????.dat files for -reindex with -prune\n");
    fs::path blocksdir = GetBlocksDir();
    for (fs::directory_iterator it(blocksdir); it != fs::directory_iterator(); it++) {
        if (fs::is_regular_file(*it) &&
            it->path().filename().string().length() == 12 &&
            it->path().filename().string().substr(8,4) == ".dat")
        {
            if (it->path().filename().string().substr(0,3) == "blk")
                mapBlockFiles[it->path().filename().string().substr(3,5)] = it->path();
            else if (it->path().filename().string().substr(0,3) == "rev")
                remove(it->path());
        }
    }

    // Remove all block files that aren't part of a contiguous set starting at
    // zero by walking the ordered map (keys are block file indices) by
    // keeping a separate counter.  Once we hit a gap (or if 0 doesn't exist)
    // start removing block files.
    int nContigCounter = 0;
    for (const std::pair<const std::string, fs::path>& item : mapBlockFiles) {
        if (atoi(item.first) == nContigCounter) {
            nContigCounter++;
            continue;
        }
        remove(item.second);
    }
}

static void ThreadImport(std::vector<fs::path> vImportFiles)
{
    const CChainParams& chainparams = Params();
    util::ThreadRename("loadblk");
    ScheduleBatchPriority();

    {
    CImportingNow imp;

    // -reindex
    if (fReindex) {
        int nFile = 0;
        while (true) {
            FlatFilePos pos(nFile, 0);
            if (!fs::exists(GetBlockPosFilename(pos)))
                break; // No block files left to reindex
            FILE *file = OpenBlockFile(pos, true);
            if (!file)
                break; // This error is logged in OpenBlockFile
            LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
            LoadExternalBlockFile(chainparams, file, &pos);
            nFile++;
        }
        pblocktree->WriteReindexing(false);
        fReindex = false;
        LogPrintf("Reindexing finished\n");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        LoadGenesisBlock(chainparams);
    }

    // hardcoded $DATADIR/bootstrap.dat
    fs::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (fs::exists(pathBootstrap)) {
        FILE *file = fsbridge::fopen(pathBootstrap, "rb");
        if (file) {
            fs::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            LogPrintf("Importing bootstrap.dat...\n");
            LoadExternalBlockFile(chainparams, file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        } else {
            LogPrintf("Warning: Could not open bootstrap file %s\n", pathBootstrap.string());
        }
    }

    // -loadblock=
    for (const fs::path& path : vImportFiles) {
        FILE *file = fsbridge::fopen(path, "rb");
        if (file) {
            LogPrintf("Importing blocks file %s...\n", path.string());
            LoadExternalBlockFile(chainparams, file);
        } else {
            LogPrintf("Warning: Could not open blocks file %s\n", path.string());
        }
    }

    // scan for better chains in the block chain database, that are not yet connected in the active best chain
    CValidationState state;
    if (!ActivateBestChain(state, chainparams)) {
        LogPrintf("Failed to connect best block (%s)\n", FormatStateMessage(state));
        StartShutdown();
        return;
    }

    if (gArgs.GetBoolArg("-stopafterblockimport", DEFAULT_STOPAFTERBLOCKIMPORT)) {
        LogPrintf("Stopping after block import\n");
        StartShutdown();
        return;
    }
    } // End scope of CImportingNow
    if (gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        LoadMempool(::mempool);
    }
    ::mempool.SetIsLoaded(!ShutdownRequested());
}

/** Sanity checks
 *  Ensure that Bitcoin is running in a usable environment with all
 *  necessary library support.
 */
static bool InitSanityCheck()
{
    if(!ECC_InitSanityCheck()) {
        InitError("Elliptic curve cryptography sanity check failure. Aborting.");
        return false;
    }

    if (!glibc_sanity_test() || !glibcxx_sanity_test())
        return false;

    if (!Random_SanityCheck()) {
        InitError("OS cryptographic RNG sanity check failure. Aborting.");
        return false;
    }

    return true;
}

static bool AppInitServers()
{
    RPCServer::OnStarted(&OnRPCStarted);
    RPCServer::OnStopped(&OnRPCStopped);
    if (!InitHTTPServer())
        return false;
    StartRPC();
    if (!StartHTTPRPC())
        return false;
    if (gArgs.GetBoolArg("-rest", DEFAULT_REST_ENABLE)) StartREST();
    StartHTTPServer();
    return true;
}

// Parameter interaction based on rules
void InitParameterInteraction()
{
    // when specifying an explicit binding address, you want to listen on it
    // even when -connect or -proxy is specified
    if (gArgs.IsArgSet("-bind")) {
        if (gArgs.SoftSetBoolArg("-listen", true))
            LogPrintf("%s: parameter interaction: -bind set -> setting -listen=1\n", __func__);
    }
    if (gArgs.IsArgSet("-whitebind")) {
        if (gArgs.SoftSetBoolArg("-listen", true))
            LogPrintf("%s: parameter interaction: -whitebind set -> setting -listen=1\n", __func__);
    }

    if (gArgs.IsArgSet("-connect")) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (gArgs.SoftSetBoolArg("-dnsseed", false))
            LogPrintf("%s: parameter interaction: -connect set -> setting -dnsseed=0\n", __func__);
        if (gArgs.SoftSetBoolArg("-listen", false))
            LogPrintf("%s: parameter interaction: -connect set -> setting -listen=0\n", __func__);
    }

    if (gArgs.IsArgSet("-proxy")) {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (gArgs.SoftSetBoolArg("-listen", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -listen=0\n", __func__);
        // to protect privacy, do not use UPNP when a proxy is set. The user may still specify -listen=1
        // to listen locally, so don't rely on this happening through -listen below.
        if (gArgs.SoftSetBoolArg("-upnp", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -upnp=0\n", __func__);
        // to protect privacy, do not discover addresses by default
        if (gArgs.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -proxy set -> setting -discover=0\n", __func__);
    }

    if (!gArgs.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (gArgs.SoftSetBoolArg("-upnp", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -upnp=0\n", __func__);
        if (gArgs.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -discover=0\n", __func__);
        if (gArgs.SoftSetBoolArg("-listenonion", false))
            LogPrintf("%s: parameter interaction: -listen=0 -> setting -listenonion=0\n", __func__);
    }

    if (gArgs.IsArgSet("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (gArgs.SoftSetBoolArg("-discover", false))
            LogPrintf("%s: parameter interaction: -externalip set -> setting -discover=0\n", __func__);
    }

    // disable whitelistrelay in blocksonly mode
    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY)) {
        if (gArgs.SoftSetBoolArg("-whitelistrelay", false))
            LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -whitelistrelay=0\n", __func__);
    }

    // Forcing relay from whitelisted hosts implies we will accept relays from them in the first place.
    if (gArgs.GetBoolArg("-whitelistforcerelay", DEFAULT_WHITELISTFORCERELAY)) {
        if (gArgs.SoftSetBoolArg("-whitelistrelay", true))
            LogPrintf("%s: parameter interaction: -whitelistforcerelay=1 -> setting -whitelistrelay=1\n", __func__);
    }
}

/**
 * Initialize global loggers.
 *
 * Note that this is called very early in the process lifetime, so you should be
 * careful about what global state you rely on here.
 */
void InitLogging()
{
    LogInstance().m_print_to_file = !gArgs.IsArgNegated("-debuglogfile");
    LogInstance().m_file_path = AbsPathForConfigVal(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    LogInstance().m_print_to_console = gArgs.GetBoolArg("-printtoconsole", !gArgs.GetBoolArg("-daemon", false));
    LogInstance().m_log_timestamps = gArgs.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    LogInstance().m_log_time_micros = gArgs.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
    LogInstance().m_log_threadnames = gArgs.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);

    fLogIPs = gArgs.GetBoolArg("-logips", DEFAULT_LOGIPS);

    std::string version_string = FormatFullVersion();
#ifdef DEBUG
    version_string += " (debug build)";
#else
    version_string += " (release build)";
#endif
    LogPrintf(PACKAGE_NAME " version %s\n", version_string);
}

namespace { // Variables internal to initialization process only

int nMaxConnections;
int nUserMaxConnections;
int nFD;
ServiceFlags nLocalServices = ServiceFlags(NODE_NETWORK | NODE_NETWORK_LIMITED);
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

bool AppInitBasicSetup()
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
    // Enable Data Execution Prevention (DEP)
    SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
#endif

    if (!SetupNetworking())
        return InitError("Initializing networking failed");

#ifndef WIN32
    if (!gArgs.GetBoolArg("-sysperms", false)) {
        umask(077);
    }

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

bool AppInitParameterInteraction()
{
    const CChainParams& chainparams = Params();
    // ********************************************************* Step 2: parameter interactions

    // also see: InitParameterInteraction()

    // Warn if network-specific options (-addnode, -connect, etc) are
    // specified in default section of config file, but not overridden
    // on the command line or in this network's section of the config file.
    std::string network = gArgs.GetChainName();
    for (const auto& arg : gArgs.GetUnsuitableSectionOnlyArgs()) {
        return InitError(strprintf(_("Config setting for %s only applied on %s network when in [%s] section.").translated, arg, network, network));
    }

    // Warn if unrecognized section name are present in the config file.
    for (const auto& section : gArgs.GetUnrecognizedSections()) {
        InitWarning(strprintf("%s:%i " + _("Section [%s] is not recognized.").translated, section.m_file, section.m_line, section.m_name));
    }

    if (!fs::is_directory(GetBlocksDir())) {
        return InitError(strprintf(_("Specified blocks directory \"%s\" does not exist.").translated, gArgs.GetArg("-blocksdir", "").c_str()));
    }

    // parse and validate enabled filter types
    std::string blockfilterindex_value = gArgs.GetArg("-blockfilterindex", DEFAULT_BLOCKFILTERINDEX);
    if (blockfilterindex_value == "" || blockfilterindex_value == "1") {
        g_enabled_filter_types = AllBlockFilterTypes();
    } else if (blockfilterindex_value != "0") {
        const std::vector<std::string> names = gArgs.GetArgs("-blockfilterindex");
        for (const auto& name : names) {
            BlockFilterType filter_type;
            if (!BlockFilterTypeByName(name, filter_type)) {
                return InitError(strprintf(_("Unknown -blockfilterindex value %s.").translated, name));
            }
            g_enabled_filter_types.insert(filter_type);
        }
    }

    // if using block pruning, then disallow txindex
    if (gArgs.GetArg("-prune", 0)) {
        if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX))
            return InitError(_("Prune mode is incompatible with -txindex.").translated);
        if (!g_enabled_filter_types.empty()) {
            return InitError(_("Prune mode is incompatible with -blockfilterindex.").translated);
        }
    }

#ifdef ENABLE_OMNICORE
    // omnicore dependency txindex
    if (gArgs.GetBoolArg("-omni", DEFAULT_OMNICORE) && !gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        return InitError(_("Omni require -txindex.").translated);
    }
#endif

    // -bind and -whitebind can't be set when not listening
    size_t nUserBind = gArgs.GetArgs("-bind").size() + gArgs.GetArgs("-whitebind").size();
    if (nUserBind != 0 && !gArgs.GetBoolArg("-listen", DEFAULT_LISTEN)) {
        return InitError("Cannot set -bind or -whitebind together with -listen=0");
    }

    // Make sure enough file descriptors are available
    int nBind = std::max(nUserBind, size_t(1));
    nUserMaxConnections = gArgs.GetArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS);
    nMaxConnections = std::max(nUserMaxConnections, 0);

    // Trim requested connection counts, to fit into system limitations
    // <int> in std::min<int>(...) to work around FreeBSD compilation issue described in #2695
    nFD = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS + MAX_ADDNODE_CONNECTIONS);
#ifdef USE_POLL
    int fd_max = nFD;
#else
    int fd_max = FD_SETSIZE;
#endif
    nMaxConnections = std::max(std::min<int>(nMaxConnections, fd_max - nBind - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS), 0);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available.").translated);
    nMaxConnections = std::min(nFD - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS, nMaxConnections);

    if (nMaxConnections < nUserMaxConnections)
        InitWarning(strprintf(_("Reducing -maxconnections from %d to %d, because of system limitations.").translated, nUserMaxConnections, nMaxConnections));

    // ********************************************************* Step 3: parameter-to-internal-flags
    if (gArgs.IsArgSet("-debug")) {
        // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
        const std::vector<std::string> categories = gArgs.GetArgs("-debug");

        if (std::none_of(categories.begin(), categories.end(),
            [](std::string cat){return cat == "0" || cat == "none";})) {
            for (const auto& cat : categories) {
                if (!LogInstance().EnableCategory(cat)) {
                    InitWarning(strprintf(_("Unsupported logging category %s=%s.").translated, "-debug", cat));
                }
            }
        }
    }

    // Now remove the logging categories which were explicitly excluded
    for (const std::string& cat : gArgs.GetArgs("-debugexclude")) {
        if (!LogInstance().DisableCategory(cat)) {
            InitWarning(strprintf(_("Unsupported logging category %s=%s.").translated, "-debugexclude", cat));
        }
    }

    // Checkmempool and checkblockindex default to true in regtest mode
    int ratio = std::min<int>(std::max<int>(gArgs.GetArg("-checkmempool", chainparams.DefaultConsistencyChecks() ? 1 : 0), 0), 1000000);
    if (ratio != 0) {
        mempool.setSanityCheck(1.0 / ratio);
    }
    fCheckBlockIndex = gArgs.GetBoolArg("-checkblockindex", chainparams.DefaultConsistencyChecks());
    fCheckpointsEnabled = gArgs.GetBoolArg("-checkpoints", DEFAULT_CHECKPOINTS_ENABLED);
    fCheckWork = gArgs.GetBoolArg("-forcecheckdeadline", DEFAULT_CHECKWORK_ENABLED);

    hashAssumeValid = uint256S(gArgs.GetArg("-assumevalid", chainparams.GetConsensus().defaultAssumeValid.GetHex()));
    if (!hashAssumeValid.IsNull())
        LogPrintf("Assuming ancestors of block %s have valid signatures.\n", hashAssumeValid.GetHex());
    else
        LogPrintf("Validating signatures for all blocks.\n");

    if (gArgs.IsArgSet("-minimumchainwork")) {
        const std::string minChainWorkStr = gArgs.GetArg("-minimumchainwork", "");
        if (!IsHexNumber(minChainWorkStr)) {
            return InitError(strprintf("Invalid non-hex (%s) minimum chain work value specified", minChainWorkStr));
        }
        nMinimumChainWork = UintToArith256(uint256S(minChainWorkStr));
    } else {
        nMinimumChainWork = UintToArith256(chainparams.GetConsensus().nMinimumChainWork);
    }
    LogPrintf("Setting nMinimumChainWork=%s\n", nMinimumChainWork.GetHex());
    if (nMinimumChainWork < UintToArith256(chainparams.GetConsensus().nMinimumChainWork)) {
        LogPrintf("Warning: nMinimumChainWork set below default value of %s\n", chainparams.GetConsensus().nMinimumChainWork.GetHex());
    }

    // mempool limits
    int64_t nMempoolSizeMax = gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    int64_t nMempoolSizeMin = gArgs.GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000 * 40;
    if (nMempoolSizeMax < 0 || nMempoolSizeMax < nMempoolSizeMin)
        return InitError(strprintf(_("-maxmempool must be at least %d MB").translated, std::ceil(nMempoolSizeMin / 1000000.0)));
    // incremental relay fee sets the minimum feerate increase necessary for BIP 125 replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (gArgs.IsArgSet("-incrementalrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-incrementalrelayfee", ""), n))
            return InitError(AmountErrMsg("incrementalrelayfee", gArgs.GetArg("-incrementalrelayfee", "")).translated);
        incrementalRelayFee = CFeeRate(n);
    }

    // -par=0 means autodetect, but nScriptCheckThreads==0 means no concurrency
    nScriptCheckThreads = gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
    if (nScriptCheckThreads <= 0)
        nScriptCheckThreads += GetNumCores();
    if (nScriptCheckThreads <= 1)
        nScriptCheckThreads = 0;
    else if (nScriptCheckThreads > MAX_SCRIPTCHECK_THREADS)
        nScriptCheckThreads = MAX_SCRIPTCHECK_THREADS;

    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg = gArgs.GetArg("-prune", 0);
    if (nPruneArg < 0) {
        return InitError(_("Prune cannot be configured with a negative value.").translated);
    }
    nPruneTarget = (uint64_t) nPruneArg * 1024 * 1024;
    if (nPruneArg == 1) {  // manual pruning: -prune=1
        LogPrintf("Block pruning enabled.  Use RPC call pruneblockchain(height) to manually prune block and undo files.\n");
        nPruneTarget = std::numeric_limits<uint64_t>::max();
        fPruneMode = true;
    } else if (nPruneTarget) {
        if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
            return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number.").translated, MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024));
        }
        LogPrintf("Prune configured to target %u MiB on disk for block and undo files.\n", nPruneTarget / 1024 / 1024);
        fPruneMode = true;
    }

    nConnectTimeout = gArgs.GetArg("-timeout", DEFAULT_CONNECT_TIMEOUT);
    if (nConnectTimeout <= 0) {
        nConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
    }

    peer_connect_timeout = gArgs.GetArg("-peertimeout", DEFAULT_PEER_CONNECT_TIMEOUT);
    if (peer_connect_timeout <= 0) {
        return InitError("peertimeout cannot be configured with a negative value.");
    }

    if (gArgs.IsArgSet("-minrelaytxfee")) {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-minrelaytxfee", ""), n)) {
            return InitError(AmountErrMsg("minrelaytxfee", gArgs.GetArg("-minrelaytxfee", "")).translated);
        }
        // High fee check is done afterward in CWallet::CreateWalletFromFile()
        ::minRelayTxFee = CFeeRate(n);
    } else if (incrementalRelayFee > ::minRelayTxFee) {
        // Allow only setting incrementalRelayFee to control both
        ::minRelayTxFee = incrementalRelayFee;
        LogPrintf("Increasing minrelaytxfee to %s to match incrementalrelayfee\n",::minRelayTxFee.ToString());
    }

    // Sanity check argument for min fee for including tx in block
    // TODO: Harmonize which arguments need sanity checking and where that happens
    if (gArgs.IsArgSet("-blockmintxfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-blockmintxfee", ""), n))
            return InitError(AmountErrMsg("blockmintxfee", gArgs.GetArg("-blockmintxfee", "")).translated);
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (gArgs.IsArgSet("-dustrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-dustrelayfee", ""), n))
            return InitError(AmountErrMsg("dustrelayfee", gArgs.GetArg("-dustrelayfee", "")).translated);
        dustRelayFee = CFeeRate(n);
    }

    fRequireStandard = !gArgs.GetBoolArg("-acceptnonstdtxn", !chainparams.RequireStandard());
    if (!chainparams.IsTestChain() && !fRequireStandard) {
        return InitError(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.NetworkIDString()));
    }
    nBytesPerSigOp = gArgs.GetArg("-bytespersigop", nBytesPerSigOp);

    if (!g_wallet_init_interface.ParameterInteraction()) return false;

    fIsBareMultisigStd = gArgs.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);
    fAcceptDatacarrier = gArgs.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER);
    nMaxDatacarrierBytes = gArgs.GetArg("-datacarriersize", nMaxDatacarrierBytes);

    // Option to startup with mocktime set (used for regression testing):
    SetMockTime(gArgs.GetArg("-mocktime", 0)); // SetMockTime(0) is a no-op

    if (gArgs.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS))
        nLocalServices = ServiceFlags(nLocalServices | NODE_BLOOM);

    if (gArgs.GetArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) < 0)
        return InitError("rpcserialversion must be non-negative.");

    if (gArgs.GetArg("-rpcserialversion", DEFAULT_RPC_SERIALIZE_VERSION) > 1)
        return InitError("unknown rpcserialversion requested.");

    nMaxTipAge = gArgs.GetArg("-maxtipage", DEFAULT_MAX_TIP_AGE);

    return true;
}

static bool LockDataDirectory(bool probeOnly)
{
    // Make sure only a single Bitcoin process is using the data directory.
    fs::path datadir = GetDataDir();
    if (!DirIsWritable(datadir)) {
        return InitError(strprintf(_("Cannot write to data directory '%s'; check permissions.").translated, datadir.string()));
    }
    if (!LockDirectory(datadir, ".lock", probeOnly)) {
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. %s is probably already running.").translated, datadir.string(), PACKAGE_NAME));
    }
    return true;
}

bool AppInitSanityChecks()
{
    // ********************************************************* Step 4: sanity checks

    // Initialize elliptic curve code
    std::string sha256_algo = SHA256AutoDetect();
    LogPrintf("Using the '%s' SHA256 implementation\n", sha256_algo);
    RandomInit();
    ECC_Start();
    globalVerifyHandle.reset(new ECCVerifyHandle());

    // Sanity check
    if (!InitSanityCheck())
        return InitError(strprintf(_("Initialization sanity check failed. %s is shutting down.").translated, PACKAGE_NAME));

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

bool AppInitMain(InitInterfaces& interfaces)
{
    const CChainParams& chainparams = Params();
    // ********************************************************* Step 4a: application initialization
    if (!CreatePidFile()) {
        // Detailed error printed inside CreatePidFile().
        return false;
    }
    if (LogInstance().m_print_to_file) {
        if (gArgs.GetBoolArg("-shrinkdebugfile", LogInstance().DefaultShrinkDebugFile())) {
            // Do this first since it both loads a bunch of debug.log into memory,
            // and because this needs to happen before any other debug.log printing
            LogInstance().ShrinkDebugFile();
        }
    }
    if (!LogInstance().StartLogging()) {
            return InitError(strprintf("Could not open debug log file %s",
                LogInstance().m_file_path.string()));
    }

    if (!LogInstance().m_log_timestamps)
        LogPrintf("Startup time: %s\n", FormatISO8601DateTime(GetTime()));
    LogPrintf("Default data directory %s\n", GetDefaultDataDir().string());
    LogPrintf("Using data directory %s\n", GetDataDir().string());

    // Only log conf file usage message if conf file actually exists.
    fs::path config_file_path = GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));
    if (fs::exists(config_file_path)) {
        LogPrintf("Config file: %s\n", config_file_path.string());
    } else if (gArgs.IsArgSet("-conf")) {
        // Warn if no conf file exists at path provided by user
        InitWarning(strprintf(_("The specified config file %s does not exist\n").translated, config_file_path.string()));
    } else {
        // Not categorizing as "Warning" because it's the default behavior
        LogPrintf("Config file: %s (not found, skipping)\n", config_file_path.string());
    }

    LogPrintf("Using at most %i automatic connections (%i file descriptors available)\n", nMaxConnections, nFD);

    // Warn about relative -datadir path.
    if (gArgs.IsArgSet("-datadir") && !fs::path(gArgs.GetArg("-datadir", "")).is_absolute()) {
        LogPrintf("Warning: relative datadir option '%s' specified, which will be interpreted relative to the " /* Continued */
                  "current working directory '%s'. This is fragile, because if bitcoin is started in the future "
                  "from a different location, it will be unable to locate the current data files. There could "
                  "also be data loss if bitcoin is started while in a temporary directory.\n",
            gArgs.GetArg("-datadir", ""), fs::current_path().string());
    }

    InitSignatureCache();
    InitScriptExecutionCache();

    LogPrintf("Using %u threads for script verification\n", nScriptCheckThreads);
    if (nScriptCheckThreads) {
        for (int i=0; i<nScriptCheckThreads-1; i++)
            threadGroup.create_thread([i]() { return ThreadScriptCheck(i); });
    }

    // Start the lightweight task scheduler thread
    CScheduler::Function serviceLoop = std::bind(&CScheduler::serviceQueue, &scheduler);
    threadGroup.create_thread(std::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));

    GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);
    GetMainSignals().RegisterWithMempoolSignals(mempool);

    // Create client interfaces for wallets that are supposed to be loaded
    // according to -wallet and -disablewallet options. This only constructs
    // the interfaces, it doesn't load wallet data. Wallets actually get loaded
    // when load() and start() interface methods are called below.
    g_wallet_init_interface.Construct(interfaces);

    /* Register RPC commands regardless of -server setting so they will be
     * available in the GUI RPC console even if external calls are disabled.
     */
#ifdef ENABLE_OMNICORE
    RegisterAllCoreRPCCommands(tableRPC, gArgs.GetBoolArg("-omni", DEFAULT_OMNICORE));
#else
    RegisterAllCoreRPCCommands(tableRPC);
#endif
    for (const auto& client : interfaces.chain_clients) {
        client->registerRpcs();
    }
    g_rpc_interfaces = &interfaces;
#if ENABLE_ZMQ
    RegisterZMQRPCCommands(tableRPC);
#endif

    /* Start the RPC server already.  It will be started in "warmup" mode
     * and not really process calls already (but it will signify connections
     * that the server is there and will be ready later).  Warmup mode will
     * be disabled when initialisation is finished.
     */
    if (gArgs.GetBoolArg("-server", false))
    {
        uiInterface.InitMessage_connect(SetRPCWarmupStatus);
        if (!AppInitServers())
            return InitError(_("Unable to start HTTP server. See debug log for details.").translated);
    }

    // ********************************************************* Step 5: verify wallet database integrity
    for (const auto& client : interfaces.chain_clients) {
        if (!client->verify()) {
            return false;
        }
    }

    // ********************************************************* Step 6: network initialization
    // Note that we absolutely cannot open any actual connections
    // until the very end ("start node") as the UTXO/block state
    // is not yet setup and may end up being set up twice if we
    // need to reindex later.

    assert(!g_banman);
    g_banman = MakeUnique<BanMan>(GetDataDir() / "banlist.dat", &uiInterface, gArgs.GetArg("-bantime", DEFAULT_MISBEHAVING_BANTIME));
    assert(!g_connman);
    g_connman = std::unique_ptr<CConnman>(new CConnman(GetRand(std::numeric_limits<uint64_t>::max()), GetRand(std::numeric_limits<uint64_t>::max())));

    peerLogic.reset(new PeerLogicValidation(g_connman.get(), g_banman.get(), scheduler, gArgs.GetBoolArg("-enablebip61", DEFAULT_ENABLE_BIP61)));
    RegisterValidationInterface(peerLogic.get());

    // sanitize comments per BIP-0014, format user agent and check total size
    std::vector<std::string> uacomments;
    for (const std::string& cmt : gArgs.GetArgs("-uacomment")) {
        if (cmt != SanitizeString(cmt, SAFE_CHARS_UA_COMMENT))
            return InitError(strprintf(_("User Agent comment (%s) contains unsafe characters.").translated, cmt));
        uacomments.push_back(cmt);
    }
    strSubVersion = FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, uacomments);
    if (strSubVersion.size() > MAX_SUBVERSION_LENGTH) {
        return InitError(strprintf(_("Total length of network version string (%i) exceeds maximum length (%i). Reduce the number or size of uacomments.").translated,
            strSubVersion.size(), MAX_SUBVERSION_LENGTH));
    }

    if (gArgs.IsArgSet("-onlynet")) {
        std::set<enum Network> nets;
        for (const std::string& snet : gArgs.GetArgs("-onlynet")) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(strprintf(_("Unknown network specified in -onlynet: '%s'").translated, snet));
            nets.insert(net);
        }
        for (int n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net))
                SetReachable(net, false);
        }
    }

    // Check for host lookup allowed before parsing any network related parameters
    fNameLookup = gArgs.GetBoolArg("-dns", DEFAULT_NAME_LOOKUP);

    bool proxyRandomize = gArgs.GetBoolArg("-proxyrandomize", DEFAULT_PROXYRANDOMIZE);
    // -proxy sets a proxy for all outgoing network traffic
    // -noproxy (or -proxy=0) as well as the empty string can be used to not set a proxy, this is the default
    std::string proxyArg = gArgs.GetArg("-proxy", "");
    SetReachable(NET_ONION, false);
    if (proxyArg != "" && proxyArg != "0") {
        CService proxyAddr;
        if (!Lookup(proxyArg.c_str(), proxyAddr, 9050, fNameLookup)) {
            return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'").translated, proxyArg));
        }

        proxyType addrProxy = proxyType(proxyAddr, proxyRandomize);
        if (!addrProxy.IsValid())
            return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'").translated, proxyArg));

        SetProxy(NET_IPV4, addrProxy);
        SetProxy(NET_IPV6, addrProxy);
        SetProxy(NET_ONION, addrProxy);
        SetNameProxy(addrProxy);
        SetReachable(NET_ONION, true); // by default, -proxy sets onion as reachable, unless -noonion later
    }

    // -onion can be used to set only a proxy for .onion, or override normal proxy for .onion addresses
    // -noonion (or -onion=0) disables connecting to .onion entirely
    // An empty string is used to not override the onion proxy (in which case it defaults to -proxy set above, or none)
    std::string onionArg = gArgs.GetArg("-onion", "");
    if (onionArg != "") {
        if (onionArg == "0") { // Handle -noonion/-onion=0
            SetReachable(NET_ONION, false);
        } else {
            CService onionProxy;
            if (!Lookup(onionArg.c_str(), onionProxy, 9050, fNameLookup)) {
                return InitError(strprintf(_("Invalid -onion address or hostname: '%s'").translated, onionArg));
            }
            proxyType addrOnion = proxyType(onionProxy, proxyRandomize);
            if (!addrOnion.IsValid())
                return InitError(strprintf(_("Invalid -onion address or hostname: '%s'").translated, onionArg));
            SetProxy(NET_ONION, addrOnion);
            SetReachable(NET_ONION, true);
        }
    }

    // see Step 2: parameter interactions for more information about these
    fListen = gArgs.GetBoolArg("-listen", DEFAULT_LISTEN);
    fDiscover = gArgs.GetBoolArg("-discover", true);
    g_relay_txes = !gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY);

    for (const std::string& strAddr : gArgs.GetArgs("-externalip")) {
        CService addrLocal;
        if (Lookup(strAddr.c_str(), addrLocal, GetListenPort(), fNameLookup) && addrLocal.IsValid())
            AddLocal(addrLocal, LOCAL_MANUAL);
        else
            return InitError(ResolveErrMsg("externalip", strAddr));
    }

#if ENABLE_ZMQ
    g_zmq_notification_interface = CZMQNotificationInterface::Create();

    if (g_zmq_notification_interface) {
        RegisterValidationInterface(g_zmq_notification_interface);
    }
#endif
    uint64_t nMaxOutboundLimit = 0; //unlimited unless -maxuploadtarget is set
    uint64_t nMaxOutboundTimeframe = MAX_UPLOAD_TIMEFRAME;

    if (gArgs.IsArgSet("-maxuploadtarget")) {
        nMaxOutboundLimit = gArgs.GetArg("-maxuploadtarget", DEFAULT_MAX_UPLOAD_TARGET)*1024*1024;
    }

    // ********************************************************* Step 7: load block chain

    fReindex = gArgs.GetBoolArg("-reindex", false);
    bool fReindexChainState = gArgs.GetBoolArg("-reindex-chainstate", false);

    // cache size calculations
    int64_t nTotalCache = (gArgs.GetArg("-dbcache", nDefaultDbCache) << 20);
    nTotalCache = std::max(nTotalCache, nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    nTotalCache = std::min(nTotalCache, nMaxDbCache << 20); // total cache cannot be greater than nMaxDbcache
    int64_t nBlockTreeDBCache = std::min(nTotalCache / 8, nMaxBlockDBCache << 20);
    nTotalCache -= nBlockTreeDBCache;
    int64_t nTxIndexCache = std::min(nTotalCache / 8, gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? nMaxTxIndexCache << 20 : 0);
    nTotalCache -= nTxIndexCache;
    int64_t filter_index_cache = 0;
    if (!g_enabled_filter_types.empty()) {
        size_t n_indexes = g_enabled_filter_types.size();
        int64_t max_cache = std::min(nTotalCache / 8, max_filter_index_cache << 20);
        filter_index_cache = max_cache / n_indexes;
        nTotalCache -= filter_index_cache * n_indexes;
    }
    int64_t nCoinDBCache = std::min(nTotalCache / 2, (nTotalCache / 4) + (1 << 23)); // use 25%-50% of the remainder for disk cache
    nCoinDBCache = std::min(nCoinDBCache, nMaxCoinsDBCache << 20); // cap total coins db cache
    nTotalCache -= nCoinDBCache;
    nCoinCacheUsage = nTotalCache; // the rest goes to in-memory cache
    int64_t nMempoolSizeMax = gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    LogPrintf("Cache configuration:\n");
    LogPrintf("* Using %.1f MiB for block index database\n", nBlockTreeDBCache * (1.0 / 1024 / 1024));
    if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        LogPrintf("* Using %.1f MiB for transaction index database\n", nTxIndexCache * (1.0 / 1024 / 1024));
    }
    for (BlockFilterType filter_type : g_enabled_filter_types) {
        LogPrintf("* Using %.1f MiB for %s block filter index database\n",
                  filter_index_cache * (1.0 / 1024 / 1024), BlockFilterTypeName(filter_type));
    }
    LogPrintf("* Using %.1f MiB for chain state database\n", nCoinDBCache * (1.0 / 1024 / 1024));
    LogPrintf("* Using %.1f MiB for in-memory UTXO set (plus up to %.1f MiB of unused mempool space)\n", nCoinCacheUsage * (1.0 / 1024 / 1024), nMempoolSizeMax * (1.0 / 1024 / 1024));

    bool fLoaded = false;
    while (!fLoaded && !ShutdownRequested()) {
        bool fReset = fReindex;
        std::string strLoadError;

        uiInterface.InitMessage(_("Loading block index...").translated);

        do {
            const int64_t load_block_index_start_time = GetTimeMillis();
            bool is_coinsview_empty;
            bool fCoinDBUpgraded = false;
            try {
                LOCK(cs_main);
                // This statement makes ::ChainstateActive() usable.
                g_chainstate = MakeUnique<CChainState>();
                UnloadBlockIndex();

                // new CBlockTreeDB tries to delete the existing file, which
                // fails if it's still open from the previous loop. Close it first:
                pblocktree.reset();
                pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, false, fReset));

                if (fReset) {
                    pblocktree->WriteReindexing(true);
                    //If we're reindexing in prune mode, wipe away unusable block files and all undo data files
                    if (fPruneMode)
                        CleanupBlockRevFiles();
                }

                if (ShutdownRequested()) break;

                // LoadBlockIndex will load fHavePruned if we've ever removed a
                // block file from disk.
                // Note that it also sets fReindex based on the disk flag!
                // From here on out fReindex and fReset mean something different!
                if (!LoadBlockIndex(chainparams)) {
                    if (ShutdownRequested()) break;
                    strLoadError = _("Error loading block database").translated;
                    break;
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!::BlockIndex().empty() &&
                        !LookupBlockIndex(chainparams.GetConsensus().hashGenesisBlock)) {
                    return InitError(_("Incorrect or no genesis block found. Wrong datadir for network?").translated);
                }

                // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
                // in the past, but is now trying to run unpruned.
                if (fHavePruned && !fPruneMode) {
                    strLoadError = _("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain").translated;
                    break;
                }

                // At this point blocktree args are consistent with what's on disk.
                // If we're not mid-reindex (based on disk + args), add a genesis block on disk
                // (otherwise we use the one already on disk).
                // This is called again in ThreadImport after the reindex completes.
                if (!fReindex && !LoadGenesisBlock(chainparams)) {
                    strLoadError = _("Error initializing block database").translated;
                    break;
                }

                // At this point we're either in reindex or we've loaded a useful
                // block tree into BlockIndex()!

                ::ChainstateActive().InitCoinsDB(
                    /* cache_size_bytes */ nCoinDBCache,
                    /* in_memory */ false,
                    /* should_wipe */ fReset || fReindexChainState);

                ::ChainstateActive().CoinsErrorCatcher().AddReadErrCallback([]() {
                    uiInterface.ThreadSafeMessageBox(
                        _("Error reading from database, shutting down.").translated,
                        "", CClientUIInterface::MSG_ERROR);
                });

                // If necessary, upgrade from older database format.
                // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!::ChainstateActive().CoinsDB().Upgrade(fCoinDBUpgraded)) {
                    strLoadError = _("Error upgrading chainstate database").translated;
                    break;
                }

                // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!::ChainstateActive().ReplayBlocks(chainparams)) {
                    strLoadError = _("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.").translated;
                    break;
                }

                // The on-disk coinsdb is now in a good state, create the cache
                ::ChainstateActive().InitCoinsCache();
                assert(::ChainstateActive().CanFlushToDisk());

                is_coinsview_empty = fReset || fReindexChainState ||
                    ::ChainstateActive().CoinsTip().GetBestBlock().IsNull();
                if (!is_coinsview_empty) {
                    // LoadChainTip initializes the chain based on CoinsTip()'s best block
                    if (!::ChainstateActive().LoadChainTip(chainparams)) {
                        strLoadError = _("Error initializing block database").translated;
                        break;
                    }
                    assert(::ChainActive().Tip() != nullptr);
                }
            } catch (const std::exception& e) {
                LogPrintf("%s\n", e.what());
                strLoadError = _("Error opening block database").translated;
                break;
            }

            try {
                if (!is_coinsview_empty) {
                    if (fCoinDBUpgraded) {
                        LogPrintf("Upgrading UTXO database");

                        // Doing upgrade
                    }
                }
            } catch (const std::exception& e) {
                LogPrintf("%s\n", e.what());
                strLoadError = _("Error opening block database").translated;
                break;
            }

            if (!fReset) {
                // Note that RewindBlockIndex MUST run even if we're about to -reindex-chainstate.
                // It both disconnects blocks based on ::ChainActive(), and drops block data in
                // BlockIndex() based on lack of available witness data.
                uiInterface.InitMessage(_("Rewinding blocks...").translated);
                if (!RewindBlockIndex(chainparams)) {
                    strLoadError = _("Unable to rewind the database to a pre-fork state. You will need to redownload the blockchain").translated;
                    break;
                }
            }

            try {
                if (!is_coinsview_empty) {
                    uiInterface.InitMessage(_("Verifying blocks...").translated);
                    if (fHavePruned && gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS) > MIN_BLOCKS_TO_KEEP) {
                        LogPrintf("Prune: pruned datadir may not have more than %d blocks; only checking available blocks\n",
                            MIN_BLOCKS_TO_KEEP);
                    }

                    // Verify by checkpoints
                    if (!chainparams.Checkpoints().mapCheckpoints.empty()) {
                        const MapCheckpoints &mapCheckpoints = chainparams.Checkpoints().mapCheckpoints;
                        CBlockIndex *pBeginResetIndex = nullptr;
                        {
                            LOCK(cs_main);

                            // 1.First invalid block on checkpoint
                            for (auto it = mapCheckpoints.cbegin(); it != mapCheckpoints.cend(); it++) {
                                CBlockIndex *pindex = LookupBlockIndex(it->second);
                                if (pindex != nullptr && !pindex->IsValid()) {
                                    pBeginResetIndex = pindex;
                                    break;
                                }
                            }
                            // 2.First valid block when block data too old
                            if (!pBeginResetIndex && ::ChainActive().Height() < mapCheckpoints.rbegin()->first) {
                                // Reset last checkpoint block when block data too old
                                for (auto it = mapCheckpoints.rbegin(); it != mapCheckpoints.rend(); it++) {
                                    if (it->first < ::ChainActive().Height()) {
                                        pBeginResetIndex = ::ChainActive()[it->first];
                                        break;
                                    }
                                }
                            }
                            // 3.Last checkpoint
                            if (!pBeginResetIndex) {
                                for (auto it = mapCheckpoints.rbegin(); it != mapCheckpoints.rend(); it++) {
                                    CBlockIndex *pindex = LookupBlockIndex(it->second);
                                    if (pindex != nullptr) {
                                        pBeginResetIndex = pindex;
                                        break;
                                    }
                                }
                            }
                        }

                        LogPrintf("Verifying blocks...");

                        // 4.Reset after block fail flags
                        if (pBeginResetIndex) {
                            {
                                LOCK(cs_main);
                                ResetBlockFailureFlags(pBeginResetIndex);
                            }
                            CValidationState state;
                            if (!ActivateBestChain(state, chainparams)) {
                                LogPrintf("%s: %s\n", __func__, FormatStateMessage(state));
                                strLoadError = _("Error initializing block database").translated;
                                break;
                            }
                        }

                        // 5.Invalid bad block
                        for (auto it = mapCheckpoints.cbegin(); it != mapCheckpoints.cend() && it->first <= ::ChainActive().Height();) {
                            CBlockIndex *pindex = ::ChainActive()[it->first];
                            if (*(pindex->phashBlock) != it->second) {
                                // Invalid
                                CValidationState state;
                                if (!InvalidateBlock(state, chainparams, pindex) || !ActivateBestChain(state, chainparams)) {
                                    LogPrintf("%s: %s\n", __func__, FormatStateMessage(state));
                                    strLoadError = _("Error initializing block database").translated;
                                    break;
                                }
                            } else {
                                it++;
                            }
                        }
                        if (!strLoadError.empty())
                            break; // Got error

                        // 6.Flush cache to db
                        ::ChainstateActive().ForceFlushStateToDisk();
                    }

                    {
                        LOCK(cs_main);
                        CBlockIndex* tip = ::ChainActive().Tip();
                        RPCNotifyBlockChange(true, tip);
                        if (tip && tip->nTime > GetAdjustedTime() + MAX_FUTURE_BLOCK_TIME) {
                            strLoadError = _("The block database contains a block which appears to be from the future. "
                                    "This may be due to your computer's date and time being set incorrectly. "
                                    "Only rebuild the block database if you are sure that your computer's date and time are correct").translated;
                            break;
                        }

                        if (!CVerifyDB().VerifyDB(chainparams, &::ChainstateActive().CoinsDB(), gArgs.GetArg("-checklevel", DEFAULT_CHECKLEVEL),
                                    gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS))) {
                            strLoadError = _("Corrupted block database detected").translated;
                            break;
                        }
                    }
                }
            } catch (const std::exception& e) {
                LogPrintf("%s\n", e.what());
                strLoadError = _("Error opening block database").translated;
                break;
            }

            fLoaded = true;
            LogPrintf(" block index %15dms\n", GetTimeMillis() - load_block_index_start_time);
        } while(false);

        if (!fLoaded && !ShutdownRequested()) {
            // first suggest a reindex
            if (!fReset) {
                bool fRet = uiInterface.ThreadSafeQuestion(
                    strLoadError + ".\n\n" + _("Do you want to rebuild the block database now?").translated,
                    strLoadError + ".\nPlease restart with -reindex or -reindex-chainstate to recover.",
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    fReindex = true;
                    AbortShutdown();
                } else {
                    LogPrintf("Aborted block database rebuild. Exiting.\n");
                    return false;
                }
            } else {
                return InitError(strLoadError);
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

    fs::path est_path = GetDataDir() / FEE_ESTIMATES_FILENAME;
    CAutoFile est_filein(fsbridge::fopen(est_path, "rb"), SER_DISK, CLIENT_VERSION);
    // Allowed to fail as this file IS missing on first startup.
    if (!est_filein.IsNull())
        ::feeEstimator.Read(est_filein);
    fFeeEstimatesInitialized = true;

    // ********************************************************* Step 8: start indexers
    if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        g_txindex = MakeUnique<TxIndex>(nTxIndexCache, false, fReindex);
        g_txindex->Start();
    }

    for (const auto& filter_type : g_enabled_filter_types) {
        InitBlockFilterIndex(filter_type, filter_index_cache, false, fReindex);
        GetBlockFilterIndex(filter_type)->Start();
    }

#ifdef ENABLE_OMNICORE
    // ********************************************************* Step 8.5: load omni core
    if (gArgs.GetBoolArg("-omni", DEFAULT_OMNICORE)) {
        assert(g_txindex != nullptr);
        std::string tail = "......";
        for (int i = 0; !g_txindex->BlockUntilSyncedToCurrentChain(); i = (i + 1) % (int)tail.size()) {
            uiInterface.InitMessage(_("Omni Layer waiting txindex").translated + tail.substr(0, i+1));
            MilliSleep(2000);
        }

        uiInterface.InitMessage(_("Parsing Omni Layer transactions...").translated);
        omnicore_api::Init();
    }
#endif

    // ********************************************************* Step 9: load wallet
    for (const auto& client : interfaces.chain_clients) {
        if (!client->load()) {
            return false;
        }
    }
#ifdef ENABLE_OMNICORE
    // Omni Core code should be initialized and wallet should now be loaded, perform an initial populat$
    if (omnicore_api::Enabled()) {
        omnicore_api::CheckWalletUpdate();
    }
#endif

    // ********************************************************* Step 10: data directory maintenance

    // if pruning, unset the service bit and perform the initial blockstore prune
    // after any wallet rescanning has taken place.
    if (fPruneMode) {
        LogPrintf("Unsetting NODE_NETWORK on prune mode\n");
        nLocalServices = ServiceFlags(nLocalServices & ~NODE_NETWORK);
        if (!fReindex) {
            uiInterface.InitMessage(_("Pruning blockstore...").translated);
            ::ChainstateActive().PruneAndFlush();
        }
    }

    if (chainparams.GetConsensus().SegwitHeight != std::numeric_limits<int>::max()) {
        // Advertise witness capabilities.
        // The option to not set NODE_WITNESS is only used in the tests and should be removed.
        nLocalServices = ServiceFlags(nLocalServices | NODE_WITNESS);
    }

    // ********************************************************* Step 11: import blocks

    if (!CheckDiskSpace(GetDataDir())) {
        InitError(strprintf(_("Error: Disk space is low for %s").translated, GetDataDir()));
        return false;
    }
    if (!CheckDiskSpace(GetBlocksDir())) {
        InitError(strprintf(_("Error: Disk space is low for %s").translated, GetBlocksDir()));
        return false;
    }

    // Either install a handler to notify us when genesis activates, or set fHaveGenesis directly.
    // No locking, as this happens before any background thread is started.
    boost::signals2::connection block_notify_genesis_wait_connection;
    if (::ChainActive().Tip() == nullptr) {
        block_notify_genesis_wait_connection = uiInterface.NotifyBlockTip_connect(BlockNotifyGenesisWait);
    } else {
        fHaveGenesis = true;
    }

#if HAVE_SYSTEM
    if (gArgs.IsArgSet("-blocknotify"))
        uiInterface.NotifyBlockTip_connect(BlockNotifyCallback);
#endif

    std::vector<fs::path> vImportFiles;
    for (const std::string& strFile : gArgs.GetArgs("-loadblock")) {
        vImportFiles.push_back(strFile);
    }

    threadGroup.create_thread(std::bind(&ThreadImport, vImportFiles));

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

    if (ShutdownRequested()) {
        return false;
    }

    // ********************************************************* Step 12: start node

    int chain_active_height;

    //// debug print
    {
        LOCK(cs_main);
        LogPrintf("block tree size = %u\n", ::BlockIndex().size());
        chain_active_height = ::ChainActive().Height();
    }
    LogPrintf("nBestHeight = %d\n", chain_active_height);

    if (gArgs.GetBoolArg("-listenonion", DEFAULT_LISTEN_ONION))
        StartTorControl();

    Discover();

    // Map ports with UPnP
    if (gArgs.GetBoolArg("-upnp", DEFAULT_UPNP)) {
        StartMapPort();
    }

    CConnman::Options connOptions;
    connOptions.nLocalServices = nLocalServices;
    connOptions.nMaxConnections = nMaxConnections;
    connOptions.m_max_outbound_full_relay = std::min(MAX_OUTBOUND_FULL_RELAY_CONNECTIONS, connOptions.nMaxConnections);
    connOptions.m_max_outbound_block_relay = std::min(MAX_BLOCKS_ONLY_CONNECTIONS, connOptions.nMaxConnections-connOptions.m_max_outbound_full_relay);
    connOptions.nMaxAddnode = MAX_ADDNODE_CONNECTIONS;
    connOptions.nMaxFeeler = 1;
    connOptions.nBestHeight = chain_active_height;
    connOptions.uiInterface = &uiInterface;
    connOptions.m_banman = g_banman.get();
    connOptions.m_msgproc = peerLogic.get();
    connOptions.nSendBufferMaxSize = 1000*gArgs.GetArg("-maxsendbuffer", DEFAULT_MAXSENDBUFFER);
    connOptions.nReceiveFloodSize = 1000*gArgs.GetArg("-maxreceivebuffer", DEFAULT_MAXRECEIVEBUFFER);
    connOptions.m_added_nodes = gArgs.GetArgs("-addnode");

    connOptions.nMaxOutboundTimeframe = nMaxOutboundTimeframe;
    connOptions.nMaxOutboundLimit = nMaxOutboundLimit;
    connOptions.m_peer_connect_timeout = peer_connect_timeout;

    for (const std::string& strBind : gArgs.GetArgs("-bind")) {
        CService addrBind;
        if (!Lookup(strBind.c_str(), addrBind, GetListenPort(), false)) {
            return InitError(ResolveErrMsg("bind", strBind));
        }
        connOptions.vBinds.push_back(addrBind);
    }
    for (const std::string& strBind : gArgs.GetArgs("-whitebind")) {
        NetWhitebindPermissions whitebind;
        std::string error;
        if (!NetWhitebindPermissions::TryParse(strBind, whitebind, error)) return InitError(error);
        connOptions.vWhiteBinds.push_back(whitebind);
    }

    for (const auto& net : gArgs.GetArgs("-whitelist")) {
        NetWhitelistPermissions subnet;
        std::string error;
        if (!NetWhitelistPermissions::TryParse(net, subnet, error)) return InitError(error);
        connOptions.vWhitelistedRange.push_back(subnet);
    }

    connOptions.vSeedNodes = gArgs.GetArgs("-seednode");

    // Initiate outbound connections unless connect=0
    connOptions.m_use_addrman_outgoing = !gArgs.IsArgSet("-connect");
    if (!connOptions.m_use_addrman_outgoing) {
        const auto connect = gArgs.GetArgs("-connect");
        if (connect.size() != 1 || connect[0] != "0") {
            connOptions.m_specified_outgoing = connect;
        }
    }
    if (!g_connman->Start(scheduler, connOptions)) {
        return false;
    }

    // ********************************************************* Step 13: finished

    SetRPCWarmupFinished();
    uiInterface.InitMessage(_("Done loading").translated);

    for (const auto& client : interfaces.chain_clients) {
        client->start(scheduler);
    }

    scheduler.scheduleEvery([]{
        g_banman->DumpBanlist();
    }, DUMP_BANS_INTERVAL * 1000);

    // PoC module dependency wallets
    if (!StartPOC())
        return false;

    return true;
}
