// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <attributes.h>
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
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace BCLog {
class Logger;
} // namespace BCLog

BCLog::Logger& LogInstance();

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
        return lhs.line() == rhs.line() && std::string_view(lhs.file_name()) == std::string_view(rhs.file_name());
    }
};

struct SourceLocationHasher {
    size_t operator()(const std::source_location& s) const noexcept
    {
        // Use CSipHasher(0, 0) as a simple way to get uniform distribution.
        return size_t(CSipHasher(0, 0)
                      .Write(s.line())
                      .Write(MakeUCharSpan(std::string_view{s.file_name()}))
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
    //! Log level constants. Most code will not need to use these directly and
    //! can use LogTrace, LogDebug, LogInfo, LogWarning, and LogError macros
    //! defined below for less verbosity and more efficiency (by not evaluating
    //! unused format arguments). See macro definitions below or "Logging"
    //! section in developer-notes.md for more detailed information.
    enum class Level {
        Trace = 0, // High-volume or detailed logging for development/debugging
        Debug,     // Reasonably noisy logging, but still usable in production
        Info,      // Default
        Warning,
        Error,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};
    constexpr size_t DEFAULT_MAX_LOG_BUFFER{1'000'000}; // buffer up to 1MB of log data prior to StartLogging
    constexpr uint64_t RATELIMIT_MAX_BYTES{1024 * 1024}; // maximum number of bytes per source location that can be logged within the RATELIMIT_WINDOW
    constexpr auto RATELIMIT_WINDOW{1h}; // time window after which log ratelimit stats are reset
    constexpr bool DEFAULT_LOGRATELIMIT{true};

    //! Fixed window rate limiter for logging.
    class LogRateLimiter
    {
    public:
        //! Keeps track of an individual source location and how many available bytes are left for logging from it.
        struct Stats {
            //! Remaining bytes
            uint64_t m_available_bytes;
            //! Number of bytes that were consumed but didn't fit in the available bytes.
            uint64_t m_dropped_bytes{0};

            Stats(uint64_t max_bytes) : m_available_bytes{max_bytes} {}
            //! Updates internal accounting and returns true if enough available_bytes were remaining
            bool Consume(uint64_t bytes);
        };

    private:
        mutable StdMutex m_mutex;

        //! Stats for each source location that has attempted to log something.
        std::unordered_map<std::source_location, Stats, SourceLocationHasher, SourceLocationEqual> m_source_locations GUARDED_BY(m_mutex);
        //! Whether any log locations are suppressed. Cached view on m_source_locations for performance reasons.
        std::atomic<bool> m_suppression_active{false};
        LogRateLimiter(uint64_t max_bytes, std::chrono::seconds reset_window);

    public:
        using SchedulerFunction = std::function<void(std::function<void()>, std::chrono::milliseconds)>;
        /**
         * @param scheduler_func    Callable object used to schedule resetting the window. The first
         *                          parameter is the function to be executed, and the second is the
         *                          reset_window interval.
         * @param max_bytes         Maximum number of bytes that can be logged for each source
         *                          location.
         * @param reset_window      Time window after which the stats are reset.
         */
        static std::shared_ptr<LogRateLimiter> Create(
            SchedulerFunction&& scheduler_func,
            uint64_t max_bytes,
            std::chrono::seconds reset_window);
        //! Maximum number of bytes logged per location per window.
        const uint64_t m_max_bytes;
        //! Interval after which the window is reset.
        const std::chrono::seconds m_reset_window;
        //! Suppression status of a source log location.
        enum class Status {
            UNSUPPRESSED,     // string fits within the limit
            NEWLY_SUPPRESSED, // suppression has started since this string
            STILL_SUPPRESSED, // suppression is still ongoing
        };
        //! Consumes `source_loc`'s available bytes corresponding to the size of the (formatted)
        //! `str` and returns its status.
        [[nodiscard]] Status Consume(
            const std::source_location& source_loc,
            const std::string& str) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
        //! Resets all usage to zero. Called periodically by the scheduler.
        void Reset() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);
        //! Returns true if any log locations are currently being suppressed.
        bool SuppressionsActive() const { return m_suppression_active; }
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

        //! Manages the rate limiting of each log location.
        std::shared_ptr<LogRateLimiter> m_limiter GUARDED_BY(m_cs);

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

        void SetRateLimiting(std::shared_ptr<LogRateLimiter> limiter) EXCLUSIVE_LOCKS_REQUIRED(!m_cs)
        {
            StdLockGuard scoped_lock(m_cs);
            m_limiter = std::move(limiter);
        }

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

    //! Object representing a particular source of log messages. Holds a logging
    //! category, a reference to the logger object to output to, and a
    //! formatting hook.
    struct Context {
        static constexpr bool log_context{true};
        Logger& logger;
        LogFlags category;

        explicit Context(Logger& logger, LogFlags category = LogFlags::ALL) : logger{logger}, category{category} {}

        template <typename... Args>
        std::string Format(util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args) const
        {
            std::string log_msg;
            try {
                log_msg = tfm::format(fmt, args...);
            } catch (tinyformat::format_error& fmterr) {
                log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
            }
            return log_msg;
        }
    };

namespace detail {
//! Internal helper to get log context object from the first macro argument.
template <bool take_category, typename Context>
requires (Context::log_context)
static const Context& GetContext(const Context& ctx LIFETIMEBOUND) { return ctx; }

template <bool take_category>
static const Context& GetContext(const Context& ctx LIFETIMEBOUND) { return ctx; }

template <bool take_category>
static Context GetContext(LogFlags category)
{
    //! Trigger compile error if caller tries to pass a category constant as a
    //! first argument to a logging call that specifies take_category == false.
    //! There is no technical reason why all logging calls could not accept
    //! category arguments, but for various reasons, such as (1) not wanting to
    //! allow users filter by category at high priority levels, and (2) wanting
    //! to incentivize developers to use lower log levels to avoid log spam,
    //! passing category constants at higher levels is forbidden.
    static_assert(take_category, "Cannot pass BCLog::LogFlags category argument to Info/Warning/Error logging call. Please switch to Debug/Trace call, or drop the category argument!");
    return Context{LogInstance(), category};
}

template <bool take_category>
static Context GetContext(std::string_view fmt)
{
    //! Trigger compile error if caller does not pass a category constant as the
    //! first argument to a logging call that specifies take_category == true.
    //! There is no technical reason why category arguments need to be required,
    //! but categories are useful for finding and filtering relevant messages
    //! when debugging, so we want to encourage them.
    static_assert(!take_category, "Missing required BCLog::LogFlags category argument for Debug/Trace logging call. Category can only be omitted for Info/Warning/Error calls.");
    return Context{LogInstance()};
}

//! Internal helper to format log arguments and call a logging function.
template <typename LogFn, typename Context, typename ContextArg, typename... Args>
requires (!std::is_convertible_v<ContextArg, std::string_view>)
void Format(LogFn&& log, Context&& ctx, ContextArg&&, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    log(ctx.Format(fmt, args...));
}
template <typename LogFn, typename Context, typename... Args>
void Format(LogFn&& log, Context&& ctx, util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
{
    log(ctx.Format(fmt, args...));
}
} // namespace detail
} // namespace BCLog

//! Determine whether logging is enabled in a context at the specified level.
template <typename Context>
static inline bool LogEnabled(const Context& ctx, BCLog::Level level)
{
    return ctx.logger.WillLogCategoryLevel(ctx.category, level) && ctx.logger.Enabled();
}

/** Return true if log accepts specified category, at the specified level. */
static inline bool LogAcceptCategory(BCLog::LogFlags category, BCLog::Level level)
{
    return LogInstance().WillLogCategoryLevel(category, level);
}

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, std::string_view str);

//! Internal helper to return first arg in a __VA_ARGS__ pack. This could be
//! simplified to `#define FirstArg_(arg, ...) arg` if not for a preprocessor
//! bug in Visual C++.
#define FirstArg_Impl(arg, ...) arg
#define FirstArg_(args) FirstArg_Impl args

//! Internal helper to conditionally log. Only evaluates arguments when needed.
#define LogPrint_(level, ratelimit, take_category, ...)                        \
    do {                                                                       \
        const auto& ctx{BCLog::detail::GetContext<take_category>(FirstArg_((__VA_ARGS__)))}; \
        if (const BCLog::Level lvl{level}; LogEnabled(ctx, lvl)) {             \
            const BCLog::LogFlags cat{take_category ? ctx.category : BCLog::LogFlags::ALL}; \
            const bool rl{ratelimit};                                          \
            std::source_location loc{std::source_location::current()};         \
            BCLog::detail::Format([&](auto&& message) {                        \
                ctx.logger.LogPrintStr(message, std::move(loc), cat, lvl, rl); \
            }, ctx, __VA_ARGS__);                                              \
        }                                                                      \
    } while (0)

//! Logging macros which output log messages at the specified levels, and avoid
//! evaluating their arguments if logging is not enabled for the level. The
//! macros accept an optional log context parameter followed by a printf-style
//! format string and arguments.
//!
//! - LogError(), LogWarning(), and LogInfo() are all enabled by default, so
//!   they should be called infrequently, in cases where they will not spam the
//!   log and take up disk space.
//!
//! - LogDebug() is enabled when debug logging is enabled, and should be used to
//!   show messages that can help users troubleshoot issues.
//!
//! - LogTrace() is enabled when both debug logging AND tracing are enabled, and
//!   should be used for fine-grained traces that will be helpful to developers.
//!
//! For more information about log levels, see the -debug and -loglevel
//! documentation, or the "Logging" section of developer notes.
//!
//! `LogDebug` and `LogTrace` macros take an initial category argument, so
//! messages can be filtered by category, but categories should be omitted at
//! higher levels:
//
//!   LogDebug(BCLog::TXRECONCILIATION, "Forget txreconciliation state of peer=%d\n", peer_id);
//!   LogInfo("Important information, no category.\n");
//!
//! Context arguments can also be passed to control log output (see class definition).
//!
//!   const BCLog::Context m_log{LogInstance(), BCLog::TXRECONCILIATION};
//!   ...
//!   LogDebug(m_log, "Forget txreconciliation state of peer=%d\n", peer_id);
//!
//! Using context objects also provides the flexibility to add extra information
//! and custom formatting to log messages, or to divert log messages to a local
//! logger instead of the global logging instance, without needing to change
//! existing log statements.
//!
//! If severity level is Info or higher, uses basic rate limiting to mitigate
//! disk filling attacks. Users enabling logging at Debug and lower levels are
//! assumed to be developers or power users who are aware that -debug may cause
//! excessive disk usage due to logging.
#define LogError(...) LogPrint_(BCLog::Level::Error, true, false, __VA_ARGS__)
#define LogWarning(...) LogPrint_(BCLog::Level::Warning, true, false, __VA_ARGS__)
#define LogInfo(...) LogPrint_(BCLog::Level::Info, true, false, __VA_ARGS__)
#define LogDebug(...) LogPrint_(BCLog::Level::Debug, false, true, __VA_ARGS__)
#define LogTrace(...) LogPrint_(BCLog::Level::Trace, false, true, __VA_ARGS__)
#define LogPrintLevel(ctx, level, ...) LogPrint_((level), (level) >= BCLog::Level::Info, true, (ctx), __VA_ARGS__)
#define LogPrintLevel_(ctx, level, ratelimit, ...) LogPrint_((level), (ratelimit), true, (ctx), __VA_ARGS__)

//! Deprecated macros.
#define LogPrintf(...) LogInfo(__VA_ARGS__)

#endif // BITCOIN_LOGGING_H
