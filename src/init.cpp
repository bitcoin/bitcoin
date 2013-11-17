// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h"
#endif

#include "init.h"

#include "addrman.h"
#include "bitcointime.h"
#include "checkpoints.h"
#include "log.h"
#include "miner.h"
#include "net.h"
#include "rpcserver.h"
#include "txdb.h"
#include "ui_interface.h"
#include "util.h"
#include "wallet.h"
#include "walletdb.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdint.h>

#ifndef WIN32
#include <signal.h>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <openssl/crypto.h>

using namespace std;
using namespace boost;

string strWalletFile;
CWallet* pwalletMain;

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files, don't count towards to fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

// Used to pass flags to the Bind() function
enum BindFlags {
    BF_NONE         = 0,
    BF_EXPLICIT     = (1U << 0),
    BF_REPORT_ERROR = (1U << 1)
};


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
// signal handler sets fRequestShutdown, which triggers
// the DetectShutdownThread(), which interrupts the main thread group.
// DetectShutdownThread() then exits, which causes AppInit() to
// continue (it .joins the shutdown thread).
// Shutdown() is then
// called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//
// Note that if running -daemon the parent process returns from AppInit2
// before adding any threads to the threadGroup, so .join_all() returns
// immediately and the parent exits from main().
//
// Shutdown for Qt is very similar, only it uses a QTimer to detect
// fRequestShutdown getting set, and then does the normal Qt
// shutdown thing.
//

volatile bool fRequestShutdown = false;

void StartShutdown()
{
    fRequestShutdown = true;
}
bool ShutdownRequested()
{
    return fRequestShutdown;
}

static CCoinsViewDB *pcoinsdbview;

void Shutdown()
{
    Log() << "Shutdown : In progress...\n";
    static CCriticalSection cs_Shutdown;
    TRY_LOCK(cs_Shutdown, lockShutdown);
    if (!lockShutdown) return;

    RenameThread("bitcoin-shutoff");
    mempool.AddTransactionsUpdated(1);
    StopRPCThreads();
    ShutdownRPCMining();
    if (pwalletMain)
        bitdb.Flush(false);
    GenerateBitcoins(false, NULL, 0);
    StopNode();
    {
        LOCK(cs_main);
        if (pwalletMain)
            pwalletMain->SetBestChain(chainActive.GetLocator());
        if (pblocktree)
            pblocktree->Flush();
        if (pcoinsTip)
            pcoinsTip->Flush();
        delete pcoinsTip; pcoinsTip = NULL;
        delete pcoinsdbview; pcoinsdbview = NULL;
        delete pblocktree; pblocktree = NULL;
    }
    if (pwalletMain)
        bitdb.Flush(true);
    boost::filesystem::remove(GetPidFile());
    UnregisterAllWallets();
    if (pwalletMain)
        delete pwalletMain;
    Log() << "Shutdown : done\n";
}

//
// Signal handlers are very limited in what they are allowed to do, so:
//
void HandleSIGTERM(int)
{
    fRequestShutdown = true;
}

void HandleSIGHUP(int)
{
    Log::fReopenDebugLog = true;
}

bool static InitError(const string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR);
    return false;
}

bool static InitWarning(const string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING);
    return true;
}

bool static Bind(const CService &addr, unsigned int flags) {
    if (!(flags & BF_EXPLICIT) && IsLimited(addr))
        return false;
    string strError;
    if (!BindListenPort(addr, strError)) {
        if (flags & BF_REPORT_ERROR)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI, daemon and RPC client
string HelpMessage(HelpMessageMode hmm)
{
    ostringstream ossUsage;
    ossUsage <<  _<string>("Options:") << "\n"
             << "  -?                     " << _<string>("This help message") << "\n"
             << "  -conf=<file>           " << _<string>("Specify configuration file (default: bitcoin.conf)") << "\n"
             << "  -datadir=<dir>         " << _<string>("Specify data directory") << "\n"
             << "  -testnet               " << _<string>("Use the test network") << "\n"
             << "  -pid=<file>            " << _<string>("Specify pid file (default: bitcoind.pid)") << "\n"
             << "  -gen                   " << _<string>("Generate coins (default: 0)") << "\n"
             << "  -wallet=<file>         " << _<string>("Specify wallet file (within data directory)") << "\n"
             << "  -dbcache=<n>           " << _<string>("Set database cache size in megabytes (default: 25)") << "\n"
             << "  -timeout=<n>           " << _<string>("Specify connection timeout in milliseconds (default: 5000)") << "\n"
             << "  -proxy=<ip:port>       " << _<string>("Connect through socks proxy") << "\n"
             << "  -socks=<n>             " << _<string>("Select the version of socks proxy to use (4-5, default: 5)") << "\n"
             << "  -onion=<ip:port>       " << _<string>("Use proxy to reach tor hidden services (default: same as -proxy)") << "\n"
             << "  -dns                   " << _<string>("Allow DNS lookups for -addnode, -seednode and -connect") << "\n"
             << "  -port=<port>           " << _<string>("Listen for connections on <port> (default: 8333 or testnet: 18333)") << "\n"
             << "  -maxconnections=<n>    " << _<string>("Maintain at most <n> connections to peers (default: 125)") << "\n"
             << "  -addnode=<ip>          " << _<string>("Add a node to connect to and attempt to keep the connection open") << "\n"
             << "  -connect=<ip>          " << _<string>("Connect only to the specified node(s)") << "\n"
             << "  -seednode=<ip>         " << _<string>("Connect to a node to retrieve peer addresses, and disconnect") << "\n"
             << "  -externalip=<ip>       " << _<string>("Specify your own public address") << "\n"
             << "  -onlynet=<net>         " << _<string>("Only connect to nodes in network <net> (IPv4, IPv6 or Tor)") << "\n"
             << "  -discover              " << _<string>("Discover own IP address (default: 1 when listening and no -externalip)") << "\n"
             << "  -checkpoints           " << _<string>("Only accept block chain matching built-in checkpoints (default: 1)") << "\n"
             << "  -listen                " << _<string>("Accept connections from outside (default: 1 if no -proxy or -connect)") << "\n"
             << "  -bind=<addr>           " << _<string>("Bind to given address and always listen on it. Use [host]:port notation for IPv6") << "\n"
             << "  -dnsseed               " << _<string>("Find peers using DNS lookup (default: 1 unless -connect)") << "\n"
             << "  -banscore=<n>          " << _<string>("Threshold for disconnecting misbehaving peers (default: 100)") << "\n"
             << "  -bantime=<n>           " << _<string>("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") << "\n"
             << "  -maxreceivebuffer=<n>  " << _<string>("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") << "\n"
             << "  -maxsendbuffer=<n>     " << _<string>("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") << "\n";
#ifdef USE_UPNP
#if USE_UPNP
    ossUsage << "  -upnp                  " << _<string>("Use UPnP to map the listening port (default: 1 when listening)") << "\n";
#else
    ossUsage << "  -upnp                  " << _<string>("Use UPnP to map the listening port (default: 0)") << "\n";
#endif
#endif
    ossUsage << "  -paytxfee=<amt>        " << _<string>("Fee per KB to add to transactions you send") << "\n"
             << "  -debug=<category>      " << _<string>("Output debugging information (default: 0, supplying <category> is optional)") << "\n"
             <<                                _<string>("If <category> is not supplied, output all debugging information.") << "\n"
             <<                                _<string>("<category> can be:")
             <<                                          " addrman, alert, coindb, db, lock, rand, rpc, selectcoins, mempool, net"; // Don't translate these and qt below
    if (hmm == HMM_BITCOIN_QT)
    {
        ossUsage << ", qt.\n";
    }
    else
    {
        ossUsage << ".\n";
    }
    ossUsage << "  -logtimestamps         " << _<string>("Prepend debug output with timestamp (default: 1)") << "\n"
             << "  -shrinkdebugfile       " << _<string>("Shrink debug.log file on client startup (default: 1 when no -debug)") << "\n"
             << "  -printtoconsole        " << _<string>("Send trace/debug info to console instead of debug.log file") << "\n"
             << "  -regtest               " << _<string>("Enter regression test mode, which uses a special chain in which blocks can be "
                                                         "solved instantly. This is intended for regression testing tools and app development.") << "\n";
#ifdef WIN32
    ossUsage << "  -printtodebugger       " << _<string>("Send trace/debug info to debugger") << "\n";
#endif

    if (hmm == HMM_BITCOIN_QT)
    {
        ossUsage << "  -server                " << _<string>("Accept command line and JSON-RPC commands") << "\n";
    }

    if (hmm == HMM_BITCOIND)
    {
#if !defined(WIN32)
        ossUsage << "  -daemon                " << _<string>("Run in the background as a daemon and accept commands") << "\n";
#endif
    }

    ossUsage << "  -rpcuser=<user>        " << _<string>("Username for JSON-RPC connections") << "\n"
             << "  -rpcpassword=<pw>      " << _<string>("Password for JSON-RPC connections") << "\n"
             << "  -rpcport=<port>        " << _<string>("Listen for JSON-RPC connections on <port> (default: 8332 or testnet: 18332)") << "\n"
             << "  -rpcallowip=<ip>       " << _<string>("Allow JSON-RPC connections from specified IP address") << "\n"
             << "  -rpcthreads=<n>        " << _<string>("Set the number of threads to service RPC calls (default: 4)") << "\n"
             << "  -blocknotify=<cmd>     " << _<string>("Execute command when the best block changes (%s in cmd is replaced by block hash)") << "\n"
             << "  -walletnotify=<cmd>    " << _<string>("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)") << "\n"
             << "  -alertnotify=<cmd>     " << _<string>("Execute command when a relevant alert is received or we see a really long fork (%s in cmd is replaced by message)") << "\n"
             << "  -upgradewallet         " << _<string>("Upgrade wallet to latest format") << "\n"
             << "  -keypool=<n>           " << _<string>("Set key pool size to <n> (default: 100)") << "\n"
             << "  -rescan                " << _<string>("Rescan the block chain for missing wallet transactions") << "\n"
             << "  -salvagewallet         " << _<string>("Attempt to recover private keys from a corrupt wallet.dat") << "\n"
             << "  -checkblocks=<n>       " << _<string>("How many blocks to check at startup (default: 288, 0 = all)") << "\n"
             << "  -checklevel=<n>        " << _<string>("How thorough the block verification is (0-4, default: 3)") << "\n"
             << "  -txindex               " << _<string>("Maintain a full transaction index (default: 0)") << "\n"
             << "  -loadblock=<file>      " << _<string>("Imports blocks from external blk000??.dat file") << "\n"
             << "  -reindex               " << _<string>("Rebuild block chain index from current blk000??.dat files") << "\n"
             << "  -par=<n>               " << _<string>("Set the number of script verification threads (up to 16, 0 = auto, <0 = leave that many cores free, default: 0)") << "\n"
             << "\n";

    ossUsage << _<string>("Block creation options:") << "\n"
             << "  -blockminsize=<n>      "   << _<string>("Set minimum block size in bytes (default: 0)") << "\n"
             << "  -blockmaxsize=<n>      "   << _<string>("Set maximum block size in bytes (default: 250000)") << "\n"
             << "  -blockprioritysize=<n> "   << _<string>("Set maximum size of high-priority/low-fee transactions in bytes (default: 27000)") << "\n"
             << "\n";

    ossUsage << _<string>("SSL options: (see the Bitcoin Wiki for SSL setup instructions)") << "\n"
             << "  -rpcssl                                  " << _<string>("Use OpenSSL (https) for JSON-RPC connections") << "\n"
             << "  -rpcsslcertificatechainfile=<file.cert>  " << _<string>("Server certificate file (default: server.cert)") << "\n"
             << "  -rpcsslprivatekeyfile=<file.pem>         " << _<string>("Server private key (default: server.pem)") << "\n"
             << "  -rpcsslciphers=<ciphers>                 " << _<string>("Acceptable ciphers (default: TLSv1.2+HIGH:TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!3DES:@STRENGTH)") << "\n";

    return ossUsage.str();
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

void ThreadImport(std::vector<boost::filesystem::path> vImportFiles)
{
    RenameThread("bitcoin-loadblk");

    // -reindex
    if (fReindex) {
        CImportingNow imp;
        int nFile = 0;
        while (true) {
            CDiskBlockPos pos(nFile, 0);
            FILE *file = OpenBlockFile(pos, true);
            if (!file)
                break;
            Log() << "Reindexing block file blk" << setfill('0') << setw(5) << (unsigned int) nFile << setfill(' ') << ".dat...\n";
            LoadExternalBlockFile(file, &pos);
            nFile++;
        }
        pblocktree->WriteReindexing(false);
        fReindex = false;
        Log() << "Reindexing finished\n";
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        InitBlockIndex();
    }

    // hardcoded $DATADIR/bootstrap.dat
    filesystem::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            CImportingNow imp;
            filesystem::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            Log() << "Importing bootstrap.dat...\n";
            LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        }
    }

    // -loadblock=
    BOOST_FOREACH(boost::filesystem::path &path, vImportFiles) {
        FILE *file = fopen(path.string().c_str(), "rb");
        if (file) {
            CImportingNow imp;
            Log() << "Importing " << path.string() << "...\n";
            LoadExternalBlockFile(file);
        }
    }
}

/** Initialize bitcoin.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInit2(boost::thread_group& threadGroup, bool fForceServer)
{
    // ********************************************************* Step 1: setup
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
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
    if (setProcDEPPol != NULL) setProcDEPPol(PROCESS_DEP_ENABLE);

    // Initialize Windows Sockets
    WSADATA wsadata;
    int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion ) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return InitError(boost::str(boost::format("Error: Winsock library failed to start (WSAStartup returned error %d)") % ret));
#endif
#ifndef WIN32
    umask(077);

    // Clean shutdown on SIGTERM
    struct sigaction sa;
    sa.sa_handler = HandleSIGTERM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    // Reopen debug.log on SIGHUP
    struct sigaction sa_hup;
    sa_hup.sa_handler = HandleSIGHUP;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    sigaction(SIGHUP, &sa_hup, NULL);

#if defined (__SVR4) && defined (__sun)
    // ignore SIGPIPE on Solaris
    signal(SIGPIPE, SIG_IGN);
#endif
#endif

    // ********************************************************* Step 2: parameter interactions

    if (mapArgs.count("-bind")) {
        // when specifying an explicit binding address, you want to listen on it
        // even when -connect or -proxy is specified
        SoftSetBoolArg("-listen", true);
    }

    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        SoftSetBoolArg("-dnsseed", false);
        SoftSetBoolArg("-listen", false);
    }

    if (mapArgs.count("-proxy")) {
        // to protect privacy, do not listen by default if a proxy server is specified
        SoftSetBoolArg("-listen", false);
    }

    if (!GetBoolArg("-listen", true)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        SoftSetBoolArg("-upnp", false);
        SoftSetBoolArg("-discover", false);
    }

    if (mapArgs.count("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        SoftSetBoolArg("-discover", false);
    }

    if (GetBoolArg("-salvagewallet", false)) {
        // Rewrite just private keys: rescan to find transactions
        SoftSetBoolArg("-rescan", true);
    }

    // Make sure enough file descriptors are available
    int nBind = std::max((int)mapArgs.count("-bind"), 1);
    nMaxConnections = GetArg("-maxconnections", 125);
    nMaxConnections = std::max(std::min(nMaxConnections, (int)(FD_SETSIZE - nBind - MIN_CORE_FILEDESCRIPTORS)), 0);
    int nFD = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_<string>("Not enough file descriptors available."));
    if (nFD - MIN_CORE_FILEDESCRIPTORS < nMaxConnections)
        nMaxConnections = nFD - MIN_CORE_FILEDESCRIPTORS;

    // ********************************************************* Step 3: parameter-to-internal-flags

    Log::fDebug = !mapMultiArgs["-debug"].empty();
    // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
    const vector<string>& categories = mapMultiArgs["-debug"];
    if (GetBoolArg("-nodebug", false) || find(categories.begin(), categories.end(), string("0")) != categories.end())
        Log::fDebug = false;

    // Check for -debugnet (deprecated)
    if (GetBoolArg("-debugnet", false))
        InitWarning(_<string>("Warning: Deprecated argument -debugnet ignored, use -debug=net"));

    fBenchmark = GetBoolArg("-benchmark", false);
    mempool.setSanityCheck(GetBoolArg("-checkmempool", RegTest()));
    Checkpoints::fEnabled = GetBoolArg("-checkpoints", true);

    // -par=0 means autodetect, but nScriptCheckThreads==0 means no concurrency
    nScriptCheckThreads = GetArg("-par", 0);
    if (nScriptCheckThreads <= 0)
        nScriptCheckThreads += boost::thread::hardware_concurrency();
    if (nScriptCheckThreads <= 1)
        nScriptCheckThreads = 0;
    else if (nScriptCheckThreads > MAX_SCRIPTCHECK_THREADS)
        nScriptCheckThreads = MAX_SCRIPTCHECK_THREADS;

    if (fDaemon || fForceServer)
        fServer = true;
    else
        fServer = GetBoolArg("-server", false);

    Log::fPrintToConsole = GetBoolArg("-printtoconsole", false);
    Log::fPrintToDebugger = GetBoolArg("-printtodebugger", false);
    Log::fLogTimestamps = GetBoolArg("-logtimestamps", true);
    bool fDisableWallet = GetBoolArg("-disablewallet", false);

    if (mapArgs.count("-timeout"))
    {
        int nNewTimeout = GetArg("-timeout", 5000);
        if (nNewTimeout > 0 && nNewTimeout < 600000)
            nConnectTimeout = nNewTimeout;
    }

    // Continue to put "/P2SH/" in the coinbase to monitor
    // BIP16 support.
    // This can be removed eventually...
    const char* pszP2SH = "/P2SH/";
    COINBASE_FLAGS << std::vector<unsigned char>(pszP2SH, pszP2SH+strlen(pszP2SH));

    // Fee-per-kilobyte amount considered the same as "free"
    // If you are mining, be careful setting this:
    // if you set it to zero then
    // a transaction spammer can cheaply fill blocks using
    // 1-satoshi-fee transactions. It should be set above the real
    // cost to you of processing a transaction.
    if (mapArgs.count("-mintxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-mintxfee"], n) && n > 0)
            CTransaction::nMinTxFee = n;
        else
            return InitError(str(_("Invalid amount for -mintxfee=<amount>: '%s'") % mapArgs["-mintxfee"]));
    }
    if (mapArgs.count("-minrelaytxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-minrelaytxfee"], n) && n > 0)
            CTransaction::nMinRelayTxFee = n;
        else
            return InitError(str(_("Invalid amount for -minrelaytxfee=<amount>: '%s'") % mapArgs["-minrelaytxfee"]));
    }

    if (mapArgs.count("-paytxfee"))
    {
        if (!ParseMoney(mapArgs["-paytxfee"], nTransactionFee))
            return InitError(str(_("Invalid amount for -paytxfee=<amount>: '%s'") % mapArgs["-paytxfee"]));
        if (nTransactionFee > 0.25 * COIN)
            InitWarning(_<string>("Warning: -paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
    }

    strWalletFile = GetArg("-wallet", "wallet.dat");

    // ********************************************************* Step 4: application initialization: dir lock, daemonize, pidfile, debug log

    string strDataDir = GetDataDir().string();

    // Wallet file must be a plain filename without a directory
    if (strWalletFile != boost::filesystem::basename(strWalletFile) + boost::filesystem::extension(strWalletFile))
        return InitError(str(_("Wallet %s resides outside data directory %s") % strWalletFile % strDataDir));

    // Make sure only a single Bitcoin process is using the data directory.
    boost::filesystem::path pathLockFile = GetDataDir() / ".lock";
    FILE* file = fopen(pathLockFile.string().c_str(), "a"); // empty lock file; created if it doesn't exist.
    if (file) fclose(file);
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock())
        return InitError(str(_("Cannot obtain a lock on data directory %s. Bitcoin is probably already running.") % strDataDir));

    if (GetBoolArg("-shrinkdebugfile", !Log::fDebug))
        ShrinkDebugFile();
    Log() << "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
    Log() << "Bitcoin version " << FormatFullVersion() << " (" << CLIENT_DATE << ")\n";
    Log() << "Using OpenSSL version " << SSLeay_version(SSLEAY_VERSION) << "\n";
    if (!Log::fLogTimestamps)
        Log() << "Startup time: " << BitcoinTime::DateTimeStrFormat("%Y-%m-%d %H:%M:%S", BitcoinTime::GetTime()) << "\n";
    Log() << "Default data directory " << GetDefaultDataDir().string() << "\n";
    Log() << "Using data directory " << strDataDir << "\n";
    Log() << "Using at most " << nMaxConnections << " connections (" << nFD << " file descriptors available)\n";
    std::ostringstream ossErrors;

    if (fDaemon)
        cout << "Bitcoin server starting\n";

    if (nScriptCheckThreads) {
        Log() << "Using " << nScriptCheckThreads << " threads for script verification\n";
        for (int i=0; i<nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
    }

    int64_t nStart;

    // ********************************************************* Step 5: verify wallet database integrity

    if (!fDisableWallet) {
        uiInterface.InitMessage(_<string>("Verifying wallet..."));

        if (!bitdb.Open(GetDataDir()))
        {
            // try moving the database env out of the way
            boost::filesystem::path pathDatabase = GetDataDir() / "database";
            boost::filesystem::path pathDatabaseBak = GetDataDir() / boost::str(boost::format("database.%d.bak") % BitcoinTime::GetTime()); 
            try {
                boost::filesystem::rename(pathDatabase, pathDatabaseBak);
                Log() << "Moved old " << pathDatabase.string() << " to " << pathDatabaseBak.string() << ". Retrying.\n";
            } catch(boost::filesystem::filesystem_error &error) {
                 // failure is ok (well, not really, but it's not worse than what we started with)
            }

            // try again
            if (!bitdb.Open(GetDataDir())) {
                // if it still fails, it probably means we can't even create the database env
                string msg = str(_("Error initializing wallet database environment %s!") % strDataDir);
                return InitError(msg);
            }
        }

        if (GetBoolArg("-salvagewallet", false))
        {
            // Recover readable keypairs:
            if (!CWalletDB::Recover(bitdb, strWalletFile, true))
                return false;
        }

        if (filesystem::exists(GetDataDir() / strWalletFile))
        {
            CDBEnv::VerifyResult r = bitdb.Verify(strWalletFile, CWalletDB::Recover);
            if (r == CDBEnv::RECOVER_OK)
            {
                string msg = str(_("Warning: wallet.dat corrupt, data salvaged!"
                                         " Original wallet.dat saved as wallet.{timestamp}.bak in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup.") % strDataDir);
                InitWarning(msg);
            }
            if (r == CDBEnv::RECOVER_FAIL)
                return InitError(_<string>("wallet.dat corrupt, salvage failed"));
        }
    } // (!fDisableWallet)

    // ********************************************************* Step 6: network initialization

    RegisterNodeSignals(GetNodeSignals());

    int nSocksVersion = GetArg("-socks", 5);
    if (nSocksVersion != 4 && nSocksVersion != 5)
        return InitError(str(_("Unknown -socks proxy version requested: %i") % nSocksVersion));

    if (mapArgs.count("-onlynet")) {
        std::set<enum Network> nets;
        BOOST_FOREACH(string snet, mapMultiArgs["-onlynet"]) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(str(_("Unknown network specified in -onlynet: '%s'") % snet));
            nets.insert(net);
        }
        for (int n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net))
                SetLimited(net);
        }
    }
#if defined(USE_IPV6)
#if ! USE_IPV6
    else
        SetLimited(NET_IPV6);
#endif
#endif

    CService addrProxy;
    bool fProxy = false;
    if (mapArgs.count("-proxy")) {
        addrProxy = CService(mapArgs["-proxy"], 9050);
        if (!addrProxy.IsValid())
            return InitError(str(_("Invalid -proxy address: '%s'") % mapArgs["-proxy"]));

        if (!IsLimited(NET_IPV4))
            SetProxy(NET_IPV4, addrProxy, nSocksVersion);
        if (nSocksVersion > 4) {
#ifdef USE_IPV6
            if (!IsLimited(NET_IPV6))
                SetProxy(NET_IPV6, addrProxy, nSocksVersion);
#endif
            SetNameProxy(addrProxy, nSocksVersion);
        }
        fProxy = true;
    }

    // -onion can override normal proxy, -noonion disables tor entirely
    // -tor here is a temporary backwards compatibility measure
    if (mapArgs.count("-tor"))
        cout << "Notice: option -tor has been replaced with -onion and will be removed in a later version.\n";
    if (!(mapArgs.count("-onion") && mapArgs["-onion"] == "0") &&
        !(mapArgs.count("-tor") && mapArgs["-tor"] == "0") &&
         (fProxy || mapArgs.count("-onion") || mapArgs.count("-tor"))) {
        CService addrOnion;
        if (!mapArgs.count("-onion") && !mapArgs.count("-tor"))
            addrOnion = addrProxy;
        else
            addrOnion = mapArgs.count("-onion")?CService(mapArgs["-onion"], 9050):CService(mapArgs["-tor"], 9050);
        if (!addrOnion.IsValid())
            return InitError(str(_("Invalid -onion address: '%s'") % (mapArgs.count("-onion") ? mapArgs["-onion"] : mapArgs["-tor"])));
        SetProxy(NET_TOR, addrOnion, 5);
        SetReachable(NET_TOR);
    }

    // see Step 2: parameter interactions for more information about these
    fNoListen = !GetBoolArg("-listen", true);
    fDiscover = GetBoolArg("-discover", true);
    fNameLookup = GetBoolArg("-dns", true);

    bool fBound = false;
    if (!fNoListen) {
        if (mapArgs.count("-bind")) {
            BOOST_FOREACH(string strBind, mapMultiArgs["-bind"]) {
                CService addrBind;
                if (!Lookup(strBind.c_str(), addrBind, GetListenPort(), false))
                    return InitError(str(_("Cannot resolve -bind address: '%s'") % strBind));
                fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR));
            }
        }
        else {
            struct in_addr inaddr_any;
            inaddr_any.s_addr = INADDR_ANY;
#ifdef USE_IPV6
            fBound |= Bind(CService(in6addr_any, GetListenPort()), BF_NONE);
#endif
            fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE);
        }
        if (!fBound)
            return InitError(_<string>("Failed to listen on any port. Use -listen=0 if you want this."));
    }

    if (mapArgs.count("-externalip")) {
        BOOST_FOREACH(string strAddr, mapMultiArgs["-externalip"]) {
            CService addrLocal(strAddr, GetListenPort(), fNameLookup);
            if (!addrLocal.IsValid())
                return InitError(str(_("Cannot resolve -externalip address: '%s'") % strAddr));
            AddLocal(CService(strAddr, GetListenPort(), fNameLookup), LOCAL_MANUAL);
        }
    }

    BOOST_FOREACH(string strDest, mapMultiArgs["-seednode"])
        AddOneShot(strDest);

    // ********************************************************* Step 7: load block chain

    fReindex = GetBoolArg("-reindex", false);

    // Upgrading to 0.8; hard-link the old blknnnn.dat files into /blocks/
    filesystem::path blocksDir = GetDataDir() / "blocks";
    if (!filesystem::exists(blocksDir))
    {
        filesystem::create_directories(blocksDir);
        bool linked = false;
        for (unsigned int i = 1; i < 10000; i++) {
            filesystem::path source = GetDataDir() / boost::str(boost::format("blk%04d.dat") % i);
            if (!filesystem::exists(source)) break;

            filesystem::path dest = blocksDir / boost::str(boost::format("blk%05d.dat") % (i - 1));
            try {
                filesystem::create_hard_link(source, dest);
                Log() << "Hardlinked " << source.string() << " -> " << dest.string() << "\n";
                linked = true;
            } catch (filesystem::filesystem_error & e) {
                // Note: hardlink creation failing is not a disaster, it just means
                // blocks will get re-downloaded from peers.
                Log() << "Error hardlinking blk" << setfill('0') << setw(4) << i << setfill(' ') << ".dat : " << e.what() << "\n";
                break;
            }
        }
        if (linked)
        {
            fReindex = true;
        }
    }

    // cache size calculations
    size_t nTotalCache = GetArg("-dbcache", 25) << 20;
    if (nTotalCache < (1 << 22))
        nTotalCache = (1 << 22); // total cache cannot be less than 4 MiB
    size_t nBlockTreeDBCache = nTotalCache / 8;
    if (nBlockTreeDBCache > (1 << 21) && !GetBoolArg("-txindex", false))
        nBlockTreeDBCache = (1 << 21); // block tree db cache shouldn't be larger than 2 MiB
    nTotalCache -= nBlockTreeDBCache;
    size_t nCoinDBCache = nTotalCache / 2; // use half of the remaining cache for coindb cache
    nTotalCache -= nCoinDBCache;
    nCoinCacheSize = nTotalCache / 300; // coins in memory require around 300 bytes

    bool fLoaded = false;
    while (!fLoaded) {
        bool fReset = fReindex;
        string strLoadError;

        uiInterface.InitMessage(_<string>("Loading block index..."));

        nStart = BitcoinTime::GetTimeMillis();
        do {
            try {
                UnloadBlockIndex();
                delete pcoinsTip;
                delete pcoinsdbview;
                delete pblocktree;

                pblocktree = new CBlockTreeDB(nBlockTreeDBCache, false, fReindex);
                pcoinsdbview = new CCoinsViewDB(nCoinDBCache, false, fReindex);
                pcoinsTip = new CCoinsViewCache(*pcoinsdbview);

                if (fReindex)
                    pblocktree->WriteReindexing(true);

                if (!LoadBlockIndex()) {
                    strLoadError = _<string>("Error loading block database");
                    break;
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!mapBlockIndex.empty() && chainActive.Genesis() == NULL)
                    return InitError(_<string>("Incorrect or no genesis block found. Wrong datadir for network?"));

                // Initialize the block index (no-op if non-empty database was already loaded)
                if (!InitBlockIndex()) {
                    strLoadError = _<string>("Error initializing block database");
                    break;
                }

                // Check for changed -txindex state
                if (fTxIndex != GetBoolArg("-txindex", false)) {
                    strLoadError = _<string>("You need to rebuild the database using -reindex to change -txindex");
                    break;
                }

                uiInterface.InitMessage(_<string>("Verifying blocks..."));
                if (!VerifyDB(GetArg("-checklevel", 3),
                              GetArg("-checkblocks", 288))) {
                    strLoadError = _<string>("Corrupted block database detected");
                    break;
                }
            } catch(std::exception &e) {
                if (Log::fDebug) Log() << e.what() << "\n";
                strLoadError = _<string>("Error opening block database");
                break;
            }

            fLoaded = true;
        } while(false);

        if (!fLoaded) {
            // first suggest a reindex
            if (!fReset) {
                bool fRet = uiInterface.ThreadSafeMessageBox(
                    strLoadError + ".\n\n" + _<string>("Do you want to rebuild the block database now?"),
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    fReindex = true;
                    fRequestShutdown = false;
                } else {
                    Log() << "Aborted block database rebuild. Exiting.\n";
                    return false;
                }
            } else {
                return InitError(strLoadError);
            }
        }
    }

    // as LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill bitcoin-qt during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown)
    {
        Log() << "Shutdown requested. Exiting.\n";
        return false;
    }
    Log() << " block index " << setw(15) << (BitcoinTime::GetTimeMillis() - nStart) << "ms\n";

    if (GetBoolArg("-printblockindex", false) || GetBoolArg("-printblocktree", false))
    {
        PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-printblock"))
    {
        string strMatch = mapArgs["-printblock"];
        int nFound = 0;
        for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0)
            {
                CBlockIndex* pindex = (*mi).second;
                CBlock block;
                ReadBlockFromDisk(block, pindex);
                block.BuildMerkleTree();
                block.print();
                Log() << "\n";
                nFound++;
            }
        }
        if (nFound == 0)
            Log() << "No blocks matching " << strMatch << " were found\n";
        return false;
    }

    // ********************************************************* Step 8: load wallet

    if (fDisableWallet) {
        pwalletMain = NULL;
        Log() << "Wallet disabled!\n";
    } else {
        uiInterface.InitMessage(_<string>("Loading wallet..."));

        nStart = BitcoinTime::GetTimeMillis();
        bool fFirstRun = true;
        pwalletMain = new CWallet(strWalletFile);
        DBErrors nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
        if (nLoadWalletRet != DB_LOAD_OK)
        {
            if (nLoadWalletRet == DB_CORRUPT)
                ossErrors << _<string>("Error loading wallet.dat: Wallet corrupted") << "\n";
            else if (nLoadWalletRet == DB_NONCRITICAL_ERROR)
            {
                string msg(_<string>("Warning: error reading wallet.dat! All keys read correctly, but transaction data"
                             " or address book entries might be missing or incorrect."));
                InitWarning(msg);
            }
            else if (nLoadWalletRet == DB_TOO_NEW)
                ossErrors << _<string>("Error loading wallet.dat: Wallet requires newer version of Bitcoin") << "\n";
            else if (nLoadWalletRet == DB_NEED_REWRITE)
            {
                ossErrors << _<string>("Wallet needed to be rewritten: restart Bitcoin to complete") << "\n";
                Log() << ossErrors.str();
                return InitError(ossErrors.str());
            }
            else
                ossErrors << _<string>("Error loading wallet.dat") << "\n";
        }

        if (GetBoolArg("-upgradewallet", fFirstRun))
        {
            int nMaxVersion = GetArg("-upgradewallet", 0);
            if (nMaxVersion == 0) // the -upgradewallet without argument case
            {
                Log() << "Performing wallet upgrade to " << FEATURE_LATEST << "\n";
                nMaxVersion = CLIENT_VERSION;
                pwalletMain->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
            }
            else
                Log() << "Allowing wallet upgrade up to " << nMaxVersion << "\n";
            if (nMaxVersion < pwalletMain->GetVersion())
                ossErrors << _<string>("Cannot downgrade wallet") << "\n";
            pwalletMain->SetMaxVersion(nMaxVersion);
        }

        if (fFirstRun)
        {
            // Create new keyUser and set as default key
            RandAddSeedPerfmon();

            CPubKey newDefaultKey;
            if (pwalletMain->GetKeyFromPool(newDefaultKey)) {
                pwalletMain->SetDefaultKey(newDefaultKey);
                if (!pwalletMain->SetAddressBook(pwalletMain->vchDefaultKey.GetID(), "", "receive"))
                    ossErrors << _<string>("Cannot write default address") << "\n";
            }

            pwalletMain->SetBestChain(chainActive.GetLocator());
        }

        Log() << ossErrors.str();
        Log() << " wallet      " << setw(15) << (BitcoinTime::GetTimeMillis() - nStart) << "ms\n";

        RegisterWallet(pwalletMain);

        CBlockIndex *pindexRescan = chainActive.Tip();
        if (GetBoolArg("-rescan", false))
            pindexRescan = chainActive.Genesis();
        else
        {
            CWalletDB walletdb(strWalletFile);
            CBlockLocator locator;
            if (walletdb.ReadBestBlock(locator))
                pindexRescan = chainActive.FindFork(locator);
            else
                pindexRescan = chainActive.Genesis();
        }
        if (chainActive.Tip() && chainActive.Tip() != pindexRescan)
        {
            uiInterface.InitMessage(_<string>("Rescanning..."));
            Log() << "Rescanning last " << (chainActive.Height() - pindexRescan->nHeight) << " blocks (from block " << pindexRescan->nHeight << ")...\n";
            nStart = BitcoinTime::GetTimeMillis();
            pwalletMain->ScanForWalletTransactions(pindexRescan, true);
            Log() << " rescan      " << setw(15) << (BitcoinTime::GetTimeMillis() - nStart) << "ms\n";
            pwalletMain->SetBestChain(chainActive.GetLocator());
            nWalletDBUpdated++;
        }
    } // (!fDisableWallet)

    // ********************************************************* Step 9: import blocks

    // scan for better chains in the block chain database, that are not yet connected in the active best chain
    CValidationState state;
    if (!ConnectBestBlock(state))
        ossErrors << "Failed to connect best block";

    std::vector<boost::filesystem::path> vImportFiles;
    if (mapArgs.count("-loadblock"))
    {
        BOOST_FOREACH(string strFile, mapMultiArgs["-loadblock"])
            vImportFiles.push_back(strFile);
    }
    threadGroup.create_thread(boost::bind(&ThreadImport, vImportFiles));

    // ********************************************************* Step 10: load peers

    uiInterface.InitMessage(_<string>("Loading addresses..."));

    nStart = BitcoinTime::GetTimeMillis();

    {
        CAddrDB adb;
        if (!adb.Read(addrman))
            Log() << "Invalid or missing peers.dat; recreating\n";
    }

    Log() << "Loaded " << addrman.size() << " addresses from peers.dat  " << (BitcoinTime::GetTimeMillis() - nStart) << "ms\n";

    // ********************************************************* Step 11: start node

    if (!CheckDiskSpace())
        return false;

    if (!ossErrors.str().empty())
        return InitError(ossErrors.str());

    RandAddSeedPerfmon();

    //// debug print
    Log() << "mapBlockIndex.size() = " << mapBlockIndex.size() << "\n";
    Log() << "nBestHeight = " << chainActive.Height() << "\n";
    Log() << "setKeyPool.size() = " << (pwalletMain ? pwalletMain->setKeyPool.size() : 0) << "\n";
    Log() << "mapWallet.size() = " << (pwalletMain ? pwalletMain->mapWallet.size() : 0) << "\n";
    Log() << "mapAddressBook.size() = " << (pwalletMain ? pwalletMain->mapAddressBook.size() : 0) << "\n";

    StartNode(threadGroup);

    // InitRPCMining is needed here so getwork/getblocktemplate in the GUI debug console works properly.
    InitRPCMining();
    if (fServer)
        StartRPCThreads();

    // Generate coins in the background
    if (pwalletMain)
        GenerateBitcoins(GetBoolArg("-gen", false), pwalletMain, GetArg("-genproclimit", -1));

    // ********************************************************* Step 12: finished

    uiInterface.InitMessage(_<string>("Done loading"));

    if (pwalletMain) {
        // Add wallet transactions that aren't already in a block to mapTransactions
        pwalletMain->ReacceptWalletTransactions();

        // Run a thread to flush wallet periodically
        threadGroup.create_thread(boost::bind(&ThreadFlushWalletDB, boost::ref(pwalletMain->strWalletFile)));
    }

    return !fRequestShutdown;
}
