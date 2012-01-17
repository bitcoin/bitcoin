// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#include "headers.h"
#include "db.h"
#include "bitcoinrpc.h"
#include "net.h"
#include "init.h"
#include "strlcpy.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#if defined(BITCOIN_NEED_QT_PLUGINS) && !defined(_BITCOIN_QT_PLUGINS_INCLUDED)
#define _BITCOIN_QT_PLUGINS_INCLUDED
#define __INSURE__
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
#endif

using namespace std;
using namespace boost;

CWallet* pwalletMain;

//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

void ExitTimeout(void* parg)
{
#ifdef WIN32
    Sleep(5000);
    ExitProcess(0);
#endif
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;
    bool fFirstThread = false;
    TRY_CRITICAL_BLOCK(cs_Shutdown)
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
        boost::filesystem::remove(GetPidFile());
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
// Start
//
#if !defined(QT_GUI)
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
#ifdef _MSC_VER
    // Turn off microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
    // Disable confusing "helpful" text message on abort, ctrl-c
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifndef WIN32
    umask(077);
#endif
#ifndef WIN32
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
    // If Qt is used, parameters are parsed in qt/bitcoin.cpp's main()
#if !defined(QT_GUI)
    ParseParameters(argc, argv);
#endif

    if (mapArgs.count("-datadir"))
    {
        if (filesystem::is_directory(filesystem::system_complete(mapArgs["-datadir"])))
        {
            filesystem::path pathDataDir = filesystem::system_complete(mapArgs["-datadir"]);
            strlcpy(pszSetDataDir, pathDataDir.string().c_str(), sizeof(pszSetDataDir));
        }
        else
        {
            fprintf(stderr, "Error: Specified directory does not exist\n");
            Shutdown(NULL);
        }
    }


    ReadConfigFile(mapArgs, mapMultiArgs); // Must be done after processing datadir

    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        string strUsage = string() +
          _("Bitcoin version") + " " + FormatFullVersion() + "\n\n" +
          _("Usage:") + "\t\t\t\t\t\t\t\t\t\t\n" +
            "  bitcoind [options]                   \t  " + "\n" +
            "  bitcoind [options] <command> [params]\t  " + _("Send command to -server or bitcoind") + "\n" +
            "  bitcoind [options] help              \t\t  " + _("List commands") + "\n" +
            "  bitcoind [options] help <command>    \t\t  " + _("Get help for a command") + "\n" +
          _("Options:") + "\n" +
            "  -conf=<file>     \t\t  " + _("Specify configuration file (default: bitcoin.conf)") + "\n" +
            "  -pid=<file>      \t\t  " + _("Specify pid file (default: bitcoind.pid)") + "\n" +
            "  -gen             \t\t  " + _("Generate coins") + "\n" +
            "  -gen=0           \t\t  " + _("Don't generate coins") + "\n" +
            "  -min             \t\t  " + _("Start minimized") + "\n" +
            "  -datadir=<dir>   \t\t  " + _("Specify data directory") + "\n" +
            "  -timeout=<n>     \t  "   + _("Specify connection timeout (in milliseconds)") + "\n" +
            "  -proxy=<ip:port> \t  "   + _("Connect through socks4 proxy") + "\n" +
            "  -dns             \t  "   + _("Allow DNS lookups for addnode and connect") + "\n" +
            "  -port=<port>     \t\t  " + _("Listen for connections on <port> (default: 8333 or testnet: 18333)") + "\n" +
            "  -maxconnections=<n>\t  " + _("Maintain at most <n> connections to peers (default: 125)") + "\n" +
            "  -addnode=<ip>    \t  "   + _("Add a node to connect to and attempt to keep the connection open") + "\n" +
            "  -connect=<ip>    \t\t  " + _("Connect only to the specified node") + "\n" +
            "  -noirc           \t  "   + _("Don't find peers using internet relay chat") + "\n" +
            "  -nolisten        \t  "   + _("Don't accept connections from outside") + "\n" +
            "  -nodnsseed       \t  "   + _("Don't bootstrap list of peers using DNS") + "\n" +
            "  -banscore=<n>    \t  "   + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n" +
            "  -bantime=<n>     \t  "   + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n" +
            "  -maxreceivebuffer=<n>\t  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 10000)") + "\n" +
            "  -maxsendbuffer=<n>\t  "   + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 10000)") + "\n" +
#ifdef USE_UPNP
#if USE_UPNP
            "  -noupnp          \t  "   + _("Don't attempt to use UPnP to map the listening port") + "\n" +
#else
            "  -upnp            \t  "   + _("Attempt to use UPnP to map the listening port") + "\n" +
#endif
#endif
            "  -paytxfee=<amt>  \t  "   + _("Fee per KB to add to transactions you send") + "\n" +
#ifdef GUI
            "  -server          \t\t  " + _("Accept command line and JSON-RPC commands") + "\n" +
#endif
#ifndef WIN32
            "  -daemon          \t\t  " + _("Run in the background as a daemon and accept commands") + "\n" +
#endif
            "  -testnet         \t\t  " + _("Use the test network") + "\n" +
            "  -debug           \t\t  " + _("Output extra debugging information") + "\n" +
            "  -logtimestamps   \t  "   + _("Prepend debug output with timestamp") + "\n" +
            "  -printtoconsole  \t  "   + _("Send trace/debug info to console instead of debug.log file") + "\n" +
#ifdef WIN32
            "  -printtodebugger \t  "   + _("Send trace/debug info to debugger") + "\n" +
#endif
            "  -rpcuser=<user>  \t  "   + _("Username for JSON-RPC connections") + "\n" +
            "  -rpcpassword=<pw>\t  "   + _("Password for JSON-RPC connections") + "\n" +
            "  -rpcport=<port>  \t\t  " + _("Listen for JSON-RPC connections on <port> (default: 8332)") + "\n" +
            "  -rpcallowip=<ip> \t\t  " + _("Allow JSON-RPC connections from specified IP address") + "\n" +
            "  -rpcconnect=<ip> \t  "   + _("Send commands to node running on <ip> (default: 127.0.0.1)") + "\n" +
            "  -blocknotify=<cmd> "     + _("Execute command when the best block changes (%s in cmd is replaced by block hash)") + "\n" +
            "  -keypool=<n>     \t  "   + _("Set key pool size to <n> (default: 100)") + "\n" +
            "  -rescan          \t  "   + _("Rescan the block chain for missing wallet transactions") + "\n";

#ifdef USE_SSL
        strUsage += string() +
            _("\nSSL options: (see the Bitcoin Wiki for SSL setup instructions)") + "\n" +
            "  -rpcssl                                \t  " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n" +
            "  -rpcsslcertificatechainfile=<file.cert>\t  " + _("Server certificate file (default: server.cert)") + "\n" +
            "  -rpcsslprivatekeyfile=<file.pem>       \t  " + _("Server private key (default: server.pem)") + "\n" +
            "  -rpcsslciphers=<ciphers>               \t  " + _("Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)") + "\n";
#endif

        strUsage += string() +
            "  -?               \t\t  " + _("This help message") + "\n";

        // Remove tabs
        strUsage.erase(std::remove(strUsage.begin(), strUsage.end(), '\t'), strUsage.end());
        fprintf(stderr, "%s", strUsage.c_str());
        return false;
    }

    fTestNet = GetBoolArg("-testnet");
    fDebug = GetBoolArg("-debug");

#ifndef WIN32
    fDaemon = GetBoolArg("-daemon");
#else
    fDaemon = false;
#endif

    if (fDaemon)
        fServer = true;
    else
        fServer = GetBoolArg("-server");

    /* force fServer when running without GUI */
#if !defined(QT_GUI)
    fServer = true;
#endif
    fPrintToConsole = GetBoolArg("-printtoconsole");
    fPrintToDebugger = GetBoolArg("-printtodebugger");
    fLogTimestamps = GetBoolArg("-logtimestamps");

#ifndef QT_GUI
    for (int i = 1; i < argc; i++)
        if (!IsSwitchChar(argv[i][0]))
            fCommandLine = true;

    if (fCommandLine)
    {
        int ret = CommandLineRPC(argc, argv);
        exit(ret);
    }
#endif

#ifndef WIN32
    if (fDaemon)
    {
        // Daemonize
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
            return false;
        }
        if (pid > 0)
        {
            CreatePidFile(GetPidFile(), pid);
            return true;
        }

        pid_t sid = setsid();
        if (sid < 0)
            fprintf(stderr, "Error: setsid() returned %d errno %d\n", sid, errno);
    }
#endif

    if (!fDebug && !pszSetDataDir[0])
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Bitcoin version %s\n", FormatFullVersion().c_str());
    printf("Default data directory %s\n", GetDefaultDataDir().c_str());

    if (GetBoolArg("-loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

    // Make sure only a single bitcoin process is using the data directory.
    string strLockFile = GetDataDir() + "/.lock";
    FILE* file = fopen(strLockFile.c_str(), "a"); // empty lock file; created if it doesn't exist.
    if (file) fclose(file);
    static boost::interprocess::file_lock lock(strLockFile.c_str());
    if (!lock.try_lock())
    {
        wxMessageBox(strprintf(_("Cannot obtain a lock on data directory %s.  Bitcoin is probably already running."), GetDataDir().c_str()), "Bitcoin");
        return false;
    }

    std::ostringstream strErrors;
    //
    // Load data files
    //
    if (fDaemon)
        fprintf(stdout, "bitcoin server starting\n");
    int64 nStart;

    InitMessage(_("Loading addresses..."));
    printf("Loading addresses...\n");
    nStart = GetTimeMillis();
    if (!LoadAddresses())
        strErrors << _("Error loading addr.dat") << "\n";
    printf(" addresses   %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    InitMessage(_("Loading block index..."));
    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        strErrors << _("Error loading blkindex.dat") << "\n";
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    InitMessage(_("Loading wallet..."));
    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun;
    pwalletMain = new CWallet("wallet.dat");
    int nLoadWalletRet = pwalletMain->LoadWallet(fFirstRun);
    if (nLoadWalletRet != DB_LOAD_OK)
    {
        if (nLoadWalletRet == DB_CORRUPT)
            strErrors << _("Error loading wallet.dat: Wallet corrupted") << "\n";
        else if (nLoadWalletRet == DB_TOO_NEW)
            strErrors << _("Error loading wallet.dat: Wallet requires newer version of Bitcoin") << "\n";
        else if (nLoadWalletRet == DB_NEED_REWRITE)
        {
            strErrors << _("Wallet needed to be rewritten: restart Bitcoin to complete") << "\n";
            wxMessageBox(strErrors.str(), "Bitcoin", wxOK | wxICON_ERROR);
            return false;
        }
        else
            strErrors << _("Error loading wallet.dat") << "\n";
    }
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    RegisterWallet(pwalletMain);

    CBlockIndex *pindexRescan = pindexBest;
    if (GetBoolArg("-rescan"))
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
        InitMessage(_("Rescanning..."));
        printf("Rescanning last %i blocks (from block %i)...\n", pindexBest->nHeight - pindexRescan->nHeight, pindexRescan->nHeight);
        nStart = GetTimeMillis();
        pwalletMain->ScanForWalletTransactions(pindexRescan, true);
        printf(" rescan      %15"PRI64d"ms\n", GetTimeMillis() - nStart);
    }

    InitMessage(_("Done loading"));
    printf("Done loading\n");

    //// debug print
    printf("mapBlockIndex.size() = %d\n",   mapBlockIndex.size());
    printf("nBestHeight = %d\n",            nBestHeight);
    printf("setKeyPool.size() = %d\n",      pwalletMain->setKeyPool.size());
    printf("mapWallet.size() = %d\n",       pwalletMain->mapWallet.size());
    printf("mapAddressBook.size() = %d\n",  pwalletMain->mapAddressBook.size());

    if (!strErrors.str().empty())
    {
        wxMessageBox(strErrors.str(), "Bitcoin", wxOK | wxICON_ERROR);
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    pwalletMain->ReacceptWalletTransactions();

    // Note: Bitcoin-QT stores several settings in the wallet, so we want
    // to load the wallet BEFORE parsing command-line arguments, so
    // the command-line/bitcoin.conf settings override GUI setting.

    //
    // Parameters
    //
    if (GetBoolArg("-printblockindex") || GetBoolArg("-printblocktree"))
    {
        PrintBlockTree();
        return false;
    }

    if (mapArgs.count("-timeout"))
    {
        int nNewTimeout = GetArg("-timeout", 5000);
        if (nNewTimeout > 0 && nNewTimeout < 600000)
            nConnectTimeout = nNewTimeout;
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

    fGenerateBitcoins = GetBoolArg("-gen");

    if (mapArgs.count("-proxy"))
    {
        fUseProxy = true;
        addrProxy = CService(mapArgs["-proxy"], 9050);
        if (!addrProxy.IsValid())
        {
            wxMessageBox(_("Invalid -proxy address"), "Bitcoin");
            return false;
        }
    }

    bool fTor = (fUseProxy && addrProxy.GetPort() == 9050);
    if (fTor)
    {
        // Use SoftSetArg here so user can override any of these if they wish.
        // Note: the GetBoolArg() calls for all of these must happen later.
        SoftSetArg("-nolisten", true);
        SoftSetArg("-noirc", true);
        SoftSetArg("-nodnsseed", true);
        SoftSetArg("-noupnp", true);
        SoftSetArg("-upnp", false);
        SoftSetArg("-dns", false);
    }

    fAllowDNS = GetBoolArg("-dns");
    fNoListen = GetBoolArg("-nolisten");

    if (fHaveUPnP)
    {
#if USE_UPNP
    if (GetBoolArg("-noupnp"))
        fUseUPnP = false;
#else
    if (GetBoolArg("-upnp"))
        fUseUPnP = true;
#endif
    }

    if (!fNoListen)
    {
        std::string strError;
        if (!BindListenPort(strError))
        {
            wxMessageBox(strError, "Bitcoin");
            return false;
        }
    }

    if (mapArgs.count("-addnode"))
    {
        BOOST_FOREACH(string strAddr, mapMultiArgs["-addnode"])
        {
            CAddress addr(CService(strAddr, GetDefaultPort(), fAllowDNS));
            addr.nTime = 0; // so it won't relay unless successfully connected
            if (addr.IsValid())
                AddAddress(addr);
        }
    }

    if (mapArgs.count("-paytxfee"))
    {
        if (!ParseMoney(mapArgs["-paytxfee"], nTransactionFee))
        {
            wxMessageBox(_("Invalid amount for -paytxfee=<amount>"), "Bitcoin");
            return false;
        }
        if (nTransactionFee > 0.25 * COIN)
            wxMessageBox(_("Warning: -paytxfee is set very high.  This is the transaction fee you will pay if you send a transaction."), "Bitcoin", wxOK | wxICON_EXCLAMATION);
    }

    //
    // Start the node
    //
    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    if (!CreateThread(StartNode, NULL))
        wxMessageBox(_("Error: CreateThread(StartNode) failed"), "Bitcoin");

    if (fServer)
        CreateThread(ThreadRPCServer, NULL);

#ifdef QT_GUI
    if(GetStartOnSystemStartup())
        SetStartOnSystemStartup(true); // Remove startup links to bitcoin-wx
#endif

#if !defined(QT_GUI)
    while (1)
        Sleep(5000);
#endif

    return true;
}

#ifdef WIN32
string StartupShortcutPath()
{
    return MyGetSpecialFolderPath(CSIDL_STARTUP, true) + "\\Bitcoin.lnk";
}

bool GetStartOnSystemStartup()
{
    return filesystem::exists(StartupShortcutPath().c_str());
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    remove(StartupShortcutPath().c_str());

    if (fAutoStart)
    {
        CoInitialize(NULL);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                                CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            TCHAR pszExePath[MAX_PATH];
            GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

            TCHAR pszArgs[5] = TEXT("-min");

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(pszArgs);

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = NULL;
            hres = psl->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}

#elif defined(LINUX)

// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

boost::filesystem::path GetAutostartDir()
{
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / fs::path("autostart");
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / fs::path(".config/autostart");
    return fs::path();
}

boost::filesystem::path GetAutostartFilePath()
{
    return GetAutostartDir() / boost::filesystem::path("bitcoin.desktop");
}

bool GetStartOnSystemStartup()
{
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != string::npos &&
            line.find("true") != string::npos)
            return false;
    }
    optionFile.close();

    return true;
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
    {
#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3
        unlink(GetAutostartFilePath().string().c_str());
#else
        unlink(GetAutostartFilePath().native_file_string().c_str());
#endif
    }
    else
    {
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
            return false;

        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), ios_base::out|ios_base::trunc);
        if (!optionFile.good())
            return false;
        // Write a bitcoin.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=Bitcoin\n";
        optionFile << "Exec=" << pszExePath << " -min\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}
#else

// TODO: OSX startup stuff; see:
// http://developer.apple.com/mac/library/documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif
