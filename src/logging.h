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
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
static const bool DEFAULT_LOGSOURCELOCATIONS = false;
static constexpr bool DEFAULT_LOGLEVELALWAYS = false;
static constexpr bool DEFAULT_RATELIMITLOGGING{true};
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct SourceLocationEqual {
    bool operator()(const std::source_location& lhs, const std::source_location& rhs) const noexcept
    {
        return strcmp(lhs.file_name(), rhs.file_name()) == 0 && lhs.line() == rhs.line();
    }
};

struct SourceLocationHasher {
    size_t operator()(const std::source_location& s) const noexcept
    {
        // Use CSipHasher(0, 0) as a simple way to get uniform distribution.
        return static_cast<size_t>(CSipHasher(0, 0)
                                       .Write(std::hash<std::string>{}(s.file_name()))
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
        NONE                       = CategoryMask{0},
        NET                        = (CategoryMask{1} <<  0),
        TOR                        = (CategoryMask{1} <<  1),
        MEMPOOL                    = (CategoryMask{1} <<  2),
        HTTP                       = (CategoryMask{1} <<  3),
        BENCH                      = (CategoryMask{1} <<  4),
        ZMQ                        = (CategoryMask{1} <<  5),
        WALLETDB                   = (CategoryMask{1} <<  6),
        RPC                        = (CategoryMask{1} <<  7),
        ESTIMATEFEE                = (CategoryMask{1} <<  8),
        ADDRMAN                    = (CategoryMask{1} <<  9),
        SELECTCOINS                = (CategoryMask{1} << 10),
        REINDEX                    = (CategoryMask{1} << 11),
        CMPCTBLOCK                 = (CategoryMask{1} << 12),
        RAND                       = (CategoryMask{1} << 13),
        PRUNE                      = (CategoryMask{1} << 14),
        PROXY                      = (CategoryMask{1} << 15),
        MEMPOOLREJ                 = (CategoryMask{1} << 16),
        LIBEVENT                   = (CategoryMask{1} << 17),
        COINDB                     = (CategoryMask{1} << 18),
        QT                         = (CategoryMask{1} << 19),
        LEVELDB                    = (CategoryMask{1} << 20),
        VALIDATION                 = (CategoryMask{1} << 21),
        I2P                        = (CategoryMask{1} << 22),
        IPC                        = (CategoryMask{1} << 23),
#ifdef DEBUG_LOCKCONTENTION
        LOCK                       = (CategoryMask{1} << 24),
#endif
        BLOCKSTORAGE               = (CategoryMask{1} << 25),
        TXRECONCILIATION           = (CategoryMask{1} << 26),
        SCAN                       = (CategoryMask{1} << 27),
        TXPACKAGES                 = (CategoryMask{1} << 28),
        UNCONDITIONAL_RATE_LIMITED = (CategoryMask{1} << 29),
        UNCONDITIONAL_ALWAYS       = (CategoryMask{1} << 30),
        ALL                        = ~NONE,
    };
    enum class Level {
        Trace = 0, // High-volume or detailed logging for development/debugging
        Debug,     // Reasonably noisy logging, but still usable in production
        Info,      // Default
        Warning,
        Error,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};
    static constexpr LogFlags DEFAULT_LOG_FLAGS{UNCONDITIONAL_RATE_LIMITED | UNCONDITIONAL_ALWAYS};
    constexpr size_t DEFAULT_MAX_LOG_BUFFER{1'000'000}; // buffer up to 1MB of log data prior to StartLogging

    //! Fixed window rate limiter for logging.
    class LogRateLimiter
    {
    private:
        //! Timestamp of the last window reset.
        std::chrono::time_point<NodeClock> m_last_reset;
        //! Remaining bytes in the current window interval.
        uint64_t m_available_bytes{WINDOW_MAX_BYTES};
        //! Number of bytes that were not consumed within the current window.
        uint64_t m_dropped_bytes{0};
        //! Reset the window if the window interval has passed since the last reset.
        void MaybeReset();

    public:
        //! Interval after which the window is reset.
        static constexpr std::chrono::hours WINDOW_SIZE{1};
        //! The maximum number of bytes that can be logged within one window.
        static constexpr uint64_t WINDOW_MAX_BYTES{1024 * 1024};

        LogRateLimiter() : m_last_reset{NodeClock::now()} {}

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

    class Logger
    {
    public:
        struct BufferedLog {
            SystemClock::time_point now;
            std::chrono::seconds mocktime;
            std::string str, logging_function, threadname;
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

        //! Category-specific log level. Overrides `m_log_level`.
        std::unordered_map<LogFlags, Level> m_category_log_levels GUARDED_BY(m_cs);

        //! If there is no category-specific log level, all logs with a severity
        //! level lower than `m_log_level` will be ignored.
        std::atomic<Level> m_log_level{DEFAULT_LOG_LEVEL};

        /** Log categories bitfield. */
        std::atomic<CategoryMask> m_categories{DEFAULT_LOG_FLAGS};

        void FormatLogStrInPlace(std::string& str, LogFlags category, Level level, const std::source_location& source_loc, std::string_view logging_function, std::string_view threadname, SystemClock::time_point now, std::chrono::seconds mocktime) const;

        std::string LogTimestampStr(SystemClock::time_point now, std::chrono::seconds mocktime) const;

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks GUARDED_BY(m_cs) {};

        /** Send a string to the log output (internal) */
        void LogPrintStr_(std::string_view str, std::string_view logging_function, const std::source_location& source_loc, BCLog::LogFlags category, BCLog::Level level)
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
        bool m_ratelimit{DEFAULT_RATELIMITLOGGING};

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        /** Send a string to the log output */
        void LogPrintStr(std::string_view str, std::string_view logging_function, const std::source_location& source_loc, BCLog::LogFlags category, BCLog::Level level)
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
inline void LogPrintFormatInternal(std::string_view logging_function, const std::source_location& source_loc, const BCLog::LogFlags flag, const BCLog::Level level, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
        }
        LogInstance().LogPrintStr(log_msg, logging_function, source_loc, flag, level);
    }
}

#define LogPrintLevel_(category, level, ...) LogPrintFormatInternal(__func__, std::source_location::current(), category, level, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::UNCONDITIONAL_RATE_LIMITED, BCLog::Level::Info, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::UNCONDITIONAL_RATE_LIMITED, BCLog::Level::Warning, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::UNCONDITIONAL_RATE_LIMITED, BCLog::Level::Error, __VA_ARGS__)

// Deprecated unconditional logging.
#define LogPrintf(...) LogInfo(__VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log conditionally, prefixing the output with the passed category name and severity level.
// Note that conditional logging is performed WITHOUT rate limiting. Users specifying
// -debug are assumed to be developers or power users who are aware that -debug may cause
// excessive disk usage due to logging.
#define LogPrintLevel(category, level, ...)               \
    do {                                                  \
        if (LogAcceptCategory((category), (level))) {     \
            LogPrintLevel_(category, level, __VA_ARGS__); \
        }                                                 \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) LogPrintLevel(category, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) LogPrintLevel(category, BCLog::Level::Trace, __VA_ARGS__)

#endif // BITCOIN_LOGGING_H
