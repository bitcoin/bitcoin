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


// enumLogContext constants can be bitwise-ORed together, both when setting log context
// and when calling OutputLogStringF. For example, a particular operation may apply to
// both Block and Chaining, so you can call OutputLogStringF(LC_Block|LC_Chaining, <etc.>
// This list is not intended to be complete and final. It is merely my
// (Jim Hyslop's) first pass at organizing the debug messages. Feel free
// to revise, merge, expand, etc. this list. Just keep in mind that there is
// no guarantee that an enum can be > 32 bits on all compilers, so don't
// add any values > 0x80000000.
enum enumLogContext
{
    LC_NoChange    =          0,
    LC_None        =          0,
    LC_All         = 0xffffffff,
    LC_Accept      = 0x00000001,
    LC_Account     = 0x00000002,
    LC_Alert       = 0x00000004, 
    LC_Bitcoin     = 0x00000008,
    LC_Block       = 0x00000010,
    LC_Chain       = 0x00000020,
    LC_Coins       = 0x00000040,
    LC_DB          = 0x00000080,
    LC_Gen         = 0x00000100,
    LC_HashRate    = 0x00000200,
    LC_IRC         = 0x00000400,
    LC_Main        = 0x00000800,
    LC_Message     = 0x00001000,
    LC_Net         = 0x00002000,
    LC_Orphan      = 0x00004000,
    LC_Params      = 0x00008000,
    LC_Priority    = 0x00010000,
    LC_RPC         = 0x00020000,
    LC_Send        = 0x00040000,
    LC_Sha         = 0x00080000,
    LC_Sys         = 0x00100000,
    LC_Test        = 0x00200000,
    LC_Thread      = 0x00400000,
    LC_Time        = 0x00800000,
    LC_Transaction = 0x01000000,
    LC_Util        = 0x02000000,
    LC_Wallet      = 0x04000000,
    LC_TBD1        = 0x08000000,
    LC_TBD2        = 0x10000000,
    LC_TBD3        = 0x20000000,
    LC_TBD4        = 0x40000000,
    LC_Legacy      = 0x80000000,
};

enum enumVerbosityLevel
{
  VL_Off,
  VL_Critical,
  VL_Error,
  VL_Warning,
  VL_Info,
  VL_Debug,
  VL_Verbose
};

// Utility function to make OR-ing the log context enums easier
inline enumLogContext operator | (enumLogContext e1, enumLogContext e2 )
{
    return static_cast<enumLogContext>( (int)e1|(int)e2 );
}

// Returns the new log output mask. Pass LC_NoChange if all you want to do is query the current mask.
enumLogContext TurnOnLogOutput(enumLogContext groupsToAdd);
// Returns the new log output mask. Pass LC_NoChange if all you want to do is query the current mask.
enumLogContext TurnOffLogOutput(enumLogContext groupsToRemove);
// Discard current groups, and use these ones instead.
void SetLogOutput(enumLogContext groupsToLog);
// Returns the previous log verbosity
enumVerbosityLevel SetLogVerbosity(enumVerbosityLevel verbosity);
bool IsLogVerbosity(enumVerbosityLevel);

// Utility functions to parse a list of log groups and the verbosity level out of a text
// string, which would come from either the command-line or an RPC command (RPC command TODO).
enumLogContext ParseLogContext(const std::string & groups);
enumVerbosityLevel ParseVerbosity(const std::string & verbosity);

// Revised function accepting the group and verbosity
int OutputLogMessageF(enumLogContext group, enumVerbosityLevel verbosity, const char* pszFormat, ...);
// Deprecated function, kept for backwards compatibility. Prefer the overloaded function above.
int OutputDebugStringF(const char* pszFormat, ...);

// This macro is also deprecated. Eventually all instances of 'printf' in the source code should
// be changed to OutputDebugStringF
#define printf OutputDebugStringF

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
extern bool fNoListen;

void RandAddSeed();
void RandAddSeedPerfmon();
int my_snprintf(char* buffer, size_t limit, const char* format, ...);
string strprintf(const char* format, ...);

// Error and exception logging functions - rewored to use new enumLogContext
bool error(enumLogContext logGroup, const char* format, ...);
void LogException(enumLogContext group, std::exception* pex, const char* pszThread);
void PrintException(enumLogContext group, std::exception* pex, const char* pszThread);
void PrintExceptionContinue(enumLogContext group, std::exception* pex, const char* pszThread);
// Old functions, retained for backwards compatibility. Prefer the overloaded versions accepting enumLogContext
// Note: some of these are declared inline to avoid compiler errors, otherwise we'd need to
// put them in a different file.
bool error(const char* format, ...);
inline void LogException(std::exception* pex, const char* pszThread)
{
    LogException(LC_All, pex, pszThread);
}
inline void PrintException(std::exception* pex, const char* pszThread)
{
    PrintException(LC_All, pex, pszThread);
}
inline void PrintExceptionContinue(std::exception* pex, const char* pszThread)
{
    PrintExceptionContinue(LC_All, pex, pszThread);
}

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
void PrintHex(enumLogContext group, enumVerbosityLevel verbosity, const T pbegin, const T pend, const char* pszFormat="%s", bool fSpaces=true)
{
    OutputLogMessageF(group, verbosity, pszFormat, HexStr(pbegin, pend, fSpaces).c_str());
}

// Maintained for backward compatibility. Prefer the overloaded version.
template<typename T>
void PrintHex(const T pbegin, const T pend, const char* pszFormat="%s", bool fSpaces=true)
{
    PrintHex(LC_All, VL_Verbose, pbegin, pend, pszFormat, fSpaces);
}

inline void PrintHex(enumLogContext group, enumVerbosityLevel verbosity, const vector<unsigned char>& vch, const char* pszFormat="%s", bool fSpaces=true)
{
    OutputLogMessageF(group, verbosity, pszFormat, HexStr(vch, fSpaces).c_str());
}

// Maintained for backward compatibility. Prefer the overloaded version.
inline void PrintHex(const vector<unsigned char>& vch, const char* pszFormat="%s", bool fSpaces=true)
{
    PrintHex(LC_All, VL_Verbose, vch, pszFormat, fSpaces);
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
