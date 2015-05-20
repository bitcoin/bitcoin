// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "bitcredit-config.h"
#endif

#include "init.h"

#include "addrman.h"
#include "checkpoints.h"
#include "bitcoin_checkpoints.h"
#include "main.h"
#include "miner.h"
#include "net.h"
#include "rpcserver.h"
#include "txdb.h"
#include "bitcoin_txdb.h"
#include "ui_interface.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "db.h"
#include "bitcoin_db.h"
#include "wallet.h"
#include "bitcoin_wallet.h"
#include "walletdb.h"
#include "bitcoin_walletdb.h"
#endif

#include <stdint.h>
#include <stdio.h>

#ifndef WIN32
#include <signal.h>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <openssl/crypto.h>

using namespace std;
using namespace boost;


#ifdef ENABLE_WALLET
uint64_t bitcredit_nAccountingEntryNumber = 0;
Credits_CDBEnv bitcredit_bitdb("credits_database", "credits_db.log");
std::string bitcredit_strWalletFile;
Credits_CWallet* bitcredit_pwalletMain;

uint64_t deposit_nAccountingEntryNumber = 0;
Credits_CDBEnv deposit_bitdb("deposit_database", "deposit_db.log");
std::string deposit_strWalletFile;
Credits_CWallet* deposit_pwalletMain;

std::string bitcoin_strWalletFile;
Bitcoin_CWallet* bitcoin_pwalletMain;
#endif

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

static Credits_CCoinsViewDB *bitcredit_pcoinsdbview;
static Bitcoin_CCoinsViewDB *bitcoin_pcoinsdbview;

void Shutdown()
{
	CNetParams * bitcredit_netParams = Credits_NetParams();
	CNetParams * bitcoin_netParams = Bitcoin_NetParams();

    LogPrintf("Shutdown : In progress...\n");
    static CCriticalSection cs_Shutdown;
    TRY_LOCK(cs_Shutdown, lockShutdown);
    if (!lockShutdown) return;

    RenameThread("bitcoin-shutoff");
    credits_mempool.AddTransactionsUpdated(1);
    bitcoin_mempool.AddTransactionsUpdated(1);
    StopRPCThreads();
    ShutdownRPCMining();
#ifdef ENABLE_WALLET
    if (bitcredit_pwalletMain)
        bitcredit_bitdb.Flush(false);
    GenerateBitcredits(false, NULL, NULL, 0, false);
#endif
#ifdef ENABLE_WALLET
    if (deposit_pwalletMain)
        deposit_bitdb.Flush(false);
#endif
#ifdef ENABLE_WALLET
    if (bitcoin_pwalletMain)
        bitcoin_bitdb.Flush(false);
#endif
    StopNode();
    bitcredit_netParams->UnregisterNodeSignals();
    {
        LOCK(credits_mainState.cs_main);
#ifdef ENABLE_WALLET
        if (bitcredit_pwalletMain)
            bitcredit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
#endif
//Deposit wallet should not be involved in the chain
//#ifdef ENABLE_WALLET
//        if (deposit_pwalletMain)
//            deposit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
//#endif
#ifdef ENABLE_WALLET
        if (bitcoin_pwalletMain) {
        	bitcoin_pwalletMain->SetBestChain(bitcoin_chainActive.GetLocator());
        }
#endif
        if (bitcredit_pblocktree)
            bitcredit_pblocktree->Flush();
        if (credits_pcoinsTip) {
            credits_pcoinsTip->Credits_Flush();
            credits_pcoinsTip->Claim_Flush();
        }
        delete credits_pcoinsTip; credits_pcoinsTip = NULL;
        delete bitcredit_pcoinsdbview; bitcredit_pcoinsdbview = NULL;
        delete bitcredit_pblocktree; bitcredit_pblocktree = NULL;
    }

#ifdef ENABLE_WALLET
    if (bitcredit_pwalletMain)
        bitcredit_bitdb.Flush(true);
#endif
#ifdef ENABLE_WALLET
    if (deposit_pwalletMain)
        deposit_bitdb.Flush(true);
#endif
    Bitcredit_UnregisterAllWallets();
#ifdef ENABLE_WALLET
    if (bitcredit_pwalletMain)
        delete bitcredit_pwalletMain;
#endif
#ifdef ENABLE_WALLET
    if (deposit_pwalletMain)
        delete deposit_pwalletMain;
#endif

#ifdef ENABLE_WALLET
    if (bitcoin_pwalletMain)
        bitcoin_bitdb.Flush(true);
#endif
    Bitcoin_UnregisterAllWallets();
#ifdef ENABLE_WALLET
    if (bitcoin_pwalletMain)
        delete bitcoin_pwalletMain;
#endif

    bitcoin_netParams->UnregisterNodeSignals();
    {
        LOCK(bitcoin_mainState.cs_main);
        if (bitcoin_pblocktree)
            bitcoin_pblocktree->Flush();
        delete bitcoin_pblocktree; bitcoin_pblocktree = NULL;

        if (bitcoin_pcoinsTip)
            bitcoin_pcoinsTip->Flush();
        delete bitcoin_pcoinsTip; bitcoin_pcoinsTip = NULL;
        delete bitcoin_pcoinsdbview; bitcoin_pcoinsdbview = NULL;
    }

    boost::filesystem::remove(GetPidFile());

    LogPrintf("Shutdown : done\n");
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
    fReopenDebugLog = true;
}

bool static InitError(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_ERROR | CClientUIInterface::NOSHOWGUI);
    return false;
}

bool static InitWarning(const std::string &str)
{
    uiInterface.ThreadSafeMessageBox(str, "", CClientUIInterface::MSG_WARNING | CClientUIInterface::NOSHOWGUI);
    return true;
}

bool static Bind(const CService &addr, unsigned int flags, CNetParams * netParams) {
    if (!(flags & BF_EXPLICIT) && IsLimited(addr, netParams))
        return false;
    std::string strError;
    if (!BindListenPort(addr, strError, netParams)) {
        if (flags & BF_REPORT_ERROR)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI, daemon and RPC client
std::string HelpMessage(HelpMessageMode hmm)
{
    string strUsage = _("Options:") + "\n";
    strUsage += "  -?                     " + _("This help message") + "\n";
    strUsage += "  -alertnotify=<cmd>     " + _("Execute command when a relevant alert is received or we see a really long fork (%s in cmd is replaced by message)") + "\n";
    strUsage += "  -blocknotify=<cmd>     " + _("Execute command when the best block changes (%s in cmd is replaced by block hash)") + "\n";
    strUsage += "  -checkblocks=<n>       " + _("How many blocks to check at startup (default: 288, 0 = all)") + "\n";
    strUsage += "  -checklevel=<n>        " + _("How thorough the block verification of -checkblocks is (0-4, default: 3)") + "\n";
    strUsage += "  -conf=<file>           " + _("Specify configuration file (default: credits.conf)") + "\n";
    if (hmm == HMM_BITCOIND)
    {
#if !defined(WIN32)
        strUsage += "  -daemon                " + _("Run in the background as a daemon and accept commands") + "\n";
#endif
    }
    strUsage += "  -datadir=<dir>         " + _("Specify data directory") + "\n";
    strUsage += "  -dbcache=<n>           " + strprintf(_("Set database cache size in megabytes (%d to %d, default: %d)"), bitcredit_nMinDbCache, bitcredit_nMaxDbCache, bitcredit_nDefaultDbCache) + "\n";
    strUsage += "  -keypool=<n>           " + _("Set key pool size to <n> (default: 100)") + "\n";
    strUsage += "  -loadblock=<file>      " + _("Imports blocks from external blk000??.dat file") + " " + _("on startup") + "\n";
    strUsage += "  -bitcoin_loadblock=<file>         " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -par=<n>               " + strprintf(_("Set the number of script verification threads (%u to %d, 0 = auto, <0 = leave that many cores free, default: %d)"), -(int)boost::thread::hardware_concurrency(), BITCREDIT_MAX_SCRIPTCHECK_THREADS, BITCREDIT_DEFAULT_SCRIPTCHECK_THREADS) + "\n";
    strUsage += "  -pid=<file>            " + _("Specify pid file (default: creditsd.pid)") + "\n";
    strUsage += "  -maxorphanblocks=<n>   " + strprintf(_("Keep at most <n> unconnectable blocks in memory (default: %u)"), BITCREDIT_DEFAULT_MAX_ORPHAN_BLOCKS) + "\n";
    strUsage += "  -bitcoin_maxorphanblocks=<n>         " + _("Same as above, for bitcoin") + "\n";

    strUsage += "  -reindex               " + _("Rebuild block chain index from current blk000??.dat files") + " " + _("on startup") + "\n";
    strUsage += "  -txindex               " + _("Maintain a full transaction index (default: 0)") + "\n";

    strUsage += "\n" + _("Connection options:") + "\n";
    strUsage += "  -addnode=<ip>          " + _("Add a node to connect to and attempt to keep the connection open") + "\n";
    strUsage += "  -bitcoin_addnode=<ip>         " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -banscore=<n>          " + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n";
    strUsage += "  -bantime=<n>           " + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n";
    strUsage += "  -bind=<addr>           " + _("Bind to given address and always listen on it. Use [host]:port notation for IPv6") + "\n";
    strUsage += "  -bitcoin_bind=<addr>         " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -claimtmpdbthreshold=<ip>          " + _("Set the number of bitcoin blocks a claim tip movement should be done without a temporary db. -1 means no tmpdb, memory only. Default -1.") + "\n";
    strUsage += "  -connect=<ip>          " + _("Connect only to the specified node(s)") + "\n";
    strUsage += "  -bitcoin_connect=<ip>          " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -discover              " + _("Discover own IP address (default: 1 when listening and no -externalip)") + "\n";
    strUsage += "  -bitcoin_discover         " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -dns                   " + _("Allow DNS lookups for -addnode, -seednode and -connect") + " " + _("(default: 1)") + "\n";
    strUsage += "  -bitcoin_dns                   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -dnsseed               " + _("Find peers using DNS lookup (default: 1 unless -connect)") + "\n";
    strUsage += "  -bitcoin_dnsseed      " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -externalip=<ip>       " + _("Specify your own public address") + "\n";
    strUsage += "  -bitcoin_externalip=<ip>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -listen                " + _("Accept connections from outside (default: 1 if no -proxy or -connect)") + "\n";
    strUsage += "  -bitcoin_listen          " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -maxconnections=<n>    " + _("Maintain at most <n> connections to peers (default: 125)") + "\n";
    strUsage += "  -bitcoin_maxconnections=<n>    " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -maxreceivebuffer=<n>  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") + "\n";
    strUsage += "  -maxsendbuffer=<n>     " + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") + "\n";
    strUsage += "  -onion=<ip:port>       " + _("Use separate SOCKS5 proxy to reach peers via Tor hidden services (default: -proxy)") + "\n";
    strUsage += "  -bitcoin_onion=<ip:port>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -onlynet=<net>         " + _("Only connect to nodes in network <net> (IPv4, IPv6 or Tor)") + "\n";
    strUsage += "  -bitcoin_onlynet=<net>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -port=<port>           " + _("Listen for connections on <port> (default: 9333 or testnet: 19333)") + "\n";
    strUsage += "  -bitcoin_port=<port>           " + _("Same as above, for bitcoin") + "(default: 8333 or testnet: 18333)" + "\n";
    strUsage += "  -proxy=<ip:port>       " + _("Connect through SOCKS proxy") + "\n";
    strUsage += "  -bitcoin_proxy=<ip:port>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -seednode=<ip>         " + _("Connect to a node to retrieve peer addresses, and disconnect") + "\n";
    strUsage += "  -bitcoin_seednode=<ip>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -socks=<n>             " + _("Select SOCKS version for -proxy (4 or 5, default: 5)") + "\n";
    strUsage += "  -timeout=<n>           " + _("Specify connection timeout in milliseconds (default: 5000)") + "\n";
#ifdef USE_UPNP
#if USE_UPNP
    strUsage += "  -upnp                  " + _("Use UPnP to map the listening port (default: 1 when listening)") + "\n";
    strUsage += "  -bitcoin_upnp                  " + _("Same as above, for bitcoin") + "\n";
#else
    strUsage += "  -upnp                  " + _("Use UPnP to map the listening port (default: 0)") + "\n";
    strUsage += "  -bitcoin_upnp                 " + _("Same as above, for bitcoin") + "\n";
#endif
#endif

#ifdef ENABLE_WALLET
    strUsage += "\n" + _("Wallet options:") + "\n";
    strUsage += "  -disablewallet         " + _("Do not load the wallet and disable wallet RPC calls") + "\n";
    strUsage += "  -paytxfee=<amt>        " + _("Fee per kB to add to transactions you send") + "\n";
    strUsage += "  -bitcoin_paytxfee=<amt>       " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -bitcredit_rescan                " + _("Rescan the block chain for missing wallet transactions") + " " + _("on startup") + "\n";
    strUsage += "  -bitcoin_rescan                " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -salvagewallet         " + _("Attempt to recover private keys from a corrupt wallet.dat") + " " + _("on startup") + "\n";
    strUsage += "  -spendzeroconfchange   " + _("Spend unconfirmed change when sending transactions (default: 1)") + "\n";
    strUsage += "  -bitcoin_spendzeroconfchange   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -upgradewallet         " + _("Upgrade wallet to latest format") + " " + _("on startup") + "\n";
    strUsage += "  -credits_wallet=<file>         " + _("Specify wallet file (within data directory)") + " " + _("(default: wallet.dat)") + "\n";
    strUsage += "  -bitcoin_wallet=<file>         " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -walletnotify=<cmd>    " + _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)") + "\n";
    strUsage += "  -zapwallettxes         " + _("Clear list of wallet transactions (diagnostic tool; implies -bitcredit_rescan)") + "\n";
#endif

    strUsage += "\n" + _("Debugging/Testing options:") + "\n";
    if (GetBoolArg("-help-debug", false))
    {
        strUsage += "  -benchmark             " + _("Show benchmark information (default: 0)") + "\n";
        strUsage += "  -checkpoints           " + _("Only accept block chain matching built-in checkpoints (default: 1)") + "\n";
        strUsage += "  -bitcoin_checkpoints            " + _("Same as above, for bitcoin") + "\n";
        strUsage += "  -dblogsize=<n>         " + _("Flush database activity from memory pool to disk log every <n> megabytes (default: 100)") + "\n";
        strUsage += "  -disablesafemode       " + _("Disable safemode, override a real safe mode event (default: 0)") + "\n";
        strUsage += "  -testsafemode          " + _("Force safe mode (default: 0)") + "\n";
        strUsage += "  -dropmessagestest=<n>  " + _("Randomly drop 1 of every <n> network messages") + "\n";
        strUsage += "  -fuzzmessagestest=<n>  " + _("Randomly fuzz 1 of every <n> network messages") + "\n";
        strUsage += "  -flushwallet           " + _("Run a thread to flush wallet periodically (default: 1)") + "\n";
    }
    strUsage += "  -debug=<category>      " + _("Output debugging information (default: 0, supplying <category> is optional)") + "\n";
    strUsage += "                         " + _("If <category> is not supplied, output all debugging information.") + "\n";
    strUsage += "                         " + _("<category> can be:");
    strUsage +=                                 " addrman, alert, coindb, db, lock, rand, rpc, selectcoins, mempool, net"; // Don't translate these and qt below
    if (hmm == HMM_BITCOIN_QT)
        strUsage += ", qt";
    strUsage += ".\n";
    strUsage += "  -gen                   " + _("Generate coins (default: 0)") + "\n";
    strUsage += "  -genproclimit=<n>      " + _("Set the processor limit for when generation is on (-1 = unlimited, default: -1)") + "\n";
    strUsage += "  -coinbasedepositdisabled      " + _("If set to true, using coinbase as part of the deposit will be disabled.. (default: false)") + "\n";
    strUsage += "  -help-debug            " + _("Show all debugging options (usage: --help -help-debug)") + "\n";
    strUsage += "  -logtimestamps         " + _("Prepend debug output with timestamp (default: 1)") + "\n";
    if (GetBoolArg("-help-debug", false))
    {
        strUsage += "  -limitfreerelay=<n>    " + _("Continuously rate-limit free transactions to <n>*1000 bytes per minute (default:15)") + "\n";
        strUsage += "  -maxsigcachesize=<n>   " + _("Limit size of signature cache to <n> entries (default: 50000)") + "\n";
    }
    strUsage += "  -mintxfee=<amt>        " + _("Fees smaller than this are considered zero fee (for transaction creation) (default:") + " " + FormatMoney(Credits_CTransaction::nMinTxFee) + ")" + "\n";
    strUsage += "  -bitcoin_mintxfee=<amt>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -minrelaytxfee=<amt>   " + _("Fees smaller than this are considered zero fee (for relaying) (default:") + " " + FormatMoney(Credits_CTransaction::nMinRelayTxFee) + ")" + "\n";
    strUsage += "  -bitcoin_minrelaytxfee=<amt>   " + _("Same as above, for bitcoin") + "\n";
    strUsage += "  -printtoconsole        " + _("Send trace/debug info to console instead of debug.log file") + "\n";
    if (GetBoolArg("-help-debug", false))
    {
        strUsage += "  -printblock=<hash>     " + _("Print block on startup, if found in block index") + "\n";
        strUsage += "  -bitcoin_printblock=<hash>           " + _("Same as above, for bitcoin") + "\n";
        strUsage += "  -printblocktree        " + _("Print block tree on startup (default: 0)") + "\n";
        strUsage += "  -bitcoin_printblocktree         " + _("Same as above, for bitcoin") + "\n";
        strUsage += "  -printpriority         " + _("Log transaction priority and fee per kB when mining blocks (default: 0)") + "\n";
        strUsage += "  -privdb                " + _("Sets the DB_PRIVATE flag in the wallet db environment (default: 1)") + "\n";
        strUsage += "  -regtest               " + _("Enter regression test mode, which uses a special chain in which blocks can be solved instantly.") + "\n";
        strUsage += "                         " + _("This is intended for regression testing tools and app development.") + "\n";
        strUsage += "                         " + _("In this mode -genproclimit controls how many blocks are generated immediately.") + "\n";
    }
    strUsage += "  -shrinkdebugfile       " + _("Shrink debug.log file on client startup (default: 1 when no -debug)") + "\n";
    strUsage += "  -testnet               " + _("Use the test network") + "\n";

    strUsage += "\n" + _("Block creation options:") + "\n";
    strUsage += "  -blockminsize=<n>      " + _("Set minimum block size in bytes (default: 0)") + "\n";
    strUsage += "  -blockmaxsize=<n>      " + strprintf(_("Set maximum block size in bytes (default: %d)"), BITCREDIT_DEFAULT_BLOCK_MAX_SIZE) + "\n";
    strUsage += "  -blockprioritysize=<n> " + strprintf(_("Set maximum size of high-priority/low-fee transactions in bytes (default: %d)"), BITCREDIT_DEFAULT_BLOCK_PRIORITY_SIZE) + "\n";

    strUsage += "\n" + _("RPC server options:") + "\n";
    strUsage += "  -server                " + _("Accept command line and JSON-RPC commands") + "\n";
    strUsage += "  -rpcbind=<addr>        " + _("Bind to given address to listen for JSON-RPC connections. Use [host]:port notation for IPv6. This option can be specified multiple times (default: bind to all interfaces)") + "\n";
    strUsage += "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n";
    strUsage += "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n";
    strUsage += "  -rpcport=<port>        " + _("Listen for JSON-RPC connections on <port> (default: 9332 or testnet: 19332)") + "\n";
    strUsage += "  -rpcallowip=<ip>       " + _("Allow JSON-RPC connections from specified IP address. This option can be specified multiple times") + "\n";
    strUsage += "  -rpcthreads=<n>        " + _("Set the number of threads to service RPC calls (default: 4)") + "\n";

    strUsage += "\n" + _("RPC SSL options: (see the Credits Wiki for SSL setup instructions)") + "\n";
    strUsage += "  -rpcssl                                  " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n";
    strUsage += "  -rpcsslcertificatechainfile=<file.cert>  " + _("Server certificate file (default: server.cert)") + "\n";
    strUsage += "  -rpcsslprivatekeyfile=<file.pem>         " + _("Server private key (default: server.pem)") + "\n";
    strUsage += "  -rpcsslciphers=<ciphers>                 " + _("Acceptable ciphers (default: TLSv1.2+HIGH:TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!3DES:@STRENGTH)") + "\n";

    return strUsage;
}

struct Bitcredit_CImportingNow
{
    Bitcredit_CImportingNow() {
        assert(credits_mainState.fImporting == false);
        credits_mainState.fImporting = true;
    }

    ~Bitcredit_CImportingNow() {
        assert(credits_mainState.fImporting == true);
        credits_mainState.fImporting = false;
    }
};
struct Bitcoin_CImportingNow
{
	Bitcoin_CImportingNow() {
        assert(bitcoin_mainState.fImporting == false);
        bitcoin_mainState.fImporting = true;
    }

    ~Bitcoin_CImportingNow() {
        assert(bitcoin_mainState.fImporting == true);
        bitcoin_mainState.fImporting = false;
    }
};

void Bitcredit_ThreadImport()
{
    RenameThread("bitcredit_bitcoin-loadblk");

    std::vector<boost::filesystem::path> vImportFiles;
    if (mapArgs.count("-loadblock"))
    {
        BOOST_FOREACH(string strFile, mapMultiArgs["-loadblock"])
				vImportFiles.push_back(strFile);
    }

    // -reindex
    if (credits_mainState.fReindex) {
        Bitcredit_CImportingNow imp;
        int nFile = 0;
        while (true) {
        	CDiskBlockPos pos(nFile, 0);
            FILE *file = Bitcredit_OpenBlockFile(pos, true);
            if (!file)
                break;
            LogPrintf("Credits: Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
            Bitcredit_LoadExternalBlockFile(file, &pos);
            nFile++;
        }
        bitcredit_pblocktree->WriteReindexing(false);
        credits_mainState.fReindex = false;
        LogPrintf("Credits: Reindexing finished\n");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        Bitcredit_InitBlockIndex();
    }

    // hardcoded $DATADIR/bitcredit_bootstrap.dat
    filesystem::path pathBootstrap = GetDataDir() / "bitcredit_bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            Bitcredit_CImportingNow imp;
            filesystem::path pathBootstrapOld = GetDataDir() / "bitcredit_bootstrap.dat.old";
            LogPrintf("Credits: Importing bitcredit_bootstrap.dat...\n");
            Bitcredit_LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        } else {
            LogPrintf("Credits: Warning: Could not open bitcredit_bootstrap.dat file %s\n", pathBootstrap.string());
        }
    }

    // -loadblock=
    BOOST_FOREACH(boost::filesystem::path &path, vImportFiles) {
        FILE *file = fopen(path.string().c_str(), "rb");
        if (file) {
            Bitcredit_CImportingNow imp;
            LogPrintf("Credits: Importing blocks file %s...\n", path.string());
            Bitcredit_LoadExternalBlockFile(file);
        } else {
            LogPrintf("Credits: Warning: Could not open blocks file %s\n", path.string());
        }
    }
}

void Bitcoin_ThreadImport()
{
    RenameThread("bitcoin_bitcoin-loadblk");

    std::vector<boost::filesystem::path> vImportFiles;
    if (mapArgs.count("-bitcoin_loadblock"))
    {
        BOOST_FOREACH(string strFile, mapMultiArgs["-bitcoin_loadblock"])
				vImportFiles.push_back(strFile);
    }

    // -reindex
    if (bitcoin_mainState.fReindex) {
        Bitcoin_CImportingNow imp;
        int nFile = 0;
        while (true) {
        	CDiskBlockPos pos(nFile, 0);
            FILE *file = Bitcoin_OpenBlockFile(pos, true);
            if (!file)
                break;
            LogPrintf("Bitcoin: Reindexing block file blk%05u.dat...\n", (unsigned int)nFile);
            Bitcoin_LoadExternalBlockFile(file, &pos);
            nFile++;
        }
        bitcoin_pblocktree->WriteReindexing(false);
        bitcoin_mainState.fReindex = false;
        LogPrintf("Bitcoin: Reindexing finished\n");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        Bitcoin_InitBlockIndex();
    }

    // hardcoded $DATADIR/bitcoin_bootstrap.dat
    filesystem::path pathBootstrap = GetDataDir() / "bitcoin_bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            Bitcoin_CImportingNow imp;
            filesystem::path pathBootstrapOld = GetDataDir() / "bitcoin_bootstrap.dat.old";
            LogPrintf("Bitcoin: Importing bitcoin_bootstrap.dat...\n");
            Bitcoin_LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        } else {
            LogPrintf("Bitcoin: Warning: Could not open bitcoin_bootstrap.dat file %s\n", pathBootstrap.string());
        }
    }

    // -loadblock=
    BOOST_FOREACH(boost::filesystem::path &path, vImportFiles) {
        FILE *file = fopen(path.string().c_str(), "rb");
        if (file) {
            Bitcoin_CImportingNow imp;
            LogPrintf("Bitcoin: Importing blocks file %s...\n", path.string());
            Bitcoin_LoadExternalBlockFile(file);
        } else {
            LogPrintf("Bitcoin: Warning: Could not open blocks file %s\n", path.string());
        }
    }

    //Invoke credits import from same thread
    Bitcredit_ThreadImport();
}

void Bitcredit_ThreadFlushWalletDB(const char * threadName, const Credits_CWallet& pwallet)
{
	const string& strFile = pwallet.strWalletFile;
	Credits_CDBEnv *pbitDb = pwallet.pbitDb;

    // Make this thread recognisable as the wallet flushing thread
    RenameThread(threadName);

    static bool fOneThread;
    if (fOneThread)
        return;
    fOneThread = true;
    if (!GetBoolArg("-flushwallet", true))
        return;

    unsigned int nLastSeen = pbitDb->nWalletDBUpdated;
    unsigned int nLastFlushed = pbitDb->nWalletDBUpdated;
    int64_t nLastWalletUpdate = GetTime();
    while (true)
    {
        MilliSleep(500);

        if (nLastSeen != pbitDb->nWalletDBUpdated)
        {
            nLastSeen = pbitDb->nWalletDBUpdated;
            nLastWalletUpdate = GetTime();
        }

        if (nLastFlushed != pbitDb->nWalletDBUpdated && GetTime() - nLastWalletUpdate >= 2)
        {
            TRY_LOCK(pbitDb->cs_db,lockDb);
            if (lockDb)
            {
                // Don't do this if any databases are in use
                int nRefCount = 0;
                map<string, int>::iterator mi = pbitDb->mapFileUseCount.begin();
                while (mi != pbitDb->mapFileUseCount.end())
                {
                    nRefCount += (*mi).second;
                    mi++;
                }

                if (nRefCount == 0)
                {
                    boost::this_thread::interruption_point();
                    map<string, int>::iterator mi = pbitDb->mapFileUseCount.find(strFile);
                    if (mi != pbitDb->mapFileUseCount.end())
                    {
                        LogPrint("db", "Flushing %s\n", strFile);
                        nLastFlushed = pbitDb->nWalletDBUpdated;
                        int64_t nStart = GetTimeMillis();

                        // Flush wallet.dat so it's self contained
                        pbitDb->CloseDb(strFile);
                        pbitDb->CheckpointLSN(strFile);

                        pbitDb->mapFileUseCount.erase(mi++);
                        LogPrint("db", "Flushed %s %dms\n", strFile, GetTimeMillis() - nStart);
                    }
                }
            }
        }
    }
}

void InitPeersFromNetParams(int start, CNetParams * params) {
    	const std::string fileName = params->AddrFileName();
        CAddrDB adb(fileName, params->MessageStart(), params->ClientVersion());
        if (!adb.Read(params->addrman)) {
            LogPrintf("Invalid or missing %s; recreating\n", fileName);
        }

        LogPrintf("Loaded %i addresses from %s  %dms\n",
        	params->addrman.size(), fileName, GetTimeMillis() - start);
}

bool InitFileDescriptors(CNetParams * netParams, std::string bindKey, std::string maxConnectionsKey) {
    const int nBind = std::max((int)mapArgs.count(bindKey), 1);
    netParams->nMaxConnections = GetArg(maxConnectionsKey, 125);
    netParams->nMaxConnections = std::max(std::min(netParams->nMaxConnections, (int)(FD_SETSIZE - nBind - MIN_CORE_FILEDESCRIPTORS)), 0);
    const int nFD = RaiseFileDescriptorLimit(netParams->nMaxConnections + MIN_CORE_FILEDESCRIPTORS);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available."));
    if (nFD - MIN_CORE_FILEDESCRIPTORS < netParams->nMaxConnections)
    	netParams->nMaxConnections = nFD - MIN_CORE_FILEDESCRIPTORS;

    return true;
}

bool InitNodeSignals(CNetParams * netParams, std::string socksKey, std::string onlyNetKey, std::string proxyKey, std::string torKey, std::string onionKey) {
	netParams->RegisterNodeSignals();

    int nSocksVersion = GetArg(socksKey, 5);
    if (nSocksVersion != 4 && nSocksVersion != 5)
        return InitError(strprintf(_("Unknown %s proxy version requested: %i"), socksKey, nSocksVersion));

    if (mapArgs.count(onlyNetKey)) {
        std::set<enum Network> nets;
        BOOST_FOREACH(std::string snet, mapMultiArgs[onlyNetKey]) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE)
                return InitError(strprintf(_("Unknown network specified in %s: '%s'"), onlyNetKey, snet));
            nets.insert(net);
        }
        for (int n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net))
                SetLimited(net, true, netParams);
        }
    }

    CService addrProxy;
    bool fProxy = false;
    if (mapArgs.count(proxyKey)) {
        addrProxy = CService(mapArgs[proxyKey], 9050);
        if (!addrProxy.IsValid())
            return InitError(strprintf(_("Invalid %s address: '%s'"), proxyKey, mapArgs[proxyKey]));

        if (!IsLimited(NET_IPV4, netParams))
            SetProxy(NET_IPV4, addrProxy, nSocksVersion);
        if (nSocksVersion > 4) {
            if (!IsLimited(NET_IPV6, netParams))
                SetProxy(NET_IPV6, addrProxy, nSocksVersion);
            SetNameProxy(addrProxy, nSocksVersion);
        }
        fProxy = true;
    }

    // -onion can override normal proxy, -noonion disables tor entirely
    // -tor here is a temporary backwards compatibility measure
    if (mapArgs.count(torKey))
        printf("Notice: option %s has been replaced with %s and will be removed in a later version.\n", torKey.c_str(), onionKey.c_str());
    if (!(mapArgs.count(onionKey) && mapArgs[onionKey] == "0") &&
        !(mapArgs.count(torKey) && mapArgs[torKey] == "0") &&
         (fProxy || mapArgs.count(onionKey) || mapArgs.count(torKey))) {
        CService addrOnion;
        if (!mapArgs.count(onionKey) && !mapArgs.count(torKey))
            addrOnion = addrProxy;
        else
            addrOnion = mapArgs.count(onionKey)?CService(mapArgs[onionKey], 9050):CService(mapArgs[torKey], 9050);
        if (!addrOnion.IsValid())
            return InitError(strprintf(_("Invalid %s address: '%s'"), onionKey, mapArgs.count(onionKey)?mapArgs[onionKey]:mapArgs[torKey]));
        SetProxy(NET_TOR, addrOnion, 5);
        SetReachable(NET_TOR, true, netParams);
    }

    return true;
}

bool InitPortBinding(CNetParams * netParams, std::string discoverKey, std::string bindKey, std::string externalIpKey, std::string seedNodeKey) {
    netParams->fDiscover = GetBoolArg(discoverKey, true);

    bool fBound = false;
    if (netParams->fListen) {
        if (mapArgs.count(bindKey)) {
            BOOST_FOREACH(std::string strBind, mapMultiArgs[bindKey]) {
                CService addrBind;
                if (!Lookup(strBind.c_str(), addrBind, netParams->GetListenPort(), false))
                    return InitError(strprintf(_("Cannot resolve %s address: '%s'"), bindKey, strBind));
                fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR), netParams);
            }
        }
        else {
            struct in_addr inaddr_any;
            inaddr_any.s_addr = INADDR_ANY;
            fBound |= Bind(CService(in6addr_any, netParams->GetListenPort()), BF_NONE, netParams);
            fBound |= Bind(CService(inaddr_any, netParams->GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE, netParams);
        }
        if (!fBound)
            return InitError(_("Failed to listen on any port. Use -listen=0 if you want this."));
    }

    if (mapArgs.count(externalIpKey)) {
        BOOST_FOREACH(string strAddr, mapMultiArgs[externalIpKey]) {
            CService addrLocal(strAddr, netParams->GetListenPort(), fNameLookup);
            if (!addrLocal.IsValid())
                return InitError(strprintf(_("Cannot resolve %s address: '%s'"), externalIpKey, strAddr));
            AddLocal(CService(strAddr, netParams->GetListenPort(), fNameLookup), LOCAL_MANUAL, netParams);
        }
    }

    BOOST_FOREACH(string strDest, mapMultiArgs[seedNodeKey])
        AddOneShot(strDest, netParams);

    return true;
}

bool Bitcredit_InitDbAndCache(int64_t& nStart) {
    size_t nTotalCache = (GetArg("-dbcache", bitcredit_nDefaultDbCache) << 20);
    if (nTotalCache < (bitcredit_nMinDbCache << 20))
        nTotalCache = (bitcredit_nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    else if (nTotalCache > (bitcredit_nMaxDbCache << 20))
        nTotalCache = (bitcredit_nMaxDbCache << 20); // total cache cannot be greater than nMaxDbCache
    size_t nBlockTreeDBCache = nTotalCache / 8;
    if (nBlockTreeDBCache > (1 << 21) && !GetBoolArg("-txindex", false))
        nBlockTreeDBCache = (1 << 21); // block tree db cache shouldn't be larger than 2 MiB
    nTotalCache -= nBlockTreeDBCache;
    //TODO - This size may not be ideal. Since this cache has been added extra it will heighten the total memory usage.
    size_t nCoinDBCache = nTotalCache / 2; // use half of the remaining cache for coindb cache
    nTotalCache -= nCoinDBCache;
    bitcredit_nCoinCacheSize = nTotalCache / 300; // coins in memory require around 300 bytes

    bool fLoaded = false;
    while (!fLoaded) {
        bool fReset = credits_mainState.fReindex;
        std::string strLoadError;

        uiInterface.InitMessage(_("Credits: Loading block index..."));

        nStart = GetTimeMillis();
        do {
            try {
                Bitcredit_UnloadBlockIndex();
                delete credits_pcoinsTip;
                delete bitcredit_pcoinsdbview;
                delete bitcredit_pblocktree;

                bitcredit_pblocktree = new Credits_CBlockTreeDB(nBlockTreeDBCache, false, credits_mainState.fReindex);
                bitcredit_pcoinsdbview = new Credits_CCoinsViewDB(nCoinDBCache, false, credits_mainState.fReindex);
                credits_pcoinsTip = new Credits_CCoinsViewCache(*bitcredit_pcoinsdbview);

                if (credits_mainState.fReindex)
                    bitcredit_pblocktree->WriteReindexing(true);

                if (!Bitcredit_LoadBlockIndex()) {
                    strLoadError = _("Credits: Error loading block database");
                    break;
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!credits_mapBlockIndex.empty() && credits_chainActive.Genesis() == NULL)
                    return InitError(_("Credits: Incorrect or no genesis block found. Wrong datadir for network?"));

                if (credits_chainActive.Genesis() != NULL && credits_chainActive.Genesis()->GetBlockHash() != Bitcredit_Params().GenesisBlock().GetHash())
                    return InitError(_("Credits: Genesis block not correct. Unable to start."));

                // Initialize the block index (no-op if non-empty database was already loaded)
                if (!Bitcredit_InitBlockIndex()) {
                    strLoadError = _("Credits: Error initializing block database");
                    break;
                }

                // Check for changed -txindex state
                if (bitcredit_fTxIndex != GetBoolArg("-txindex", false)) {
                    strLoadError = _("Credits: You need to rebuild the database using -reindex to change -txindex");
                    break;
                }

                uiInterface.InitMessage(_("Credits: Verifying blocks..."));
                if (!Bitcredit_CVerifyDB().VerifyDB(GetArg("-checklevel", 3),
                              GetArg("-checkblocks", 288))) {
                    strLoadError = _("Credits: Corrupted block database detected");
                    break;
                }
            } catch(std::exception &e) {
                if (fDebug) LogPrintf("%s\n", e.what());
                strLoadError = _("Credits: Error opening block database");
                break;
            }

            fLoaded = true;
        } while(false);

        if (!fLoaded) {
            // first suggest a reindex
            if (!fReset) {
                bool fRet = uiInterface.ThreadSafeMessageBox(
                    strLoadError + ".\n\n" + _("Credits: Do you want to rebuild the block database now?"),
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    credits_mainState.fReindex = true;
                    fRequestShutdown = false;
                } else {
                    LogPrintf("Credits: Aborted block database rebuild. Exiting.\n");
                    return false;
                }
            } else {
                return InitError(strLoadError);
            }
        }
    }
    return true;
}

bool Bitcoin_InitDbAndCache(int64_t& nStart) {
    size_t nTotalCache = (GetArg("-dbcache", bitcoin_nDefaultDbCache) << 20);
    if (nTotalCache < (bitcoin_nMinDbCache << 20))
        nTotalCache = (bitcoin_nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    else if (nTotalCache > (bitcoin_nMaxDbCache << 20))
        nTotalCache = (bitcoin_nMaxDbCache << 20); // total cache cannot be greater than nMaxDbCache
    size_t nBlockTreeDBCache = nTotalCache / 8;
    if (nBlockTreeDBCache > (1 << 21) && !GetBoolArg("-txindex", false))
        nBlockTreeDBCache = (1 << 21); // block tree db cache shouldn't be larger than 2 MiB
    nTotalCache -= nBlockTreeDBCache;
    const size_t nCoinDBCache = nTotalCache / 2; // use half of the remaining cache for coindb cache
    nTotalCache -= nCoinDBCache;
    bitcoin_nCoinCacheSize = nTotalCache / 300; // coins in memory require around 300 bytes

    bool fLoaded = false;
    while (!fLoaded) {
        bool fReset = bitcoin_mainState.fReindex;
        std::string strLoadError;

        uiInterface.InitMessage(_("Bitcoin: Loading block index..."));

        nStart = GetTimeMillis();
        do {
            try {
                Bitcoin_UnloadBlockIndex();
                delete bitcoin_pcoinsTip;
                delete bitcoin_pcoinsdbview;

                delete bitcoin_pblocktree;

                bitcoin_pblocktree = new Bitcoin_CBlockTreeDB(nBlockTreeDBCache, false, bitcoin_mainState.fReindex);

                bitcoin_pcoinsdbview = new Bitcoin_CCoinsViewDB(nCoinDBCache, false, bitcoin_mainState.fReindex);
                bitcoin_pcoinsTip = new Bitcoin_CCoinsViewCache(*bitcoin_pcoinsdbview);

                if (bitcoin_mainState.fReindex)
                    bitcoin_pblocktree->WriteReindexing(true);

                if (!Bitcoin_LoadBlockIndex()) {
                    strLoadError = _("Bitcoin: Error loading block database");
                    break;
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!bitcoin_mapBlockIndex.empty() && bitcoin_chainActive.Genesis() == NULL)
                    return InitError(_("Bitcoin: Incorrect or no genesis block found. Wrong datadir for network?"));

                // Initialize the block index (no-op if non-empty database was already loaded)
                if (!Bitcoin_InitBlockIndex()) {
                    strLoadError = _("Bitcoin: Error initializing block database");
                    break;
                }

                // Check for changed -txindex state
                if (bitcoin_fTxIndex != GetBoolArg("-txindex", false)) {
                    strLoadError = _("Bitcoin: You need to rebuild the database using -reindex to change -txindex");
                    break;
                }

                uiInterface.InitMessage(_("Bitcoin: Verifying blocks..."));
                if (!Bitcoin_CVerifyDB().VerifyDB(GetArg("-checklevel", 3),
                              GetArg("-checkblocks", 288))) {
                    strLoadError = _("Bitcoin: Corrupted block database detected");
                    break;
                }
            } catch(std::exception &e) {
                if (fDebug) LogPrintf("%s\n", e.what());
                strLoadError = _("Bitcoin: Error opening block database");
                break;
            }

            fLoaded = true;
        } while(false);

        if (!fLoaded) {
            // first suggest a reindex
            if (!fReset) {
                bool fRet = uiInterface.ThreadSafeMessageBox(
                    strLoadError + ".\n\n" + _("Bitcoin: Do you want to rebuild the block database now?"),
                    "", CClientUIInterface::MSG_ERROR | CClientUIInterface::BTN_ABORT);
                if (fRet) {
                    bitcoin_mainState.fReindex = true;
                    fRequestShutdown = false;
                } else {
                    LogPrintf("Aborted bitcoin block database rebuild. Exiting.\n");
                    return false;
                }
            } else {
                return InitError(strLoadError);
            }
        }
    }
    return true;
}

bool Bitcredit_AppInit2(boost::thread_group& threadGroup) {
	CNetParams * bitcoin_netParams = Bitcoin_NetParams();
	CNetParams * bitcredit_netParams = Credits_NetParams();

    // ********************************************************* Step 2: parameter interactions

    if (mapArgs.count("-bitcoin_bind")) {
        // when specifying an explicit binding address, you want to listen on it
        // even when -connect or -proxy is specified
        if (SoftSetBoolArg("-bitcoin_listen", true))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_bind set -> setting -bitcoin_listen=1\n");
    }
    if (mapArgs.count("-bind")) {
        // when specifying an explicit binding address, you want to listen on it
        // even when -connect or -proxy is specified
        if (SoftSetBoolArg("-listen", true))
            LogPrintf("AppInit2 : parameter interaction: -bind set -> setting -listen=1\n");
    }

    if (mapArgs.count("-bitcoin_connect") && mapMultiArgs["-bitcoin_connect"].size() > 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (SoftSetBoolArg("-bitcoin_dnsseed", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_connect set -> setting -bitcoin_dnsseed=0\n");
        if (SoftSetBoolArg("-bitcoin_listen", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_connect set -> setting -bitcoin_listen=0\n");
    }
    if (mapArgs.count("-connect") && mapMultiArgs["-connect"].size() > 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (SoftSetBoolArg("-dnsseed", false))
            LogPrintf("AppInit2 : parameter interaction: -connect set -> setting -dnsseed=0\n");
        if (SoftSetBoolArg("-listen", false))
            LogPrintf("AppInit2 : parameter interaction: -connect set -> setting -listen=0\n");
    }

    if (mapArgs.count("-bitcoin_proxy")) {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (SoftSetBoolArg("-bitcoin_listen", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_proxy set -> setting -bitcoin_listen=0\n");
    }
    if (mapArgs.count("-proxy")) {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (SoftSetBoolArg("-listen", false))
            LogPrintf("AppInit2 : parameter interaction: -proxy set -> setting -listen=0\n");
    }

    if (!GetBoolArg("-bitcoin_listen", true)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (SoftSetBoolArg("-bitcoin_upnp", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_listen=0 -> setting -bitcoin_upnp=0\n");
        if (SoftSetBoolArg("-bitcoin_discover", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_listen=0 -> setting -bitcoin_discover=0\n");
    }
    if (!GetBoolArg("-listen", true)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (SoftSetBoolArg("-upnp", false))
            LogPrintf("AppInit2 : parameter interaction: -listen=0 -> setting -upnp=0\n");
        if (SoftSetBoolArg("-discover", false))
            LogPrintf("AppInit2 : parameter interaction: -listen=0 -> setting -discover=0\n");
    }

    if (mapArgs.count("-bitcoin_externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (SoftSetBoolArg("-bitcoin_discover", false))
            LogPrintf("AppInit2 : parameter interaction: -bitcoin_externalip set -> setting -bitcoin_discover=0\n");
    }
    if (mapArgs.count("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (SoftSetBoolArg("-discover", false))
            LogPrintf("AppInit2 : parameter interaction: -externalip set -> setting -discover=0\n");
    }

    if (GetBoolArg("-salvagewallet", false)) {
        // Rewrite just private keys: rescan to find transactions
        if (SoftSetBoolArg("-bitcredit_rescan", true))
            LogPrintf("AppInit2 : parameter interaction: -salvagewallet=1 -> setting -bitcredit_rescan=1\n");
    }

    // -zapwallettx implies a rescan
    if (GetBoolArg("-zapwallettxes", false)) {
        if (SoftSetBoolArg("-bitcredit_rescan", true))
            LogPrintf("AppInit2 : parameter interaction: -zapwallettxes=1 -> setting -bitcredit_rescan=1\n");
    }

    // Make sure enough file descriptors are available
    if(!InitFileDescriptors(bitcoin_netParams, "-bitcoin_bind", "-bitcoin_maxconnections")) {
    	return false;
    }
    // Make sure enough file descriptors are available
    if(!InitFileDescriptors(bitcredit_netParams, "-bind", "-maxconnections")) {
    	return false;
    }

    // ********************************************************* Step 3: parameter-to-internal-flags

    fDebug = !mapMultiArgs["-debug"].empty();
    // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
    const vector<string>& categories = mapMultiArgs["-debug"];
    if (GetBoolArg("-nodebug", false) || find(categories.begin(), categories.end(), string("0")) != categories.end())
        fDebug = false;

    // Check for -debugnet (deprecated)
    if (GetBoolArg("-debugnet", false))
        InitWarning(_("Warning: Deprecated argument -debugnet ignored, use -debug=net"));

    bitcoin_fBenchmark = GetBoolArg("-benchmark", false);
    bitcoin_mempool.setSanityCheck(GetBoolArg("-checkmempool", Bitcoin_RegTest()));
    Checkpoints::bitcoin_fEnabled = GetBoolArg("-bitcoin_checkpoints", true);
    // -par=0 means autodetect, but nScriptCheckThreads==0 means no concurrency
    bitcoin_nScriptCheckThreads = GetArg("-par", BITCOIN_DEFAULT_SCRIPTCHECK_THREADS);
    if (bitcoin_nScriptCheckThreads <= 0)
        bitcoin_nScriptCheckThreads += boost::thread::hardware_concurrency();
    if (bitcoin_nScriptCheckThreads <= 1)
        bitcoin_nScriptCheckThreads = 0;
    else if (bitcoin_nScriptCheckThreads > BITCOIN_MAX_SCRIPTCHECK_THREADS)
        bitcoin_nScriptCheckThreads = BITCOIN_MAX_SCRIPTCHECK_THREADS;

    bitcredit_fBenchmark = GetBoolArg("-benchmark", false);
    credits_mempool.setSanityCheck(GetBoolArg("-checkmempool", Bitcredit_RegTest()));
    Checkpoints::bitcredit_fEnabled = GetBoolArg("-checkpoints", true);
    // -par=0 means autodetect, but nScriptCheckThreads==0 means no concurrency
    bitcredit_nScriptCheckThreads = GetArg("-par", BITCREDIT_DEFAULT_SCRIPTCHECK_THREADS);
    if (bitcredit_nScriptCheckThreads <= 0)
        bitcredit_nScriptCheckThreads += boost::thread::hardware_concurrency();
    if (bitcredit_nScriptCheckThreads <= 1)
        bitcredit_nScriptCheckThreads = 0;
    else if (bitcredit_nScriptCheckThreads > BITCREDIT_MAX_SCRIPTCHECK_THREADS)
        bitcredit_nScriptCheckThreads = BITCREDIT_MAX_SCRIPTCHECK_THREADS;

    fServer = GetBoolArg("-server", false);
    fPrintToConsole = GetBoolArg("-printtoconsole", false);
    fLogTimestamps = GetBoolArg("-logtimestamps", true);
    setvbuf(stdout, NULL, _IOLBF, 0);
#ifdef ENABLE_WALLET
    bool fDisableWallet = GetBoolArg("-disablewallet", false);
#endif

    // Continue to put "/P2SH/" in the coinbase to monitor
    // BIP16 support.
    // This can be removed eventually...
    const char* pszP2SH = "/P2SH/";
    BITCOIN_COINBASE_FLAGS << std::vector<unsigned char>(pszP2SH, pszP2SH+strlen(pszP2SH));
    BITCREDIT_COINBASE_FLAGS << std::vector<unsigned char>(pszP2SH, pszP2SH+strlen(pszP2SH));

    // Fee-per-kilobyte amount considered the same as "free"
    // If you are mining, be careful setting this:
    // if you set it to zero then
    // a transaction spammer can cheaply fill blocks using
    // 1-satoshi-fee transactions. It should be set above the real
    // cost to you of processing a transaction.
    if (mapArgs.count("-bitcoin_mintxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-bitcoin_mintxfee"], n) && n > 0)
            Bitcoin_CTransaction::nMinTxFee = n;
        else
            return InitError(strprintf(_("Invalid amount for -bitcoin_mintxfee=<amount>: '%s'"), mapArgs["-bitcoin_mintxfee"]));
    }
    if (mapArgs.count("-mintxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-mintxfee"], n) && n > 0)
            Credits_CTransaction::nMinTxFee = n;
        else
            return InitError(strprintf(_("Invalid amount for -mintxfee=<amount>: '%s'"), mapArgs["-mintxfee"]));
    }

    if (mapArgs.count("-bitcoin_minrelaytxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-bitcoin_minrelaytxfee"], n) && n > 0)
            Bitcoin_CTransaction::nMinRelayTxFee = n;
        else
            return InitError(strprintf(_("Invalid amount for -bitcoin_minrelaytxfee=<amount>: '%s'"), mapArgs["-bitcoin_minrelaytxfee"]));
    }
    if (mapArgs.count("-minrelaytxfee"))
    {
        int64_t n = 0;
        if (ParseMoney(mapArgs["-minrelaytxfee"], n) && n > 0)
            Credits_CTransaction::nMinRelayTxFee = n;
        else
            return InitError(strprintf(_("Invalid amount for -minrelaytxfee=<amount>: '%s'"), mapArgs["-minrelaytxfee"]));
    }

#ifdef ENABLE_WALLET
    if (mapArgs.count("-bitcoin_paytxfee"))
    {
        if (!ParseMoney(mapArgs["-bitcoin_paytxfee"], bitcoin_nTransactionFee))
            return InitError(strprintf(_("Invalid amount for -bitcoin_paytxfee=<amount>: '%s'"), mapArgs["-bitcoin_paytxfee"]));
        if (bitcoin_nTransactionFee > bitcoin_nHighTransactionFeeWarning)
            InitWarning(_("Warning: -bitcoin_paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
    }
    bitcoin_bSpendZeroConfChange = GetArg("-bitcoin_spendzeroconfchange", true);

    bitcoin_strWalletFile = GetArg("-bitcoin_wallet", "bitcoin_wallet.dat");
#endif
#ifdef ENABLE_WALLET
    if (mapArgs.count("-paytxfee"))
    {
        if (!ParseMoney(mapArgs["-paytxfee"], credits_nTransactionFee))
            return InitError(strprintf(_("Invalid amount for -paytxfee=<amount>: '%s'"), mapArgs["-paytxfee"]));
        if (credits_nTransactionFee > credits_nHighTransactionFeeWarning)
            InitWarning(_("Warning: -paytxfee is set very high! This is the transaction fee you will pay if you send a transaction."));
    }
    credits_bSpendZeroConfChange = GetArg("-spendzeroconfchange", true);

    bitcredit_strWalletFile = GetArg("-credits_wallet", "credits_wallet.dat");
#endif
#ifdef ENABLE_WALLET
    deposit_strWalletFile = GetArg("-deposit_wallet", "deposit_wallet.dat");
#endif

    // ********************************************************* Step 4: application initialization: dir lock, daemonize, pidfile, debug log

    std::string strDataDir = GetDataDir().string();
#ifdef ENABLE_WALLET
    // Wallet file must be a plain filename without a directory
    if (bitcoin_strWalletFile != boost::filesystem::basename(bitcoin_strWalletFile) + boost::filesystem::extension(bitcoin_strWalletFile))
        return InitError(strprintf(_("Bitcoin: Wallet %s resides outside data directory %s"), bitcoin_strWalletFile, strDataDir));
#endif
#ifdef ENABLE_WALLET
    // Wallet file must be a plain filename without a directory
    if (deposit_strWalletFile != boost::filesystem::basename(deposit_strWalletFile) + boost::filesystem::extension(deposit_strWalletFile))
        return InitError(strprintf(_("Deposit: Wallet %s resides outside data directory %s"), deposit_strWalletFile, strDataDir));
#endif
#ifdef ENABLE_WALLET
    // Wallet file must be a plain filename without a directory
    if (bitcredit_strWalletFile != boost::filesystem::basename(bitcredit_strWalletFile) + boost::filesystem::extension(bitcredit_strWalletFile))
        return InitError(strprintf(_("Credits: Wallet %s resides outside data directory %s"), bitcredit_strWalletFile, strDataDir));
#endif
    // Make sure only a single Bitcoin process is using the data directory.
    boost::filesystem::path pathLockFile = GetDataDir() / ".lock";
    FILE* file = fopen(pathLockFile.string().c_str(), "a"); // empty lock file; created if it doesn't exist.
    if (file) fclose(file);
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock())
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. Credits Core is probably already running."), strDataDir));

    const boost::filesystem::path tmpDirPath = GetDataDir() / ".tmp";
	TryRemoveDirectory(tmpDirPath);
	TryCreateDirectory(tmpDirPath);

    if (GetBoolArg("-shrinkdebugfile", !fDebug))
        ShrinkDebugFile();
    LogPrintf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    LogPrintf("Bitcoin version %s (%s)\n", FormatFullVersion(), CLIENT_DATE);
    LogPrintf("Credits version %s (%s)\n", FormatFullVersion(), CLIENT_DATE);
    LogPrintf("Using OpenSSL version %s\n", SSLeay_version(SSLEAY_VERSION));
#ifdef ENABLE_WALLET
    LogPrintf("Using BerkeleyDB version %s\n", DbEnv::version(0, 0, 0));
#endif
    if (!fLogTimestamps)
        LogPrintf("Startup time: %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()));
    LogPrintf("Default data directory %s\n", GetDefaultDataDir().string());
    LogPrintf("Using data directory %s\n", strDataDir);
    LogPrintf("Bitcoin using at most %i connections\n", bitcoin_netParams->nMaxConnections);
    LogPrintf("Credits using at most %i connections\n", bitcredit_netParams->nMaxConnections);
    std::ostringstream strErrors;

    if (bitcoin_nScriptCheckThreads) {
        LogPrintf("Bitcoin using %u threads for script verification\n", bitcoin_nScriptCheckThreads);
        for (int i=0; i<bitcoin_nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&Bitcoin_ThreadScriptCheck);
    }
    if (bitcredit_nScriptCheckThreads) {
        LogPrintf("Credits using %u threads for script verification\n", bitcredit_nScriptCheckThreads);
        for (int i=0; i<bitcredit_nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&Bitcredit_ThreadScriptCheck);
    }

    int64_t nStart;

    // ********************************************************* Step 5: verify wallet database integrity
#ifdef ENABLE_WALLET
    if (!fDisableWallet) {
        LogPrintf("Bitcoin: Using wallet %s\n", bitcoin_strWalletFile);
        uiInterface.InitMessage(_("Bitcoin: Verifying wallet..."));

        if (!bitcoin_bitdb.Open(GetDataDir()))
        {
            // try moving the database env out of the way
            boost::filesystem::path pathDatabase = GetDataDir() / "bitcoin_database";
            boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("bitcoin_database.%d.bak", GetTime());
            try {
                boost::filesystem::rename(pathDatabase, pathDatabaseBak);
                LogPrintf("Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
            } catch(boost::filesystem::filesystem_error &error) {
                 // failure is ok (well, not really, but it's not worse than what we started with)
            }

            // try again
            if (!bitcoin_bitdb.Open(GetDataDir())) {
                // if it still fails, it probably means we can't even create the database env
                string msg = strprintf(_("Bitcoin: Error initializing wallet database environment %s!"), strDataDir);
                return InitError(msg);
            }
        }

        if (GetBoolArg("-salvagewallet", false))
        {
            // Recover readable keypairs:
            if (!Bitcoin_CWalletDB::Recover(bitcoin_bitdb, bitcoin_strWalletFile, true))
                return false;
        }

        if (filesystem::exists(GetDataDir() / bitcoin_strWalletFile))
        {
            Bitcoin_CDBEnv::VerifyResult r = bitcoin_bitdb.Verify(bitcoin_strWalletFile, Bitcoin_CWalletDB::Recover);
            if (r == Bitcoin_CDBEnv::RECOVER_OK)
            {
                string msg = strprintf(_("Bitcoin: Warning: bitcoin_wallet.dat corrupt, data salvaged!"
                                         " Original bitcoin_wallet.dat saved as bitcoin_wallet.{timestamp}.bak in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup."), strDataDir);
                InitWarning(msg);
            }
            if (r == Bitcoin_CDBEnv::RECOVER_FAIL)
                return InitError(_("bitcoin_wallet.dat corrupt, salvage failed"));
        }
    } // (!fDisableWallet)
#endif // ENABLE_WALLET

#ifdef ENABLE_WALLET
    if (!fDisableWallet) {
        LogPrintf("Deposit: Using wallet %s\n", deposit_strWalletFile);
        uiInterface.InitMessage(_("Deposit: Verifying wallet..."));

        if (!deposit_bitdb.Open(GetDataDir()))
        {
            // try moving the database env out of the way
            boost::filesystem::path pathDatabase = GetDataDir() / deposit_bitdb.dbName;
            boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("deposit_database.%d.bak", GetTime());
            try {
                boost::filesystem::rename(pathDatabase, pathDatabaseBak);
                LogPrintf("Deposit: Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
            } catch(boost::filesystem::filesystem_error &error) {
                 // failure is ok (well, not really, but it's not worse than what we started with)
            }

            // try again
            if (!deposit_bitdb.Open(GetDataDir())) {
                // if it still fails, it probably means we can't even create the database env
                string msg = strprintf(_("Deposit: Error initializing wallet database environment %s!"), strDataDir);
                return InitError(msg);
            }
        }

        if (GetBoolArg("-salvagewallet", false))
        {
            // Recover readable keypairs:
            if (!Credits_CWalletDB::Recover(deposit_bitdb, deposit_strWalletFile, true, deposit_nAccountingEntryNumber))
                return false;
        }

        if (filesystem::exists(GetDataDir() / deposit_strWalletFile))
        {
            Credits_CDBEnv::VerifyResult r = deposit_bitdb.Verify(deposit_strWalletFile, deposit_nAccountingEntryNumber, Credits_CWalletDB::Recover);
            if (r == Credits_CDBEnv::RECOVER_OK)
            {
                string msg = strprintf(_("Deposit: Warning: deposit_wallet.dat corrupt, data salvaged!"
                                         " Original deposit_wallet.dat saved as deposit_wallet.{timestamp}.bak in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup."), strDataDir);
                InitWarning(msg);
            }
            if (r == Credits_CDBEnv::RECOVER_FAIL)
                return InitError(_("deposit_wallet.dat corrupt, salvage failed"));
        }
    } // (!fDisableWallet)
#endif // ENABLE_WALLET

#ifdef ENABLE_WALLET
    if (!fDisableWallet) {
        LogPrintf("Credits: Using wallet %s\n", bitcredit_strWalletFile);
        uiInterface.InitMessage(_("Credits: Verifying wallet..."));

        if (!bitcredit_bitdb.Open(GetDataDir()))
        {
            // try moving the database env out of the way
            boost::filesystem::path pathDatabase = GetDataDir() / bitcredit_bitdb.dbName;
            boost::filesystem::path pathDatabaseBak = GetDataDir() / strprintf("credits_database.%d.bak", GetTime());
            try {
                boost::filesystem::rename(pathDatabase, pathDatabaseBak);
                LogPrintf("Credits: Moved old %s to %s. Retrying.\n", pathDatabase.string(), pathDatabaseBak.string());
            } catch(boost::filesystem::filesystem_error &error) {
                 // failure is ok (well, not really, but it's not worse than what we started with)
            }

            // try again
            if (!bitcredit_bitdb.Open(GetDataDir())) {
                // if it still fails, it probably means we can't even create the database env
                string msg = strprintf(_("Credits: Error initializing wallet database environment %s!"), strDataDir);
                return InitError(msg);
            }
        }

        if (GetBoolArg("-salvagewallet", false))
        {
            // Recover readable keypairs:
            if (!Credits_CWalletDB::Recover(bitcredit_bitdb, bitcredit_strWalletFile, true, bitcredit_nAccountingEntryNumber))
                return false;
        }

        if (filesystem::exists(GetDataDir() / bitcredit_strWalletFile))
        {
            Credits_CDBEnv::VerifyResult r = bitcredit_bitdb.Verify(bitcredit_strWalletFile, bitcredit_nAccountingEntryNumber, Credits_CWalletDB::Recover);
            if (r == Credits_CDBEnv::RECOVER_OK)
            {
                string msg = strprintf(_("Credits: Warning: credits_wallet.dat corrupt, data salvaged!"
                                         " Original credits_wallet.dat saved as credits_wallet.{timestamp}.bak in %s; if"
                                         " your balance or transactions are incorrect you should"
                                         " restore from a backup."), strDataDir);
                InitWarning(msg);
            }
            if (r == Credits_CDBEnv::RECOVER_FAIL)
                return InitError(_("credits_wallet.dat corrupt, salvage failed"));
        }
    } // (!fDisableWallet)
#endif // ENABLE_WALLET

    // ********************************************************* Step 6: network initialization

    if(!InitNodeSignals(bitcoin_netParams, "-bitcoin_socks", "-bitcoin_onlynet", "-bitcoin_proxy", "-bitcoin_tor", "-bitcoin_onion")) {
    	return false;
    }
    if(!InitNodeSignals(bitcredit_netParams, "-socks", "-onlynet", "-proxy", "-tor", "-onion")) {
    	return false;
    }

    // see Step 2: parameter interactions for more information about these
    bitcredit_netParams->fListen = GetBoolArg("-listen", true);
    bitcoin_netParams->fListen = GetBoolArg("-bitcoin_listen", true);
    fNameLookup = GetBoolArg("-bitcoin_dns", true);

    if(!InitPortBinding(bitcoin_netParams, "-bitcoin_discover", "-bitcoin_bind", "-bitcoin_externalip", "bitcoin_seednode")) {
    	return false;
    }
    if(!InitPortBinding(bitcredit_netParams, "-discover", "-bind", "-externalip", "seednode")) {
    	return false;
    }

    // ********************************************************* Step 7: load block chain

    bitcoin_mainState.fReindex = GetBoolArg("-reindex", false);
    InitDataDir("bitcoin_blocks");
    credits_mainState.fReindex = GetBoolArg("-reindex", false);
    InitDataDir("credits_blocks");

    // cache size calculations
    if(!Bitcoin_InitDbAndCache(nStart)) {
    	return false;
    }
    // As LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown)
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }
    LogPrintf("bitcoin block index %15dms\n", GetTimeMillis() - nStart);
    // cache size calculations
    if(!Bitcredit_InitDbAndCache(nStart)) {
    	return false;
    }
    // As LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown)
    {
        LogPrintf("Shutdown requested. Exiting.\n");
        return false;
    }
    LogPrintf("credits block index %15dms\n", GetTimeMillis() - nStart);

    if (GetBoolArg("-bitcoin_printblockindex", false) || GetBoolArg("-bitcoin_printblocktree", false))
    {
        Bitcoin_PrintBlockTree();
        return false;
    }
    if (GetBoolArg("-printblockindex", false) || GetBoolArg("-printblocktree", false))
    {
        Bitcredit_PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-bitcoin_printblock"))
    {
        string strMatch = mapArgs["-bitcoin_printblock"];
        int nFound = 0;
        for (map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.begin(); mi != bitcoin_mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (boost::algorithm::starts_with(hash.ToString(), strMatch))
            {
                Bitcoin_CBlockIndex* pindex = (*mi).second;
                Bitcoin_CBlock block;
                Bitcoin_ReadBlockFromDisk(block, pindex);
                block.BuildMerkleTree();
                block.print();
                LogPrintf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            LogPrintf("No blocks matching %s were found\n", strMatch);
        return false;
    }
    if (mapArgs.count("-printblock"))
    {
        string strMatch = mapArgs["-printblock"];
        int nFound = 0;
        for (map<uint256, Credits_CBlockIndex*>::iterator mi = credits_mapBlockIndex.begin(); mi != credits_mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (boost::algorithm::starts_with(hash.ToString(), strMatch))
            {
                Credits_CBlockIndex* pindex = (*mi).second;
                Credits_CBlock block;
                Credits_ReadBlockFromDisk(block, pindex);
                block.BuildMerkleTree();
                block.BuildSigMerkleTree();
                block.print();
                LogPrintf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            LogPrintf("No blocks matching %s were found\n", strMatch);
        return false;
    }

    // ********************************************************* Step 8: load wallet
#ifdef ENABLE_WALLET
    if (fDisableWallet) {
        bitcoin_pwalletMain = NULL;
        LogPrintf("Bitcoin: Wallet disabled!\n");
    } else {
        if (GetBoolArg("-zapwallettxes", false)) {
            uiInterface.InitMessage(_("Zapping all transactions from wallet..."));

            bitcoin_pwalletMain = new Bitcoin_CWallet(bitcoin_strWalletFile);
            Bitcoin_DBErrors nZapWalletRet = bitcoin_pwalletMain->ZapWalletTx();
            if (nZapWalletRet != BITCOIN_DB_LOAD_OK) {
                uiInterface.InitMessage(_("Error loading bitcoin_wallet.dat: Wallet corrupted"));
                return false;
            }

            delete bitcoin_pwalletMain;
            bitcoin_pwalletMain = NULL;
        }

        uiInterface.InitMessage(_("Loading bitcoin wallet..."));

        nStart = GetTimeMillis();
        bool fFirstRun = true;
        bitcoin_pwalletMain = new Bitcoin_CWallet(bitcoin_strWalletFile);
        Bitcoin_DBErrors nLoadWalletRet = bitcoin_pwalletMain->LoadWallet(fFirstRun);
        if (nLoadWalletRet != BITCOIN_DB_LOAD_OK)
        {
            if (nLoadWalletRet == BITCOIN_DB_CORRUPT)
                strErrors << _("Error loading bitcoin_wallet.dat: Wallet corrupted") << "\n";
            else if (nLoadWalletRet == BITCOIN_DB_NONCRITICAL_ERROR)
            {
                string msg(_("Warning: error reading bitcoin_wallet.dat! All keys read correctly, but transaction data"
                             " or address book entries might be missing or incorrect."));
                InitWarning(msg);
            }
            else if (nLoadWalletRet == BITCOIN_DB_TOO_NEW)
                strErrors << _("Error loading bitcoin_wallet.dat: Wallet requires newer version of Bitcoin") << "\n";
            else if (nLoadWalletRet == BITCOIN_DB_NEED_REWRITE)
            {
                strErrors << _("Bitcoin wallet needed to be rewritten: restart to complete") << "\n";
                LogPrintf("%s", strErrors.str());
                return InitError(strErrors.str());
            }
            else
                strErrors << _("Error loading bitcoin_wallet.dat") << "\n";
        }

        if (GetBoolArg("-upgradewallet", fFirstRun))
        {
            int nMaxVersion = GetArg("-upgradewallet", 0);
            if (nMaxVersion == 0) // the -upgradewallet without argument case
            {
                LogPrintf("Performing bitcoin wallet upgrade to %i\n", BITCOIN_FEATURE_LATEST);
                nMaxVersion = Bitcoin_Params().ClientVersion();
                bitcoin_pwalletMain->SetMinVersion(BITCOIN_FEATURE_LATEST); // permanently upgrade the wallet immediately
            }
            else
                LogPrintf("Allowing bitcoin wallet upgrade up to %i\n", nMaxVersion);
            if (nMaxVersion < bitcoin_pwalletMain->GetVersion())
                strErrors << _("Cannot downgrade bitcoin wallet") << "\n";
            bitcoin_pwalletMain->SetMaxVersion(nMaxVersion);
        }

        if (fFirstRun)
        {
            // Create new keyUser and set as default key
            RandAddSeedPerfmon();

            CPubKey newDefaultKey;
            if (bitcoin_pwalletMain->GetKeyFromPool(newDefaultKey)) {
                bitcoin_pwalletMain->SetDefaultKey(newDefaultKey);
                if (!bitcoin_pwalletMain->SetAddressBook(bitcoin_pwalletMain->vchDefaultKey.GetID(), "", "receive"))
                    strErrors << _("Cannot write bitcoin default address") << "\n";
            }

            bitcoin_pwalletMain->SetBestChain(bitcoin_chainActive.GetLocator());
        }

        LogPrintf("%s", strErrors.str());
        LogPrintf("bitcoin wallet      %15dms\n", GetTimeMillis() - nStart);

        Bitcoin_RegisterWallet(bitcoin_pwalletMain);

        Bitcoin_CBlockIndex *pindexRescan = (Bitcoin_CBlockIndex *)bitcoin_chainActive.Tip();
        if (GetBoolArg("-bitcoin_rescan", false))
            pindexRescan = bitcoin_chainActive.Genesis();
        else
        {
            Bitcoin_CWalletDB walletdb(bitcoin_strWalletFile);
            CBlockLocator locator;
            if (walletdb.ReadBestBlock(locator))
                pindexRescan = bitcoin_chainActive.FindFork(locator);
            else
                pindexRescan = bitcoin_chainActive.Genesis();
        }
        if (bitcoin_chainActive.Tip() && bitcoin_chainActive.Tip() != pindexRescan)
        {
            uiInterface.InitMessage(_("Rescanning bitcoin wallet..."));
            LogPrintf("Bitcoin: Rescanning last %i blocks (from block %i)...\n", bitcoin_chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);
            nStart = GetTimeMillis();
            bitcoin_pwalletMain->ScanForWalletTransactions(pindexRescan, true);
            LogPrintf("bitcoin rescan      %15dms\n", GetTimeMillis() - nStart);
            bitcoin_pwalletMain->SetBestChain(bitcoin_chainActive.GetLocator());
            bitcoin_nWalletDBUpdated++;
        }
    } // (!fDisableWallet)
#else // ENABLE_WALLET
    LogPrintf("No bitcoin wallet compiled in!\n");
#endif // !ENABLE_WALLET

#ifdef ENABLE_WALLET
    if (fDisableWallet) {
        deposit_pwalletMain = NULL;
        LogPrintf("Deposit wallet disabled!\n");
    } else {
        if (GetBoolArg("-zapwallettxes", false)) {
            uiInterface.InitMessage(_("Zapping all transactions from deposit wallet..."));

            deposit_pwalletMain = new Credits_CWallet(deposit_strWalletFile, &deposit_bitdb);
            Credits_DBErrors nZapWalletRet = deposit_pwalletMain->ZapWalletTx();
            if (nZapWalletRet != CREDITS_DB_LOAD_OK) {
                uiInterface.InitMessage(_("Error loading deposit_wallet.dat: Wallet corrupted"));
                return false;
            }

            delete deposit_pwalletMain;
            deposit_pwalletMain = NULL;
        }

        uiInterface.InitMessage(_("Loading deposit wallet..."));

        nStart = GetTimeMillis();
        bool fFirstRun = true;
        deposit_pwalletMain = new Credits_CWallet(deposit_strWalletFile, &deposit_bitdb);
        Credits_DBErrors nLoadWalletRet = deposit_pwalletMain->LoadWallet(fFirstRun, deposit_nAccountingEntryNumber);
        if (nLoadWalletRet != CREDITS_DB_LOAD_OK)
        {
            if (nLoadWalletRet == BITCREDIT_DB_CORRUPT)
                strErrors << _("Error loading deposit_wallet.dat: Wallet corrupted") << "\n";
            else if (nLoadWalletRet == BITCREDIT_DB_NONCRITICAL_ERROR)
            {
                string msg(_("Warning: error reading deposit_wallet.dat! All keys read correctly, but transaction data"
                             " or address book entries might be missing or incorrect."));
                InitWarning(msg);
            }
            else if (nLoadWalletRet == BITCREDIT_DB_TOO_NEW)
                strErrors << _("Error loading deposit_wallet.dat: Wallet requires newer version of Bitcredit") << "\n";
            else if (nLoadWalletRet == CREDITS_DB_NEED_REWRITE)
            {
                strErrors << _("Deposit wallet needed to be rewritten: restart Credits to complete") << "\n";
                LogPrintf("%s", strErrors.str());
                return InitError(strErrors.str());
            }
            else
                strErrors << _("Error loading deposit_wallet.dat") << "\n";
        }

        if (GetBoolArg("-upgradewallet", fFirstRun))
        {
            int nMaxVersion = GetArg("-upgradewallet", 0);
            if (nMaxVersion == 0) // the -upgradewallet without argument case
            {
                LogPrintf("Performing deposit wallet upgrade to %i\n", CREDITS_FEATURE_LATEST);
                nMaxVersion = CREDITS_CLIENT_VERSION;
                deposit_pwalletMain->SetMinVersion(CREDITS_FEATURE_LATEST); // permanently upgrade the wallet immediately
            }
            else
                LogPrintf("Allowing deposit wallet upgrade up to %i\n", nMaxVersion);
            if (nMaxVersion < deposit_pwalletMain->GetVersion())
                strErrors << _("Cannot downgrade deposit wallet") << "\n";
            deposit_pwalletMain->SetMaxVersion(nMaxVersion);
        }

        if (fFirstRun)
        {
            // Create new keyUser and set as default key
            RandAddSeedPerfmon();

            CPubKey newDefaultKey;
            if (deposit_pwalletMain->GetKeyFromPool(newDefaultKey)) {
                deposit_pwalletMain->SetDefaultKey(newDefaultKey);
                if (!deposit_pwalletMain->SetAddressBook(deposit_pwalletMain->vchDefaultKey.GetID(), "", "receive"))
                    strErrors << _("Cannot write deposit default address") << "\n";
            }

//            deposit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
        }

        LogPrintf("%s", strErrors.str());
        LogPrintf("deposit wallet      %15dms\n", GetTimeMillis() - nStart);

        //Deposit wallet should not be involved in the chain
//        Bitcredit_RegisterWallet(deposit_pwalletMain);
//
//        Credits_CBlockIndex *pindexRescan = (Credits_CBlockIndex *)credits_chainActive.Tip();
//        if (GetBoolArg("-bitcredit_rescan", false))
//            pindexRescan = credits_chainActive.Genesis();
//        else
//        {
//            Credits_CWalletDB walletdb(deposit_strWalletFile, &deposit_bitdb);
//            CBlockLocator locator;
//            if (walletdb.ReadBestBlock(locator))
//                pindexRescan = credits_chainActive.FindFork(locator);
//            else
//                pindexRescan = credits_chainActive.Genesis();
//        }
//        if (credits_chainActive.Tip() && credits_chainActive.Tip() != pindexRescan)
//        {
//            uiInterface.InitMessage(_("Rescanning deposit wallet..."));
//            LogPrintf("Deposit: Rescanning last %i blocks (from block %i)...\n", credits_chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);
//            nStart = GetTimeMillis();
//            deposit_pwalletMain->ScanForWalletTransactions(bitcoin_pwalletMain, pindexRescan, true);
//            LogPrintf("deposit rescan      %15dms\n", GetTimeMillis() - nStart);
//            deposit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
//            deposit_bitdb.nWalletDBUpdated++;
//        }
    } // (!fDisableWallet)
#else // ENABLE_WALLET
    LogPrintf("No deposit wallet compiled in!\n");
#endif // !ENABLE_WALLET

#ifdef ENABLE_WALLET
    if (fDisableWallet) {
        bitcredit_pwalletMain = NULL;
        LogPrintf("Credits wallet disabled!\n");
    } else {
        if (GetBoolArg("-zapwallettxes", false)) {
            uiInterface.InitMessage(_("Zapping all transactions from credits wallet..."));

            bitcredit_pwalletMain = new Credits_CWallet(bitcredit_strWalletFile, &bitcredit_bitdb);
            Credits_DBErrors nZapWalletRet = bitcredit_pwalletMain->ZapWalletTx();
            if (nZapWalletRet != CREDITS_DB_LOAD_OK) {
                uiInterface.InitMessage(_("Error loading credits_wallet.dat: Wallet corrupted"));
                return false;
            }

            delete bitcredit_pwalletMain;
            bitcredit_pwalletMain = NULL;
        }

        uiInterface.InitMessage(_("Loading credits wallet..."));

        nStart = GetTimeMillis();
        bool fFirstRun = true;
        bitcredit_pwalletMain = new Credits_CWallet(bitcredit_strWalletFile, &bitcredit_bitdb);
        Credits_DBErrors nLoadWalletRet = bitcredit_pwalletMain->LoadWallet(fFirstRun, bitcredit_nAccountingEntryNumber);
        if (nLoadWalletRet != CREDITS_DB_LOAD_OK)
        {
            if (nLoadWalletRet == BITCREDIT_DB_CORRUPT)
                strErrors << _("Error loading credits_wallet.dat: Wallet corrupted") << "\n";
            else if (nLoadWalletRet == BITCREDIT_DB_NONCRITICAL_ERROR)
            {
                string msg(_("Warning: error reading credits_wallet.dat! All keys read correctly, but transaction data"
                             " or address book entries might be missing or incorrect."));
                InitWarning(msg);
            }
            else if (nLoadWalletRet == BITCREDIT_DB_TOO_NEW)
                strErrors << _("Error loading credits_wallet.dat: Wallet requires newer version of Bitcoin") << "\n";
            else if (nLoadWalletRet == CREDITS_DB_NEED_REWRITE)
            {
                strErrors << _("Credits wallet needed to be rewritten: restart Bitcoin to complete") << "\n";
                LogPrintf("%s", strErrors.str());
                return InitError(strErrors.str());
            }
            else
                strErrors << _("Error loading credits_wallet.dat") << "\n";
        }

        if (GetBoolArg("-upgradewallet", fFirstRun))
        {
            int nMaxVersion = GetArg("-upgradewallet", 0);
            if (nMaxVersion == 0) // the -upgradewallet without argument case
            {
                LogPrintf("Performing credits wallet upgrade to %i\n", CREDITS_FEATURE_LATEST);
                nMaxVersion = CREDITS_CLIENT_VERSION;
                bitcredit_pwalletMain->SetMinVersion(CREDITS_FEATURE_LATEST); // permanently upgrade the wallet immediately
            }
            else
                LogPrintf("Allowing credits wallet upgrade up to %i\n", nMaxVersion);
            if (nMaxVersion < bitcredit_pwalletMain->GetVersion())
                strErrors << _("Cannot downgrade credits wallet") << "\n";
            bitcredit_pwalletMain->SetMaxVersion(nMaxVersion);
        }

        if (fFirstRun)
        {
            // Create new keyUser and set as default key
            RandAddSeedPerfmon();

            CPubKey newDefaultKey;
            if (bitcredit_pwalletMain->GetKeyFromPool(newDefaultKey)) {
                bitcredit_pwalletMain->SetDefaultKey(newDefaultKey);
                if (!bitcredit_pwalletMain->SetAddressBook(bitcredit_pwalletMain->vchDefaultKey.GetID(), "", "receive"))
                    strErrors << _("Cannot write credits default address") << "\n";
            }

            bitcredit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
        }

        LogPrintf("%s", strErrors.str());
        LogPrintf("credits wallet      %15dms\n", GetTimeMillis() - nStart);

        Bitcredit_RegisterWallet(bitcredit_pwalletMain);

        Credits_CBlockIndex *pindexRescan = (Credits_CBlockIndex *)credits_chainActive.Tip();
        if (GetBoolArg("-bitcredit_rescan", false))
            pindexRescan = credits_chainActive.Genesis();
        else
        {
            Credits_CWalletDB walletdb(bitcredit_strWalletFile, &bitcredit_bitdb);
            CBlockLocator locator;
            if (walletdb.ReadBestBlock(locator))
                pindexRescan = credits_chainActive.FindFork(locator);
            else
                pindexRescan = credits_chainActive.Genesis();
        }
        if (credits_chainActive.Tip() && credits_chainActive.Tip() != pindexRescan)
        {
            uiInterface.InitMessage(_("Rescanning credits wallet..."));
            LogPrintf("Credits: Rescanning last %i blocks (from block %i)...\n", credits_chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);
            nStart = GetTimeMillis();
            bitcredit_pwalletMain->ScanForWalletTransactions(bitcoin_pwalletMain, pindexRescan, true);
            LogPrintf("credits rescan      %15dms\n", GetTimeMillis() - nStart);
            bitcredit_pwalletMain->SetBestChain(credits_chainActive.GetLocator());
            bitcredit_bitdb.nWalletDBUpdated++;
        }
    } // (!fDisableWallet)
#else // ENABLE_WALLET
    LogPrintf("No credits wallet compiled in!\n");
#endif // !ENABLE_WALLET


    // ********************************************************* Step 9: import blocks

    // scan for better chains in the block chain database, that are not yet connected in the active best chain
    CValidationState bitcoin_state;
    if (!Bitcoin_ActivateBestChain(bitcoin_state))
        strErrors << "Bitcoin: Failed to connect best block";
    CValidationState bitcredit_state;
    if (!Bitcredit_ActivateBestChain(bitcredit_state))
        strErrors << "Credits: Failed to connect best block";

    //Will trigger Bitcredit_ThreadImport once done
    threadGroup.create_thread(boost::bind(&Bitcoin_ThreadImport));

    // ********************************************************* Step 10: load peers

    uiInterface.InitMessage(_("Loading bitcoin addresses..."));
    InitPeersFromNetParams(GetTimeMillis(), Bitcoin_NetParams());

    uiInterface.InitMessage(_("Loading credits addresses..."));
    InitPeersFromNetParams(GetTimeMillis(), Credits_NetParams());

    // ********************************************************* Step 11: start node
    if (!Bitcoin_CheckDiskSpace())
        return false;
    if (!Bitcredit_CheckDiskSpace())
        return false;

    if (!strErrors.str().empty())
        return InitError(strErrors.str());

    RandAddSeedPerfmon();

    //// debug print
    LogPrintf("bitcoin mapBlockIndex.size() = %u\n",   bitcoin_mapBlockIndex.size());
    LogPrintf("bitcoin nBestHeight = %d\n",                   bitcoin_chainActive.Height());

    LogPrintf("credits mapBlockIndex.size() = %u\n",   credits_mapBlockIndex.size());
    LogPrintf("credits nBestHeight = %d\n",                   credits_chainActive.Height());

#ifdef ENABLE_WALLET
    LogPrintf("bitcoin setKeyPool.size() = %u\n",      bitcoin_pwalletMain ? bitcoin_pwalletMain->setKeyPool.size() : 0);
    LogPrintf("bitcoin mapWallet.size() = %u\n",       bitcoin_pwalletMain ? bitcoin_pwalletMain->mapWallet.size() : 0);
    LogPrintf("bitcoin mapAddressBook.size() = %u\n",  bitcoin_pwalletMain ? bitcoin_pwalletMain->mapAddressBook.size() : 0);
#endif

#ifdef ENABLE_WALLET
    LogPrintf("deposit setKeyPool.size() = %u\n",      deposit_pwalletMain ? deposit_pwalletMain->setKeyPool.size() : 0);
    LogPrintf("deposit mapWallet.size() = %u\n",       deposit_pwalletMain ? deposit_pwalletMain->mapWallet.size() : 0);
    LogPrintf("deposit mapAddressBook.size() = %u\n",  deposit_pwalletMain ? deposit_pwalletMain->mapAddressBook.size() : 0);
#endif

#ifdef ENABLE_WALLET
    LogPrintf("credits setKeyPool.size() = %u\n",      bitcredit_pwalletMain ? bitcredit_pwalletMain->setKeyPool.size() : 0);
    LogPrintf("credits mapWallet.size() = %u\n",       bitcredit_pwalletMain ? bitcredit_pwalletMain->mapWallet.size() : 0);
    LogPrintf("credits mapAddressBook.size() = %u\n",  bitcredit_pwalletMain ? bitcredit_pwalletMain->mapAddressBook.size() : 0);
#endif



    StartNode(threadGroup);

    bool coinbaseDepositDisabled = GetBoolArg("-coinbasedepositdisabled", false);
    // InitRPCMining is needed here so getwork/getblocktemplate in the GUI debug console works properly.
    InitRPCMining(coinbaseDepositDisabled);
    if (fServer)
        StartRPCThreads();

#ifdef ENABLE_WALLET
    // Generate coins in the background
    if (bitcredit_pwalletMain)
        GenerateBitcredits(GetBoolArg("-gen", false), bitcredit_pwalletMain, deposit_pwalletMain, GetArg("-genproclimit", -1), coinbaseDepositDisabled);
#endif

    // ********************************************************* Step 12: finished

    uiInterface.InitMessage(_("Done loading bitcredit"));

#ifdef ENABLE_WALLET
    if (bitcoin_pwalletMain) {
        // Add wallet transactions that aren't already in a block to mapTransactions
        bitcoin_pwalletMain->ReacceptWalletTransactions();

        // Run a thread to flush wallet periodically
        threadGroup.create_thread(boost::bind(&Bitcoin_ThreadFlushWalletDB, boost::ref(bitcoin_pwalletMain->strWalletFile)));
    }
#endif

#ifdef ENABLE_WALLET
    if (deposit_pwalletMain) {
        // Add wallet transactions that aren't already in a block to mapTransactions
//        deposit_pwalletMain->ReacceptWalletTransactions();

        // Run a thread to flush wallet periodically
        threadGroup.create_thread(boost::bind(&Bitcredit_ThreadFlushWalletDB, "deposit-wallet", boost::ref(*deposit_pwalletMain)));
    }
#endif

#ifdef ENABLE_WALLET
    if (bitcredit_pwalletMain) {
        // Add wallet transactions that aren't already in a block to mapTransactions
        bitcredit_pwalletMain->ReacceptWalletTransactions();

        // Run a thread to flush wallet periodically
        threadGroup.create_thread(boost::bind(&Bitcredit_ThreadFlushWalletDB, "bitcredit-wallet", boost::ref(*bitcredit_pwalletMain)));
    }
#endif

    return !fRequestShutdown;
}

/** Initialize bitcoin.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInit2(boost::thread_group& threadGroup) {
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
    {
        return InitError(strprintf("Error: Winsock library failed to start (WSAStartup returned error %d)", ret));
    }
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

#ifndef WIN32
    // Print stack trace and try to exit immediately on SIGSEGV
    struct sigaction sa_segv;
    sa_segv.sa_handler = HandleSIGSEGV;
    sigemptyset(&sa_segv.sa_mask);
    sa_segv.sa_flags = 0;
    sigaction(SIGSEGV, &sa_segv, NULL);
#endif

#if defined (__SVR4) && defined (__sun)
    // ignore SIGPIPE on Solaris
    signal(SIGPIPE, SIG_IGN);
#endif
#endif

    if(!Bitcredit_AppInit2(threadGroup)) {
    	return false;
    }
    return true;
}
