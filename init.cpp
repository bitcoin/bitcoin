// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"




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






//////////////////////////////////////////////////////////////////////////////
//
// Startup folder
//

#ifdef __WXMSW__
string StartupShortcutPath()
{
    return MyGetSpecialFolderPath(CSIDL_STARTUP, true) + "\\Bitcoin.lnk";
}

bool GetStartOnSystemStartup()
{
    return filesystem::exists(StartupShortcutPath().c_str());
}

void SetStartOnSystemStartup(bool fAutoStart)
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

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);

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
            }
            psl->Release();
        }
        CoUninitialize();
    }
}

#elif defined(__WXGTK__)

//
// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html
//

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

void SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
    {
        unlink(GetAutostartFilePath().native_file_string().c_str());
    }
    else
    {
        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), ios_base::out|ios_base::trunc);
        if (!optionFile.good())
        {
            wxMessageBox(_("Cannot write autostart/bitcoin.desktop file"), "Bitcoin");
            return;
        }
        // Write a bitcoin.desktop file to the autostart directory:
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1);
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=Bitcoin\n";
        optionFile << "Exec=" << pszExePath << "\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
}
#else

// TODO: OSX startup stuff; see:
// http://developer.apple.com/mac/library/documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
void SetStartOnSystemStartup(bool fAutoStart) { }

#endif







//////////////////////////////////////////////////////////////////////////////
//
// CMyApp
//

// Define a new application
class CMyApp : public wxApp
{
public:
    wxLocale m_locale;

    CMyApp(){};
    ~CMyApp(){};
    bool OnInit();
    bool OnInit2();
    int OnExit();

    // Hook Initialize so we can start without GUI
    virtual bool Initialize(int& argc, wxChar** argv);

    // 2nd-level exception handling: we get all the exceptions occurring in any
    // event handler here
    virtual bool OnExceptionInMainLoop();

    // 3rd, and final, level exception handling: whenever an unhandled
    // exception is caught, this function is called
    virtual void OnUnhandledException();

    // and now for something different: this function is called in case of a
    // crash (e.g. dereferencing null pointer, division by 0, ...)
    virtual void OnFatalException();
};

IMPLEMENT_APP(CMyApp)

bool CMyApp::Initialize(int& argc, wxChar** argv)
{
    if (argc > 1 && argv[1][0] != '-' && (!fWindows || argv[1][0] != '/') &&
        wxString(argv[1]) != "start")
    {
        fCommandLine = true;
    }
    else if (!fGUI)
    {
        fDaemon = true;
    }
    else
    {
        // wxApp::Initialize will remove environment-specific parameters,
        // so it's too early to call ParseParameters yet
        for (int i = 1; i < argc; i++)
        {
            wxString str = argv[i];
            #ifdef __WXMSW__
            if (str.size() >= 1 && str[0] == '/')
                str[0] = '-';
            char pszLower[MAX_PATH];
            strlcpy(pszLower, str.c_str(), sizeof(pszLower));
            strlwr(pszLower);
            str = pszLower;
            #endif
            // haven't decided which argument to use for this yet
            if (str == "-daemon" || str == "-d" || str == "start")
                fDaemon = true;
        }
    }

#ifdef __WXGTK__
    if (fDaemon || fCommandLine)
    {
        // Call the original Initialize while suppressing error messages
        // and ignoring failure.  If unable to initialize GTK, it fails
        // near the end so hopefully the last few things don't matter.
        {
            wxLogNull logNo;
            wxApp::Initialize(argc, argv);
        }

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
                pthread_exit((void*)0);
        }

        return true;
    }
#endif

    return wxApp::Initialize(argc, argv);
}

bool CMyApp::OnInit()
{
    bool fRet = false;
    try
    {
        fRet = OnInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "OnInit()");
    } catch (...) {
        PrintException(NULL, "OnInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

extern int g_isPainting;

bool CMyApp::OnInit2()
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
#if defined(__WXMSW__) && defined(__WXDEBUG__) && wxUSE_GUI
    // Disable malfunctioning wxWidgets debug assertion
    g_isPainting = 10000;
#endif
#if wxUSE_GUI
    wxImage::AddHandler(new wxPNGHandler);
#endif
#if defined(__WXMSW__ ) || defined(__WXMAC__)
    SetAppName("Bitcoin");
#else
    SetAppName("bitcoin");
#endif
#ifndef __WXMSW__
    umask(077);
#endif
#ifdef __WXMSW__
#if wxUSE_UNICODE
    // Hack to set wxConvLibc codepage to UTF-8 on Windows,
    // may break if wxMBConv_win32 implementation in strconv.cpp changes.
    class wxMBConv_win32 : public wxMBConv
    {
    public:
        long m_CodePage;
        size_t m_minMBCharWidth;
    };
    if (((wxMBConv_win32*)&wxConvLibc)->m_CodePage == CP_ACP)
        ((wxMBConv_win32*)&wxConvLibc)->m_CodePage = CP_UTF8;
#endif
#endif

    // Load locale/<lang>/LC_MESSAGES/bitcoin.mo language file
    m_locale.Init(wxLANGUAGE_DEFAULT, 0);
    m_locale.AddCatalogLookupPathPrefix("locale");
    if (!fWindows)
    {
        m_locale.AddCatalogLookupPathPrefix("/usr/share/locale");
        m_locale.AddCatalogLookupPathPrefix("/usr/local/share/locale");
    }
    m_locale.AddCatalog("wxstd"); // wxWidgets standard translations, if any
    m_locale.AddCatalog("bitcoin");

    //
    // Parameters
    //
    if (fCommandLine)
    {
        int ret = CommandLineRPC(argc, argv);
        exit(ret);
    }

    ParseParameters(argc, argv);
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        wxString strUsage = string() +
          _("Usage:") + "\t\t\t\t\t\t\t\t\t\t\n" +
            "  bitcoin [options]       \t" + "\n" +
            "  bitcoin [command]       \t" + _("Send command to bitcoin running with -server or -daemon\n") +
            "  bitcoin [command] -?    \t" + _("Get help for a command\n") +
            "  bitcoin help            \t" + _("List commands\n") +
          _("Options:\n") +
            "  -gen            \t  " + _("Generate coins\n") +
            "  -gen=0          \t  " + _("Don't generate coins\n") +
            "  -min            \t  " + _("Start minimized\n") +
            "  -datadir=<dir>  \t  " + _("Specify data directory\n") +
            "  -proxy=<ip:port>\t  " + _("Connect through socks4 proxy\n") +
            "  -addnode=<ip>   \t  " + _("Add a node to connect to\n") +
            "  -connect=<ip>   \t  " + _("Connect only to the specified node\n") +
            "  -server         \t  " + _("Accept command line and JSON-RPC commands\n") +
            "  -daemon         \t  " + _("Run in the background as a daemon and accept commands\n") +
            "  -?              \t  " + _("This help message\n");

#if defined(__WXMSW__) && wxUSE_GUI
        // Tabs make the columns line up in the message box
        wxMessageBox(strUsage, "Bitcoin", wxOK);
#else
        // Remove tabs
        strUsage.Replace("\t", "");
        fprintf(stderr, "%s", ((string)strUsage).c_str());
#endif
        return false;
    }

    if (mapArgs.count("-datadir"))
        strlcpy(pszSetDataDir, mapArgs["-datadir"].c_str(), sizeof(pszSetDataDir));

    if (mapArgs.count("-debug"))
        fDebug = true;

    if (mapArgs.count("-printtodebugger"))
        fPrintToDebugger = true;

    if (!fDebug && !pszSetDataDir[0])
        ShrinkDebugFile();
    printf("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    printf("Bitcoin version %d.%d.%d%s beta, OS version %s\n", VERSION/10000, (VERSION/100)%100, VERSION%100, pszSubVer, ((string)wxGetOsDescription()).c_str());
    printf("System default language is %d %s\n", m_locale.GetSystemLanguage(), ((string)m_locale.GetSysName()).c_str());
    printf("Language file %s (%s)\n", (string("locale/") + (string)m_locale.GetCanonicalName() + "/LC_MESSAGES/bitcoin.mo").c_str(), ((string)m_locale.GetLocale()).c_str());
    printf("Default data directory %s\n", GetDefaultDataDir().c_str());

    if (mapArgs.count("-loadblockindextest"))
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
#ifdef __WXMSW__
    // todo: wxSingleInstanceChecker wasn't working on Linux, never deleted its lock file
    //  maybe should go by whether successfully bind port 8333 instead
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
            // TODO: find out how to do this in Linux, or replace with wxWidgets commands
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

    // Bind to the port early so we can tell if another instance is already running.
    // This is a backup to wxSingleInstanceChecker, which doesn't work on Linux.
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
        wxMessageBox(strErrors, "Bitcoin");
        return false;
    }

    // Add wallet transactions that aren't already in a block to mapTransactions
    ReacceptWalletTransactions();

    //
    // Parameters
    //
    if (mapArgs.count("-printblockindex") || mapArgs.count("-printblocktree"))
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

    if (mapArgs.count("-gen"))
    {
        if (mapArgs["-gen"].empty())
            fGenerateBitcoins = true;
        else
            fGenerateBitcoins = (atoi(mapArgs["-gen"].c_str()) != 0);
    }

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

    //
    // Create the main window and start the node
    //
    if (!fDaemon)
        CreateMainWindow();

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    if (!CreateThread(StartNode, NULL))
        wxMessageBox("Error: CreateThread(StartNode) failed", "Bitcoin");

    if (mapArgs.count("-server") || fDaemon)
        CreateThread(ThreadRPCServer, NULL);

    if (fFirstRun)
        SetStartOnSystemStartup(true);

    return true;
}

int CMyApp::OnExit()
{
    Shutdown(NULL);
    return wxApp::OnExit();
}

bool CMyApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }
    return true;
}

void CMyApp::OnUnhandledException()
{
    // this shows how we may let some exception propagate uncaught
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnUnhandledException()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnUnhandledException()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }
}

void CMyApp::OnFatalException()
{
    wxMessageBox(_("Program has crashed and will terminate.  "), "Bitcoin", wxOK | wxICON_ERROR);
}
