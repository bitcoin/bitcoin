// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <threadsafety.h>
#include <tinyformat.h>
#include <util/fs.h>
#include <util/string.h>
#include <util/time.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
static const bool DEFAULT_LOGSOURCELOCATIONS = false;
static constexpr bool DEFAULT_LOGLEVELALWAYS = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct LogCategory {
    std::string category;
    bool active;
};

namespace BCLog {
    enum LogFlags {
        NET,
        TOR,
        MEMPOOL,
        HTTP,
        BENCH,
        ZMQ,
        WALLETDB,
        RPC,
        ESTIMATEFEE,
        ADDRMAN,
        SELECTCOINS,
        REINDEX,
        CMPCTBLOCK,
        RAND,
        PRUNE,
        PROXY,
        MEMPOOLREJ,
        LIBEVENT,
        COINDB,
        QT,
        LEVELDB,
        VALIDATION,
        I2P,
        IPC,
#ifdef DEBUG_LOCKCONTENTION
        LOCK,
#endif
        BLOCKSTORAGE,
        TXRECONCILIATION,
        SCAN,
        TXPACKAGES,
        // Add new entries before this line, and also add a
        // corresponding entry to LOG_CATEGORIES_BY_STR.

        // The following have no representation in m_categories:
        ALL, // this is also the size of the bitset
        NONE,
    };

    template<typename FlagType, size_t BITS, typename T>
    class AtomicBitSet {
    private:
        // bits in a single T (an integer type)
        static constexpr size_t BITS_PER_T = 8 * sizeof(T);

        // number of Ts needed for BITS bits (round up)
        static constexpr size_t NT = (BITS + BITS_PER_T - 1) / BITS_PER_T;

        std::atomic<T> m_bits[NT]{0};

    public:
        AtomicBitSet() = default;
        AtomicBitSet(FlagType f) { set(f); }

        AtomicBitSet(const AtomicBitSet&) = delete;
        AtomicBitSet(AtomicBitSet&&) = delete;
        AtomicBitSet& operator=(const AtomicBitSet&) = delete;
        AtomicBitSet& operator=(AtomicBitSet&&) = delete;
        ~AtomicBitSet() = default;

        constexpr size_t size() const { return BITS; }

        // Although any() and none() use all bits to calculate their
        // result, the bits may not be sampled at the same instant.
        bool any() const
        {
            for (const auto& i : m_bits) {
                if (i > 0) return true;
            }
            return false;
        }
        bool none() const { return !any(); }

        void set()
        {
            for (size_t i = 0; i < NT; ++i) {
                m_bits[i] = ~T{0};
            }
        }
        void reset()
        {
            for (size_t i = 0; i < NT; ++i) {
                m_bits[i] = T{0};
            }
        }
        void set(FlagType f)
        {
            if (f < 0 || f >= BITS) return;
            m_bits[f/BITS_PER_T] |= (T{1} << (f % BITS_PER_T));
        }
        void reset(FlagType f)
        {
            if (f < 0 || f >= BITS) return;
            m_bits[f/BITS_PER_T] &= ~(T{1} << (f % BITS_PER_T));
        }
        bool test(FlagType f) const
        {
            if (f < 0 || f >= BITS) return false;
            const T i{m_bits[f/BITS_PER_T].load(std::memory_order_relaxed)};
            return i & (T{1} << (f % BITS_PER_T));
        }
    };
    using LogCategoryMask = AtomicBitSet<LogFlags, LogFlags::ALL, uint64_t>;

    enum class Level {
        Trace = 0, // High-volume or detailed logging for development/debugging
        Debug,     // Reasonably noisy logging, but still usable in production
        Info,      // Default
        Warning,
        Error,
    };
    constexpr auto DEFAULT_LOG_LEVEL{Level::Debug};
    constexpr size_t DEFAULT_MAX_LOG_BUFFER{1'000'000}; // buffer up to 1MB of log data prior to StartLogging

    class Logger
    {
    public:
        struct BufferedLog {
            SystemClock::time_point now;
            std::chrono::seconds mocktime;
            std::string str, logging_function, source_file, threadname;
            int source_line;
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

        /**
         * m_started_new_line is a state variable that will suppress printing of
         * the timestamp when multiple calls are made that don't end in a
         * newline.
         */
        std::atomic_bool m_started_new_line{true};

        //! Category-specific log level. Overrides `m_log_level`.
        std::unordered_map<LogFlags, Level> m_category_log_levels GUARDED_BY(m_cs);

        //! If there is no category-specific log level, all logs with a severity
        //! level lower than `m_log_level` will be ignored.
        std::atomic<Level> m_log_level{DEFAULT_LOG_LEVEL};

        /** Log categories bitfield. */
        LogCategoryMask m_categories{BCLog::NONE};

        void FormatLogStrInPlace(std::string& str, LogFlags category, Level level, std::string_view source_file, int source_line, std::string_view logging_function, std::string_view threadname, SystemClock::time_point now, std::chrono::seconds mocktime) const;

        std::string LogTimestampStr(SystemClock::time_point now, std::chrono::seconds mocktime) const;

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks GUARDED_BY(m_cs) {};

        /** Send a string to the log output (internal) */
        void LogPrintStr_(std::string_view str, std::string_view logging_function, std::string_view source_file, int source_line, BCLog::LogFlags category, BCLog::Level level)
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
        void LogPrintStr(std::string_view str, std::string_view logging_function, std::string_view source_file, int source_line, BCLog::LogFlags category, BCLog::Level level)
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

        const LogCategoryMask& GetCategoryMask() const { return m_categories; }

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

// Be conservative when using functions that
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a user's disk with debug.log entries.

template <typename... Args>
static inline void LogPrintf_(std::string_view logging_function, std::string_view source_file, const int source_line, const BCLog::LogFlags flag, const BCLog::Level level, const char* fmt, const Args&... args)
{
    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            /* Original format string will have newline so don't add one here */
            log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt;
        }
        LogInstance().LogPrintStr(log_msg, logging_function, source_file, source_line, flag, level);
    }
}

#define LogPrintLevel_(category, level, ...) LogPrintf_(__func__, __FILE__, __LINE__, category, level, __VA_ARGS__)

// Log unconditionally.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Warning, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Error, __VA_ARGS__)

// Deprecated unconditional logging.
#define LogPrintf(...) LogInfo(__VA_ARGS__)
#define LogPrintfCategory(category, ...) LogPrintLevel_(category, BCLog::Level::Info, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log conditionally, prefixing the output with the passed category name and severity level.
#define LogPrintLevel(category, level, ...)               \
    do {                                                  \
        if (LogAcceptCategory((category), (level))) {     \
            LogPrintLevel_(category, level, __VA_ARGS__); \
        }                                                 \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) LogPrintLevel(category, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) LogPrintLevel(category, BCLog::Level::Trace, __VA_ARGS__)

// Deprecated conditional logging
#define LogPrint(category, ...)  LogDebug(category, __VA_ARGS__)

#endif // BITCOIN_LOGGING_H
