// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <fs.h>
#include <tinyformat.h>
#include <threadsafety.h>

#include <atomic>
#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS  = false;
static const bool DEFAULT_LOGIPS         = false;
static const bool DEFAULT_LOGTIMESTAMPS  = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogThreadNames;
extern bool fLogIPs;

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

        DASH        = CHAINLOCKS | GOBJECT | INSTANTSEND | KEEPASS | LLMQ | LLMQ_DKG
                    | LLMQ_SIGS | MNPAYMENTS | MNSYNC | COINJOIN | SPORK | NETCONN,

        NET_NETCONN = NET | NETCONN, // use this to have something logged in NET and NETCONN as well
        //End Dash

        ALL         = ~(uint64_t)0,
    };

    class Logger
    {
    private:
        mutable StdMutex m_cs; // Can not use Mutex from sync.h because in debug mode it would cause a deadlock when a potential deadlock was detected

        FILE* m_fileout GUARDED_BY(m_cs) = nullptr;
        std::list<std::string> m_msgs_before_open GUARDED_BY(m_cs);
        bool m_buffering GUARDED_BY(m_cs) = true; //!< Buffer messages before logging can be started.

        /**
         * m_started_new_line is a state variable that will suppress printing of
         * the timestamp when multiple calls are made that don't end in a
         * newline.
         */
        std::atomic_bool m_started_new_line{true};

        /** Log categories bitfield. */
        std::atomic<uint64_t> m_categories{0};

        std::string LogTimestampStr(const std::string& str);
        std::string LogThreadNameStr(const std::string &str);

    public:
        bool m_print_to_console = false;
        bool m_print_to_file = false;

        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_thread_names = DEFAULT_LOGTHREADNAMES;

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        /** Send a string to the log output */
        void LogPrintStr(const std::string& str);

        /** Returns whether logs will be written to any output */
        bool Enabled() const
        {
            StdLockGuard scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file;
        }

        /** Start logging (and flush all buffered messages) */
        bool StartLogging();

        void ShrinkDebugFile();

        uint64_t GetCategoryMask() const { return m_categories.load(); }

        void EnableCategory(LogFlags flag);
        bool EnableCategory(const std::string& str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(const std::string& str);

        bool WillLogCategory(LogFlags category) const;

        bool DefaultShrinkDebugFile() const;
    };

} // namespace BCLog

extern BCLog::Logger* const g_logger;

/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(BCLog::LogFlags category)
{
    return g_logger->WillLogCategory(category);
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a string with the list of active log categories */
std::string ListActiveLogCategoriesString();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str);

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
    if (g_logger->Enabled()) { \
        std::string _log_msg_; /* Unlikely name to avoid shadowing variables */ \
        try { \
            _log_msg_ = tfm::format(__VA_ARGS__); \
        } catch (tinyformat::format_error &e) { \
            /* Original format string will have newline so don't add one here */ \
            _log_msg_ = "Error \"" + std::string(e.what()) + "\" while formatting log message: " + FormatStringFromLogArgs(__VA_ARGS__); \
        } \
        g_logger->LogPrintStr(_log_msg_); \
    } \
} while(0)

#define LogPrint(category, ...) do { \
    if (LogAcceptCategory((category))) { \
        LogPrintf(__VA_ARGS__); \
    } \
} while(0)
#endif // USE_COVERAGE

#endif // BITCOIN_LOGGING_H
