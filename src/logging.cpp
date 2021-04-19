// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <util.h>
#include <utilstrencodings.h>

#include <list>
#include <mutex>

const char * const DEFAULT_DEBUGLOGFILE = "debug.log";

bool fPrintToConsole = false;
bool fPrintToDebugLog = true;

bool fLogTimestamps = DEFAULT_LOGTIMESTAMPS;
bool fLogTimeMicros = DEFAULT_LOGTIMEMICROS;
bool fLogThreadNames = DEFAULT_LOGTHREADNAMES;
bool fLogIPs = DEFAULT_LOGIPS;
std::atomic<bool> fReopenDebugLog(false);

/** Log categories bitfield. */
std::atomic<uint64_t> logCategories(0);
/**
 * LogPrintf() has been broken a couple of times now
 * by well-meaning people adding mutexes in the most straightforward way.
 * It breaks because it may be called by global destructors during shutdown.
 * Since the order of destruction of static/global objects is undefined,
 * defining a mutex as a global object doesn't work (the mutex gets
 * destroyed, and then some later destructor calls OutputDebugStringF,
 * maybe indirectly, and you get a core dump at shutdown trying to lock
 * the mutex).
 */

static std::once_flag debugPrintInitFlag;

/**
 * We use std::call_once() to make sure mutexDebugLog and
 * vMsgsBeforeOpenLog are initialized in a thread-safe manner.
 *
 * NOTE: fileout, mutexDebugLog and sometimes vMsgsBeforeOpenLog
 * are leaked on exit. This is ugly, but will be cleaned up by
 * the OS/libc. When the shutdown sequence is fully audited and
 * tested, explicit destruction of these objects can be implemented.
 */
static FILE* fileout = nullptr;
static std::mutex* mutexDebugLog = nullptr;
static std::list<std::string>* vMsgsBeforeOpenLog;

static int FileWriteStr(const std::string &str, FILE *fp)
{
    return fwrite(str.data(), 1, str.size(), fp);
}

static void DebugPrintInit()
{
    assert(mutexDebugLog == nullptr);
    mutexDebugLog = new std::mutex();
    vMsgsBeforeOpenLog = new std::list<std::string>;
}

fs::path GetDebugLogPath()
{
    fs::path logfile(gArgs.GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    return AbsPathForConfigVal(logfile);
}

bool OpenDebugLog()
{
    std::call_once(debugPrintInitFlag, &DebugPrintInit);
    std::lock_guard<std::mutex> scoped_lock(*mutexDebugLog);

    assert(fileout == nullptr);
    assert(vMsgsBeforeOpenLog);
    fs::path pathDebug = GetDebugLogPath();

    fileout = fsbridge::fopen(pathDebug, "a");
    if (!fileout) {
        return false;
    }

    setbuf(fileout, nullptr); // unbuffered
    // dump buffered messages from before we opened the log
    while (!vMsgsBeforeOpenLog->empty()) {
        FileWriteStr(vMsgsBeforeOpenLog->front(), fileout);
        vMsgsBeforeOpenLog->pop_front();
    }

    delete vMsgsBeforeOpenLog;
    vMsgsBeforeOpenLog = nullptr;
    return true;
}

struct CLogCategoryDesc
{
    uint64_t flag;
    std::string category;
};

const CLogCategoryDesc LogCategories[] =
{
    {BCLog::NONE, "0"},
    {BCLog::NONE, "none"},
    {BCLog::NET, "net"},
    {BCLog::TOR, "tor"},
    {BCLog::MEMPOOL, "mempool"},
    {BCLog::HTTP, "http"},
    {BCLog::BENCHMARK, "bench"},
    {BCLog::ZMQ, "zmq"},
    {BCLog::DB, "db"},
    {BCLog::RPC, "rpc"},
    {BCLog::ESTIMATEFEE, "estimatefee"},
    {BCLog::ADDRMAN, "addrman"},
    {BCLog::SELECTCOINS, "selectcoins"},
    {BCLog::REINDEX, "reindex"},
    {BCLog::CMPCTBLOCK, "cmpctblock"},
    {BCLog::RANDOM, "rand"},
    {BCLog::PRUNE, "prune"},
    {BCLog::PROXY, "proxy"},
    {BCLog::MEMPOOLREJ, "mempoolrej"},
    {BCLog::LIBEVENT, "libevent"},
    {BCLog::COINDB, "coindb"},
    {BCLog::QT, "qt"},
    {BCLog::LEVELDB, "leveldb"},
    {BCLog::ALL, "1"},
    {BCLog::ALL, "all"},

    //Start Dash
    {BCLog::CHAINLOCKS, "chainlocks"},
    {BCLog::GOBJECT, "gobject"},
    {BCLog::INSTANTSEND, "instantsend"},
    {BCLog::KEEPASS, "keepass"},
    {BCLog::LLMQ, "llmq"},
    {BCLog::LLMQ_DKG, "llmq-dkg"},
    {BCLog::LLMQ_SIGS, "llmq-sigs"},
    {BCLog::MNPAYMENTS, "mnpayments"},
    {BCLog::MNSYNC, "mnsync"},
    {BCLog::COINJOIN, "coinjoin"},
    {BCLog::SPORK, "spork"},
    {BCLog::NETCONN, "netconn"},
    //End Dash
};

bool GetLogCategory(uint64_t *f, const std::string *str)
{
    if (f && str) {
        if (*str == "") {
            *f = BCLog::ALL;
            return true;
        }
        if (*str == "dash") {
            *f = BCLog::CHAINLOCKS
                 | BCLog::GOBJECT
                 | BCLog::INSTANTSEND
                 | BCLog::KEEPASS
                 | BCLog::LLMQ
                 | BCLog::LLMQ_DKG
                 | BCLog::LLMQ_SIGS
                 | BCLog::MNPAYMENTS
                 | BCLog::MNSYNC
                 | BCLog::COINJOIN
                 | BCLog::SPORK;
            return true;
        }
        for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
            if (LogCategories[i].category == *str) {
                *f = LogCategories[i].flag;
                return true;
            }
        }
    }
    return false;
}

std::string ListLogCategories()
{
    std::string ret;
    int outcount = 0;
    for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
        // Omit the special cases.
        if (LogCategories[i].flag != BCLog::NONE && LogCategories[i].flag != BCLog::ALL) {
            if (outcount != 0) ret += ", ";
            ret += LogCategories[i].category;
            outcount++;
        }
    }
    return ret;
}

std::vector<CLogCategoryActive> ListActiveLogCategories()
{
    std::vector<CLogCategoryActive> ret;
    for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
        // Omit the special cases.
        if (LogCategories[i].flag != BCLog::NONE && LogCategories[i].flag != BCLog::ALL) {
            CLogCategoryActive catActive;
            catActive.category = LogCategories[i].category;
            catActive.active = LogAcceptCategory(LogCategories[i].flag);
            ret.push_back(catActive);
        }
    }
    return ret;
}

std::string ListActiveLogCategoriesString()
{
    if (logCategories == BCLog::NONE)
        return "0";
    if (logCategories == BCLog::ALL)
        return "1";

    std::string ret;
    int outcount = 0;
    for (unsigned int i = 0; i < ARRAYLEN(LogCategories); i++) {
        // Omit the special cases.
        if (LogCategories[i].flag != BCLog::NONE && LogCategories[i].flag != BCLog::ALL && LogAcceptCategory(LogCategories[i].flag)) {
            if (outcount != 0) ret += ", ";
            ret += LogCategories[i].category;
            outcount++;
        }
    }
    return ret;
}

/**
 * fStartedNewLine is a state variable held by the calling context that will
 * suppress printing of the timestamp when multiple calls are made that don't
 * end in a newline. Initialize it to true, and hold it, in the calling context.
 */
static std::string LogTimestampStr(const std::string &str, std::atomic_bool *fStartedNewLine)
{
    std::string strStamped;

    if (!fLogTimestamps)
        return str;

    if (*fStartedNewLine) {
        int64_t nTimeMicros = GetTimeMicros();
        strStamped = FormatISO8601DateTime(nTimeMicros/1000000);
        if (fLogTimeMicros) {
            strStamped.pop_back();
            strStamped += strprintf(".%06dZ", nTimeMicros%1000000);
        }
        int64_t mocktime = GetMockTime();
        if (mocktime) {
            strStamped += " (mocktime: " + FormatISO8601DateTime(mocktime) + ")";
        }
        strStamped += ' ' + str;
    } else
        strStamped = str;

    return strStamped;
}

/**
 * fStartedNewLine is a state variable held by the calling context that will
 * suppress printing of the thread name when multiple calls are made that don't
 * end in a newline. Initialize it to true, and hold/manage it, in the calling context.
 */
static std::string LogThreadNameStr(const std::string &str, std::atomic_bool *fStartedNewLine)
{
    std::string strThreadLogged;

    if (!fLogThreadNames)
        return str;

    std::string strThreadName = GetThreadName();

    if (*fStartedNewLine)
        strThreadLogged = strprintf("%16s | %s", strThreadName.c_str(), str.c_str());
    else
        strThreadLogged = str;

    return strThreadLogged;
}

int LogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    static std::atomic_bool fStartedNewLine(true);

    std::string strThreadLogged = LogThreadNameStr(str, &fStartedNewLine);
    std::string strTimestamped = LogTimestampStr(strThreadLogged, &fStartedNewLine);

    if (!str.empty() && str[str.size()-1] == '\n')
        fStartedNewLine = true;
    else
        fStartedNewLine = false;

    if (fPrintToConsole) {
        // print to console
        ret = fwrite(strTimestamped.data(), 1, strTimestamped.size(), stdout);
        fflush(stdout);
    }
    if (fPrintToDebugLog) {
        std::call_once(debugPrintInitFlag, &DebugPrintInit);
        std::lock_guard<std::mutex> scoped_lock(*mutexDebugLog);

        // buffer if we haven't opened the log yet
        if (fileout == nullptr) {
            assert(vMsgsBeforeOpenLog);
            ret = strTimestamped.length();
            vMsgsBeforeOpenLog->push_back(strTimestamped);
        }
        else
        {
            // reopen the log file, if requested
            if (fReopenDebugLog) {
                fReopenDebugLog = false;
                fs::path pathDebug = GetDebugLogPath();
                if (fsbridge::freopen(pathDebug,"a",fileout) != nullptr)
                    setbuf(fileout, nullptr); // unbuffered
            }

            ret = FileWriteStr(strTimestamped, fileout);
        }
    }
    return ret;
}

void ShrinkDebugFile()
{
    // Amount of debug.log to save at end when shrinking (must fit in memory)
    constexpr size_t RECENT_DEBUG_HISTORY_SIZE = 10 * 1000000;
    // Scroll debug.log if it's getting too big
    fs::path pathLog = GetDebugLogPath();
    FILE* file = fsbridge::fopen(pathLog, "r");

    // Special files (e.g. device nodes) may not have a size.
    size_t log_size = 0;
    try {
        log_size = fs::file_size(pathLog);
    } catch (boost::filesystem::filesystem_error &) {}

    // If debug.log file is more than 10% bigger the RECENT_DEBUG_HISTORY_SIZE
    // trim it down by saving only the last RECENT_DEBUG_HISTORY_SIZE bytes
    if (file && log_size > 11 * (RECENT_DEBUG_HISTORY_SIZE / 10))
    {
        // Restart the file with some of the end
        std::vector<char> vch(RECENT_DEBUG_HISTORY_SIZE, 0);
        fseek(file, -((long)vch.size()), SEEK_END);
        int nBytes = fread(vch.data(), 1, vch.size(), file);
        fclose(file);

        file = fsbridge::fopen(pathLog, "w");
        if (file)
        {
            fwrite(vch.data(), 1, nBytes, file);
            fclose(file);
        }
    }
    else if (file != nullptr)
        fclose(file);
}
