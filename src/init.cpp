// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/dash-config.h>
#endif

#include <init.h>

#include <addrman.h>
#include <amount.h>
#include <banman.h>
#include <base58.h>
#include <chain.h>
#include <chainparams.h>
#include <checkpoints.h>
#include <node/coinstats.h>
#include <compat/sanity.h>
#include <consensus/validation.h>
#include <fs.h>
#include <hash.h>
#include <httpserver.h>
#include <httprpc.h>
#include <index/txindex.h>
#include <key.h>
#include <validation.h>
#include <miner.h>
#include <netbase.h>
#include <net.h>
#include <net_permissions.h>
#include <net_processing.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <rpc/server.h>
#include <rpc/register.h>
#include <rpc/blockchain.h>
#include <script/standard.h>
#include <script/sigcache.h>
#include <scheduler.h>
#include <shutdown.h>
#include <timedata.h>
#include <txdb.h>
#include <txmempool.h>
#include <torcontrol.h>
#include <ui_interface.h>
#include <util/asmap.h>
#include <util/error.h>
#include <util/moneystr.h>
#include <util/system.h>
#include <util/threadnames.h>
#include <util/validation.h>
#include <validationinterface.h>

#include <masternode/activemasternode.h>
#include <coinjoin/coinjoin-server.h>
#include <dsnotificationinterface.h>
#include <flat-database.h>
#include <governance/governance.h>
#include <masternode/masternode-meta.h>
#include <masternode/masternode-sync.h>
#include <masternode/masternode-utils.h>
#include <messagesigner.h>
#include <netfulfilledman.h>
#include <spork.h>
#include <warnings.h>
#include <walletinitinterface.h>

#include <evo/deterministicmns.h>
#include <llmq/quorums.h>
#include <llmq/quorums_init.h>
#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_signing.h>
#include <llmq/quorums_utils.h>

#include <statsd_client.h>

#include <stdint.h>
#include <stdio.h>

#include <bls/bls.h>

#ifndef WIN32
#include <signal.h>
#include <sys/stat.h>
#endif

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <openssl/crypto.h>

#if ENABLE_ZMQ
#include <zmq/zmqnotificationinterface.h>
#include <zmq/zmqrpc.h>
#endif

bool fFeeEstimatesInitialized = false;
static const bool DEFAULT_PROXYRANDOMIZE = true;
static const bool DEFAULT_REST_ENABLE = false;
static const bool DEFAULT_STOPAFTERBLOCKIMPORT = false;

// Dump addresses to banlist.dat every 15 minutes (900s)
static constexpr int DUMP_BANS_INTERVAL = 60 * 15;

std::unique_ptr<CConnman> g_connman;
std::unique_ptr<PeerLogicValidation> peerLogic;
std::unique_ptr<BanMan> g_banman;

#if !(ENABLE_WALLET)
class DummyWalletInit : public WalletInitInterface {
public:

    void AddWalletOptions() const override;
    bool ParameterInteraction() const override {return true;}
    void RegisterRPC(CRPCTable &) const override {}
    bool Verify() const override {return true;}
    bool Open() const override {LogPrintf("No wallet support compiled in!\n"); return true;}
    void Start(CScheduler& scheduler) const override {}
    void Flush() const override {}
    void Stop() const override {}
    void Close() const override {}

    // Dash Specific WalletInitInterface InitCoinJoinSettings
    void AutoLockMasternodeCollaterals() const override {}
    void InitCoinJoinSettings() const override {}
    void InitKeePass() const override {}
    bool InitAutoBackup() const override {return true;}
};

void DummyWalletInit::AddWalletOptions() const
{
    std::vector<std::string> opts = {"-createwalletbackups=<n>", "-disablewallet", "-instantsendnotify=<cmd>",
        "-keypool=<n>", "-rescan=<mode>", "-salvagewallet", "-spendzeroconfchange", "-upgradewallet",
        "-wallet=<path>", "-walletbackupsdir=<dir>", "-walletbroadcast", "-walletdir=<dir>",
        "-walletnotify=<cmd>", "-zapwallettxes=<mode>", "-discardfee=<amt>", "-fallbackfee=<amt>",
        "-mintxfee=<amt>", "-paytxfee=<amt>", "-txconfirmtarget=<n>", "-hdseed=<hex>", "-mnemonic=<text>",
        "-mnemonicpassphrase=<text>", "-usehd", "-keepass", "-keepassid=<id>", "-keepasskey=<key>",
        "-keepassname=<name>", "-keepassport=<port>", "-enablecoinjoin", "-coinjoinamount=<n>",
        "-coinjoinautostart", "-coinjoindenomsgoal=<n>", "-coinjoindenomshardcap=<n>", "-coinjoinmultisession",
        "-coinjoinrounds=<n>", "-coinjoinsessions=<n>", "-dblogsize=<n>", "-flushwallet", "-privdb",
        "-walletrejectlongchains"};
    gArgs.AddHiddenArgs(opts);
}

const WalletInitInterface& g_wallet_init_interface = DummyWalletInit();
#endif

static CDSNotificationInterface* pdsNotificationInterface = nullptr;

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files don't count towards the fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

static const char* FEE_ESTIMATES_FILENAME="fee_estimates.dat";

static const char* DEFAULT_ASMAP_FILENAME="ip_asn.map";

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

/**
 * This is a minimally invasive approach to shutdown on LevelDB read errors from the
 * chainstate, while keeping user interface out of the common library, which is shared
 * between dashd, and dash-qt and non-server tools.
*/
class CCoinsViewErrorCatcher final : public CCoinsViewBacked
{
public:
    explicit CCoinsViewErrorCatcher(CCoinsView* view) : CCoinsViewBacked(view) {}
    bool GetCoin(const COutPoint &outpoint, Coin &coin) const override {
        try {
            return CCoinsViewBacked::GetCoin(outpoint, coin);
        } catch(const std::runtime_error& e) {
            uiInterface.ThreadSafeMessageBox(_("Error reading from database, shutting down."), "", CClientUIInterface::MSG_ERROR);
            LogPrintf("Error reading from database: %s\n", e.what());
            // Starting the shutdown sequence and returning false to the caller would be
            // interpreted as 'entry not found' (as opposed to unable to read data), and
            // could lead to invalid interpretation. Just exit immediately, as we can't
            // continue anyway, and all writes should be atomic.
            abort();
        }
    }
    // Writes do not need similar protection, as failure to write is handled by the caller.
};

static std::unique_ptr<CCoinsViewErrorCatcher> pcoinscatcher;
static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

static boost::thread_group threadGroup;
static CScheduler scheduler;

void Interrupt()
{
    InterruptHTTPServer();
    InterruptHTTPRPC();
    InterruptRPC();
    InterruptREST();
    InterruptTorControl();
    llmq::InterruptLLMQSystem();
    InterruptMapPort();
    if (g_connman)
        g_connman->Interrupt();
    if (g_txindex) {
        g_txindex->Interrupt();
    }
}

/** Preparing steps before shutting down or restarting the wallet */
void PrepareShutdown()
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
    StopHTTPRPC();
    StopREST();
    StopRPC();
    StopHTTPServer();
    llmq::StopLLMQSystem();

    // fRPCInWarmup should be `false` if we completed the loading sequence
    // before a shutdown request was received
    std::string statusmessage;
    bool fRPCInWarmup = RPCIsInWarmup(&statusmessage);

    g_wallet_init_interface.Flush();
    StopMapPort();

    // Because these depend on each-other, we make sure that neither can be
    // using the other before destroying them.
    if (peerLogic) UnregisterValidationInterface(peerLogic.get());
    if (g_connman) g_connman->Stop();
    if (g_txindex) g_txindex->Stop();

    StopTorControl();

    // After everything has been shut down, but before things get flushed, stop the
    // CScheduler/checkqueue threadGroup
    threadGroup.interrupt_all();
    threadGroup.join_all();
    StopScriptCheckWorkerThreads();

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    if (!fRPCInWarmup) {
        // STORE DATA CACHES INTO SERIALIZED DAT FILES
        CFlatDB<CMasternodeMetaMan> flatdb1("mncache.dat", "magicMasternodeCache");
        flatdb1.Dump(mmetaman);
        CFlatDB<CNetFulfilledRequestManager> flatdb4("netfulfilled.dat", "magicFulfilledCache");
        flatdb4.Dump(netfulfilledman);
        CFlatDB<CSporkManager> flatdb6("sporks.dat", "magicSporkCache");
        flatdb6.Dump(sporkManager);
        if (!fDisableGovernance) {
            CFlatDB<CGovernanceManager> flatdb3("governance.dat", "magicGovernanceCache");
            flatdb3.Dump(governance);
        }
    }

    // After the threads that potentially access these pointers have been stopped,
    // destruct and reset all to nullptr.
    peerLogic.reset();
    g_connman.reset();
    g_banman.reset();
    g_txindex.reset();

    if (g_is_mempool_loaded && gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        DumpMempool();
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
    if (pcoinsTip != nullptr) {
        FlushStateToDisk();
    }

    // After there are no more peers/RPC left to give us new data which may generate
    // CValidationInterface callbacks, flush them...
    GetMainSignals().FlushBackgroundCallbacks();

    // Any future callbacks will be dropped. This should absolutely be safe - if
    // missing a callback results in an unrecoverable situation, unclean shutdown
    // would too. The only reason to do the above flushes is to let the wallet catch
    // up with our current chain to avoid any strange pruning edge cases and make
    // next startup faster by avoiding rescan.

    {
        LOCK(cs_main);
        if (pcoinsTip != nullptr) {
            FlushStateToDisk();
        }
        pcoinsTip.reset();
        pcoinscatcher.reset();
        pcoinsdbview.reset();
        pblocktree.reset();
        llmq::DestroyLLMQSystem();
        deterministicMNManager.reset();
        evoDb.reset();
    }
    g_wallet_init_interface.Stop();

#if ENABLE_ZMQ
    if (g_zmq_notification_interface) {
        UnregisterValidationInterface(g_zmq_notification_interface);
        delete g_zmq_notification_interface;
        g_zmq_notification_interface = nullptr;
    }
#endif

    if (pdsNotificationInterface) {
        UnregisterValidationInterface(pdsNotificationInterface);
        delete pdsNotificationInterface;
        pdsNotificationInterface = nullptr;
    }
    if (fMasternodeMode) {
        UnregisterValidationInterface(activeMasternodeManager);
    }

    // make sure to clean up BLS keys before global destructors are called (they have allocated from the secure memory pool)
    activeMasternodeInfo.blsKeyOperator.reset();
    activeMasternodeInfo.blsPubKeyOperator.reset();

#ifndef WIN32
    try {
        fs::remove(GetPidFile());
    } catch (const fs::filesystem_error& e) {
        LogPrintf("%s: Unable to remove pidfile: %s\n", __func__, e.what());
    }
#endif
    UnregisterAllValidationInterfaces();
    GetMainSignals().UnregisterBackgroundSignalScheduler();
    GetMainSignals().UnregisterWithMempoolSignals(mempool);
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
void Shutdown()
{
    // Shutdown part 1: prepare shutdown
    if(!RestartRequested()) {
        PrepareShutdown();
    }
    // Shutdown part 2: delete wallet instance
    g_wallet_init_interface.Close();
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
    g_logger->m_reopen_file = true;
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

static void OnRPCStarted()
{
    uiInterface.NotifyBlockTip_connect(&RPCNotifyBlockChange);
}

static void OnRPCStopped()
{
    uiInterface.NotifyBlockTip_disconnect(&RPCNotifyBlockChange);
    RPCNotifyBlockChange(false, nullptr);
    g_best_block_cv.notify_all();
    LogPrint(BCLog::RPC, "RPC stopped.\n");
}

std::string GetSupportedSocketEventsStr()
{
    std::string strSupportedModes = "'select'";
#ifdef USE_POLL
    strSupportedModes += ", 'poll'";
#endif
#ifdef USE_EPOLL
    strSupportedModes += ", 'epoll'";
#endif
#ifdef USE_KQUEUE
    strSupportedModes += ", 'kqueue'";
#endif
    return strSupportedModes;
}

void SetupServerArgs()
{
    const auto defaultBaseParams = CreateBaseChainParams(CBaseChainParams::MAIN);
    const auto testnetBaseParams = CreateBaseChainParams(CBaseChainParams::TESTNET);
    const auto defaultChainParams = CreateChainParams(CBaseChainParams::MAIN);
    const auto testnetChainParams = CreateChainParams(CBaseChainParams::TESTNET);

    Consensus::Params devnetConsensus = CreateChainParams(CBaseChainParams::DEVNET, true)->GetConsensus();
    Consensus::LLMQParams devnetLLMQ = devnetConsensus.llmqs.at(Consensus::LLMQ_DEVNET);

    const auto regtestLLMQ = CreateChainParams(CBaseChainParams::REGTEST)->GetConsensus().llmqs.at(Consensus::LLMQ_TEST);

    // Hidden Options
    std::vector<std::string> hidden_args = {"-rpcssl", "-benchmark", "-h", "-help", "-socks", "-tor", "-debugnet", "-whitelistalwaysrelay",
        "-blockminsize", "-dbcrashratio", "-forcecompactdb",
        // GUI args. These will be overwritten by SetupUIArgs for the GUI
        "-allowselfsignedrootcertificates", "-choosedatadir", "-lang=<lang>", "-min", "-resetguisettings", "-rootcertificates=<file>", "-splash", "-uiplatform"};


    // Set all of the args and their help
    // When adding new options to the categories, please keep and ensure alphabetical ordering.
    gArgs.AddArg("-?", "Print this help message and exit", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-alertnotify=<cmd>", "Execute command when a relevant alert is received or we see a really long fork (%s in cmd is replaced by message)", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-assumevalid=<hex>", strprintf("If this block is in the chain assume that it and its ancestors are valid and potentially skip their script verification (0 to verify all, default: %s, testnet: %s)", defaultChainParams->GetConsensus().defaultAssumeValid.GetHex(), testnetChainParams->GetConsensus().defaultAssumeValid.GetHex()), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blocksdir=<dir>", "Specify directory to hold blocks subdirectory for *.dat files (default: <datadir>)", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blocknotify=<cmd>", "Execute command when the best block changes (%s in cmd is replaced by block hash)", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blockreconstructionextratxn=<n>", strprintf("Extra transactions to keep in memory for compact block reconstructions (default: %u)", DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-blocksonly", strprintf("Whether to operate in a blocks only mode (default: %u)", DEFAULT_BLOCKSONLY), true, OptionsCategory::OPTIONS);
    gArgs.AddArg("-conf=<file>", strprintf("Specify configuration file. Relative paths will be prefixed by datadir location. (default: %s)", BITCOIN_CONF_FILENAME), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-datadir=<dir>", "Specify data directory", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-dbbatchsize", strprintf("Maximum database write batch size in bytes (default: %u)", nDefaultDbBatchSize), true, OptionsCategory::OPTIONS);
    gArgs.AddArg("-dbcache=<n>", strprintf("Set database cache size in megabytes (%d to %d, default: %d)", nMinDbCache, nMaxDbCache, nDefaultDbCache), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-debuglogfile=<file>", strprintf("Specify location of debug log file. Relative paths will be prefixed by a net-specific datadir location. (-nodebuglogfile to disable; default: %s)", DEFAULT_DEBUGLOGFILE), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-includeconf=<file>", "Specify additional configuration file, relative to the -datadir path (only useable from configuration file, not command line)", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-loadblock=<file>", "Imports blocks from external blk000??.dat file on startup", false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-maxmempool=<n>", strprintf("Keep the transaction memory pool below <n> megabytes (default: %u)", DEFAULT_MAX_MEMPOOL_SIZE), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-maxorphantxsize=<n>", strprintf("Maximum total size of all orphan transactions in megabytes (default: %u)", DEFAULT_MAX_ORPHAN_TRANSACTIONS_SIZE), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-maxrecsigsage=<n>", strprintf("Number of seconds to keep LLMQ recovery sigs (default: %u)", llmq::DEFAULT_MAX_RECOVERED_SIGS_AGE), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-mempoolexpiry=<n>", strprintf("Do not keep transactions in the mempool longer than <n> hours (default: %u)", DEFAULT_MEMPOOL_EXPIRY), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-minimumchainwork=<hex>", strprintf("Minimum work assumed to exist on a valid chain in hex (default: %s, testnet: %s)", defaultChainParams->GetConsensus().nMinimumChainWork.GetHex(), testnetChainParams->GetConsensus().nMinimumChainWork.GetHex()), true, OptionsCategory::OPTIONS);
    gArgs.AddArg("-par=<n>", strprintf("Set the number of script verification threads (%u to %d, 0 = auto, <0 = leave that many cores free, default: %d)",
        -GetNumCores(), MAX_SCRIPTCHECK_THREADS, DEFAULT_SCRIPTCHECK_THREADS), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-persistmempool", strprintf("Whether to save the mempool on shutdown and load on restart (default: %u)", DEFAULT_PERSIST_MEMPOOL), false, OptionsCategory::OPTIONS);
#ifndef WIN32
    gArgs.AddArg("-pid=<file>", strprintf("Specify pid file. Relative paths will be prefixed by a net-specific datadir location. (default: %s)", BITCOIN_PID_FILENAME), false, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-pid");
#endif
    gArgs.AddArg("-prune=<n>", strprintf("Reduce storage requirements by enabling pruning (deleting) of old blocks. This allows the pruneblockchain RPC to be called to delete specific blocks, and enables automatic pruning of old blocks if a target size in MiB is provided. This mode is incompatible with -txindex, -rescan and -disablegovernance=false. "
            "Warning: Reverting this setting requires re-downloading the entire blockchain. "
            "(default: 0 = disable pruning blocks, 1 = allow manual pruning via RPC, >%u = automatically prune block files to stay under the specified target size in MiB)", MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024), false, OptionsCategory::OPTIONS);
    gArgs.AddArg("-syncmempool", strprintf("Sync mempool from other nodes on start (default: %u)", DEFAULT_SYNC_MEMPOOL), false, OptionsCategory::OPTIONS);
#ifndef WIN32
    gArgs.AddArg("-sysperms", "Create new files with system default permissions, instead of umask 077 (only effective with disabled wallet functionality)", false, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-sysperms");
#endif
    gArgs.AddArg("-version", "Print version and exit", false, OptionsCategory::OPTIONS);

    gArgs.AddArg("-addressindex", strprintf("Maintain a full address index, used to query for the balance, txids and unspent outputs for addresses (default: %u)", DEFAULT_ADDRESSINDEX), false, OptionsCategory::INDEXING);
    gArgs.AddArg("-reindex", "Rebuild chain state and block index from the blk*.dat files on disk", false, OptionsCategory::INDEXING);
    gArgs.AddArg("-reindex-chainstate", "Rebuild chain state from the currently indexed blocks. When in pruning mode or if blocks on disk might be corrupted, use full -reindex instead.", false, OptionsCategory::INDEXING);
    gArgs.AddArg("-spentindex", strprintf("Maintain a full spent index, used to query the spending txid and input index for an outpoint (default: %u)", DEFAULT_SPENTINDEX), false, OptionsCategory::INDEXING);
    gArgs.AddArg("-timestampindex", strprintf("Maintain a timestamp index for block hashes, used to query blocks hashes by a range of timestamps (default: %u)", DEFAULT_TIMESTAMPINDEX), false, OptionsCategory::INDEXING);
    gArgs.AddArg("-txindex", strprintf("Maintain a full transaction index, used by the getrawtransaction rpc call (default: %u)", DEFAULT_TXINDEX), false, OptionsCategory::INDEXING);

    gArgs.AddArg("-asmap=<file>", strprintf("Specify asn mapping used for bucketing of the peers (default: %s). Relative paths will be prefixed by the net-specific datadir location.", DEFAULT_ASMAP_FILENAME), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-addnode=<ip>", "Add a node to connect to and attempt to keep the connection open (see the `addnode` RPC command help for more info). This option can be specified multiple times to add multiple nodes.", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-allowprivatenet", strprintf("Allow RFC1918 addresses to be relayed and connected to (default: %u)", DEFAULT_ALLOWPRIVATENET), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-banscore=<n>", strprintf("Threshold for disconnecting misbehaving peers (default: %u)", DEFAULT_BANSCORE_THRESHOLD), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-bantime=<n>", strprintf("Number of seconds to keep misbehaving peers from reconnecting (default: %u)", DEFAULT_MISBEHAVING_BANTIME), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-bind=<addr>", "Bind to given address and always listen on it. Use [host]:port notation for IPv6", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-connect=<ip>", "Connect only to the specified node; -noconnect disables automatic connections (the rules for this peer are the same as for -addnode). This option can be specified multiple times to connect to multiple nodes.", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-discover", "Discover own IP addresses (default: 1 when listening and no -externalip or -proxy)", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-dns", strprintf("Allow DNS lookups for -addnode, -seednode and -connect (default: %u)", DEFAULT_NAME_LOOKUP), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-dnsseed", "Query for peer addresses via DNS lookup, if low on addresses (default: 1 unless -connect used)", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-enablebip61", strprintf("Send reject messages per BIP61 (default: %u)", DEFAULT_ENABLE_BIP61), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-externalip=<ip>", "Specify your own public address", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-forcednsseed", strprintf("Always query for peer addresses via DNS lookup (default: %u)", DEFAULT_FORCEDNSSEED), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-listen", "Accept connections from outside (default: 1 if no -proxy or -connect)", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-listenonion", strprintf("Automatically create Tor hidden service (default: %d)", DEFAULT_LISTEN_ONION), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxconnections=<n>", strprintf("Maintain at most <n> connections to peers (temporary service connections excluded) (default: %u)", DEFAULT_MAX_PEER_CONNECTIONS), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxreceivebuffer=<n>", strprintf("Maximum per-connection receive buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXRECEIVEBUFFER), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxsendbuffer=<n>", strprintf("Maximum per-connection send buffer, <n>*1000 bytes (default: %u)", DEFAULT_MAXSENDBUFFER), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxtimeadjustment", strprintf("Maximum allowed median peer time offset adjustment. Local perspective of time may be influenced by peers forward or backward by this amount. (default: %u seconds)", DEFAULT_MAX_TIME_ADJUSTMENT), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-maxuploadtarget=<n>", strprintf("Tries to keep outbound traffic under the given target (in MiB per 24h), 0 = no limit (default: %d)", DEFAULT_MAX_UPLOAD_TARGET), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-onion=<ip:port>", "Use separate SOCKS5 proxy to reach peers via Tor hidden services, set -noonion to disable (default: -proxy)", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-onlynet=<net>", "Make outgoing connections only through network <net> (ipv4, ipv6 or onion). Incoming connections are not affected by this option. This option can be specified multiple times to allow multiple networks.", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-peerbloomfilters", strprintf("Support filtering of blocks and transaction with bloom filters (default: %u)", DEFAULT_PEERBLOOMFILTERS), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-peertimeout=<n>", strprintf("Specify p2p connection timeout in seconds. This option determines the amount of time a peer may be inactive before the connection to it is dropped. (minimum: 1, default: %d)", DEFAULT_PEER_CONNECT_TIMEOUT), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-permitbaremultisig", strprintf("Relay non-P2SH multisig (default: %u)", DEFAULT_PERMIT_BAREMULTISIG), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-port=<port>", strprintf("Listen for connections on <port> (default: %u or testnet: %u)", defaultChainParams->GetDefaultPort(), testnetChainParams->GetDefaultPort()), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-proxy=<ip:port>", "Connect through SOCKS5 proxy, set -noproxy to disable (default: disabled)", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-proxyrandomize", strprintf("Randomize credentials for every proxy connection. This enables Tor stream isolation (default: %u)", DEFAULT_PROXYRANDOMIZE), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-seednode=<ip>", "Connect to a node to retrieve peer addresses, and disconnect. This option can be specified multiple times to connect to multiple nodes.", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-socketevents=<mode>", "Socket events mode, which must be one of 'select', 'poll', 'epoll' or 'kqueue', depending on your system (default: Linux - 'epoll', FreeBSD/Apple - 'kqueue', Windows - 'select')", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-timeout=<n>", strprintf("Specify connection timeout in milliseconds (minimum: 1, default: %d)", DEFAULT_CONNECT_TIMEOUT), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-torcontrol=<ip>:<port>", strprintf("Tor control port to use if onion listening enabled (default: %s)", DEFAULT_TOR_CONTROL), false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-torpassword=<pass>", "Tor control port password (default: empty)", false, OptionsCategory::CONNECTION);
#ifdef USE_UPNP
#if USE_UPNP
    gArgs.AddArg("-upnp", "Use UPnP to map the listening port (default: 1 when listening and no -proxy)", false, OptionsCategory::CONNECTION);
#else
    gArgs.AddArg("-upnp", strprintf("Use UPnP to map the listening port (default: %u)", 0), false, OptionsCategory::CONNECTION);
#endif
#else
    hidden_args.emplace_back("-upnp");
#endif
    gArgs.AddArg("-whitebind=<addr>", "Bind to given address and whitelist peers connecting to it. Use [host]:port notation for IPv6", false, OptionsCategory::CONNECTION);
    gArgs.AddArg("-whitelist=<IP address or network>", "Whitelist peers connecting from the given IP address (e.g. 1.2.3.4) or CIDR notated network (e.g. 1.2.3.0/24). Can be specified multiple times."
        " Whitelisted peers cannot be DoS banned and their transactions are always relayed, even if they are already in the mempool, useful e.g. for a gateway", false, OptionsCategory::CONNECTION);

    g_wallet_init_interface.AddWalletOptions();

#if ENABLE_ZMQ
    gArgs.AddArg("-zmqpubhashblock=<address>", "Enable publish hash block in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashchainlock=<address>", "Enable publish hash block (locked via ChainLocks) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashgovernanceobject=<address>", "Enable publish hash of governance objects (like proposals) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashgovernancevote=<address>", "Enable publish hash of governance votes in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashinstantsenddoublespend=<address>", "Enable publish transaction hashes of attempted InstantSend double spend in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashrecoveredsig=<address>", "Enable publish message hash of recovered signatures (recovered by LLMQs) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashtx=<address>", "Enable publish hash transaction in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubhashtxlock=<address>", "Enable publish hash transaction (locked via InstantSend) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawblock=<address>", "Enable publish raw block in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawchainlock=<address>", "Enable publish raw block (locked via ChainLocks) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawchainlocksig=<address>", "Enable publish raw block (locked via ChainLocks) and CLSIG message in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawgovernancevote=<address>", "Enable publish raw governance objects (like proposals) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawgovernanceobject=<address>", "Enable publish raw governance votes in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawinstantsenddoublespend=<address>", "Enable publish raw transactions of attempted InstantSend double spend in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawrecoveredsig=<address>", "Enable publish raw recovered signatures (recovered by LLMQs) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawtx=<address>", "Enable publish raw transaction in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawtxlock=<address>", "Enable publish raw transaction (locked via InstantSend) in <address>", false, OptionsCategory::ZMQ);
    gArgs.AddArg("-zmqpubrawtxlocksig=<address>", "Enable publish raw transaction (locked via InstantSend) and ISLOCK in <address>", false, OptionsCategory::ZMQ);
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
#endif

    gArgs.AddArg("-checkblockindex", strprintf("Do a full consistency check for mapBlockIndex, setBlockIndexCandidates, chainActive and mapBlocksUnlinked occasionally. (default: %u)", defaultChainParams->DefaultConsistencyChecks()), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkblocks=<n>", strprintf("How many blocks to check at startup (default: %u, 0 = all)", DEFAULT_CHECKBLOCKS), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checklevel=<n>", strprintf("How thorough the block verification of -checkblocks is: "
        "level 0 reads the blocks from disk, "
        "level 1 verifies block validity, "
        "level 2 verifies undo data, "
        "level 3 checks disconnection of tip blocks, "
        "and level 4 tries to reconnect the blocks, "
        "each level includes the checks of the previous levels "
        "(0-4, default: %u)", DEFAULT_CHECKLEVEL), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkmempool=<n>", strprintf("Run checks every <n> transactions (default: %u)", defaultChainParams->DefaultConsistencyChecks()), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-checkpoints", strprintf("Disable expensive verification for known chain history (default: %u)", DEFAULT_CHECKPOINTS_ENABLED), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-deprecatedrpc=<method>", "Allows deprecated RPC method(s) to be used", true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-dropmessagestest=<n>", "Randomly drop 1 of every <n> network messages", true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitancestorcount=<n>", strprintf("Do not accept transactions if number of in-mempool ancestors is <n> or more (default: %u)", DEFAULT_ANCESTOR_LIMIT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitancestorsize=<n>", strprintf("Do not accept transactions whose size with all in-mempool ancestors exceeds <n> kilobytes (default: %u)", DEFAULT_ANCESTOR_SIZE_LIMIT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitdescendantcount=<n>", strprintf("Do not accept transactions if any ancestor would have <n> or more in-mempool descendants (default: %u)", DEFAULT_DESCENDANT_LIMIT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-limitdescendantsize=<n>", strprintf("Do not accept transactions if any ancestor would have more than <n> kilobytes of in-mempool descendants (default: %u).", DEFAULT_DESCENDANT_SIZE_LIMIT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-stopafterblockimport", strprintf("Stop running after importing blocks from disk (default: %u)", DEFAULT_STOPAFTERBLOCKIMPORT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-stopatheight", strprintf("Stop running after reaching the given height in the main chain (default: %u)", DEFAULT_STOPATHEIGHT), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-vbparams=<deployment>:<start>:<end>(:<window>:<threshold>)", "Use given start/end times for specified version bits deployment (regtest-only). Specifying window and threshold is optional.", true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-watchquorums=<n>", strprintf("Watch and validate quorum communication (default: %u)", llmq::DEFAULT_WATCH_QUORUMS), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-addrmantest", "Allows to test address relay on localhost", true, OptionsCategory::DEBUG_TEST);

    gArgs.AddArg("-budgetparams=<masternode>:<budget>:<superblock>", "Override masternode, budget and superblock start heights", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-debug=<category>", "Output debugging information (default: -nodebug, supplying <category> is optional). "
        "If <category> is not supplied or if <category> = 1, output all debugging information. <category> can be: " + ListLogCategories() + ".", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-debugexclude=<category>", strprintf("Exclude debugging information for a category. Can be used in conjunction with -debug=1 to output debug logs for all categories except one or more specified categories."), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-dip3params=<activation>:<enforcement>", "Override DIP3 activation and enforcement heights", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-dip8params=<activation>", "Override DIP8 activation height", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-disablegovernance", strprintf("Disable governance validation (0-1, default: %u)", 0), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-help-debug", "Print help message with debugging options and exit", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-highsubsidyblocks=<n>", strprintf("The number of blocks with a higher than normal subsidy to mine at the start of a devnet (default: %u)", devnetConsensus.nHighSubsidyBlocks), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-highsubsidyfactor=<n>", strprintf("The factor to multiply the normal block subsidy by while in the highsubsidyblocks window of a devnet (default: %u)", devnetConsensus.nHighSubsidyFactor), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-llmqchainlocks=<quorum name>", strprintf("Override the default LLMQ type used for ChainLocks on a devnet. Allows using ChainLocks with smaller LLMQs. (default: %s)", devnetConsensus.llmqs.at(devnetConsensus.llmqTypeChainLocks).name), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-llmqdevnetparams=<size:threshold>", strprintf("Override the default LLMQ size for the LLMQ_DEVNET quorum (default: %u:%u)", devnetLLMQ.size, devnetLLMQ.threshold), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-llmqinstantsend=<quorum name>", strprintf("Override the default LLMQ type used for InstantSend on a devnet. Allows using InstantSend with smaller LLMQs. (default: %s)", devnetConsensus.llmqs.at(devnetConsensus.llmqTypeInstantSend).name), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-llmqtestparams=<size:threshold>", strprintf("Override the default LLMQ size for the LLMQ_TEST quorum (default: %u:%u)", regtestLLMQ.size, regtestLLMQ.threshold), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logips", strprintf("Include IP addresses in debug output (default: %u)", DEFAULT_LOGIPS), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logtimemicros", strprintf("Add microsecond precision to debug timestamps (default: %u)", DEFAULT_LOGTIMEMICROS), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logtimestamps", strprintf("Prepend debug output with timestamp (default: %u)", DEFAULT_LOGTIMESTAMPS), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-logthreadnames", strprintf("Prepend debug output with name of the originating thread (only available on platforms supporting thread_local) (default: %u)", DEFAULT_LOGTHREADNAMES), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-maxsigcachesize=<n>", strprintf("Limit sum of signature cache and script execution cache sizes to <n> MiB (default: %u)", DEFAULT_MAX_SIG_CACHE_SIZE), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-maxtipage=<n>", strprintf("Maximum tip age in seconds to consider node in initial block download (default: %u)", DEFAULT_MAX_TIP_AGE), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-mocktime=<n>", "Replace actual time with <n> seconds since epoch (default: 0)", true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction or raw transaction; setting this too low may abort large transactions (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-minimumdifficultyblocks=<n>", strprintf("The number of blocks that can be mined with the minimum difficulty at the start of a devnet (default: %u)", devnetConsensus.nMinimumDifficultyBlocks), false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-minsporkkeys=<n>", "Overrides minimum spork signers to change spork value. Only useful for regtest and devnet. Using this on mainnet or testnet will ban you.", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printpriority", strprintf("Log transaction fee per kB when mining blocks (default: %u)", DEFAULT_PRINTPRIORITY), true, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-printtoconsole", "Send trace/debug info to console (default: 1 when no -daemon. To disable logging to file, set -nodebuglogfile)", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-pushversion", "Protocol version to report to other nodes", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-shrinkdebugfile", "Shrink debug.log file on client startup (default: 1 when no -debug)", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-sporkaddr=<dashaddress>", "Override spork address. Only useful for regtest and devnet. Using this on mainnet or testnet will ban you.", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-sporkkey=<privatekey>", "Set the private key to be used for signing spork messages.", false, OptionsCategory::DEBUG_TEST);
    gArgs.AddArg("-uacomment=<cmt>", "Append comment to the user agent string", false, OptionsCategory::DEBUG_TEST);

    SetupChainParamsBaseOptions();

    gArgs.AddArg("-llmq-data-recovery=<n>", strprintf("Enable automated quorum data recovery (default: %u)", llmq::DEFAULT_ENABLE_QUORUM_DATA_RECOVERY), false, OptionsCategory::MASTERNODE);
    gArgs.AddArg("-llmq-qvvec-sync=<quorum_name>:<mode>", strprintf("Defines from which LLMQ type the masternode should sync quorum verification vectors. Can be used multiple times with different LLMQ types. <mode>: %d (sync always from all quorums of the type defined by <quorum_name>), %d (sync from all quorums of the type defined by <quorum_name> if a member of any of the quorums)", (int32_t)llmq::QvvecSyncMode::Always, (int32_t)llmq::QvvecSyncMode::OnlyIfTypeMember), false, OptionsCategory::MASTERNODE);
    gArgs.AddArg("-masternodeblsprivkey=<hex>", "Set the masternode BLS private key and enable the client to act as a masternode", false, OptionsCategory::MASTERNODE);
    gArgs.AddArg("-platform-user=<user>", "Set the username for the \"platform user\", a restricted user intended to be used by Dash Platform, to the specified username.", false, OptionsCategory::MASTERNODE);

    gArgs.AddArg("-acceptnonstdtxn", strprintf("Relay and mine \"non-standard\" transactions (%sdefault: %u)", "testnet/regtest only; ", !testnetChainParams->RequireStandard()), true, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-dustrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to defined dust, the value of an output such that it will cost more than its value in fees at this fee rate to spend it. (default: %s)", CURRENCY_UNIT, FormatMoney(DUST_RELAY_TX_FEE)), true, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-incrementalrelayfee=<amt>", strprintf("Fee rate (in %s/kB) used to define cost of relay, used for mempool limiting and BIP 125 replacement. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_INCREMENTAL_RELAY_FEE)), true, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-bytespersigop", strprintf("Minimum bytes per sigop in transactions we relay and mine (default: %u)", DEFAULT_BYTES_PER_SIGOP), false, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-datacarrier", strprintf("Relay and mine data carrier transactions (default: %u)", DEFAULT_ACCEPT_DATACARRIER), false, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-datacarriersize", strprintf("Maximum size of data in data carrier transactions we relay and mine (default: %u)", MAX_OP_RETURN_RELAY), false, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-minrelaytxfee=<amt>", strprintf("Fees (in %s/kB) smaller than this are considered zero fee for relaying, mining and transaction creation (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_MIN_RELAY_TX_FEE)), false, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-whitelistforcerelay", strprintf("Force relay of transactions from whitelisted peers even if they violate local relay policy (default: %d)", DEFAULT_WHITELISTFORCERELAY), false, OptionsCategory::NODE_RELAY);
    gArgs.AddArg("-whitelistrelay", strprintf("Accept relayed transactions received from whitelisted peers even when not relaying transactions (default: %d)", DEFAULT_WHITELISTRELAY), false, OptionsCategory::NODE_RELAY);

    gArgs.AddArg("-blockmaxsize=<n>", strprintf("Set maximum block size in bytes (default: %d)", DEFAULT_BLOCK_MAX_SIZE), false, OptionsCategory::BLOCK_CREATION);
    gArgs.AddArg("-blockmintxfee=<amt>", strprintf("Set lowest fee rate (in %s/kB) for transactions to be included in block creation. (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_BLOCK_MIN_TX_FEE)), false, OptionsCategory::BLOCK_CREATION);
    gArgs.AddArg("-blockversion=<n>", "Override block version to test forking scenarios", true, OptionsCategory::BLOCK_CREATION);

    gArgs.AddArg("-rest", strprintf("Accept public REST requests (default: %u)", DEFAULT_REST_ENABLE), false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcallowip=<ip>", "Allow JSON-RPC connections from specified source. Valid for <ip> are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24). This option can be specified multiple times", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcauth=<userpw>", "Username and hashed password for JSON-RPC connections. The field <userpw> comes in the format: <USERNAME>:<SALT>$<HASH>. A canonical python script is included in share/rpcuser. The client then connects normally using the rpcuser=<USERNAME>/rpcpassword=<PASSWORD> pair of arguments. This option can be specified multiple times", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcbind=<addr>[:port]", "Bind to given address to listen for JSON-RPC connections. Do not expose the RPC server to untrusted networks such as the public internet! This option is ignored unless -rpcallowip is also passed. Port is optional and overrides -rpcport. Use [host]:port notation for IPv6. This option can be specified multiple times (default: 127.0.0.1 and ::1 i.e., localhost, or if -rpcallowip has been specified, 0.0.0.0 and :: i.e., all addresses)", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpccookiefile=<loc>", "Location of the auth cookie. Relative paths will be prefixed by a net-specific datadir location. (default: data dir)", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcpassword=<pw>", "Password for JSON-RPC connections", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcport=<port>", strprintf("Listen for JSON-RPC connections on <port> (default: %u or testnet: %u)", defaultBaseParams->RPCPort(), testnetBaseParams->RPCPort()), false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcservertimeout=<n>", strprintf("Timeout during HTTP requests (default: %d)", DEFAULT_HTTP_SERVER_TIMEOUT), true, OptionsCategory::RPC);
    gArgs.AddArg("-rpcthreads=<n>", strprintf("Set the number of threads to service RPC calls (default: %d)", DEFAULT_HTTP_THREADS), false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcuser=<user>", "Username for JSON-RPC connections", false, OptionsCategory::RPC);
    gArgs.AddArg("-rpcworkqueue=<n>", strprintf("Set the depth of the work queue to service RPC calls (default: %d)", DEFAULT_HTTP_WORKQUEUE), true, OptionsCategory::RPC);
    gArgs.AddArg("-server", "Accept command line and JSON-RPC commands", false, OptionsCategory::RPC);

    gArgs.AddArg("-statsenabled", strprintf("Publish internal stats to statsd (default: %u)", DEFAULT_STATSD_ENABLE), false, OptionsCategory::STATSD);
    gArgs.AddArg("-statshost=<ip>", strprintf("Specify statsd host (default: %s)", DEFAULT_STATSD_HOST), false, OptionsCategory::STATSD);
    gArgs.AddArg("-statshostname=<ip>", strprintf("Specify statsd host name (default: %s)", DEFAULT_STATSD_HOSTNAME), false, OptionsCategory::STATSD);
    gArgs.AddArg("-statsport=<port>", strprintf("Specify statsd port (default: %u)", DEFAULT_STATSD_PORT), false, OptionsCategory::STATSD);
    gArgs.AddArg("-statsns=<ns>", strprintf("Specify additional namespace prefix (default: %s)", DEFAULT_STATSD_NAMESPACE), false, OptionsCategory::STATSD);
    gArgs.AddArg("-statsperiod=<seconds>", strprintf("Specify the number of seconds between periodic measurements (default: %d)", DEFAULT_STATSD_PERIOD), false, OptionsCategory::STATSD);
#if HAVE_DECL_DAEMON
    gArgs.AddArg("-daemon", "Run in the background as a daemon and accept commands", false, OptionsCategory::OPTIONS);
#else
    hidden_args.emplace_back("-daemon");
#endif

    // Add the hidden options
    gArgs.AddHiddenArgs(hidden_args);
}

std::string LicenseInfo()
{
    const std::string URL_SOURCE_CODE = "<https://github.com/dashpay/dash>";
    const std::string URL_WEBSITE = "<https://dash.org>";

    return CopyrightHolders(_("Copyright (C)"), 2014, COPYRIGHT_YEAR) + "\n" +
           "\n" +
           strprintf(_("Please contribute if you find %s useful. "
                       "Visit %s for further information about the software."),
               PACKAGE_NAME, URL_WEBSITE) +
           "\n" +
           strprintf(_("The source code is available from %s."),
               URL_SOURCE_CODE) +
           "\n" +
           "\n" +
           _("This is experimental software.") + "\n" +
           strprintf(_("Distributed under the MIT software license, see the accompanying file %s or %s"), "COPYING", "<https://opensource.org/licenses/MIT>") + "\n" +
           "\n" +
           strprintf(_("This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit %s and cryptographic software written by Eric Young and UPnP software written by Thomas Bernard."), "<https://www.openssl.org>") +
           "\n";
}

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
            CDiskBlockPos pos(nFile, 0);
            if (!fs::exists(GetBlockPosFilename(pos, "blk")))
                break; // No block files left to reindex
            FILE *file = OpenBlockFile(pos, true);
            if (!file)
                break; // This error is logged in OpenBlockFile
            LogPrintf("Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
            LoadExternalBlockFile(chainparams, file, &pos);
            if (ShutdownRequested()) {
                LogPrintf("Shutdown requested. Exit %s\n", __func__);
                return;
            }
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
            if (ShutdownRequested()) {
                LogPrintf("Shutdown requested. Exit %s\n", __func__);
                return;
            }
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

    // force UpdatedBlockTip to initialize nCachedBlockHeight for DS, MN payments and budgets
    // but don't call it directly to prevent triggering of other listeners like zmq etc.
    // GetMainSignals().UpdatedBlockTip(chainActive.Tip());
    pdsNotificationInterface->InitializeCurrentBlockTip();

    {
        // Get all UTXOs for each MN collateral in one go so that we can fill coin cache early
        // and reduce further locking overhead for cs_main in other parts of code including GUI
        LogPrintf("Filling coin cache with masternode UTXOs...\n");
        LOCK(cs_main);
        int64_t nStart = GetTimeMillis();
        auto mnList = deterministicMNManager->GetListAtChainTip();
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            Coin coin;
            GetUTXOCoin(dmn->collateralOutpoint, coin);
        });
        LogPrintf("Filling coin cache with masternode UTXOs: done in %dms\n", GetTimeMillis() - nStart);
    }

    if (fMasternodeMode) {
        assert(activeMasternodeManager);
        const CBlockIndex* pindexTip;
        {
            LOCK(cs_main);
            pindexTip = chainActive.Tip();
        }
        activeMasternodeManager->Init(pindexTip);
    }

    g_wallet_init_interface.AutoLockMasternodeCollaterals();

    if (gArgs.GetArg("-persistmempool", DEFAULT_PERSIST_MEMPOOL)) {
        LoadMempool();
    }
    g_is_mempool_loaded = !ShutdownRequested();
}

void PeriodicStats()
{
    assert(gArgs.GetBoolArg("-statsenabled", DEFAULT_STATSD_ENABLE));
    CCoinsStats stats;
    FlushStateToDisk();
    if (GetUTXOStats(pcoinsdbview.get(), stats)) {
        statsClient.gauge("utxoset.tx", stats.nTransactions, 1.0f);
        statsClient.gauge("utxoset.txOutputs", stats.nTransactionOutputs, 1.0f);
        statsClient.gauge("utxoset.dbSizeBytes", stats.nDiskSize, 1.0f);
        statsClient.gauge("utxoset.blockHeight", stats.nHeight, 1.0f);
        statsClient.gauge("utxoset.totalAmount", (double)stats.nTotalAmount / (double)COIN, 1.0f);
    } else {
        // something went wrong
        LogPrintf("%s: GetUTXOStats failed\n", __func__);
    }

    // short version of GetNetworkHashPS(120, -1);
    CBlockIndex *tip;
    {
        LOCK(cs_main);
        tip = chainActive.Tip();
        assert(tip);
    }
    CBlockIndex *pindex = tip;
    int64_t minTime = pindex->GetBlockTime();
    int64_t maxTime = minTime;
    for (int i = 0; i < 120 && pindex->pprev != nullptr; i++) {
        pindex = pindex->pprev;
        int64_t time = pindex->GetBlockTime();
        minTime = std::min(time, minTime);
        maxTime = std::max(time, maxTime);
    }
    arith_uint256 workDiff = tip->nChainWork - pindex->nChainWork;
    int64_t timeDiff = maxTime - minTime;
    double nNetworkHashPS = workDiff.getdouble() / timeDiff;

    statsClient.gaugeDouble("network.hashesPerSecond", nNetworkHashPS);
    statsClient.gaugeDouble("network.terahashesPerSecond", nNetworkHashPS / 1e12);
    statsClient.gaugeDouble("network.petahashesPerSecond", nNetworkHashPS / 1e15);
    statsClient.gaugeDouble("network.exahashesPerSecond", nNetworkHashPS / 1e18);
    // No need for cs_main, we never use null tip here
    statsClient.gaugeDouble("network.difficulty", (double)GetDifficulty(tip));

    statsClient.gauge("transactions.txCacheSize", pcoinsTip->GetCacheSize(), 1.0f);
    statsClient.gauge("transactions.totalTransactions", tip->nChainTx, 1.0f);

    statsClient.gauge("transactions.mempool.totalTransactions", mempool.size(), 1.0f);
    statsClient.gauge("transactions.mempool.totalTxBytes", (int64_t) mempool.GetTotalTxSize(), 1.0f);
    statsClient.gauge("transactions.mempool.memoryUsageBytes", (int64_t) mempool.DynamicMemoryUsage(), 1.0f);
    statsClient.gauge("transactions.mempool.minFeePerKb", mempool.GetMinFee(gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000).GetFeePerK(), 1.0f);
}

/** Sanity checks
 *  Ensure that Dash Core is running in a usable environment with all
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

    if (!BLSInit()) {
        return false;
    }

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

    if (gArgs.GetBoolArg("-litemode", false)) {
        if (gArgs.SoftSetBoolArg("-disablegovernance", true)) {
            LogPrintf("%s: parameter interaction: -litemode=true -> setting -disablegovernance=true\n", __func__);
        }
    }

    int64_t nPruneArg = gArgs.GetArg("-prune", 0);
    if (nPruneArg > 0) {
        if (gArgs.SoftSetBoolArg("-disablegovernance", true)) {
            LogPrintf("%s: parameter interaction: -prune=%d -> setting -disablegovernance=true\n", __func__, nPruneArg);
        }
        if (gArgs.SoftSetBoolArg("-txindex", false)) {
            LogPrintf("%s: parameter interaction: -prune=%d -> setting -txindex=false\n", __func__, nPruneArg);
        }
    }

    // Make sure additional indexes are recalculated correctly in VerifyDB
    // (we must reconnect blocks whenever we disconnect them for these indexes to work)
    bool fAdditionalIndexes =
        gArgs.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX) ||
        gArgs.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX) ||
        gArgs.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX);

    if (fAdditionalIndexes && gArgs.GetArg("-checklevel", DEFAULT_CHECKLEVEL) < 4) {
        gArgs.ForceSetArg("-checklevel", "4");
        LogPrintf("%s: parameter interaction: additional indexes -> setting -checklevel=4\n", __func__);
    }

    if (gArgs.IsArgSet("-masternodeblsprivkey") && gArgs.SoftSetBoolArg("-disablewallet", true)) {
        LogPrintf("%s: parameter interaction: -masternodeblsprivkey set -> setting -disablewallet=1\n", __func__);
    }

    // Warn if network-specific options (-addnode, -connect, etc) are
    // specified in default section of config file, but not overridden
    // on the command line or in this network's section of the config file.
    gArgs.WarnForSectionOnlyArgs();
}

static std::string ResolveErrMsg(const char * const optname, const std::string& strBind)
{
    return strprintf(_("Cannot resolve -%s address: '%s'"), optname, strBind);
}

/**
 * Initialize global loggers.
 *
 * Note that this is called very early in the process lifetime, so you should be
 * careful about what global state you rely on here.
 */
void InitLogging()
{
    g_logger->m_print_to_file = !gArgs.IsArgNegated("-debuglogfile");
    g_logger->m_file_path = AbsPathForConfigVal(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    g_logger->m_print_to_console = gArgs.GetBoolArg("-printtoconsole", !gArgs.GetBoolArg("-daemon", false));
    g_logger->m_log_timestamps = gArgs.GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    g_logger->m_log_time_micros = gArgs.GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
    g_logger->m_log_threadnames = gArgs.GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);

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
    // Minimum supported OS versions: WinXP SP3, WinVista >= SP1, Win Server 2008
    // A failure is non-critical and needs no further attention!
#ifndef PROCESS_DEP_ENABLE
    // We define this here, because GCCs winbase.h limits this to _WIN32_WINNT >= 0x0601 (Windows 7),
    // which is not correct. Can be removed, when GCCs winbase.h is fixed!
#define PROCESS_DEP_ENABLE 0x00000001
#endif
    typedef BOOL (WINAPI *PSETPROCDEPPOL)(DWORD);
    PSETPROCDEPPOL setProcDEPPol = (PSETPROCDEPPOL)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetProcessDEPPolicy");
    if (setProcDEPPol != nullptr) setProcDEPPol(PROCESS_DEP_ENABLE);
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

    if (!fs::is_directory(GetBlocksDir())) {
        return InitError(strprintf(_("Specified blocks directory \"%s\" does not exist."), gArgs.GetArg("-blocksdir", "").c_str()));
    }

    // if using block pruning, then disallow txindex and require disabling governance validation
    if (gArgs.GetArg("-prune", 0)) {
        if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX))
            return InitError(_("Prune mode is incompatible with -txindex."));
        if (!gArgs.GetBoolArg("-disablegovernance", false)) {
            return InitError(_("Prune mode is incompatible with -disablegovernance=false."));
        }
    }

    if (gArgs.IsArgSet("-devnet")) {
        // Require setting of ports when running devnet
        if (gArgs.GetArg("-listen", DEFAULT_LISTEN) && !gArgs.IsArgSet("-port")) {
            return InitError(_("-port must be specified when -devnet and -listen are specified"));
        }
        if (gArgs.GetArg("-server", false) && !gArgs.IsArgSet("-rpcport")) {
            return InitError(_("-rpcport must be specified when -devnet and -server are specified"));
        }
        if (gArgs.GetArgs("-devnet").size() > 1) {
            return InitError(_("-devnet can only be specified once"));
        }
    }

    fAllowPrivateNet = gArgs.GetBoolArg("-allowprivatenet", DEFAULT_ALLOWPRIVATENET);

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
    nFD = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS + MAX_ADDNODE_CONNECTIONS + nBind);
#ifdef USE_POLL
    int fd_max = nFD;
#else
    int fd_max = FD_SETSIZE;
#endif
    nMaxConnections = std::max(std::min<int>(nMaxConnections, fd_max - nBind - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS), 0);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available."));
    nMaxConnections = std::min(nFD - MIN_CORE_FILEDESCRIPTORS - MAX_ADDNODE_CONNECTIONS, nMaxConnections);

    if (nMaxConnections < nUserMaxConnections)
        InitWarning(strprintf(_("Reducing -maxconnections from %d to %d, because of system limitations."), nUserMaxConnections, nMaxConnections));

    // ********************************************************* Step 3: parameter-to-internal-flags
    if (gArgs.IsArgSet("-debug")) {
        // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
        const std::vector<std::string> categories = gArgs.GetArgs("-debug");

        if (std::none_of(categories.begin(), categories.end(),
            [](std::string cat){return cat == "0" || cat == "none";})) {
            for (const auto& cat : categories) {
                if (!g_logger->EnableCategory(cat)) {
                    InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debug", cat));
                }
            }
        }
    }

    // Now remove the logging categories which were explicitly excluded
    for (const std::string& cat : gArgs.GetArgs("-debugexclude")) {
        if (!g_logger->DisableCategory(cat)) {
            InitWarning(strprintf(_("Unsupported logging category %s=%s."), "-debugexclude", cat));
        }
    }

    // Check for -debugnet
    if (gArgs.GetBoolArg("-debugnet", false))
        InitWarning(_("Unsupported argument -debugnet ignored, use -debug=net."));
    // Check for -socks - as this is a privacy risk to continue, exit here
    if (gArgs.IsArgSet("-socks"))
        return InitError(_("Unsupported argument -socks found. Setting SOCKS version isn't possible anymore, only SOCKS5 proxies are supported."));
    // Check for -tor - as this is a privacy risk to continue, exit here
    if (gArgs.GetBoolArg("-tor", false))
        return InitError(_("Unsupported argument -tor found, use -onion."));

    if (gArgs.GetBoolArg("-benchmark", false))
        InitWarning(_("Unsupported argument -benchmark ignored, use -debug=bench."));

    if (gArgs.GetBoolArg("-whitelistalwaysrelay", false))
        InitWarning(_("Unsupported argument -whitelistalwaysrelay ignored, use -whitelistrelay and/or -whitelistforcerelay."));

    if (gArgs.IsArgSet("-blockminsize"))
        InitWarning("Unsupported argument -blockminsize ignored.");

    // Checkmempool and checkblockindex default to true in regtest mode
    int ratio = std::min<int>(std::max<int>(gArgs.GetArg("-checkmempool", chainparams.DefaultConsistencyChecks() ? 1 : 0), 0), 1000000);
    if (ratio != 0) {
        mempool.setSanityCheck(1.0 / ratio);
    }
    fCheckBlockIndex = gArgs.GetBoolArg("-checkblockindex", chainparams.DefaultConsistencyChecks());
    fCheckpointsEnabled = gArgs.GetBoolArg("-checkpoints", DEFAULT_CHECKPOINTS_ENABLED);

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
        return InitError(strprintf(_("-maxmempool must be at least %d MB"), std::ceil(nMempoolSizeMin / 1000000.0)));
    // incremental relay fee sets the minimum feerate increase necessary for BIP 125 replacement in the mempool
    // and the amount the mempool min fee increases above the feerate of txs evicted due to mempool limiting.
    if (gArgs.IsArgSet("-incrementalrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-incrementalrelayfee", ""), n))
            return InitError(AmountErrMsg("incrementalrelayfee", gArgs.GetArg("-incrementalrelayfee", "")));
        incrementalRelayFee = CFeeRate(n);
    }

    // block pruning; get the amount of disk space (in MiB) to allot for block & undo files
    int64_t nPruneArg = gArgs.GetArg("-prune", 0);
    if (nPruneArg < 0) {
        return InitError(_("Prune cannot be configured with a negative value."));
    }
    nPruneTarget = (uint64_t) nPruneArg * 1024 * 1024;
    if (nPruneArg == 1) {  // manual pruning: -prune=1
        LogPrintf("Block pruning enabled.  Use RPC call pruneblockchain(height) to manually prune block and undo files.\n");
        nPruneTarget = std::numeric_limits<uint64_t>::max();
        fPruneMode = true;
    } else if (nPruneTarget) {
        if (gArgs.GetBoolArg("-regtest", false)) {
            // we use 1MB blocks to test this on regtest
            if (nPruneTarget < 550 * 1024 * 1024) {
                return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), 550));
            }
        } else {
            if (nPruneTarget < MIN_DISK_SPACE_FOR_BLOCK_FILES) {
                return InitError(strprintf(_("Prune configured below the minimum of %d MiB.  Please use a higher number."), MIN_DISK_SPACE_FOR_BLOCK_FILES / 1024 / 1024));
            }
        }
        LogPrintf("Prune configured to target %uMiB on disk for block and undo files.\n", nPruneTarget / 1024 / 1024);
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
            return InitError(AmountErrMsg("minrelaytxfee", gArgs.GetArg("-minrelaytxfee", "")));
        }
        // High fee check is done afterward in WalletParameterInteraction()
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
            return InitError(AmountErrMsg("blockmintxfee", gArgs.GetArg("-blockmintxfee", "")));
    }

    // Feerate used to define dust.  Shouldn't be changed lightly as old
    // implementations may inadvertently create non-standard transactions
    if (gArgs.IsArgSet("-dustrelayfee"))
    {
        CAmount n = 0;
        if (!ParseMoney(gArgs.GetArg("-dustrelayfee", ""), n))
            return InitError(AmountErrMsg("dustrelayfee", gArgs.GetArg("-dustrelayfee", "")));
        dustRelayFee = CFeeRate(n);
    }

    fRequireStandard = !gArgs.GetBoolArg("-acceptnonstdtxn", !chainparams.RequireStandard());
    if (chainparams.RequireStandard() && !fRequireStandard)
        return InitError(strprintf("acceptnonstdtxn is not currently supported for %s chain", chainparams.NetworkIDString()));
    nBytesPerSigOp = gArgs.GetArg("-bytespersigop", nBytesPerSigOp);

    if (!g_wallet_init_interface.ParameterInteraction()) return false;

    fIsBareMultisigStd = gArgs.GetBoolArg("-permitbaremultisig", DEFAULT_PERMIT_BAREMULTISIG);
    fAcceptDatacarrier = gArgs.GetBoolArg("-datacarrier", DEFAULT_ACCEPT_DATACARRIER);
    nMaxDatacarrierBytes = gArgs.GetArg("-datacarriersize", nMaxDatacarrierBytes);

    // Option to startup with mocktime set (used for regression testing):
    SetMockTime(gArgs.GetArg("-mocktime", 0)); // SetMockTime(0) is a no-op

    if (gArgs.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS))
        nLocalServices = ServiceFlags(nLocalServices | NODE_BLOOM);

    nMaxTipAge = gArgs.GetArg("-maxtipage", DEFAULT_MAX_TIP_AGE);

    if (gArgs.IsArgSet("-vbparams")) {
        // Allow overriding version bits parameters for testing
        if (!chainparams.MineBlocksOnDemand()) {
            return InitError("Version bits parameters may only be overridden on regtest.");
        }
        for (const std::string& strDeployment : gArgs.GetArgs("-vbparams")) {
            std::vector<std::string> vDeploymentParams;
            boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
            if (vDeploymentParams.size() != 3 && vDeploymentParams.size() != 5 && vDeploymentParams.size() != 7) {
                return InitError("Version bits parameters malformed, expecting deployment:start:end or deployment:start:end:window:threshold or deployment:start:end:window:thresholdstart:thresholdmin:falloffcoeff");
            }
            int64_t nStartTime, nTimeout, nWindowSize = -1, nThresholdStart = -1, nThresholdMin = -1, nFalloffCoeff = -1;
            if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
                return InitError(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
            }
            if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
                return InitError(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
            }
            if (vDeploymentParams.size() >= 5) {
                if (!ParseInt64(vDeploymentParams[3], &nWindowSize)) {
                    return InitError(strprintf("Invalid nWindowSize (%s)", vDeploymentParams[3]));
                }
                if (!ParseInt64(vDeploymentParams[4], &nThresholdStart)) {
                    return InitError(strprintf("Invalid nThresholdStart (%s)", vDeploymentParams[4]));
                }
            }
            if (vDeploymentParams.size() == 7) {
                if (!ParseInt64(vDeploymentParams[5], &nThresholdMin)) {
                    return InitError(strprintf("Invalid nThresholdMin (%s)", vDeploymentParams[5]));
                }
                if (!ParseInt64(vDeploymentParams[6], &nFalloffCoeff)) {
                    return InitError(strprintf("Invalid nFalloffCoeff (%s)", vDeploymentParams[6]));
                }
            }
            bool found = false;
            for (int j=0; j<(int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j)
            {
                if (vDeploymentParams[0].compare(VersionBitsDeploymentInfo[j].name) == 0) {
                    UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff);
                    found = true;
                    LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, window=%ld, thresholdstart=%ld, thresholdmin=%ld, falloffcoeff=%ld\n",
                            vDeploymentParams[0], nStartTime, nTimeout, nWindowSize, nThresholdStart, nThresholdMin, nFalloffCoeff);
                    break;
                }
            }
            if (!found) {
                return InitError(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
            }
        }
    }

    if (gArgs.IsArgSet("-dip3params")) {
        // Allow overriding dip3 parameters for testing
        if (!chainparams.MineBlocksOnDemand()) {
            return InitError("DIP3 parameters may only be overridden on regtest.");
        }
        std::string strDIP3Params = gArgs.GetArg("-dip3params", "");
        std::vector<std::string> vDIP3Params;
        boost::split(vDIP3Params, strDIP3Params, boost::is_any_of(":"));
        if (vDIP3Params.size() != 2) {
            return InitError("DIP3 parameters malformed, expecting DIP3ActivationHeight:DIP3EnforcementHeight");
        }
        int nDIP3ActivationHeight, nDIP3EnforcementHeight;
        if (!ParseInt32(vDIP3Params[0], &nDIP3ActivationHeight)) {
            return InitError(strprintf("Invalid nDIP3ActivationHeight (%s)", vDIP3Params[0]));
        }
        if (!ParseInt32(vDIP3Params[1], &nDIP3EnforcementHeight)) {
            return InitError(strprintf("Invalid nDIP3EnforcementHeight (%s)", vDIP3Params[1]));
        }
        UpdateDIP3Parameters(nDIP3ActivationHeight, nDIP3EnforcementHeight);
    }

    if (gArgs.IsArgSet("-dip8params")) {
        // Allow overriding dip8 activation height for testing
        if (!chainparams.MineBlocksOnDemand()) {
            return InitError("DIP8 activation height may only be overridden on regtest.");
        }
        UpdateDIP8Parameters(gArgs.GetArg("-dip8params", Params().GetConsensus().DIP0008Height));
    }

    if (gArgs.IsArgSet("-budgetparams")) {
        // Allow overriding budget parameters for testing
        if (!chainparams.MineBlocksOnDemand()) {
            return InitError("Budget parameters may only be overridden on regtest.");
        }

        std::string strBudgetParams = gArgs.GetArg("-budgetparams", "");
        std::vector<std::string> vBudgetParams;
        boost::split(vBudgetParams, strBudgetParams, boost::is_any_of(":"));
        if (vBudgetParams.size() != 3) {
            return InitError("Budget parameters malformed, expecting masternodePaymentsStartBlock:budgetPaymentsStartBlock:superblockStartBlock");
        }
        int nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock;
        if (!ParseInt32(vBudgetParams[0], &nMasternodePaymentsStartBlock)) {
            return InitError(strprintf("Invalid nMasternodePaymentsStartBlock (%s)", vBudgetParams[0]));
        }
        if (!ParseInt32(vBudgetParams[1], &nBudgetPaymentsStartBlock)) {
            return InitError(strprintf("Invalid nBudgetPaymentsStartBlock (%s)", vBudgetParams[1]));
        }
        if (!ParseInt32(vBudgetParams[2], &nSuperblockStartBlock)) {
            return InitError(strprintf("Invalid nSuperblockStartBlock (%s)", vBudgetParams[2]));
        }
        UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
    }

    if (chainparams.NetworkIDString() == CBaseChainParams::DEVNET) {
        int nMinimumDifficultyBlocks = gArgs.GetArg("-minimumdifficultyblocks", chainparams.GetConsensus().nMinimumDifficultyBlocks);
        int nHighSubsidyBlocks = gArgs.GetArg("-highsubsidyblocks", chainparams.GetConsensus().nHighSubsidyBlocks);
        int nHighSubsidyFactor = gArgs.GetArg("-highsubsidyfactor", chainparams.GetConsensus().nHighSubsidyFactor);
        UpdateDevnetSubsidyAndDiffParams(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
    } else if (gArgs.IsArgSet("-minimumdifficultyblocks") || gArgs.IsArgSet("-highsubsidyblocks") || gArgs.IsArgSet("-highsubsidyfactor")) {
        return InitError("Difficulty and subsidy parameters may only be overridden on devnet.");
    }

    if (chainparams.NetworkIDString() == CBaseChainParams::DEVNET) {
        std::string strLLMQTypeChainLocks = gArgs.GetArg("-llmqchainlocks", llmq::GetLLMQParams(Params().GetConsensus().llmqTypeChainLocks).name);
        std::string strLLMQTypeInstantSend = gArgs.GetArg("-llmqinstantsend", llmq::GetLLMQParams(Params().GetConsensus().llmqTypeInstantSend).name);
        Consensus::LLMQType llmqTypeChainLocks = Consensus::LLMQ_NONE;
        Consensus::LLMQType llmqTypeInstantSend = Consensus::LLMQ_NONE;
        for (const auto& p : Params().GetConsensus().llmqs) {
            if (p.second.name == strLLMQTypeChainLocks) {
                llmqTypeChainLocks = p.first;
            }
            if (p.second.name == strLLMQTypeInstantSend) {
                llmqTypeInstantSend = p.first;
            }
        }
        if (llmqTypeChainLocks == Consensus::LLMQ_NONE) {
            return InitError("Invalid LLMQ type specified for -llmqchainlocks.");
        }
        if (llmqTypeInstantSend == Consensus::LLMQ_NONE) {
            return InitError("Invalid LLMQ type specified for -llmqinstantsend.");
        }
        UpdateDevnetLLMQChainLocks(llmqTypeChainLocks);
        UpdateDevnetLLMQInstantSend(llmqTypeInstantSend);
    } else if (gArgs.IsArgSet("-llmqchainlocks")) {
        return InitError("LLMQ type for ChainLocks can only be overridden on devnet.");
    } else if (gArgs.IsArgSet("-llmqinstantsend")) {
        return InitError("LLMQ type for InstantSend can only be overridden on devnet.");
    }

    if (chainparams.NetworkIDString() == CBaseChainParams::DEVNET) {
        if (gArgs.IsArgSet("-llmqdevnetparams")) {
            std::string s = gArgs.GetArg("-llmqdevnetparams", "");
            std::vector<std::string> v;
            boost::split(v, s, boost::is_any_of(":"));
            int size, threshold;
            if (v.size() != 2 || !ParseInt32(v[0], &size) || !ParseInt32(v[1], &threshold)) {
                return InitError("Invalid -llmqdevnetparams specified");
            }
            UpdateLLMQDevnetParams(size, threshold);
        }
    } else if (gArgs.IsArgSet("-llmqdevnetparams")) {
        return InitError("LLMQ devnet params can only be overridden on devnet.");
    }

    if (chainparams.NetworkIDString() == CBaseChainParams::REGTEST) {
        if (gArgs.IsArgSet("-llmqtestparams")) {
            std::string s = gArgs.GetArg("-llmqtestparams", "");
            std::vector<std::string> v;
            boost::split(v, s, boost::is_any_of(":"));
            int size, threshold;
            if (v.size() != 2 || !ParseInt32(v[0], &size) || !ParseInt32(v[1], &threshold)) {
                return InitError("Invalid -llmqtestparams specified");
            }
            UpdateLLMQTestParams(size, threshold);
        }
    } else if (gArgs.IsArgSet("-llmqtestparams")) {
        return InitError("LLMQ test params can only be overridden on regtest.");
    }

    try {
        const bool fRecoveryEnabled{llmq::CLLMQUtils::QuorumDataRecoveryEnabled()};
        const bool fQuorumVvecRequestsEnabled{llmq::CLLMQUtils::GetEnabledQuorumVvecSyncEntries().size() > 0};
        if (!fRecoveryEnabled && fQuorumVvecRequestsEnabled) {
            InitWarning("-llmq-qvvec-sync set but recovery is disabled due to -llmq-data-recovery=0");
        }
    } catch (const std::invalid_argument& e) {
        return InitError(e.what());
    }

    if (gArgs.IsArgSet("-maxorphantx")) {
        InitWarning("-maxorphantx is not supported anymore. Use -maxorphantxsize instead.");
    }

    if (gArgs.IsArgSet("-masternode")) {
        InitWarning(_("-masternode option is deprecated and ignored, specifying -masternodeblsprivkey is enough to start this node as a masternode."));
    }

    if (gArgs.IsArgSet("-masternodeblsprivkey")) {
        if (!gArgs.GetBoolArg("-listen", DEFAULT_LISTEN) && Params().RequireRoutableExternalIP()) {
            return InitError("Masternode must accept connections from outside, set -listen=1");
        }
        if (!gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
            return InitError("Masternode must have transaction index enabled, set -txindex=1");
        }
        if (!gArgs.GetBoolArg("-peerbloomfilters", DEFAULT_PEERBLOOMFILTERS)) {
            return InitError("Masternode must have bloom filters enabled, set -peerbloomfilters=1");
        }
        if (gArgs.GetArg("-prune", 0) > 0) {
            return InitError("Masternode must have no pruning enabled, set -prune=0");
        }
        if (gArgs.GetArg("-maxconnections", DEFAULT_MAX_PEER_CONNECTIONS) < DEFAULT_MAX_PEER_CONNECTIONS) {
            return InitError(strprintf("Masternode must be able to handle at least %d connections, set -maxconnections=%d", DEFAULT_MAX_PEER_CONNECTIONS, DEFAULT_MAX_PEER_CONNECTIONS));
        }
        if (gArgs.GetBoolArg("-disablegovernance", false)) {
            return InitError(_("You can not disable governance validation on a masternode."));
        }
    }

    if (gArgs.IsArgSet("-litemode")) {
        InitWarning(_("-litemode is deprecated.") + (gArgs.GetBoolArg("-litemode", false) ? (" "  + _("Its replacement -disablegovernance has been forced instead.")) : ( " " + _("It has been replaced by -disablegovernance."))));
        gArgs.ForceRemoveArg("-litemode");
    }

    fDisableGovernance = gArgs.GetBoolArg("-disablegovernance", false);
    LogPrintf("fDisableGovernance %d\n", fDisableGovernance);

    if (fDisableGovernance) {
        InitWarning(_("You are starting with governance validation disabled.") + (fPruneMode ? " " + _("This is expected because you are running a pruned node.") : ""));
    }

    return true;
}

static bool LockDataDirectory(bool probeOnly)
{
    // Make sure only a single Dash Core process is using the data directory.
    fs::path datadir = GetDataDir();
    if (!DirIsWritable(datadir)) {
        return InitError(strprintf(_("Cannot write to data directory '%s'; check permissions."), datadir.string()));
    }
    if (!LockDirectory(datadir, ".lock", probeOnly)) {
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. %s is probably already running."), datadir.string(), _(PACKAGE_NAME)));
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
        return InitError(strprintf(_("Initialization sanity check failed. %s is shutting down."), _(PACKAGE_NAME)));

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

bool AppInitMain()
{
    const CChainParams& chainparams = Params();
    // ********************************************************* Step 4a: application initialization
#ifndef WIN32
    CreatePidFile(GetPidFile(), getpid());
#endif
    if (g_logger->m_print_to_file) {
        if (gArgs.GetBoolArg("-shrinkdebugfile", g_logger->DefaultShrinkDebugFile())) {
            // Do this first since it both loads a bunch of debug.log into memory,
            // and because this needs to happen before any other debug.log printing
            g_logger->ShrinkDebugFile();
        }
    }
    if (!g_logger->StartLogging()) {
        return InitError(strprintf("Could not open debug log file %s",
                                   g_logger->m_file_path.string()));
    }

    if (!g_logger->m_log_timestamps)
        LogPrintf("Startup time: %s\n", FormatISO8601DateTime(GetTime()));
    LogPrintf("Default data directory %s\n", GetDefaultDataDir().string());
    LogPrintf("Using data directory %s\n", GetDataDir().string());

    // Only log conf file usage message if conf file actually exists.
    fs::path config_file_path = GetConfigFile(gArgs.GetArg("-conf", BITCOIN_CONF_FILENAME));
    if (fs::exists(config_file_path)) {
        LogPrintf("Config file: %s\n", config_file_path.string());
    } else if (gArgs.IsArgSet("-conf")) {
        // Warn if no conf file exists at path provided by user
        InitWarning(strprintf(_("The specified config file %s does not exist\n"), config_file_path.string()));
    } else {
        // Not categorizing as "Warning" because it's the default behavior
        LogPrintf("Config file: %s (not found, skipping)\n", config_file_path.string());
    }

    LogPrintf("Using at most %i automatic connections (%i file descriptors available)\n", nMaxConnections, nFD);

    // Warn about relative -datadir path.
    if (gArgs.IsArgSet("-datadir") && !fs::path(gArgs.GetArg("-datadir", "")).is_absolute()) {
        LogPrintf("Warning: relative datadir option '%s' specified, which will be interpreted relative to the " /* Continued */
                  "current working directory '%s'. This is fragile, because if Dash Core is started in the future "
                  "from a different location, it will be unable to locate the current data files. There could "
                  "also be data loss if Dash Core is started while in a temporary directory.\n",
            gArgs.GetArg("-datadir", ""), fs::current_path().string());
    }

    InitSignatureCache();
    InitScriptExecutionCache();

    int script_threads = gArgs.GetArg("-par", DEFAULT_SCRIPTCHECK_THREADS);
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
        g_parallel_script_checks = true;
        StartScriptCheckWorkerThreads(script_threads);
    }

    std::vector<std::string> vSporkAddresses;
    if (gArgs.IsArgSet("-sporkaddr")) {
        vSporkAddresses = gArgs.GetArgs("-sporkaddr");
    } else {
        vSporkAddresses = Params().SporkAddresses();
    }
    for (const auto& address: vSporkAddresses) {
        if (!sporkManager.SetSporkAddress(address)) {
            return InitError(_("Invalid spork address specified with -sporkaddr"));
        }
    }

    int minsporkkeys = gArgs.GetArg("-minsporkkeys", Params().MinSporkKeys());
    if (!sporkManager.SetMinSporkKeys(minsporkkeys)) {
        return InitError(_("Invalid minimum number of spork signers specified with -minsporkkeys"));
    }


    if (gArgs.IsArgSet("-sporkkey")) { // spork priv key
        if (!sporkManager.SetPrivKey(gArgs.GetArg("-sporkkey", ""))) {
            return InitError(_("Unable to sign spork message, wrong key?"));
        }
    }

    // Start the lightweight task scheduler thread
    CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, &scheduler);
    threadGroup.create_thread(boost::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));

    GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);
    GetMainSignals().RegisterWithMempoolSignals(mempool);

    tableRPC.InitPlatformRestrictions();

    /* Register RPC commands regardless of -server setting so they will be
     * available in the GUI RPC console even if external calls are disabled.
     */
    RegisterAllCoreRPCCommands(tableRPC);
    g_wallet_init_interface.RegisterRPC(tableRPC);
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
            return InitError(_("Unable to start HTTP server. See debug log for details."));
    }

    // ********************************************************* Step 5: verify wallet database integrity

    if (!g_wallet_init_interface.InitAutoBackup()) return false;
    if (!g_wallet_init_interface.Verify()) return false;

    // Initialize KeePass Integration
    g_wallet_init_interface.InitKeePass();
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

    if (chainparams.NetworkIDString() == CBaseChainParams::DEVNET) {
        // Add devnet name to user agent. This allows to disconnect nodes immediately if they don't belong to our own devnet
        uacomments.push_back(strprintf("devnet.%s", gArgs.GetDevNetName()));
    }

    for (const std::string& cmt : gArgs.GetArgs("-uacomment")) {
        if (cmt != SanitizeString(cmt, SAFE_CHARS_UA_COMMENT))
            return InitError(strprintf(_("User Agent comment (%s) contains unsafe characters."), cmt));
        uacomments.push_back(cmt);
    }
    strSubVersion = FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, uacomments);
    if (strSubVersion.size() > MAX_SUBVERSION_LENGTH) {
        return InitError(strprintf(_("Total length of network version string (%i) exceeds maximum length (%i). Reduce the number or size of uacomments."),
            strSubVersion.size(), MAX_SUBVERSION_LENGTH));
    }

    if (gArgs.IsArgSet("-onlynet")) {
        std::set<enum Network> nets;
        for (const std::string& snet : gArgs.GetArgs("-onlynet")) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(strprintf(_("Unknown network specified in -onlynet: '%s'"), snet));
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
            return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'"), proxyArg));
        }

        proxyType addrProxy = proxyType(proxyAddr, proxyRandomize);
        if (!addrProxy.IsValid())
            return InitError(strprintf(_("Invalid -proxy address or hostname: '%s'"), proxyArg));

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
                return InitError(strprintf(_("Invalid -onion address or hostname: '%s'"), onionArg));
            }
            proxyType addrOnion = proxyType(onionProxy, proxyRandomize);
            if (!addrOnion.IsValid())
                return InitError(strprintf(_("Invalid -onion address or hostname: '%s'"), onionArg));
            SetProxy(NET_ONION, addrOnion);
            SetReachable(NET_ONION, true);
        }
    }

    // see Step 2: parameter interactions for more information about these
    fListen = gArgs.GetBoolArg("-listen", DEFAULT_LISTEN);
    fDiscover = gArgs.GetBoolArg("-discover", true);
    fRelayTxes = !gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY);

    for (const std::string& strAddr : gArgs.GetArgs("-externalip")) {
        CService addrLocal;
        if (Lookup(strAddr.c_str(), addrLocal, GetListenPort(), fNameLookup) && addrLocal.IsValid())
            AddLocal(addrLocal, LOCAL_MANUAL);
        else
            return InitError(ResolveErrMsg("externalip", strAddr));
    }

    // Read asmap file if configured
    if (gArgs.IsArgSet("-asmap")) {
        fs::path asmap_path = fs::path(gArgs.GetArg("-asmap", ""));
        if (asmap_path.empty()) {
            asmap_path = DEFAULT_ASMAP_FILENAME;
        }
        if (!asmap_path.is_absolute()) {
            asmap_path = GetDataDir() / asmap_path;
        }
        if (!fs::exists(asmap_path)) {
            InitError(strprintf(_("Could not find asmap file %s"), asmap_path));
            return false;
        }
        std::vector<bool> asmap = CAddrMan::DecodeAsmap(asmap_path);
        if (asmap.size() == 0) {
            InitError(strprintf(_("Could not parse asmap file %s"), asmap_path));
            return false;
        }
        const uint256 asmap_version = SerializeHash(asmap);
        g_connman->SetAsmap(std::move(asmap));
        LogPrintf("Using asmap version %s for IP bucketing\n", asmap_version.ToString());
    } else {
        LogPrintf("Using /16 prefix for IP bucketing\n");
    }

#if ENABLE_ZMQ
    g_zmq_notification_interface = CZMQNotificationInterface::Create();

    if (g_zmq_notification_interface) {
        RegisterValidationInterface(g_zmq_notification_interface);
    }
#endif

    pdsNotificationInterface = new CDSNotificationInterface(*g_connman);
    RegisterValidationInterface(pdsNotificationInterface);

    uint64_t nMaxOutboundLimit = 0; //unlimited unless -maxuploadtarget is set
    uint64_t nMaxOutboundTimeframe = MAX_UPLOAD_TIMEFRAME;

    if (gArgs.IsArgSet("-maxuploadtarget")) {
        nMaxOutboundLimit = gArgs.GetArg("-maxuploadtarget", DEFAULT_MAX_UPLOAD_TARGET)*1024*1024;
    }

    // ********************************************************* Step 7a: Load sporks

    uiInterface.InitMessage(_("Loading sporks cache..."));
    CFlatDB<CSporkManager> flatdb6("sporks.dat", "magicSporkCache");
    if (!flatdb6.Load(sporkManager)) {
        return InitError(_("Failed to load sporks cache from") + "\n" + (GetDataDir() / "sporks.dat").string());
    }

    // ********************************************************* Step 7b: load block chain

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
    int64_t nCoinDBCache = std::min(nTotalCache / 2, (nTotalCache / 4) + (1 << 23)); // use 25%-50% of the remainder for disk cache
    nCoinDBCache = std::min(nCoinDBCache, nMaxCoinsDBCache << 20); // cap total coins db cache
    nTotalCache -= nCoinDBCache;
    nCoinCacheUsage = nTotalCache; // the rest goes to in-memory cache
    int64_t nMempoolSizeMax = gArgs.GetArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000;
    int64_t nEvoDbCache = 1024 * 1024 * 16; // TODO
    LogPrintf("Cache configuration:\n");
    LogPrintf("* Using %.1fMiB for block index database\n", nBlockTreeDBCache * (1.0 / 1024 / 1024));
    if (gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)) {
        LogPrintf("* Using %.1fMiB for transaction index database\n", nTxIndexCache * (1.0 / 1024 / 1024));
    }
    LogPrintf("* Using %.1fMiB for chain state database\n", nCoinDBCache * (1.0 / 1024 / 1024));
    LogPrintf("* Using %.1fMiB for in-memory UTXO set (plus up to %.1fMiB of unused mempool space)\n", nCoinCacheUsage * (1.0 / 1024 / 1024), nMempoolSizeMax * (1.0 / 1024 / 1024));

    bool fLoaded = false;

    while (!fLoaded && !ShutdownRequested()) {
        bool fReset = fReindex;
        std::string strLoadError;

        uiInterface.InitMessage(_("Loading block index..."));

        LOCK(cs_main);

        do {
            const int64_t load_block_index_start_time = GetTimeMillis();
            try {
                UnloadBlockIndex();
                pcoinsTip.reset();
                pcoinsdbview.reset();
                pcoinscatcher.reset();
                // new CBlockTreeDB tries to delete the existing file, which
                // fails if it's still open from the previous loop. Close it first:
                pblocktree.reset();
                pblocktree.reset(new CBlockTreeDB(nBlockTreeDBCache, false, fReset));
                llmq::DestroyLLMQSystem();
                // Same logic as above with pblocktree
                evoDb.reset();
                evoDb.reset(new CEvoDB(nEvoDbCache, false, fReset || fReindexChainState));
                deterministicMNManager.reset();
                deterministicMNManager.reset(new CDeterministicMNManager(*evoDb));

                llmq::InitLLMQSystem(*evoDb, false, fReset || fReindexChainState);

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
                    strLoadError = _("Error loading block database");
                    break;
                }

                if (!fDisableGovernance && !gArgs.GetBoolArg("-txindex", DEFAULT_TXINDEX)
                   && chainparams.NetworkIDString() != CBaseChainParams::REGTEST) { // TODO remove this when pruning is fixed. See https://github.com/dashpay/dash/pull/1817 and https://github.com/dashpay/dash/pull/1743
                    return InitError(_("Transaction index can't be disabled with governance validation enabled. Either start with -disablegovernance command line switch or enable transaction index."));
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!mapBlockIndex.empty() && !LookupBlockIndex(chainparams.GetConsensus().hashGenesisBlock)) {
                    return InitError(_("Incorrect or no genesis block found. Wrong datadir for network?"));
                }

                if (!chainparams.GetConsensus().hashDevnetGenesisBlock.IsNull() && !mapBlockIndex.empty() && mapBlockIndex.count(chainparams.GetConsensus().hashDevnetGenesisBlock) == 0)
                    return InitError(_("Incorrect or no devnet genesis block found. Wrong datadir for devnet specified?"));

                // Check for changed -addressindex state
                if (fAddressIndex != gArgs.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to change -addressindex");
                    break;
                }

                // Check for changed -timestampindex state
                if (fTimestampIndex != gArgs.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to change -timestampindex");
                    break;
                }

                // Check for changed -spentindex state
                if (fSpentIndex != gArgs.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX)) {
                    strLoadError = _("You need to rebuild the database using -reindex to change -spentindex");
                    break;
                }

                // Check for changed -prune state.  What we are concerned about is a user who has pruned blocks
                // in the past, but is now trying to run unpruned.
                if (fHavePruned && !fPruneMode) {
                    strLoadError = _("You need to rebuild the database using -reindex to go back to unpruned mode.  This will redownload the entire blockchain");
                    break;
                }

                // At this point blocktree args are consistent with what's on disk.
                // If we're not mid-reindex (based on disk + args), add a genesis block on disk
                // (otherwise we use the one already on disk).
                // This is called again in ThreadImport after the reindex completes.
                if (!fReindex && !LoadGenesisBlock(chainparams)) {
                    strLoadError = _("Error initializing block database");
                    break;
                }

                // At this point we're either in reindex or we've loaded a useful
                // block tree into mapBlockIndex!

                pcoinsdbview.reset(new CCoinsViewDB(nCoinDBCache, false, fReset || fReindexChainState));
                pcoinscatcher.reset(new CCoinsViewErrorCatcher(pcoinsdbview.get()));

                // If necessary, upgrade from older database format.
                // This is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!pcoinsdbview->Upgrade()) {
                    strLoadError = _("Error upgrading chainstate database");
                    break;
                }

                // ReplayBlocks is a no-op if we cleared the coinsviewdb with -reindex or -reindex-chainstate
                if (!ReplayBlocks(chainparams, pcoinsdbview.get())) {
                    strLoadError = _("Unable to replay blocks. You will need to rebuild the database using -reindex-chainstate.");
                    break;
                }

                // The on-disk coinsdb is now in a good state, create the cache
                pcoinsTip.reset(new CCoinsViewCache(pcoinscatcher.get()));

                // flush evodb
                if (!evoDb->CommitRootTransaction()) {
                    strLoadError = _("Failed to commit EvoDB");
                    break;
                }

                bool is_coinsview_empty = fReset || fReindexChainState || pcoinsTip->GetBestBlock().IsNull();
                if (!is_coinsview_empty) {
                    // LoadChainTip sets chainActive based on pcoinsTip's best block
                    if (!LoadChainTip(chainparams)) {
                        strLoadError = _("Error initializing block database");
                        break;
                    }
                    assert(chainActive.Tip() != NULL);
                }

                if (is_coinsview_empty && !evoDb->IsEmpty()) {
                    // EvoDB processed some blocks earlier but we have no blocks anymore, something is wrong
                    strLoadError = _("Error initializing block database");
                    break;
                }

                if (!deterministicMNManager->UpgradeDBIfNeeded() || !llmq::quorumBlockProcessor->UpgradeDB()) {
                    strLoadError = _("Error upgrading evo database");
                    break;
                }

                if (!is_coinsview_empty) {
                    uiInterface.InitMessage(_("Verifying blocks..."));
                    if (fHavePruned && gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS) > MIN_BLOCKS_TO_KEEP) {
                        LogPrintf("Prune: pruned datadir may not have more than %d blocks; only checking available blocks\n",
                            MIN_BLOCKS_TO_KEEP);
                    }

                    CBlockIndex* tip = chainActive.Tip();
                    RPCNotifyBlockChange(true, tip);
                    if (tip && tip->nTime > GetAdjustedTime() + 2 * 60 * 60) {
                        strLoadError = _("The block database contains a block which appears to be from the future. "
                                "This may be due to your computer's date and time being set incorrectly. "
                                "Only rebuild the block database if you are sure that your computer's date and time are correct");
                        break;
                    }

                    if (!CVerifyDB().VerifyDB(chainparams, pcoinsdbview.get(), gArgs.GetArg("-checklevel", DEFAULT_CHECKLEVEL),
                                  gArgs.GetArg("-checkblocks", DEFAULT_CHECKBLOCKS))) {
                        strLoadError = _("Corrupted block database detected");
                        break;
                    }

                    ResetBlockFailureFlags(nullptr);
                }
            } catch (const std::exception& e) {
                LogPrintf("%s\n", e.what());
                strLoadError = _("Error opening block database");
                break;
            }

            fLoaded = true;
            LogPrintf(" block index %15dms\n", GetTimeMillis() - load_block_index_start_time);
        } while(false);

        if (!fLoaded && !ShutdownRequested()) {
            // first suggest a reindex
            if (!fReset) {
                bool fRet = uiInterface.ThreadSafeQuestion(
                    strLoadError + ".\n\n" + _("Do you want to rebuild the block database now?"),
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

    // ********************************************************* Step 9: load wallet
    if (!g_wallet_init_interface.Open()) return false;

    // As InitLoadWallet can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    if (ShutdownRequested())
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }
    // ********************************************************* Step 10: data directory maintenance


    // if pruning, unset the service bit and perform the initial blockstore prune
    // after any wallet rescanning has taken place.
    if (fPruneMode) {
        LogPrintf("Unsetting NODE_NETWORK on prune mode\n");
        nLocalServices = ServiceFlags(nLocalServices & ~NODE_NETWORK);
        if (!fReindex) {
            uiInterface.InitMessage(_("Pruning blockstore..."));
            PruneAndFlush();
        }
    }

    // As PruneAndFlush can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    if (ShutdownRequested())
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    // ********************************************************* Step 10a: Prepare Masternode related stuff
    fMasternodeMode = false;
    std::string strMasterNodeBLSPrivKey = gArgs.GetArg("-masternodeblsprivkey", "");
    if (!strMasterNodeBLSPrivKey.empty()) {
        auto binKey = ParseHex(strMasterNodeBLSPrivKey);
        CBLSSecretKey keyOperator(binKey);
        if (!keyOperator.IsValid()) {
            return InitError(_("Invalid masternodeblsprivkey. Please see documentation."));
        }
        fMasternodeMode = true;
        activeMasternodeInfo.blsKeyOperator = std::make_unique<CBLSSecretKey>(keyOperator);
        activeMasternodeInfo.blsPubKeyOperator = std::make_unique<CBLSPublicKey>(activeMasternodeInfo.blsKeyOperator->GetPublicKey());
        LogPrintf("MASTERNODE:\n");
        LogPrintf("  blsPubKeyOperator: %s\n", keyOperator.GetPublicKey().ToString());
    }

    if(fMasternodeMode) {
        // Create and register activeMasternodeManager, will init later in ThreadImport
        activeMasternodeManager = new CActiveMasternodeManager();
        RegisterValidationInterface(activeMasternodeManager);
    }

    if (activeMasternodeInfo.blsKeyOperator == nullptr) {
        activeMasternodeInfo.blsKeyOperator = std::make_unique<CBLSSecretKey>();
    }
    if (activeMasternodeInfo.blsPubKeyOperator == nullptr) {
        activeMasternodeInfo.blsPubKeyOperator = std::make_unique<CBLSPublicKey>();
    }

    // ********************************************************* Step 10b: setup CoinJoin

    g_wallet_init_interface.InitCoinJoinSettings();
    CCoinJoin::InitStandardDenominations();

    // ********************************************************* Step 10b: Load cache data

    // LOAD SERIALIZED DAT FILES INTO DATA CACHES FOR INTERNAL USE

    bool fLoadCacheFiles = !(fReindex || fReindexChainState);
    {
        LOCK(cs_main);
        // was blocks/chainstate deleted?
        if (chainActive.Tip() == nullptr) {
            fLoadCacheFiles = false;
        }
    }
    fs::path pathDB = GetDataDir();
    std::string strDBName;

    strDBName = "mncache.dat";
    uiInterface.InitMessage(_("Loading masternode cache..."));
    CFlatDB<CMasternodeMetaMan> flatdb1(strDBName, "magicMasternodeCache");
    if (fLoadCacheFiles) {
        if(!flatdb1.Load(mmetaman)) {
            return InitError(_("Failed to load masternode cache from") + "\n" + (pathDB / strDBName).string());
        }
    } else {
        CMasternodeMetaMan mmetamanTmp;
        if(!flatdb1.Dump(mmetamanTmp)) {
            return InitError(_("Failed to clear masternode cache at") + "\n" + (pathDB / strDBName).string());
        }
    }

    strDBName = "governance.dat";
    uiInterface.InitMessage(_("Loading governance cache..."));
    CFlatDB<CGovernanceManager> flatdb3(strDBName, "magicGovernanceCache");
    if (fLoadCacheFiles && !fDisableGovernance) {
        if(!flatdb3.Load(governance)) {
            return InitError(_("Failed to load governance cache from") + "\n" + (pathDB / strDBName).string());
        }
        governance.InitOnLoad();
    } else {
        CGovernanceManager governanceTmp;
        if(!flatdb3.Dump(governanceTmp)) {
            return InitError(_("Failed to clear governance cache at") + "\n" + (pathDB / strDBName).string());
        }
    }

    strDBName = "netfulfilled.dat";
    uiInterface.InitMessage(_("Loading fulfilled requests cache..."));
    CFlatDB<CNetFulfilledRequestManager> flatdb4(strDBName, "magicFulfilledCache");
    if (fLoadCacheFiles) {
        if(!flatdb4.Load(netfulfilledman)) {
            return InitError(_("Failed to load fulfilled requests cache from") + "\n" + (pathDB / strDBName).string());
        }
    } else {
        CNetFulfilledRequestManager netfulfilledmanTmp;
        if(!flatdb4.Dump(netfulfilledmanTmp)) {
            return InitError(_("Failed to clear fulfilled requests cache at") + "\n" + (pathDB / strDBName).string());
        }
    }

    // ********************************************************* Step 10c: schedule Dash-specific tasks

    scheduler.scheduleEvery(std::bind(&CNetFulfilledRequestManager::DoMaintenance, std::ref(netfulfilledman)), 60 * 1000);
    scheduler.scheduleEvery(std::bind(&CMasternodeSync::DoMaintenance, std::ref(masternodeSync), std::ref(*g_connman)), 1 * 1000);
    scheduler.scheduleEvery(std::bind(&CMasternodeUtils::DoMaintenance, std::ref(*g_connman)), 1 * 1000);

    if (!fDisableGovernance) {
        scheduler.scheduleEvery(std::bind(&CGovernanceManager::DoMaintenance, std::ref(governance), std::ref(*g_connman)), 60 * 5 * 1000);
    }

    if (fMasternodeMode) {
        scheduler.scheduleEvery(std::bind(&CCoinJoinServer::DoMaintenance, std::ref(coinJoinServer), std::ref(*g_connman)), 1 * 1000);
    }

    if (gArgs.GetBoolArg("-statsenabled", DEFAULT_STATSD_ENABLE)) {
        int nStatsPeriod = std::min(std::max((int)gArgs.GetArg("-statsperiod", DEFAULT_STATSD_PERIOD), MIN_STATSD_PERIOD), MAX_STATSD_PERIOD);
        scheduler.scheduleEvery(PeriodicStats, nStatsPeriod * 1000);
    }

    llmq::StartLLMQSystem();

    // ********************************************************* Step 11: import blocks

    if (!CheckDiskSpace(/* additional_bytes */ 0, /* blocks_dir */ false)) {
        InitError(strprintf(_("Error: Disk space is low for %s"), GetDataDir()));
        return false;
    }
    if (!CheckDiskSpace(/* additional_bytes */ 0, /* blocks_dir */ true)) {
        InitError(strprintf(_("Error: Disk space is low for %s"), GetBlocksDir()));
        return false;
    }

    // Either install a handler to notify us when genesis activates, or set fHaveGenesis directly.
    // No locking, as this happens before any background thread is started.
    if (chainActive.Tip() == nullptr) {
        uiInterface.NotifyBlockTip_connect(BlockNotifyGenesisWait);
    } else {
        fHaveGenesis = true;
    }

    if (gArgs.IsArgSet("-blocknotify"))
        uiInterface.NotifyBlockTip_connect(BlockNotifyCallback);

    std::vector<fs::path> vImportFiles;
    for (const std::string& strFile : gArgs.GetArgs("-loadblock")) {
        vImportFiles.push_back(strFile);
    }

    threadGroup.create_thread(boost::bind(&ThreadImport, vImportFiles));

    // Wait for genesis block to be processed
    {
        WAIT_LOCK(g_genesis_wait_mutex, lock);
        // We previously could hang here if StartShutdown() is called prior to
        // ThreadImport getting started, so instead we just wait on a timer to
        // check ShutdownRequested() regularly.
        while (!fHaveGenesis && !ShutdownRequested()) {
            g_genesis_wait_cv.wait_for(lock, std::chrono::milliseconds(500));
        }
        uiInterface.NotifyBlockTip_disconnect(BlockNotifyGenesisWait);
    }

    // As importing blocks can take several minutes, it's possible the user
    // requested to kill the GUI during one of the last operations. If so, exit.
    if (ShutdownRequested()) {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }

    // ********************************************************* Step 12: start node

    int chain_active_height;

    //// debug print
    {
        LOCK(cs_main);
        LogPrintf("mapBlockIndex.size() = %u\n", mapBlockIndex.size());
        chain_active_height = chainActive.Height();
    }
    LogPrintf("chainActive.Height() = %d\n",   chain_active_height);
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
    connOptions.nMaxOutbound = std::min(MAX_OUTBOUND_CONNECTIONS, connOptions.nMaxConnections);
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

    std::string strSocketEventsMode = gArgs.GetArg("-socketevents", DEFAULT_SOCKETEVENTS);
    if (strSocketEventsMode == "select") {
        connOptions.socketEventsMode = CConnman::SOCKETEVENTS_SELECT;
#ifdef USE_POLL
    } else if (strSocketEventsMode == "poll") {
        connOptions.socketEventsMode = CConnman::SOCKETEVENTS_POLL;
#endif
#ifdef USE_EPOLL
    } else if (strSocketEventsMode == "epoll") {
        connOptions.socketEventsMode = CConnman::SOCKETEVENTS_EPOLL;
#endif
#ifdef USE_KQUEUE
    } else if (strSocketEventsMode == "kqueue") {
        connOptions.socketEventsMode = CConnman::SOCKETEVENTS_KQUEUE;
#endif
    } else {
        return InitError(strprintf(_("Invalid -socketevents ('%s') specified. Only these modes are supported: %s"), strSocketEventsMode, GetSupportedSocketEventsStr()));
    }

    if (!g_connman->Start(scheduler, connOptions)) {
        return false;
    }

    // ********************************************************* Step 13: finished

    SetRPCWarmupFinished();
    uiInterface.InitMessage(_("Done loading"));

    g_wallet_init_interface.Start(scheduler);

    scheduler.scheduleEvery([]{
        g_banman->DumpBanlist();
    }, DUMP_BANS_INTERVAL * 1000);

    return true;
}
