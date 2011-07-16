// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#include "headers.h"
#include "db.h"
#include "rpc.h"
#include "net.h"
#include "init.h"
#include "strlcpy.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <sstream>

using namespace std;
using namespace boost;
namespace fs = boost::filesystem;

static fs::path pathDataDir;
static fs::path pathConfigFile;
static fs::path pathPidFile;

option<string> helpOpt(
    "init", "show", "help", "", "",
    "[=allopts]", _("This help message or full option list"));

option<string> datadirOpt(
    "init", "show", "datadir", GetDefaultDataDir(),
    "=<dir>", _("Specify data directory"));

option<string> confOpt(
    "init", "show", "conf", "bitcoin.conf",
    "=<file>", _("Specify configuration file"));

option<bool> testnetOpt(
    "init", "show", "testnet", false, true,
    "", _("Use the test network"));

option<bool> debugOpt("init", "hide", "debug", false, true);

#ifdef GUI
option<bool> serverOpt(
    "init", "show", "server", false, true,
    "", _("Accept command line and JSON-RPC commands"));
#endif

#ifndef __WXMSW__
option<bool> daemonOpt(
    "init", "show", "daemon", false, true,
    "", _("Run in the background as a daemon and accept commands"));

option<string> pidfileOpt(
    "init", "hide", "pid", "bitcoin.pid",
    "=<file>", _("Specify pid file"));
#endif

option<bool> genOpt(
    "init", "hide", "gen", false, true,
    "", _("Generate coins"));

option<bool> rescanOpt(
    "wallet", "show", "rescan", false, true,
    "", _("Rescan the block chain for missing wallet transactions"));

option<money> paytxfeeOpt(
    "wallet", "show", "paytxfee", 0,
    "=<amt>", _("Fee per KB to add to transactions you send"));

#ifdef USE_UPNP
#if USE_UPNP
option<bool> upnpOpt(
    "net", "show", "noupnp", true, false,
    "", _("Don't attempt to use UPnP to map the listening port"));
#else
option<bool> upnpOpt(
    "net", "show", "upnp", false, true,
    "", _("Attempt to use UPnP to map the listening port"));
#endif
#endif

option<string> proxyOpt(
    "net", "show", "proxy", "",
    "=<ip:port>", _("Connect through socks4 proxy"));

option<bool> nodnsseedOpt("net", "hide", "nodnsseed", false, true);
option<bool> loadblockindextestOpt("init", "test", "loadblockindextest", false, true);
option<string> printblockOpt("init", "hide", "printblock", "");
option<bool> printblocktreeOpt("init", "hide", "printblocktree", false, true);
option<bool> printblockindexOpt("init", "hide", "printblockindex", false, true);

option<bool> printtoconsoleOpt("log", "hide", "printtoconsole", false, true);
option<bool> printtodebuggerOpt("log", "hide", "printtodebugger", false, true);
option<bool> logtimestampsOpt("log", "hide", "logtimestamps", false, true);



//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

void ExitTimeout(void* parg)
{
#ifdef __WXMSW__
    Sleep(5000);
    ExitProcess(0);
#endif
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;
    bool fFirstThread;
    CRITICAL_BLOCK(cs_Shutdown)
    {
        fFirstThread = !fTaken;
        fTaken = true;
    }
    static bool fExit;
    if (fFirstThread)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        DBFlush(false);
        StopNode();
        DBFlush(true);
        fs::remove(pathPidFile);
        UnregisterWallet(pwalletMain);
        delete pwalletMain;
        CreateThread(ExitTimeout, NULL);
        Sleep(50);
        printf("Bitcoin exiting\n\n");
        fExit = true;
        exit(0);
    }
    else
    {
        while (!fExit)
            Sleep(500);
        Sleep(100);
        ExitThread(0);
    }
}

void HandleSIGTERM(int)
{
    fRequestShutdown = true;
}



//////////////////////////////////////////////////////////////////////////////
//
// Help
//
void PrintHelp()
{
    std::ostringstream ss;
    if (helpOpt() == "")
    {
        ss << _("Bitcoin version") << " " << FormatFullVersion() << endl;
        ss << endl;
        ss << _("Usage:") << endl;
        ss << "  bitcoin [options]                     " << endl;
        ss << "  bitcoin [options] <command> [params]  "
           << _("Send command to -server or bitcoind") << endl;
        ss << "  bitcoin [options] help                "
           << _("List commands") << endl;
        ss << "  bitcoin [options] help <command>      "
           << _("Get help for a command") << endl;
        ss << endl;
        ss << _("Options:") << endl;
        ss << GetOptionsDescriptions("init", "show");
        ss << _("Wallet options:") << endl;
        ss << GetOptionsDescriptions("wallet", "show");
        ss << _("Network options:") << endl;
        ss << GetOptionsDescriptions("net", "show");
        ss << _("Server options (see the Bitcoin Wiki for SSL setup instructions):") << endl;
        ss << GetOptionsDescriptions("rpc", "show");
    }
    else if (helpOpt() == "allopts")
    {
        ss << _("All options:") << endl;
        ss << GetOptionsDescriptions("*", "show,hide", true);
    }
    else if (helpOpt() == "dumpopts")
    {
        ss << _("All options:") << endl;
        ss << GetOptionsDescriptions("*", "*", true);
    }
    else
    {
        ss << _("Options:") << endl;
        ss << GetOptionsDescriptions(helpOpt(), "show,hide", true);
    }

#if defined(__WXMSW__) && defined(GUI)
    wxMessageBox(ss.str(), "Bitcoin", wxOK);
#else
    cerr << ss.str();
#endif

}



//////////////////////////////////////////////////////////////////////////////
//
// Start
//
#ifndef GUI
int main(int argc, char* argv[])
{
    bool fRet = false;
    fRet = AppInit(argc, argv);

    if (fRet && fDaemon)
        return 0;

    return 1;
}
#endif

bool AppInit(int argc, char* argv[])
{
    bool fRet = false;
    try
    {
        fRet = AppInit2(argc, argv);
    }
    catch (std::exception& e) {
        PrintException(&e, "AppInit()");
    } catch (...) {
        PrintException(NULL, "AppInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

bool AppInit2(int argc, char* argv[])
{
    string strErrors;

#ifdef _MSC_VER
    // Turn off microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
    // Disable confusing "helpful" text message on abort, ctrl-c
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifndef __WXMSW__
    umask(077);
#endif
#ifndef __WXMSW__
    // Clean shutdown on SIGTERM
    struct sigaction sa;
    sa.sa_handler = HandleSIGTERM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
#endif

    //
    // Parameters
    //

    // Parse command line options
    if (!ParseCommandLine(argc, argv, strErrors))
    {
        cerr << strErrors << endl;
        exit(1);
    }

    if (+helpOpt)
    {
        PrintHelp();
        exit(0);
    }

    // Compute pathDataDir and pathConfigFile from command line
    pathDataDir = fs::system_complete(datadirOpt());
    pathConfigFile = confOpt();
    if (!pathConfigFile.is_complete())
        pathConfigFile = pathDataDir / pathConfigFile;

    strConfigFile = pathConfigFile.string();

    if (+confOpt && !fs::exists(pathConfigFile)) {
        strErrors += _("Error: Specified configuration file does not exist");
        wxMessageBox(strErrors, "Bitcoin");
        exit(1);
    }

    // Parse configuration file
    if (!UpdateConfig(strErrors))
    {
        wxMessageBox(strErrors, "Bitcoin");
        exit(1);
    }

    // Update datadir in case it was defined in config file
    if (+datadirOpt)
    {
        pathDataDir = fs::system_complete(datadirOpt());
        if (!fs::is_directory(pathDataDir))
        {
            strErrors += _("Error: Specified directory does not exist");
            wxMessageBox(strErrors, "Bitcoin");
            exit(1);
        }
    }

    if (!fs::exists(pathDataDir))
        fs::create_directory(pathDataDir);

    fTestNet = testnetOpt();
    if (fTestNet)
    {
        pathDataDir = pathDataDir / "testnet";
        if (!fs::exists(pathDataDir))
            fs::create_directory(pathDataDir);
    }

    strDataDir = pathDataDir.string();

    // Warnings
    if (nTransactionFee > 0.25 * COIN)
        wxMessageBox(_("Warning: -paytxfee is set very high.  This is the transaction fee you will pay if you send a transaction."), "Bitcoin", wxOK | wxICON_EXCLAMATION);


    // Configuration parameters listed below cannot be dynamicaly updated
    fDebug = debugOpt();

#ifndef __WXMSW__
    fDaemon = daemonOpt();
#else
    fDaemon = false;
#endif

#ifndef GUI
    fServer = true;
#else
    fServer = fDaemon || serverOpt();
#endif

    for (int i = 1; i < argc; i++)
        if (!IsSwitchChar(argv[i][0]))
            fCommandLine = true;

    if (fCommandLine)
    {
        int ret = CommandLineRPC(argc, argv);
        exit(ret);
    }

#ifndef __WXMSW__
    if (fDaemon)
    {
        // Daemonize
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Error: fork() errno %d\n", errno);
            return false;
        }
        if (pid > 0)
        {
            fs::path path = pidfileOpt();
            if (!path.is_complete())
                path =  pathDataDir / path;
            pathPidFile = path;
            fs::ofstream(pathPidFile) << pid << endl;;
            return true;
        }

        pid_t sid = setsid();
        if (sid < 0)
            fprintf(stderr, "Error: setsid() errno %d\n", errno);
    }
#endif

    if (!fDebug)
        ShrinkDebugFile();
    printf("\n\n\n\n\n");
    printf("Bitcoin version %s\n", FormatFullVersion().c_str());
#ifdef GUI
    printf("OS version %s\n", ((string)wxGetOsDescription()).c_str());
    printf("System default language is %d %s\n",
           g_locale.GetSystemLanguage(),
           ((string)g_locale.GetSysName()).c_str());
    printf("Language file %s (%s)\n",
           (string("locale/") + (string)g_locale.GetCanonicalName() +
            "/LC_MESSAGES/bitcoin.mo").c_str(),
           ((string)g_locale.GetLocale()).c_str());
#endif
    printf("Default data directory %s\n", GetDefaultDataDir().c_str());
    printf("Using data directory %s\n", strDataDir.c_str());

    if (loadblockindextestOpt())
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

    //
    // Make sure only a single bitcoin process is using the data directory.
    //
    fs::path pathLockFile = pathDataDir / ".lock";
    if (!fs::exists(pathLockFile))  // create empty .lock if it doesn't exist
        fs::ofstream(pathLockFile).close();
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock())
    {
        wxMessageBox(strprintf(_("Cannot obtain a lock on data directory %s.  Bitcoin is probably already running."), GetDataDir().c_str()), "Bitcoin");
        return false;
    }

    // Bind to the port early so we can tell if another instance is
    // already running.
    if (!BindListenPort(strErrors))
    {
        wxMessageBox(strErrors, "Bitcoin");
        return false;
    }

    //
    // Load data files
    //
    if (fDaemon)
        fprintf(stdout, "bitcoin server starting\n");
    strErrors = "";
    int64 nStart;

    printf("Loading addresses...\n");
    nStart = GetTimeMillis();
    if (!LoadAddresses())
        strErrors += string(_("Error loading")) + " addr.dat\n";
    printf(" addresses   %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        strErrors += string(_("Error loading")) + " blkindex.dat\n";
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun;
    pwalletMain = new CWallet("wallet.dat");
    int nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT)
            strErrors += string(_("Error loading")) + "wallet.dat" + ", " +
                _("Wallet corrupted") + "\n";
        else if (nLoadWalletRet == DB_TOO_NEW)
            strErrors += string(_("Error loading")) + "wallet.dat" + ", " +
                _("Wallet requires newer version of Bitcoin") + "\n";
        else
            strErrors += string(_("Error loading")) + " wallet.dat\n";
    }
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    RegisterWallet(pwalletMain);

    CBlockIndex *pindexRescan = pindexBest;
    if (rescanOpt())
        pindexRescan = pindexGenesisBlock;
    else
    {
        CWalletDB walletdb("wallet.dat");
        CBlockLocator locator;
        if (walletdb.ReadBestBlock(locator))
            pindexRescan = locator.GetBlockIndex();
    }
    if (pindexBest != pindexRescan)
    {
        printf("Rescanning last %i blocks (from block %i)...\n",
               pindexBest->nHeight - pindexRescan->nHeight,
               pindexRescan->nHeight);
        nStart = GetTimeMillis();
        pwalletMain->ScanForWalletTransactions(pindexRescan, true);
        printf(" rescan      %15"PRI64d"ms\n", GetTimeMillis() - nStart);
    }

    printf("Done loading\n");

        //// debug print
        printf("mapBlockIndex.size() = %d\n",   mapBlockIndex.size());
        printf("nBestHeight = %d\n",            nBestHeight);
        printf("setKeyPool.size() = %d\n",      pwalletMain->setKeyPool.size());
        printf("mapPubKeys.size() = %d\n",      mapPubKeys.size());
        printf("mapWallet.size() = %d\n",       pwalletMain->mapWallet.size());
        printf("mapAddressBook.size() = %d\n",  pwalletMain->mapAddressBook.size());

    if (!strErrors.empty())
    {
        wxMessageBox(strErrors, "Bitcoin", wxOK | wxICON_ERROR);
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    pwalletMain->ReacceptWalletTransactions();

    //
    // Parameters
    //
    if (printblockindexOpt() || printblocktreeOpt())
    {
        PrintBlockTree();
        return false;
    }

    if (+printblockOpt)
    {
        string strMatch = printblockOpt();
        int nFound = 0;
        for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
        {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0)
            {
                CBlockIndex* pindex = (*mi).second;
                CBlock block;
                block.ReadFromDisk(pindex);
                block.BuildMerkleTree();
                block.print();
                printf("\n");
                nFound++;
            }
        }
        if (nFound == 0)
            printf("No blocks matching %s were found\n", strMatch.c_str());
        return false;
    }

    if (+addnodeOpt)
    {
        bool fAllowDNS = dnsOpt();
        BOOST_FOREACH(string strAddr, addnodeOpt())
        {
            CAddress addr(strAddr, fAllowDNS);
            addr.nTime = 0; // so it won't relay unless successfully connected
            if (addr.IsValid())
                AddAddress(addr);
        }
    }

    if (nodnsseedOpt())
        printf("DNS seeding disabled\n");
    else
        DNSAddressSeed();



    //
    // Create the main window and start the node
    //
#ifdef GUI
    if (!fDaemon)
        CreateMainWindow();
#endif

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    if (!CreateThread(StartNode, NULL))
        wxMessageBox("Error: CreateThread(StartNode) failed", "Bitcoin");

    if (fServer)
        CreateThread(ThreadRPCServer, NULL);

#if defined(__WXMSW__) && defined(GUI)
    if (fFirstRun)
        SetStartOnSystemStartup(true);
#endif

#ifndef GUI
    while (1)
        Sleep(5000);
#endif

    return true;
}


bool UpdateConfig(string &strError)
{

    if (!ParseConfigFile(strError))
        return false;

    if (+proxyOpt)
    {
        CAddress address(proxyOpt());
        if (!address.IsValid())
        {
            strError += _("Invalid -proxy address");
            return false;
        }
        fUseProxy = true;
        addrProxy = address;
    }
    else
        fUseProxy = false;

    fPrintToConsole = printtoconsoleOpt();
    fPrintToDebugger = printtodebuggerOpt();
    fLogTimestamps = logtimestampsOpt();

    nTransactionFee = paytxfeeOpt();

    fGenerateBitcoins = genOpt();

#ifdef USE_UPNP
    fUseUPnP = fHaveUPnP && upnpOpt();
#endif

    return true;
}


