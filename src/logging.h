// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <crypto/siphash.h>
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/time.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <list>
#include <mutex>
#include <source_location>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
static const bool DEFAULT_LOGSOURCELOCATIONS = false;
static constexpr bool DEFAULT_LOGLEVELALWAYS = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct SourceLocationEqual {
    bool operator()(const std::source_location& lhs, const std::source_location& rhs) const noexcept
    {
        return lhs.line() == rhs.line() && strcmp(lhs.file_name(), rhs.file_name()) == 0;
    }
};

struct SourceLocationHasher {
    size_t operator()(const std::source_location& s) const noexcept
    {
        // Use CSipHasher(0, 0) as a simple way to get uniform distribution.
        return static_cast<size_t>(CSipHasher(0, 0)
                                       .Write(std::hash<std::string_view>{}(s.file_name()))
                                       .Write(std::hash<int>{}(s.line()))
                                       .Finalize());
    }
};

struct LogCategory {
    std::string category;
    bool active;
};

namespace BCLog {
    using CategoryMask = uint64_t;
    enum LogFlags : CategoryMask {
        NONE        = CategoryMask{0},
        NET         = (CategoryMask{1} <<  0),
        TOR         = (CategoryMask{1} <<  1),
        MEMPOOL     = (CategoryMask{1} <<  2),
        HTTP        = (CategoryMask{1} <<  3),
        BENCH       = (CategoryMask{1} <<  4),
        ZMQ         = (CategoryMask{1} <<  5),
        WALLETDB    = (CategoryMask{1} <<  6),
        RPC         = (CategoryMask{1} <<  7),
        ESTIMATEFEE = (CategoryMask{1} <<  8),
        ADDRMAN     = (CategoryMask{1} <<  9),
        SELECTCOINS = (CategoryMask{1} << 10),
        REINDEX     = (CategoryMask{1} << 11),
        CMPCTBLOCK  = (CategoryMask{1} << 12),
        RAND        = (CategoryMask{1} << 13),
        PRUNE       = (CategoryMask{1} << 14),
        PROXY       = (CategoryMask{1} << 15),
        MEMPOOLREJ  = (CategoryMask{1} << 16),
        LIBEVENT    = (CategoryMask{1} << 17),
        COINDB      = (CategoryMask{1} << 18),
        QT          = (CategoryMask{1} << 19),
        LEVELDB     = (CategoryMask{1} << 20),
        VALIDATION  = (CategoryMask{1} << 21),
        I2P         = (CategoryMask{1} << 22),
        IPC         = (CategoryMask{1} << 23),
#ifdef DEBUG_LOCKCONTENTION
        LOCK        = (CategoryMask{1} << 24),
#endif
        BLOCKSTORAGE = (CategoryMask{1} << 25),
        TXRECONCILIATION = (CategoryMask{1} << 26),
        SCAN        = (CategoryMask{1} << 27),
        TXPACKAGES  = (CategoryMask{1} << 28),
        ALL         = ~NONE,
    };
    enum class Level {
        Trace = 0, // High-volume or detailed logging for development/debugging
        Debug,     // Reasonably noisy logging, but still usable in production
        Info,      // Default
        Warning,
        Error,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};
    constexpr size_t DEFAULT_MAX_LOG_BUFFER{1'000'000}; // buffer up to 1MB of log data prior to StartLogging
    constexpr uint64_t RATELIMIT_MAX_BYTES{1024 * 1024}; // maximum number of bytes that can be logged within one window

    //! Keeps track of an individual source location and how many available bytes are left for logging from it.
    class SourceLocationCounter
    {
    private:
        //! Remaining bytes in the current window interval.
        uint64_t m_available_bytes{RATELIMIT_MAX_BYTES};
        //! Number of bytes that were not consumed within the current window.
        uint64_t m_dropped_bytes{0};

    public:
        //! Consume bytes from the window if enough bytes are available.
        //!
        //! Returns whether or not enough bytes were available.
        bool Consume(uint64_t bytes);

        uint64_t GetAvailableBytes() const
        {
            return m_available_bytes;
        }

        uint64_t GetDroppedBytes() const
        {
            return m_dropped_bytes;
        }
    };

    /**
     * Fixed window rate limiter for logging.
     *
     * This class is not thread-safe.
     */
    class LogRateLimiter
    {
    private:
        //! Timestamp of the last window reset.
        std::chrono::time_point<NodeClock> m_last_reset;

        //! Counters for each source location that has attempted to log something.
        std::unordered_map<std::source_location, SourceLocationCounter, SourceLocationHasher, SourceLocationEqual> m_source_locations;
        //! Set of source file locations that were dropped on the last log attempt.
        std::unordered_set<std::source_location, SourceLocationHasher, SourceLocationEqual> m_suppressed_locations;

        //! Attempts to reset the logging window if the window interval has passed. This will clear
        //! m_source_locations and m_suppressed_locations if a reset occurs.
        void MaybeResetWindow(std::string&);

    public:
        //! Interval after which the window is reset.
        static constexpr std::chrono::hours WINDOW_SIZE{1};
        //! Consumes `source_loc`'s available bytes corresponding to the size of the (formatted)
        //! `str` and returns true if it exceeds the rate limit allowance in the current time window.
        bool NeedsRateLimiting(const std::source_location& source_loc, std::string& str);

        LogRateLimiter() : m_last_reset{NodeClock::now()} {}

        friend class Logger;
    };

    class Logger
    {
    public:
        struct BufferedLog {
            SystemClock::time_point now;
            std::chrono::seconds mocktime;
            std::string str, threadname;
            std::source_location source_loc;
            LogFlags category;
            Level level;
        };

    private:
        mutable StdMutex m_cs; // Can not use Mutex from sync.h because in debug mode it would cause a deadlock when a potential deadlock was detected

        FILE* m_fileout GUARDED_BY(m_cs) = nullptr;
        std::list<BufferedLog> m_msgs_before_open GUARDED_BY(m_cs);
        bool m_buffering GUARDED_BY(m_cs) = true; //!< Buffer messages before logging can be started.
        size_t m_max_buffer_memusage GUARDED_BY(m_cs){DEFAULT_MAX_LOG_BUFFER};
        size_t m_cur_buffer_memusage GUARDED_BY(m_cs){0};
        size_t m_buffer_lines_discarded GUARDED_BY(m_cs){0};

        //! Keeps track of source location counters and the current logging window.
        LogRateLimiter m_limiter GUARDED_BY(m_cs);

        //! Category-specific log level. Overrides `m_log_level`.
        std::unordered_map<LogFlags, Level> m_category_log_levels GUARDED_BY(m_cs);

        //! If there is no category-specific log level, all logs with a severity
        //! level lower than `m_log_level` will be ignored.
        std::atomic<Level> m_log_level{DEFAULT_LOG_LEVEL};

        /** Log categories bitfield. */
        std::atomic<CategoryMask> m_categories{BCLog::NONE};

        void FormatLogStrInPlace(std::string& str, LogFlags category, Level level, const std::source_location& source_loc, std::string_view threadname, SystemClock::time_point now, std::chrono::seconds mocktime) const;

        std::string LogTimestampStr(SystemClock::time_point now, std::chrono::seconds mocktime) const;

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks GUARDED_BY(m_cs) {};

        /** Send a string to the log output (internal) */
        void LogPrintStr_(std::string_view str, std::source_location&& source_loc, BCLog::LogFlags category, BCLog::Level level, bool should_ratelimit)
            EXCLUSIVE_LOCKS_REQUIRED(m_cs);

        std::string GetLogPrefix(LogFlags category, Level level) const;

    public:
        bool m_print_to_console = false;
        bool m_print_to_file = false;

        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_threadnames = DEFAULT_LOGTHREADNAMES;
        bool m_log_sourcelocations = DEFAULT_LOGSOURCELOCATIONS;
        bool m_always_print_category_level = DEFAULT_LOGLEVELALWAYS;

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        /** Send a string to the log output */
        void LogPrintStr(std::string_view str, std::source_location&& source_loc, BCLog::LogFlags category, BCLog::Level level, bool should_ratelimit)
            EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        /** Returns whether logs will be written to any output */
        bool Enabled() const EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file || !m_print_callbacks.empty();
        }

        /** Connect a slot to the print signal and return the connection */
        std::list<std::function<void(const std::string&)>>::iterator PushBackCallback(std::function<void(const std::string&)> fun) EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.push_back(std::move(fun));
            return --m_print_callbacks.end();
        }

        /** Delete a connection */
        void DeleteCallback(std::list<std::function<void(const std::string&)>>::iterator it) EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            m_print_callbacks.erase(it);
        }

        /** Start logging (and flush all buffered messages) */
        bool StartLogging() EXCLUSIVE_LOCKS_REQUIRED(!m_cs);
        /** Only for testing */
        void DisconnectTestLogger() EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        /** Only used in testing to reset m_limiter. */
        void ResetLimiter() EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        /** Disable logging
         * This offers a slight speedup and slightly smaller memory usage
         * compared to leaving the logging system in its default state.
         * Mostly intended for libbitcoin-kernel apps that don't want any logging.
         * Should be used instead of StartLogging().
         */
        void DisableLogging() EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        void ShrinkDebugFile();

        std::unordered_map<LogFlags, Level> CategoryLevels() const EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            return m_category_log_levels;
        }
        void SetCategoryLogLevel(const std::unordered_map<LogFlags, Level>& levels) EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            m_category_log_levels = levels;
        }
        bool SetCategoryLogLevel(std::string_view category_str, std::string_view level_str) EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        Level LogLevel() const { return m_log_level.load(); }
        void SetLogLevel(Level level) { m_log_level = level; }
        bool SetLogLevel(std::string_view level);

        CategoryMask GetCategoryMask() const { return m_categories.load(); }

        void EnableCategory(LogFlags flag);
        bool EnableCategory(std::string_view str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(std::string_view str);

        bool WillLogCategory(LogFlags category) const;
        bool WillLogCategoryLevel(LogFlags category, Level level) const EXCLUSIVE_LOCKS_REQUIRED(!m_cs);

        /** Returns a vector of the log categories in alphabetical order. */
        std::vector<LogCategory> LogCategoriesList() const;
        /** Returns a string with the log categories in alphabetical order. */
        std::string LogCategoriesString() const
        {
            return util::Join(LogCategoriesList(), ", ", [&](const LogCategory& i) { return i.category; });
        };

        //! Returns a string with all user-selectable log levels.
        std::string LogLevelsString() const;

        //! Returns the string representation of a log level.
        static std::string LogLevelToStr(BCLog::Level level);

        bool DefaultShrinkDebugFile() const;
    };

} // namespace BCLog

BCLog::Logger& LogInstance();

/** Return true if log accepts specified category, at the specified level. */
static inline bool LogAcceptCategory(BCLog::LogFlags category, BCLog::Level level)
{
    return LogInstance().WillLogCategoryLevel(category, level);
}

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, std::string_view str);

template <typename... Args>
inline void LogPrintFormatInternal(std::source_location&& source_loc, const BCLog::LogFlags flag, const BCLog::Level level, const bool should_ratelimit, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
        }
        LogInstance().LogPrintStr(log_msg, std::move(source_loc), flag, level, should_ratelimit);
    }
}

#define LogPrintLevel_(category, level, should_ratelimit, ...) LogPrintFormatInternal(std::source_location::current(), category, level, should_ratelimit, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)

// Deprecated unconditional logging.
#define LogPrintf(...) LogInfo(__VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log by prefixing the output with the passed category name and severity level. This can either
// log conditionally if the category is allowed or unconditionally if level >= BCLog::Level::Info
// is passed. If this function logs unconditionally, logging to disk is rate-limited. This is
// important so that callers don't need to worry about accidentally introducing a disk-fill
// vulnerability if level >= Info is used. Additionally, users specifying -debug are assumed to be
// developers or power users who are aware that -debug may cause excessive disk usage due to logging.
#define LogPrintLevel(category, level, ...)                           \
    do {                                                              \
        if (LogAcceptCategory((category), (level))) {                 \
            bool rate_limit{level >= BCLog::Level::Info};             \
            LogPrintLevel_(category, level, rate_limit, __VA_ARGS__); \
        }                                                             \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) LogPrintLevel(category, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) LogPrintLevel(category, BCLog::Level::Trace, __VA_ARGS__)

#endif // BITCOIN_LOGGING_H
