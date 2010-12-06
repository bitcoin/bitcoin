// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"







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

#ifndef GUI
int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
        if (!IsSwitchChar(argv[i][0]))
            fCommandLine = true;
    fDaemon = !fCommandLine;

#ifdef __WXGTK__
    if (!fCommandLine)
    {
        // Daemonize
        pid_t pid = fork();
        if (pid < 0)
        {
            fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
            return 1;
        }
        if (pid > 0)
            pthread_exit((void*)0);
    }
#endif

    if (!AppInit(argc, argv))
        return 1;

    while (!fShutdown)
        Sleep(1000000);
    return 0;
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
#endif

    //
    // Parameters
    //
    ParseParameters(argc, argv);

    if (mapArgs.count("-datadir"))
    {
        filesystem::path pathDataDir = filesystem::system_complete(mapArgs["-datadir"]);
        strlcpy(pszSetDataDir, pathDataDir.string().c_str(), sizeof(pszSetDataDir));
    }

    ReadConfigFile(mapArgs, mapMultiArgs); // Must be done after processing datadir

    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        string strUsage = string() +
          _("Usage:") + "\t\t\t\t\t\t\t\t\t\t\n" +
            "  bitcoin [options]                   \t  " + "\n" +
            "  bitcoin [options] <command> [params]\t  " + _("Send command to -server or bitcoind\n") +
            "  bitcoin [options] help              \t\t  " + _("List commands\n") +
            "  bitcoin [options] help <command>    \t\t  " + _("Get help for a command\n") +
          _("Options:\n") +
            "  -conf=<file>     \t\t  " + _("Specify configuration file (default: bitcoin.conf)\n") +
            "  -gen             \t\t  " + _("Generate coins\n") +
            "  -gen=0           \t\t  " + _("Don't generate coins\n") +
            "  -min             \t\t  " + _("Start minimized\n") +
            "  -datadir=<dir>   \t\t  " + _("Specify data directory\n") +
            "  -proxy=<ip:port> \t  "   + _("Connect through socks4 proxy\n") +
            "  -addnode=<ip>    \t  "   + _("Add a node to connect to\n") +
            "  -connect=<ip>    \t\t  " + _("Connect only to the specified node\n") +
            "  -paytxfee=<amt>  \t  "   + _("Fee per KB to add to transactions you send\n") +
            "  -server          \t\t  " + _("Accept command line and JSON-RPC commands\n") +
            "  -daemon          \t\t  " + _("Run in the background as a daemon and accept commands\n") +
            "  -testnet         \t\t  " + _("Use the test network\n") +
            "  -rpcuser=<user>  \t  "   + _("Username for JSON-RPC connections\n") +
            "  -rpcpassword=<pw>\t  "   + _("Password for JSON-RPC connections\n") +
            "  -rpcport=<port>  \t\t  " + _("Listen for JSON-RPC connections on <port>\n") +
            "  -rpcallowip=<ip> \t\t  " + _("Allow JSON-RPC connections from specified IP address\n") +
            "  -rpcconnect=<ip> \t  "   + _("Send commands to node running on <ip>\n");

#ifdef USE_SSL
        strUsage += string() +
            _("\nSSL options: (see the Bitcoin Wiki for SSL setup instructions)\n") +
            "  -rpcssl=1                             \t  " + _("Use OpenSSL (https) for JSON-RPC connections\n") +
            "  -rpcsslcertificatchainfile=<file.cert>\t  " + _("Server certificate file (default: server.cert)\n") +
            "  -rpcsslprivatekeyfile=<file.pem>      \t  " + _("Server private key (default: server.pem)\n") +
            "  -rpcsslciphers=<ciphers>              \t  " + _("Acceptable ciphers (default: TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH)\n");
#endif

        strUsage += string() +
            "  -?               \t\t  " + _("This help message\n");

#if defined(__WXMSW__) && defined(GUI)
        // Tabs make the columns line up in the message box
        wxMessageBox(strUsage, "Bitcoin", wxOK);
#else
        // Remove tabs
        strUsage.erase(std::remove(strUsage.begin(), strUsage.end(), '\t'), strUsage.end());
        fprintf(stderr, "%s", strUsage.c_str());
#endif
        return false;
    }

    fDebug = GetBoolArg("-debug");

    fPrintToDebugger = GetBoolArg("-printtodebugger");

    fTestNet = GetBoolArg("-testnet");

    if (fCommandLine)
    {
        int ret = CommandLineRPC(argc, argv);
        exit(ret);
    }

    if (!fDebug && !pszSetDataDir[0])
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Bitcoin version %s%s beta\n", FormatVersion(VERSION).c_str(), pszSubVer);
#ifdef GUI
    printf("OS version %s\n", ((string)wxGetOsDescription()).c_str());
    printf("System default language is %d %s\n", g_locale.GetSystemLanguage(), ((string)g_locale.GetSysName()).c_str());
    printf("Language file %s (%s)\n", (string("locale/") + (string)g_locale.GetCanonicalName() + "/LC_MESSAGES/bitcoin.mo").c_str(), ((string)g_locale.GetLocale()).c_str());
#endif
    printf("Default data directory %s\n", GetDefaultDataDir().c_str());

    if (GetBoolArg("-loadblockindextest"))
    {
        CTxDB txdb("r");
        txdb.LoadBlockIndex();
        PrintBlockTree();
        return false;
    }

    //
    // Limit to single instance per user
    // Required to protect the database files if we're going to keep deleting log.*
    //
#if defined(__WXMSW__) && defined(GUI)
    // wxSingleInstanceChecker doesn't work on Linux
    wxString strMutexName = wxString("bitcoin_running.") + getenv("HOMEPATH");
    for (int i = 0; i < strMutexName.size(); i++)
        if (!isalnum(strMutexName[i]))
            strMutexName[i] = '.';
    wxSingleInstanceChecker* psingleinstancechecker = new wxSingleInstanceChecker(strMutexName);
    if (psingleinstancechecker->IsAnotherRunning())
    {
        printf("Existing instance found\n");
        unsigned int nStart = GetTime();
        loop
        {
            // Show the previous instance and exit
            HWND hwndPrev = FindWindowA("wxWindowClassNR", "Bitcoin");
            if (hwndPrev)
            {
                if (IsIconic(hwndPrev))
                    ShowWindow(hwndPrev, SW_RESTORE);
                SetForegroundWindow(hwndPrev);
                return false;
            }

            if (GetTime() > nStart + 60)
                return false;

            // Resume this instance if the other exits
            delete psingleinstancechecker;
            Sleep(1000);
            psingleinstancechecker = new wxSingleInstanceChecker(strMutexName);
            if (!psingleinstancechecker->IsAnotherRunning())
                break;
        }
    }
#endif

    // Make sure only a single bitcoin process is using the data directory.
    string strLockFile = GetDataDir() + "/.lock";
    FILE* file = fopen(strLockFile.c_str(), "a"); // empty lock file; created if it doesn't exist.
    fclose(file);
    static boost::interprocess::file_lock lock(strLockFile.c_str());
    if (!lock.try_lock())
    {
        wxMessageBox(strprintf(_("Cannot obtain a lock on data directory %s.  Bitcoin is probably already running."), GetDataDir().c_str()), "Bitcoin");
        return false;
    }

    // Bind to the port early so we can tell if another instance is already running.
    string strErrors;
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
        strErrors += _("Error loading addr.dat      \n");
    printf(" addresses   %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        strErrors += _("Error loading blkindex.dat      \n");
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun;
    if (!LoadWallet(fFirstRun))
        strErrors += _("Error loading wallet.dat      \n");
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Done loading\n");

        //// debug print
        printf("mapBlockIndex.size() = %d\n",   mapBlockIndex.size());
        printf("nBestHeight = %d\n",            nBestHeight);
        printf("mapKeys.size() = %d\n",         mapKeys.size());
        printf("mapPubKeys.size() = %d\n",      mapPubKeys.size());
        printf("mapWallet.size() = %d\n",       mapWallet.size());
        printf("mapAddressBook.size() = %d\n",  mapAddressBook.size());

    if (!strErrors.empty())
    {
        wxMessageBox(strErrors, "Bitcoin", wxOK | wxICON_ERROR);
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    ReacceptWalletTransactions();

    //
    // Parameters
    //
    if (GetBoolArg("-printblockindex") || GetBoolArg("-printblocktree"))
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
        addrProxy = CAddress(mapArgs["-proxy"]);
        if (!addrProxy.IsValid())
        {
            wxMessageBox(_("Invalid -proxy address"), "Bitcoin");
            return false;
        }
    }

    if (mapArgs.count("-addnode"))
    {
        foreach(string strAddr, mapMultiArgs["-addnode"])
        {
            CAddress addr(strAddr, NODE_NETWORK);
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

    if (GetBoolArg("-server") || fDaemon)
        CreateThread(ThreadRPCServer, NULL);

#if defined(__WXMSW__) && defined(GUI)
    if (fFirstRun)
        SetStartOnSystemStartup(true);
#endif

    return true;
}
