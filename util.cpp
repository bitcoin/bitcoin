// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"


map<string, string> mapArgs;
map<string, vector<string> > mapMultiArgs;
bool fDebug = false;
bool fPrintToConsole = false;
bool fPrintToDebugger = false;
char pszSetDataDir[MAX_PATH] = "";
bool fRequestShutdown = false;
bool fShutdown = false;
bool fDaemon = false;
bool fCommandLine = false;
string strMiscWarning;





// Workaround for "multiple definition of `_tls_used'"
// http://svn.boost.org/trac/boost/ticket/4258
extern "C" void tss_cleanup_implemented() { }





// Init openssl library multithreading support
static boost::interprocess::interprocess_mutex** ppmutexOpenSSL;
void locking_callback(int mode, int i, const char* file, int line)
{
    if (mode & CRYPTO_LOCK)
        ppmutexOpenSSL[i]->lock();
    else
        ppmutexOpenSSL[i]->unlock();
}

// Init
class CInit
{
public:
    CInit()
    {
        // Init openssl library multithreading support
        ppmutexOpenSSL = (boost::interprocess::interprocess_mutex**)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(boost::interprocess::interprocess_mutex*));
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            ppmutexOpenSSL[i] = new boost::interprocess::interprocess_mutex();
        CRYPTO_set_locking_callback(locking_callback);

#ifdef __WXMSW__
        // Seed random number generator with screen scrape and other hardware sources
        RAND_screen();
#endif

        // Seed random number generator with performance counter
        RandAddSeed();
    }
    ~CInit()
    {
        // Shutdown openssl library multithreading support
        CRYPTO_set_locking_callback(NULL);
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            delete ppmutexOpenSSL[i];
        OPENSSL_free(ppmutexOpenSSL);
    }
}
instance_of_cinit;








void RandAddSeed()
{
    // Seed with CPU performance counter
    int64 nCounter = GetPerformanceCounter();
    RAND_add(&nCounter, sizeof(nCounter), 1.5);
    memset(&nCounter, 0, sizeof(nCounter));
}

void RandAddSeedPerfmon()
{
    RandAddSeed();

    // This can take up to 2 seconds, so only do it every 10 minutes
    static int64 nLastPerfmon;
    if (GetTime() < nLastPerfmon + 10 * 60)
        return;
    nLastPerfmon = GetTime();

#ifdef __WXMSW__
    // Don't need this on Linux, OpenSSL automatically uses /dev/urandom
    // Seed with the entire set of perfmon data
    unsigned char pdata[250000];
    memset(pdata, 0, sizeof(pdata));
    unsigned long nSize = sizeof(pdata);
    long ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, pdata, &nSize);
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    if (ret == ERROR_SUCCESS)
    {
        RAND_add(pdata, nSize, nSize/100.0);
        memset(pdata, 0, nSize);
        printf("%s RandAddSeed() %d bytes\n", DateTimeStrFormat("%x %H:%M", GetTime()).c_str(), nSize);
    }
#endif
}

uint64 GetRand(uint64 nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64 nRange = (UINT64_MAX / nMax) * nMax;
    uint64 nRand = 0;
    do
        RAND_bytes((unsigned char*)&nRand, sizeof(nRand));
    while (nRand >= nRange);
    return (nRand % nMax);
}

int GetRandInt(int nMax)
{
    return GetRand(nMax);
}











inline int OutputDebugStringF(const char* pszFormat, ...)
{
    int ret = 0;
    if (fPrintToConsole)
    {
        // print to console
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
        ret = vprintf(pszFormat, arg_ptr);
        va_end(arg_ptr);
    }
    else
    {
        // print to debug.log
        char pszFile[MAX_PATH+100];
        GetDataDir(pszFile);
        strlcat(pszFile, "/debug.log", sizeof(pszFile));
        FILE* fileout = fopen(pszFile, "a");
        if (fileout)
        {
            //// Debug print useful for profiling
            //fprintf(fileout, " %"PRI64d" ", GetTimeMillis());
            va_list arg_ptr;
            va_start(arg_ptr, pszFormat);
            ret = vfprintf(fileout, pszFormat, arg_ptr);
            va_end(arg_ptr);
            fclose(fileout);
        }
    }

#ifdef __WXMSW__
    if (fPrintToDebugger)
    {
        // accumulate a line at a time
        static CCriticalSection cs_OutputDebugStringF;
        CRITICAL_BLOCK(cs_OutputDebugStringF)
        {
            static char pszBuffer[50000];
            static char* pend;
            if (pend == NULL)
                pend = pszBuffer;
            va_list arg_ptr;
            va_start(arg_ptr, pszFormat);
            int limit = END(pszBuffer) - pend - 2;
            int ret = _vsnprintf(pend, limit, pszFormat, arg_ptr);
            va_end(arg_ptr);
            if (ret < 0 || ret >= limit)
            {
                pend = END(pszBuffer) - 2;
                *pend++ = '\n';
            }
            else
                pend += ret;
            *pend = '\0';
            char* p1 = pszBuffer;
            char* p2;
            while (p2 = strchr(p1, '\n'))
            {
                p2++;
                char c = *p2;
                *p2 = '\0';
                OutputDebugStringA(p1);
                *p2 = c;
                p1 = p2;
            }
            if (p1 != pszBuffer)
                memmove(pszBuffer, p1, pend - p1 + 1);
            pend -= (p1 - pszBuffer);
        }
    }
#endif
    return ret;
}


// Safer snprintf
//  - prints up to limit-1 characters
//  - output string is always null terminated even if limit reached
//  - return value is the number of characters actually printed
int my_snprintf(char* buffer, size_t limit, const char* format, ...)
{
    if (limit == 0)
        return 0;
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit-1] = 0;
    }
    return ret;
}


string strprintf(const char* format, ...)
{
    char buffer[50000];
    char* p = buffer;
    int limit = sizeof(buffer);
    int ret;
    loop
    {
        va_list arg_ptr;
        va_start(arg_ptr, format);
        ret = _vsnprintf(p, limit, format, arg_ptr);
        va_end(arg_ptr);
        if (ret >= 0 && ret < limit)
            break;
        if (p != buffer)
            delete p;
        limit *= 2;
        p = new char[limit];
        if (p == NULL)
            throw std::bad_alloc();
    }
    string str(p, p+ret);
    if (p != buffer)
        delete p;
    return str;
}


bool error(const char* format, ...)
{
    char buffer[50000];
    int limit = sizeof(buffer);
    va_list arg_ptr;
    va_start(arg_ptr, format);
    int ret = _vsnprintf(buffer, limit, format, arg_ptr);
    va_end(arg_ptr);
    if (ret < 0 || ret >= limit)
    {
        ret = limit - 1;
        buffer[limit-1] = 0;
    }
    printf("ERROR: %s\n", buffer);
    return false;
}


void ParseString(const string& str, char c, vector<string>& v)
{
    if (str.empty())
        return;
    string::size_type i1 = 0;
    string::size_type i2;
    loop
    {
        i2 = str.find(c, i1);
        if (i2 == str.npos)
        {
            v.push_back(str.substr(i1));
            return;
        }
        v.push_back(str.substr(i1, i2-i1));
        i1 = i2+1;
    }
}


string FormatMoney(int64 n, bool fPlus)
{
    n /= CENT;
    string str = strprintf("%"PRI64d".%02"PRI64d, (n > 0 ? n : -n)/100, (n > 0 ? n : -n)%100);
    for (int i = 6; i < str.size(); i += 4)
        if (isdigit(str[str.size() - i - 1]))
            str.insert(str.size() - i, 1, ',');
    if (n < 0)
        str.insert((unsigned int)0, 1, '-');
    else if (fPlus && n > 0)
        str.insert((unsigned int)0, 1, '+');
    return str;
}


bool ParseMoney(const string& str, int64& nRet)
{
    return ParseMoney(str.c_str(), nRet);
}

bool ParseMoney(const char* pszIn, int64& nRet)
{
    string strWhole;
    int64 nCents = 0;
    const char* p = pszIn;
    while (isspace(*p))
        p++;
    for (; *p; p++)
    {
        if (*p == ',' && p > pszIn && isdigit(p[-1]) && isdigit(p[1]) && isdigit(p[2]) && isdigit(p[3]) && !isdigit(p[4]))
            continue;
        if (*p == '.')
        {
            p++;
            if (isdigit(*p))
            {
                nCents = 10 * (*p++ - '0');
                if (isdigit(*p))
                    nCents += (*p++ - '0');
            }
            break;
        }
        if (isspace(*p))
            break;
        if (!isdigit(*p))
            return false;
        strWhole.insert(strWhole.end(), *p);
    }
    for (; *p; p++)
        if (!isspace(*p))
            return false;
    if (strWhole.size() > 14)
        return false;
    if (nCents < 0 || nCents > 99)
        return false;
    int64 nWhole = atoi64(strWhole);
    int64 nPreValue = nWhole * 100 + nCents;
    int64 nValue = nPreValue * CENT;
    if (nValue / CENT != nPreValue)
        return false;
    if (nValue / COIN != nWhole)
        return false;
    nRet = nValue;
    return true;
}


vector<unsigned char> ParseHex(const char* psz)
{
    static char phexdigit[256] =
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
      -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

    // convert hex dump to vector
    vector<unsigned char> vch;
    loop
    {
        while (isspace(*psz))
            psz++;
        char c = phexdigit[(unsigned char)*psz++];
        if (c == -1)
            break;
        unsigned char n = (c << 4);
        c = phexdigit[(unsigned char)*psz++];
        if (c == -1)
            break;
        n |= c;
        vch.push_back(n);
    }
    return vch;
}

vector<unsigned char> ParseHex(const string& str)
{
    return ParseHex(str.c_str());
}


void ParseParameters(int argc, char* argv[])
{
    mapArgs.clear();
    mapMultiArgs.clear();
    for (int i = 1; i < argc; i++)
    {
        char psz[10000];
        strlcpy(psz, argv[i], sizeof(psz));
        char* pszValue = (char*)"";
        if (strchr(psz, '='))
        {
            pszValue = strchr(psz, '=');
            *pszValue++ = '\0';
        }
        #ifdef __WXMSW__
        _strlwr(psz);
        if (psz[0] == '/')
            psz[0] = '-';
        #endif
        if (psz[0] != '-')
            break;
        mapArgs[psz] = pszValue;
        mapMultiArgs[psz].push_back(pszValue);
    }
}


const char* wxGetTranslation(const char* pszEnglish)
{
#ifdef GUI
    // Wrapper of wxGetTranslation returning the same const char* type as was passed in
    static CCriticalSection cs;
    CRITICAL_BLOCK(cs)
    {
        // Look in cache
        static map<string, char*> mapCache;
        map<string, char*>::iterator mi = mapCache.find(pszEnglish);
        if (mi != mapCache.end())
            return (*mi).second;

        // wxWidgets translation
        wxString strTranslated = wxGetTranslation(wxString(pszEnglish, wxConvUTF8));

        // We don't cache unknown strings because caller might be passing in a
        // dynamic string and we would keep allocating memory for each variation.
        if (strcmp(pszEnglish, strTranslated.utf8_str()) == 0)
            return pszEnglish;

        // Add to cache, memory doesn't need to be freed.  We only cache because
        // we must pass back a pointer to permanently allocated memory.
        char* pszCached = new char[strlen(strTranslated.utf8_str())+1];
        strcpy(pszCached, strTranslated.utf8_str());
        mapCache[pszEnglish] = pszCached;
        return pszCached;
    }
    return NULL;
#else
    return pszEnglish;
#endif
}


bool WildcardMatch(const char* psz, const char* mask)
{
    loop
    {
        switch (*mask)
        {
        case '\0':
            return (*psz == '\0');
        case '*':
            return WildcardMatch(psz, mask+1) || (*psz && WildcardMatch(psz+1, mask));
        case '?':
            if (*psz == '\0')
                return false;
            break;
        default:
            if (*psz != *mask)
                return false;
            break;
        }
        psz++;
        mask++;
    }
}

bool WildcardMatch(const string& str, const string& mask)
{
    return WildcardMatch(str.c_str(), mask.c_str());
}








void FormatException(char* pszMessage, std::exception* pex, const char* pszThread)
{
#ifdef __WXMSW__
    char pszModule[MAX_PATH];
    pszModule[0] = '\0';
    GetModuleFileNameA(NULL, pszModule, sizeof(pszModule));
#else
    const char* pszModule = "bitcoin";
#endif
    if (pex)
        snprintf(pszMessage, 1000,
            "EXCEPTION: %s       \n%s       \n%s in %s       \n", typeid(*pex).name(), pex->what(), pszModule, pszThread);
    else
        snprintf(pszMessage, 1000,
            "UNKNOWN EXCEPTION       \n%s in %s       \n", pszModule, pszThread);
}

void LogException(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n%s", pszMessage);
}

void PrintException(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n\n************************\n%s\n", pszMessage);
    fprintf(stderr, "\n\n************************\n%s\n", pszMessage);
    strMiscWarning = pszMessage;
#ifdef GUI
    if (wxTheApp && !fDaemon)
        MyMessageBox(pszMessage, "Bitcoin", wxOK | wxICON_ERROR);
#endif
    throw;
}

void ThreadOneMessageBox(string strMessage)
{
    // Skip message boxes if one is already open
    static bool fMessageBoxOpen;
    if (fMessageBoxOpen)
        return;
    fMessageBoxOpen = true;
    ThreadSafeMessageBox(strMessage, "Bitcoin", wxOK | wxICON_EXCLAMATION);
    fMessageBoxOpen = false;
}

void PrintExceptionContinue(std::exception* pex, const char* pszThread)
{
    char pszMessage[10000];
    FormatException(pszMessage, pex, pszThread);
    printf("\n\n************************\n%s\n", pszMessage);
    fprintf(stderr, "\n\n************************\n%s\n", pszMessage);
    strMiscWarning = pszMessage;
#ifdef GUI
    if (wxTheApp && !fDaemon)
        boost::thread(bind(ThreadOneMessageBox, string(pszMessage)));
#endif
}








#ifdef __WXMSW__
typedef WINSHELLAPI BOOL (WINAPI *PSHGETSPECIALFOLDERPATHA)(HWND hwndOwner, LPSTR lpszPath, int nFolder, BOOL fCreate);

string MyGetSpecialFolderPath(int nFolder, bool fCreate)
{
    char pszPath[MAX_PATH+100] = "";

    // SHGetSpecialFolderPath isn't always available on old Windows versions
    HMODULE hShell32 = LoadLibraryA("shell32.dll");
    if (hShell32)
    {
        PSHGETSPECIALFOLDERPATHA pSHGetSpecialFolderPath =
            (PSHGETSPECIALFOLDERPATHA)GetProcAddress(hShell32, "SHGetSpecialFolderPathA");
        if (pSHGetSpecialFolderPath)
            (*pSHGetSpecialFolderPath)(NULL, pszPath, nFolder, fCreate);
        FreeModule(hShell32);
    }

    // Backup option
    if (pszPath[0] == '\0')
    {
        if (nFolder == CSIDL_STARTUP)
        {
            strcpy(pszPath, getenv("USERPROFILE"));
            strcat(pszPath, "\\Start Menu\\Programs\\Startup");
        }
        else if (nFolder == CSIDL_APPDATA)
        {
            strcpy(pszPath, getenv("APPDATA"));
        }
    }

    return pszPath;
}
#endif

string GetDefaultDataDir()
{
    // Windows: C:\Documents and Settings\username\Application Data\Bitcoin
    // Mac: ~/Library/Application Support/Bitcoin
    // Unix: ~/.bitcoin
#ifdef __WXMSW__
    // Windows
    return MyGetSpecialFolderPath(CSIDL_APPDATA, true) + "\\Bitcoin";
#else
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
        pszHome = (char*)"/";
    string strHome = pszHome;
    if (strHome[strHome.size()-1] != '/')
        strHome += '/';
#ifdef __WXMAC_OSX__
    // Mac
    strHome += "Library/Application Support/";
    filesystem::create_directory(strHome.c_str());
    return strHome + "Bitcoin";
#else
    // Unix
    return strHome + ".bitcoin";
#endif
#endif
}

void GetDataDir(char* pszDir)
{
    // pszDir must be at least MAX_PATH length.
    if (pszSetDataDir[0] != 0)
    {
        strlcpy(pszDir, pszSetDataDir, MAX_PATH);
        static bool fMkdirDone;
        if (!fMkdirDone)
        {
            fMkdirDone = true;
            filesystem::create_directory(pszDir);
        }
    }
    else
    {
        // This can be called during exceptions by printf, so we cache the
        // value so we don't have to do memory allocations after that.
        static char pszCachedDir[MAX_PATH];
        if (pszCachedDir[0] == 0)
        {
            strlcpy(pszCachedDir, GetDefaultDataDir().c_str(), sizeof(pszCachedDir));
            filesystem::create_directory(pszCachedDir);
        }
        strlcpy(pszDir, pszCachedDir, MAX_PATH);
    }
}

string GetDataDir()
{
    char pszDir[MAX_PATH];
    GetDataDir(pszDir);
    return pszDir;
}

string GetConfigFile()
{
    namespace fs = boost::filesystem;
    fs::path pathConfig(GetArg("-conf", "bitcoin.conf"));
    if (!pathConfig.is_complete())
        pathConfig = fs::path(GetDataDir()) / pathConfig;
    return pathConfig.string();
}

void ReadConfigFile(map<string, string>& mapSettingsRet,
                    map<string, vector<string> >& mapMultiSettingsRet)
{
    namespace fs = boost::filesystem;
    namespace pod = boost::program_options::detail;

    fs::ifstream streamConfig(GetConfigFile());
    if (!streamConfig.good())
        return;

    set<string> setOptions;
    setOptions.insert("*");
    
    for (pod::config_file_iterator it(streamConfig, setOptions), end; it != end; ++it)
    {
        // Don't overwrite existing settings so command line settings override bitcoin.conf
        string strKey = string("-") + it->string_key;
        if (mapSettingsRet.count(strKey) == 0)
            mapSettingsRet[strKey] = it->value[0];
        mapMultiSettingsRet[strKey].push_back(it->value[0]);
    }
}

int GetFilesize(FILE* file)
{
    int nSavePos = ftell(file);
    int nFilesize = -1;
    if (fseek(file, 0, SEEK_END) == 0)
        nFilesize = ftell(file);
    fseek(file, nSavePos, SEEK_SET);
    return nFilesize;
}

void ShrinkDebugFile()
{
    // Scroll debug.log if it's getting too big
    string strFile = GetDataDir() + "/debug.log";
    FILE* file = fopen(strFile.c_str(), "r");
    if (file && GetFilesize(file) > 10 * 1000000)
    {
        // Restart the file with some of the end
        char pch[200000];
        fseek(file, -sizeof(pch), SEEK_END);
        int nBytes = fread(pch, 1, sizeof(pch), file);
        fclose(file);
        if (file = fopen(strFile.c_str(), "w"))
        {
            fwrite(pch, 1, nBytes, file);
            fclose(file);
        }
    }
}








//
// "Never go to sea with two chronometers; take one or three."
// Our three time sources are:
//  - System clock
//  - Median of other nodes's clocks
//  - The user (asking the user to fix the system clock if the first two disagree)
//
int64 GetTime()
{
    return time(NULL);
}

static int64 nTimeOffset = 0;

int64 GetAdjustedTime()
{
    return GetTime() + nTimeOffset;
}

void AddTimeData(unsigned int ip, int64 nTime)
{
    int64 nOffsetSample = nTime - GetTime();

    // Ignore duplicates
    static set<unsigned int> setKnown;
    if (!setKnown.insert(ip).second)
        return;

    // Add data
    static vector<int64> vTimeOffsets;
    if (vTimeOffsets.empty())
        vTimeOffsets.push_back(0);
    vTimeOffsets.push_back(nOffsetSample);
    printf("Added time data, samples %d, offset %+"PRI64d" (%+"PRI64d" minutes)\n", vTimeOffsets.size(), vTimeOffsets.back(), vTimeOffsets.back()/60);
    if (vTimeOffsets.size() >= 5 && vTimeOffsets.size() % 2 == 1)
    {
        sort(vTimeOffsets.begin(), vTimeOffsets.end());
        int64 nMedian = vTimeOffsets[vTimeOffsets.size()/2];
        // Only let other nodes change our time by so much
        if (abs64(nMedian) < 70 * 60)
        {
            nTimeOffset = nMedian;
        }
        else
        {
            nTimeOffset = 0;
            // If nobody else has the same time as us, give a warning
            bool fMatch = false;
            foreach(int64 nOffset, vTimeOffsets)
                if (nOffset != 0 && abs64(nOffset) < 5 * 60)
                    fMatch = true;
            static bool fDone;
            if (!fMatch && !fDone)
            {
                fDone = true;
                string strMessage = _("Warning: Please check that your computer's date and time are correct.  If your clock is wrong Bitcoin will not work properly.");
                strMiscWarning = strMessage;
                printf("*** %s\n", strMessage.c_str());
                boost::thread(bind(ThreadSafeMessageBox, strMessage+" ", string("Bitcoin"), wxOK | wxICON_EXCLAMATION, (wxWindow*)NULL, -1, -1));
            }
        }
        foreach(int64 n, vTimeOffsets)
            printf("%+"PRI64d"  ", n);
        printf("|  nTimeOffset = %+"PRI64d"  (%+"PRI64d" minutes)\n", nTimeOffset, nTimeOffset/60);
    }
}
