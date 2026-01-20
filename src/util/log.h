// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <logging/categories.h> // IWYU pragma: export
#include <threadsafety.h>
#include <tinyformat.h>
#include <util/check.h>
#include <util/string.h>
#include <util/threadnames.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <source_location>
#include <string>

/// Like std::source_location, but allowing to override the function name.
class SourceLocation
{
public:
    /// The func argument must be constructed from the C++11 __func__ macro.
    /// Ref: https://en.cppreference.com/w/cpp/language/function.html#func
    /// Non-static string literals are not supported.
    SourceLocation(const char* func,
                   std::source_location loc = std::source_location::current())
        : m_func{func}, m_loc{loc} {}

    std::string_view file_name() const { return m_loc.file_name(); }
    std::uint_least32_t line() const { return m_loc.line(); }
    std::string_view function_name_short() const { return m_func; }

private:
    std::string_view m_func;
    std::source_location m_loc;
};

namespace util::log {

enum class Level {
    Trace = 0, // High-volume or detailed logging for development/debugging
    Debug,     // Reasonably noisy logging, but still usable in production
    Info,      // Default
    Warning,
    Error,
};

/** Structured log entry passed to registered callbacks. */
struct Entry {
    //! Opaque to util::log; interpreted by consumers (e.g., BCLog::LogFlags).
    uint64_t category;
    std::string message;
    SourceLocation source_loc;
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
    std::string thread_name{util::ThreadGetInternalName()};
    Level level;
    bool should_ratelimit{false}; //!< Hint for consumers if this entry should be ratelimited
};

/**
 * Dispatcher is responsible for producing logs. It forwards structured log entries to one or
 * multiple logging sinks (e.g. BCLog::Logger) through its registered callbacks.
 *
 * Log consumption (including printing and rate limiting) should be implemented by the sink.
 */
class Dispatcher
{
public:
    //! Type for callbacks invoked for each log entry that passes filtering.
    using Callback = std::function<void(const Entry&)>;
    //! Type for opaque handles returned by RegisterCallback(), used to unregister.
    using CallbackHandle = std::list<Callback>::iterator;
    //! Type for predicates called before logging; returns true if entry should be dispatched.
    using FilterFunc = std::function<bool(Level level, uint64_t category)>;

    /**
     * Construct a Dispatcher.
     * @param[in] filter  Optional predicate to filter log entries before dispatch to minimize
     *                    overhead (e.g. string formatting) for entries that all of the sinks would
     *                    be discarding anyway (e.g. due to a low logging level).
     *                    If null, all entries are dispatched (when callbacks are registered).
     */
    Dispatcher(FilterFunc filter = nullptr) : m_filter{std::move(filter)} {}

    /**
     * Register a callback to receive log entries.
     * @param[in] callback  Invoked for each log entry that passes filtering.
     * @return Handle to use with UnregisterCallback().
     */
    [[nodiscard]] CallbackHandle RegisterCallback(Callback callback) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /**
     * Unregister a previously registered callback.
     * @param[in] handle  Handle previously returned by RegisterCallback().
     */
    void UnregisterCallback(CallbackHandle handle) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** @return true if any callbacks are registered. */
    bool Enabled() const { return m_callback_count.load(std::memory_order_acquire) > 0; }

    /** @return true if Enabled() and the filter (if set) passes for the provided level and category. */
    bool WillLog(Level level, uint64_t category) const { return Enabled() && (!m_filter || m_filter(level, category)); }

    /**
     * Format message and dispatch to all registered callbacks. No-op if WillLog() doesn't pass.
     * @param[in] level             Severity level.
     * @param[in] category          Opaque category identifier for consumer filtering.
     * @param[in] loc               Source location of the log call.
     * @param[in] should_ratelimit  Hint for consumers if this entry should be ratelimited.
     * @param[in] fmt               Format string.
     * @param[in] args              Format arguments.
     */
    template <typename... Args>
    void Log(Level level, uint64_t category, SourceLocation&& loc, bool should_ratelimit,
             util::ConstevalFormatString<sizeof...(Args)> fmt, const Args&... args)
        EXCLUSIVE_LOCKS_REQUIRED(!m_mutex)
    {
        if (!WillLog(level, category)) return;

        std::string message;
        try {
            message = tfm::format(fmt, args...);
        } catch (const tinyformat::format_error& e) {
            message = std::string{"Format error: "} + e.what() + " in: " + fmt.fmt;
        }

        Entry entry{
            .category = category,
            .message = std::move(message),
            .source_loc = std::move(loc),
            .level = level,
            .should_ratelimit = should_ratelimit,
        };

        StdLockGuard lock{m_mutex};
        for (const auto& callback : m_callbacks) {
            callback(entry);
        }
    }

private:
    mutable StdMutex m_mutex;
    //! Callbacks to be executed when an eligible log statement is produced.
    std::list<Callback> m_callbacks GUARDED_BY(m_mutex);
    //! Lock-free size of m_callbacks for fast checks.
    std::atomic<size_t> m_callback_count{0};
    //! Optional filter set in constructor; @see WillLog().
    const FilterFunc m_filter{nullptr};
};

/**
 * Return a reference to the global dispatcher.
 *
 * This function is declared here but implemented differently by different binaries, see e.g.
 * logging.cpp and bitcoinkernel.cpp.
 */
Dispatcher& g_dispatcher();

} // namespace util::log

/**
 * Return true if global logger will log at the specified category and level.
 * @see util::log::Dispatcher::WillLog(Level level, uint64_t category)
 */
static inline bool LogAcceptCategory(uint64_t category, util::log::Level level)
{
    return util::log::g_dispatcher().WillLog(level, category);
}

// Allow __func__ to be used in any context without warnings:
// NOLINTNEXTLINE(bugprone-lambda-function-name)
#define LogPrintLevel_(category, level, should_ratelimit, ...) util::log::g_dispatcher().Log(level, category, SourceLocation{__func__}, should_ratelimit, __VA_ARGS__)

// Arguments are always evaluated, even when logging is disabled.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, util::log::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)

// Use a macro instead of a function to prevent evaluating arguments when the log will be dropped anyway.
#define detail_LogIfCategoryAndLevelEnabled(category, level, ...)                     \
    do {                                                                              \
        Assume(level < util::log::Level::Info);                                       \
        if (LogAcceptCategory((category), (level))) {                                 \
            LogPrintLevel_(category, level, /*should_ratelimit=*/false, __VA_ARGS__); \
        }                                                                             \
    } while (0)

#define LogDebug(category, ...) detail_LogIfCategoryAndLevelEnabled(category, util::log::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) detail_LogIfCategoryAndLevelEnabled(category, util::log::Level::Trace, __VA_ARGS__)

#endif // BITCOIN_UTIL_LOG_H
