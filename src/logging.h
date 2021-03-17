// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <fs.h>
#include <tinyformat.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS  = false;
static const bool DEFAULT_LOGIPS         = false;
static const bool DEFAULT_LOGTIMESTAMPS  = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fPrintToConsole;
extern bool fPrintToDebugLog;

extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogThreadNames;
extern bool fLogIPs;
extern std::atomic<bool> fReopenDebugLog;

extern std::atomic<uint64_t> logCategories;

struct CLogCategoryActive
{
    std::string category;
    bool active;
};

namespace BCLog {
    enum LogFlags : uint64_t {
        NONE        = 0,
        NET         = (1 <<  0),
        TOR         = (1 <<  1),
        MEMPOOL     = (1 <<  2),
        HTTP        = (1 <<  3),
        BENCHMARK   = (1 <<  4),
        ZMQ         = (1 <<  5),
        DB          = (1 <<  6),
        RPC         = (1 <<  7),
        ESTIMATEFEE = (1 <<  8),
        ADDRMAN     = (1 <<  9),
        SELECTCOINS = (1 << 10),
        REINDEX     = (1 << 11),
        CMPCTBLOCK  = (1 << 12),
        RANDOM      = (1 << 13),
        PRUNE       = (1 << 14),
        PROXY       = (1 << 15),
        MEMPOOLREJ  = (1 << 16),
        LIBEVENT    = (1 << 17),
        COINDB      = (1 << 18),
        QT          = (1 << 19),
        LEVELDB     = (1 << 20),

        //Start Dash
        CHAINLOCKS  = ((uint64_t)1 << 32),
        GOBJECT     = ((uint64_t)1 << 33),
        INSTANTSEND = ((uint64_t)1 << 34),
        KEEPASS     = ((uint64_t)1 << 35),
        LLMQ        = ((uint64_t)1 << 36),
        LLMQ_DKG    = ((uint64_t)1 << 37),
        LLMQ_SIGS   = ((uint64_t)1 << 38),
        MNPAYMENTS  = ((uint64_t)1 << 39),
        MNSYNC      = ((uint64_t)1 << 40),
        COINJOIN    = ((uint64_t)1 << 41),
        SPORK       = ((uint64_t)1 << 42),
        NETCONN     = ((uint64_t)1 << 43),
        //End Dash

        NET_NETCONN = NET | NETCONN, // use this to have something logged in NET and NETCONN as well

        ALL         = ~(uint64_t)0,
    };
}
/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(uint64_t category)
{
    return (logCategories.load(std::memory_order_relaxed) & category) != 0;
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a string with the list of active log categories */
std::string ListActiveLogCategoriesString();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flags in f */
bool GetLogCategory(uint64_t *f, const std::string *str);

/** Send a string to the log output */
int LogPrintStr(const std::string &str);

/** Formats a string without throwing exceptions. Instead, it'll return an error string instead of formatted string. */
template<typename... Args>
std::string SafeStringFormat(const std::string& fmt, const Args&... args)
{
    try {
        return tinyformat::format(fmt, args...);
    } catch (std::runtime_error& fmterr) {
        std::string message = tinyformat::format("\n****TINYFORMAT ERROR****\n    err=\"%s\"\n    fmt=\"%s\"\n", fmterr.what(), fmt);
        fprintf(stderr, "%s", message.c_str());
        return message;
    }
}

/** Get format string from VA_ARGS for error reporting */
template<typename... Args> std::string FormatStringFromLogArgs(const char *fmt, const Args&... args) { return fmt; }

static inline void MarkUsed() {}
template<typename T, typename... Args> static inline void MarkUsed(const T& t, const Args&... args)
{
    (void)t;
    MarkUsed(args...);
}

// Be conservative when using LogPrintf/error or other things which
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a user's disk with debug.log entries.

#ifdef USE_COVERAGE
#define LogPrintf(...) do { MarkUsed(__VA_ARGS__); } while(0)
#define LogPrint(category, ...) do { MarkUsed(__VA_ARGS__); } while(0)
#else
#define LogPrintf(...) do { \
    if (fPrintToConsole || fPrintToDebugLog) { \
        std::string _log_msg_; /* Unlikely name to avoid shadowing variables */ \
        try { \
            _log_msg_ = tfm::format(__VA_ARGS__); \
        } catch (tinyformat::format_error &e) { \
            /* Original format string will have newline so don't add one here */ \
            _log_msg_ = "Error \"" + std::string(e.what()) + "\" while formatting log message: " + FormatStringFromLogArgs(__VA_ARGS__); \
        } \
        LogPrintStr(_log_msg_); \
    } \
} while(0)

#define LogPrint(category, ...) do { \
    if (LogAcceptCategory((category))) { \
        LogPrintf(__VA_ARGS__); \
    } \
} while(0)
#endif // USE_COVERAGE

fs::path GetDebugLogPath();
bool OpenDebugLog();
void ShrinkDebugFile();

#endif // BITCOIN_LOGGING_H
