// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.


#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64  int64;
typedef unsigned __int64  uint64;
#else
typedef long long  int64;
typedef unsigned long long  uint64;
#endif
#if defined(_MSC_VER) && _MSC_VER < 1300
#define for  if (false) ; else for
#endif
#ifndef _MSC_VER
#define __forceinline  inline
#endif

#define foreach             BOOST_FOREACH
#define loop                for (;;)
#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))
#define UBEGIN(a)           ((unsigned char*)&(a))
#define UEND(a)             ((unsigned char*)&((&(a))[1]))
#define ARRAYLEN(array)     (sizeof(array)/sizeof((array)[0]))
#define printf              OutputDebugStringF

#ifdef snprintf
#undef snprintf
#endif
#define snprintf my_snprintf

#ifndef PRI64d
#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MSVCRT__)
#define PRI64d  "I64d"
#define PRI64u  "I64u"
#define PRI64x  "I64x"
#else
#define PRI64d  "lld"
#define PRI64u  "llu"
#define PRI64x  "llx"
#endif
#endif

// This is needed because the foreach macro can't get over the comma in pair<t1, t2>
#define PAIRTYPE(t1, t2)    pair<t1, t2>

// Used to bypass the rule against non-const reference to temporary
// where it makes sense with wrappers such as CFlatData or CTxDB
template<typename T>
inline T& REF(const T& val)
{
    return (T&)val;
}

// Align by increasing pointer, must have extra space at end of buffer
template <size_t nBytes, typename T>
T* alignup(T* p)
{
    union
    {
        T* ptr;
        size_t n;
    } u;
    u.ptr = p;
    u.n = (u.n + (nBytes-1)) & ~(nBytes-1);
    return u.ptr;
}

#ifdef __WXMSW__
#define MSG_NOSIGNAL        0
#define MSG_DONTWAIT        0
#ifndef UINT64_MAX
#define UINT64_MAX          _UI64_MAX
#define INT64_MAX           _I64_MAX
#define INT64_MIN           _I64_MIN
#endif
#ifndef S_IRUSR
#define S_IRUSR             0400
#define S_IWUSR             0200
#endif
#define unlink              _unlink
typedef int socklen_t;
#else
#define WSAGetLastError()   errno
#define WSAEWOULDBLOCK      EWOULDBLOCK
#define WSAEMSGSIZE         EMSGSIZE
#define WSAEINTR            EINTR
#define WSAEINPROGRESS      EINPROGRESS
#define WSAEADDRINUSE       EADDRINUSE
#define WSAENOTSOCK         EBADF
#define INVALID_SOCKET      (SOCKET)(~0)
#define SOCKET_ERROR        -1
typedef u_int SOCKET;
#define _vsnprintf(a,b,c,d) vsnprintf(a,b,c,d)
#define strlwr(psz)         to_lower(psz)
#define _strlwr(psz)        to_lower(psz)
#define MAX_PATH            1024
#define Beep(n1,n2)         (0)
inline void Sleep(int64 n)
{
    boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(n));
}
#endif

inline int myclosesocket(SOCKET& hSocket)
{
    if (hSocket == INVALID_SOCKET)
        return WSAENOTSOCK;
#ifdef __WXMSW__
    int ret = closesocket(hSocket);
#else
    int ret = close(hSocket);
#endif
    hSocket = INVALID_SOCKET;
    return ret;
}
#define closesocket(s)      myclosesocket(s)

#ifndef GUI
inline const char* _(const char* psz)
{
    return psz;
}
#endif










extern map<string, string> mapArgs;
extern map<string, vector<string> > mapMultiArgs;
extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugger;
extern char pszSetDataDir[MAX_PATH];
extern bool fRequestShutdown;
extern bool fShutdown;
extern bool fDaemon;
extern bool fCommandLine;
extern string strMiscWarning;
extern bool fTestNet;

void RandAddSeed();
void RandAddSeedPerfmon();
int OutputDebugStringF(const char* pszFormat, ...);
int my_snprintf(char* buffer, size_t limit, const char* format, ...);
string strprintf(const char* format, ...);
bool error(const char* format, ...);
void LogException(std::exception* pex, const char* pszThread);
void PrintException(std::exception* pex, const char* pszThread);
void PrintExceptionContinue(std::exception* pex, const char* pszThread);
void ParseString(const string& str, char c, vector<string>& v);
string FormatMoney(int64 n, bool fPlus=false);
bool ParseMoney(const string& str, int64& nRet);
bool ParseMoney(const char* pszIn, int64& nRet);
vector<unsigned char> ParseHex(const char* psz);
vector<unsigned char> ParseHex(const string& str);
void ParseParameters(int argc, char* argv[]);
const char* wxGetTranslation(const char* psz);
bool WildcardMatch(const char* psz, const char* mask);
bool WildcardMatch(const string& str, const string& mask);
int GetFilesize(FILE* file);
void GetDataDir(char* pszDirRet);
string GetConfigFile();
void ReadConfigFile(map<string, string>& mapSettingsRet, map<string, vector<string> >& mapMultiSettingsRet);
#ifdef __WXMSW__
string MyGetSpecialFolderPath(int nFolder, bool fCreate);
#endif
string GetDefaultDataDir();
string GetDataDir();
void ShrinkDebugFile();
int GetRandInt(int nMax);
uint64 GetRand(uint64 nMax);
int64 GetTime();
int64 GetAdjustedTime();
void AddTimeData(unsigned int ip, int64 nTime);













// Wrapper to automatically initialize critical sections
class CCriticalSection
{
#ifdef __WXMSW__
protected:
    CRITICAL_SECTION cs;
public:
    explicit CCriticalSection() { InitializeCriticalSection(&cs); }
    ~CCriticalSection() { DeleteCriticalSection(&cs); }
    void Enter() { EnterCriticalSection(&cs); }
    void Leave() { LeaveCriticalSection(&cs); }
    bool TryEnter() { return TryEnterCriticalSection(&cs); }
#else
protected:
    boost::interprocess::interprocess_recursive_mutex mutex;
public:
    explicit CCriticalSection() { }
    ~CCriticalSection() { }
    void Enter() { mutex.lock(); }
    void Leave() { mutex.unlock(); }
    bool TryEnter() { return mutex.try_lock(); }
#endif
public:
    const char* pszFile;
    int nLine;
};

// Automatically leave critical section when leaving block, needed for exception safety
class CCriticalBlock
{
protected:
    CCriticalSection* pcs;
public:
    CCriticalBlock(CCriticalSection& csIn) { pcs = &csIn; pcs->Enter(); }
    ~CCriticalBlock() { pcs->Leave(); }
};

// WARNING: This will catch continue and break!
// break is caught with an assertion, but there's no way to detect continue.
// I'd rather be careful than suffer the other more error prone syntax.
// The compiler will optimise away all this loop junk.
#define CRITICAL_BLOCK(cs)     \
    for (bool fcriticalblockonce=true; fcriticalblockonce; assert(("break caught by CRITICAL_BLOCK!", !fcriticalblockonce)), fcriticalblockonce=false)  \
    for (CCriticalBlock criticalblock(cs); fcriticalblockonce && (cs.pszFile=__FILE__, cs.nLine=__LINE__, true); fcriticalblockonce=false, cs.pszFile=NULL, cs.nLine=0)

class CTryCriticalBlock
{
protected:
    CCriticalSection* pcs;
public:
    CTryCriticalBlock(CCriticalSection& csIn) { pcs = (csIn.TryEnter() ? &csIn : NULL); }
    ~CTryCriticalBlock() { if (pcs) pcs->Leave(); }
    bool Entered() { return pcs != NULL; }
};

#define TRY_CRITICAL_BLOCK(cs)     \
    for (bool fcriticalblockonce=true; fcriticalblockonce; assert(("break caught by TRY_CRITICAL_BLOCK!", !fcriticalblockonce)), fcriticalblockonce=false)  \
    for (CTryCriticalBlock criticalblock(cs); fcriticalblockonce && (fcriticalblockonce = criticalblock.Entered()) && (cs.pszFile=__FILE__, cs.nLine=__LINE__, true); fcriticalblockonce=false, cs.pszFile=NULL, cs.nLine=0)










inline string i64tostr(int64 n)
{
    return strprintf("%"PRI64d, n);
}

inline string itostr(int n)
{
    return strprintf("%d", n);
}

inline int64 atoi64(const char* psz)
{
#ifdef _MSC_VER
    return _atoi64(psz);
#else
    return strtoll(psz, NULL, 10);
#endif
}

inline int64 atoi64(const string& str)
{
#ifdef _MSC_VER
    return _atoi64(str.c_str());
#else
    return strtoll(str.c_str(), NULL, 10);
#endif
}

inline int atoi(const string& str)
{
    return atoi(str.c_str());
}

inline int roundint(double d)
{
    return (int)(d > 0 ? d + 0.5 : d - 0.5);
}

inline int64 roundint64(double d)
{
    return (int64)(d > 0 ? d + 0.5 : d - 0.5);
}

inline int64 abs64(int64 n)
{
    return (n >= 0 ? n : -n);
}

template<typename T>
string HexStr(const T itbegin, const T itend, bool fSpaces=false)
{
    if (itbegin == itend)
        return "";
    const unsigned char* pbegin = (const unsigned char*)&itbegin[0];
    const unsigned char* pend = pbegin + (itend - itbegin) * sizeof(itbegin[0]);
    string str;
    str.reserve((pend-pbegin) * (fSpaces ? 3 : 2));
    for (const unsigned char* p = pbegin; p != pend; p++)
        str += strprintf((fSpaces && p != pend-1 ? "%02x " : "%02x"), *p);
    return str;
}

inline string HexStr(const vector<unsigned char>& vch, bool fSpaces=false)
{
    return HexStr(vch.begin(), vch.end(), fSpaces);
}

template<typename T>
string HexNumStr(const T itbegin, const T itend, bool f0x=true)
{
    if (itbegin == itend)
        return "";
    const unsigned char* pbegin = (const unsigned char*)&itbegin[0];
    const unsigned char* pend = pbegin + (itend - itbegin) * sizeof(itbegin[0]);
    string str = (f0x ? "0x" : "");
    str.reserve(str.size() + (pend-pbegin) * 2);
    for (const unsigned char* p = pend-1; p >= pbegin; p--)
        str += strprintf("%02x", *p);
    return str;
}

inline string HexNumStr(const vector<unsigned char>& vch, bool f0x=true)
{
    return HexNumStr(vch.begin(), vch.end(), f0x);
}

template<typename T>
void PrintHex(const T pbegin, const T pend, const char* pszFormat="%s", bool fSpaces=true)
{
    printf(pszFormat, HexStr(pbegin, pend, fSpaces).c_str());
}

inline void PrintHex(const vector<unsigned char>& vch, const char* pszFormat="%s", bool fSpaces=true)
{
    printf(pszFormat, HexStr(vch, fSpaces).c_str());
}

inline int64 GetPerformanceCounter()
{
    int64 nCounter = 0;
#ifdef __WXMSW__
    QueryPerformanceCounter((LARGE_INTEGER*)&nCounter);
#else
    timeval t;
    gettimeofday(&t, NULL);
    nCounter = t.tv_sec * 1000000 + t.tv_usec;
#endif
    return nCounter;
}

inline int64 GetTimeMillis()
{
    return (posix_time::ptime(posix_time::microsec_clock::universal_time()) -
            posix_time::ptime(gregorian::date(1970,1,1))).total_milliseconds();
}

inline string DateTimeStrFormat(const char* pszFormat, int64 nTime)
{
    time_t n = nTime;
    struct tm* ptmTime = gmtime(&n);
    char pszTime[200];
    strftime(pszTime, sizeof(pszTime), pszFormat, ptmTime);
    return pszTime;
}

template<typename T>
void skipspaces(T& it)
{
    while (isspace(*it))
        ++it;
}

inline bool IsSwitchChar(char c)
{
#ifdef __WXMSW__
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

inline string GetArg(const string& strArg, const string& strDefault)
{
    if (mapArgs.count(strArg))
        return mapArgs[strArg];
    return strDefault;
}

inline int64 GetArg(const string& strArg, int64 nDefault)
{
    if (mapArgs.count(strArg))
        return atoi64(mapArgs[strArg]);
    return nDefault;
}

inline bool GetBoolArg(const string& strArg)
{
    if (mapArgs.count(strArg))
    {
        if (mapArgs[strArg].empty())
            return true;
        return (atoi(mapArgs[strArg]) != 0);
    }
    return false;
}

inline string FormatVersion(int nVersion)
{
    if (nVersion%100 == 0)
        return strprintf("%d.%d.%d", nVersion/1000000, (nVersion/10000)%100, (nVersion/100)%100);
    else
        return strprintf("%d.%d.%d.%d", nVersion/1000000, (nVersion/10000)%100, (nVersion/100)%100, nVersion%100);
}










inline void heapchk()
{
#ifdef __WXMSW__
    /// for debugging
    //if (_heapchk() != _HEAPOK)
    //    DebugBreak();
#endif
}

// Randomize the stack to help protect against buffer overrun exploits
#define IMPLEMENT_RANDOMIZE_STACK(ThreadFn)     \
    {                                           \
        static char nLoops;                     \
        if (nLoops <= 0)                        \
            nLoops = GetRand(20) + 1;           \
        if (nLoops-- > 1)                       \
        {                                       \
            ThreadFn;                           \
            return;                             \
        }                                       \
    }

#define CATCH_PRINT_EXCEPTION(pszFn)     \
    catch (std::exception& e) {          \
        PrintException(&e, (pszFn));     \
    } catch (...) {                      \
        PrintException(NULL, (pszFn));   \
    }










template<typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]), (unsigned char*)&hash1);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T1, typename T2, typename T3>
inline uint256 Hash(const T1 p1begin, const T1 p1end,
                    const T2 p2begin, const T2 p2end,
                    const T3 p3begin, const T3 p3end)
{
    static unsigned char pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (unsigned char*)&p1begin[0]), (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (unsigned char*)&p2begin[0]), (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Update(&ctx, (p3begin == p3end ? pblank : (unsigned char*)&p3begin[0]), (p3end - p3begin) * sizeof(p3begin[0]));
    SHA256_Final((unsigned char*)&hash1, &ctx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}

template<typename T>
uint256 SerializeHash(const T& obj, int nType=SER_GETHASH, int nVersion=VERSION)
{
    // Most of the time is spent allocating and deallocating CDataStream's
    // buffer.  If this ever needs to be optimized further, make a CStaticStream
    // class with its buffer on the stack.
    CDataStream ss(nType, nVersion);
    ss.reserve(10000);
    ss << obj;
    return Hash(ss.begin(), ss.end());
}

inline uint160 Hash160(const vector<unsigned char>& vch)
{
    uint256 hash1;
    SHA256(&vch[0], vch.size(), (unsigned char*)&hash1);
    uint160 hash2;
    RIPEMD160((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
    return hash2;
}











// Note: It turns out we might have been able to use boost::thread
// by using TerminateThread(boost::thread.native_handle(), 0);
#ifdef __WXMSW__
typedef HANDLE pthread_t;

inline pthread_t CreateThread(void(*pfn)(void*), void* parg, bool fWantHandle=false)
{
    DWORD nUnused = 0;
    HANDLE hthread =
        CreateThread(
            NULL,                        // default security
            0,                           // inherit stack size from parent
            (LPTHREAD_START_ROUTINE)pfn, // function pointer
            parg,                        // argument
            0,                           // creation option, start immediately
            &nUnused);                   // thread identifier
    if (hthread == NULL)
    {
        printf("Error: CreateThread() returned %d\n", GetLastError());
        return (pthread_t)0;
    }
    if (!fWantHandle)
    {
        CloseHandle(hthread);
        return (pthread_t)-1;
    }
    return hthread;
}

inline void SetThreadPriority(int nPriority)
{
    SetThreadPriority(GetCurrentThread(), nPriority);
}
#else
inline pthread_t CreateThread(void(*pfn)(void*), void* parg, bool fWantHandle=false)
{
    pthread_t hthread = 0;
    int ret = pthread_create(&hthread, NULL, (void*(*)(void*))pfn, parg);
    if (ret != 0)
    {
        printf("Error: pthread_create() returned %d\n", ret);
        return (pthread_t)0;
    }
    if (!fWantHandle)
        return (pthread_t)-1;
    return hthread;
}

#define THREAD_PRIORITY_LOWEST          PRIO_MAX
#define THREAD_PRIORITY_BELOW_NORMAL    2
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_ABOVE_NORMAL    0

inline void SetThreadPriority(int nPriority)
{
    // It's unclear if it's even possible to change thread priorities on Linux,
    // but we really and truly need it for the generation threads.
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif
}

inline bool TerminateThread(pthread_t hthread, unsigned int nExitCode)
{
    return (pthread_cancel(hthread) == 0);
}

inline void ExitThread(unsigned int nExitCode)
{
    pthread_exit((void*)nExitCode);
}
#endif





inline bool AffinityBugWorkaround(void(*pfn)(void*))
{
#ifdef __WXMSW__
    // Sometimes after a few hours affinity gets stuck on one processor
    DWORD dwProcessAffinityMask = -1;
    DWORD dwSystemAffinityMask = -1;
    GetProcessAffinityMask(GetCurrentProcess(), &dwProcessAffinityMask, &dwSystemAffinityMask);
    DWORD dwPrev1 = SetThreadAffinityMask(GetCurrentThread(), dwProcessAffinityMask);
    DWORD dwPrev2 = SetThreadAffinityMask(GetCurrentThread(), dwProcessAffinityMask);
    if (dwPrev2 != dwProcessAffinityMask)
    {
        printf("AffinityBugWorkaround() : SetThreadAffinityMask=%d, ProcessAffinityMask=%d, restarting thread\n", dwPrev2, dwProcessAffinityMask);
        if (!CreateThread(pfn, NULL))
            printf("Error: CreateThread() failed\n");
        return true;
    }
#endif
    return false;
}
