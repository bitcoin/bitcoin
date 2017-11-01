// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Server/client environment: argument handling, config file parsing,
 * logging, thread wrappers
 */
#ifndef BITCOIN_UTIL_H
#define BITCOIN_UTIL_H

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "allowed_args.h"
#include "compat.h"
#include "fs.h"
#include "tinyformat.h"
#include "utiltime.h"

#include <exception>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/signals2/signal.hpp>
#include <boost/thread/exceptions.hpp>

#ifdef DEBUG_ASSERTION
/// If DEBUG_ASSERTION is enabled this asserts when the predicate is false.
//  If DEBUG_ASSERTION is disabled and the predicate is false, it executes the execInRelease statements.
//  Typically, the programmer will error out -- return false, raise an exception, etc in the execInRelease code.
//  DO NOT USE break or continue inside the DbgAssert!
#define DbgAssert(pred, execInRelease) assert(pred)
#else
#define DbgStringify(x) #x
#define DbgStringifyIntLiteral(x) DbgStringify(x)
#define DbgAssert(pred, execInRelease)                                                                        \
    do                                                                                                        \
    {                                                                                                         \
        if (!(pred))                                                                                          \
        {                                                                                                     \
            LogPrintStr(std::string(                                                                          \
                __FILE__ "(" DbgStringifyIntLiteral(__LINE__) "): Debug Assertion failed: \"" #pred "\"\n")); \
            execInRelease;                                                                                    \
        }                                                                                                     \
    } while (0)
#endif

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS = true;
static const bool DEFAULT_LOGTIMESTAMPS = true;

// For bitcoin-cli
extern const char DEFAULT_RPCCONNECT[];
static const int DEFAULT_HTTP_CLIENT_TIMEOUT = 900;

/** Signals for translation. */
class CTranslationInterface
{
public:
    /** Translate a message to the native language of the user. */
    boost::signals2::signal<std::string(const char *psz)> Translate;
};

extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;
extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugLog;
extern bool fServer;
extern std::string strMiscWarning;
extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogIPs;
extern volatile bool fReopenDebugLog;
extern CTranslationInterface translationInterface;

extern const char *const BITCOIN_CONF_FILENAME;
extern const char *const BITCOIN_PID_FILENAME;

/**
 * Translation function: Call Translate signal on UI interface, which returns a boost::optional result.
 * If no translation slot is registered, nothing is returned, and simply return the input.
 */
inline std::string _(const char *psz)
{
    boost::optional<std::string> rv = translationInterface.Translate(psz);
    return rv ? (*rv) : psz;
}

void SetupEnvironment();
bool SetupNetworking();

/** Return true if log accepts specified category */
bool LogAcceptCategory(const char *category);
/** Send a string to the log output */
int LogPrintStr(const std::string &str);

#define LogPrintf(...) LogPrint(NULL, __VA_ARGS__)

template <typename T1, typename... Args>
static inline int LogPrint(const char *category, const char *fmt, const T1 &v1, const Args &... args)
{
    if (!LogAcceptCategory(category))
        return 0;
    return LogPrintStr(tfm::format(fmt, v1, args...));
}

template <typename T1, typename... Args>
bool error(const char *fmt, const T1 &v1, const Args &... args)
{
    LogPrintStr("ERROR: " + tfm::format(fmt, v1, args...) + "\n");
    return false;
}

/**
 * Zero-arg versions of logging and error, these are not covered by
 * the variadic templates above (and don't take format arguments but
 * bare strings).
 */
static inline int LogPrint(const char *category, const char *s)
{
    if (!LogAcceptCategory(category))
        return 0;
    return LogPrintStr(s);
}
static inline bool error(const char *s)
{
    LogPrintStr(std::string("ERROR: ") + s + "\n");
    return false;
}

/**
 Format an amount of bytes with a unit symbol attached, such as MB, KB, GB.
 Uses Kilobytes x1000, not Kibibytes x1024.

 Output value has two digits after the dot. No space between unit symbol and
 amount.

 Also works for negative amounts. The maximum unit supported is 1 Exabyte (EB).
 This formatting is used by the thinblock statistics functions, and this
 is a factored-out utility function.

 @param [value] The value to format
 @return String with unit
 */
extern std::string formatInfoUnit(double value);

void PrintExceptionContinue(const std::exception *pex, const char *pszThread);
void ParseParameters(int argc, const char *const argv[], const AllowedArgs::AllowedArgs &allowedArgs);
void FileCommit(FILE *fileout);
bool TruncateFile(FILE *file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
bool RenameOver(fs::path src, fs::path dest);
bool TryCreateDirectories(const fs::path &p);
fs::path GetDefaultDataDir();
const fs::path &GetDataDir(bool fNetSpecific = true);
void ClearDatadirCache();
fs::path GetConfigFile(const std::string &confPath);
#ifndef WIN32
fs::path GetPidFile();
void CreatePidFile(const fs::path &path, pid_t pid);
#endif
void ReadConfigFile(std::map<std::string, std::string> &mapSettingsRet,
    std::map<std::string, std::vector<std::string> > &mapMultiSettingsRet,
    const AllowedArgs::AllowedArgs &allowedArgs);
#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
void OpenDebugLog();
void ShrinkDebugFile();
void runCommand(const std::string &strCommand);

inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

/**
 * Return string argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. "1")
 * @return command-line argument or default value
 */
std::string GetArg(const std::string &strArg, const std::string &strDefault);

/**
 * Return integer argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (e.g. 1)
 * @return command-line argument (0 if invalid number) or default value
 */
int64_t GetArg(const std::string &strArg, int64_t nDefault);

/**
 * Return boolean argument or default value
 *
 * @param strArg Argument to get (e.g. "-foo")
 * @param default (true or false)
 * @return command-line argument or default value
 */
bool GetBoolArg(const std::string &strArg, bool fDefault);

/**
 * Set an argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param strValue Value (e.g. "1")
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetArg(const std::string &strArg, const std::string &strValue);

/**
 * Set a boolean argument if it doesn't already have a value
 *
 * @param strArg Argument to set (e.g. "-foo")
 * @param fValue Value (e.g. false)
 * @return true if argument gets set, false if it already had a value
 */
bool SoftSetBoolArg(const std::string &strArg, bool fValue);

/**
 * Return the number of physical cores available on the current system.
 * @note This does not count virtual cores, such as those provided by HyperThreading
 * when boost is newer than 1.56.
 */
int GetNumCores();

void SetThreadPriority(int nPriority);
void RenameThread(const char *name);

/**
 * .. and a wrapper that just calls func once
 */
template <typename Callable>
void TraceThread(const char *name, Callable func)
{
    std::string s = strprintf("bitcoin-%s", name);
    RenameThread(s.c_str());
    try
    {
        LogPrintf("%s thread start\n", name);
        func();
        LogPrintf("%s thread exit\n", name);
    }
    catch (const boost::thread_interrupted &)
    {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    }
    catch (const std::exception &e)
    {
        PrintExceptionContinue(&e, name);
        throw;
    }
    catch (...)
    {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

std::string CopyrightHolders(const std::string &strPrefix);

#endif // BITCOIN_UTIL_H
