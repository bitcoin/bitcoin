// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <threadsafety.h>
#include <tinyformat.h>
#include <util/source.h> // IWYU pragma: export
#include <util/string.h>
#include <util/threadnames.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <string>

namespace util::log {

enum class Level {
    Trace = 0, // High-volume or detailed logging for development/debugging
    Debug,     // Reasonably noisy logging, but still usable in production
    Info,      // Default
    Warning,
    Error,
};

/** Log entry passed to registered callbacks. */
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
 * Dispatcher is responsible for producing logs. It forwards log entries to one or
 * multiple logging sinks (e.g. BCLog::Logger) through its registered callbacks.
 *
 * Log consumption (including printing and rate limiting) should be implemented by the sink.
 */
class Dispatcher
{
public:
    //! Type for callbacks invoked for each log entry.
    using Callback = std::function<void(const Entry&)>;
    //! Type for opaque handles returned by RegisterCallback(), used to unregister.
    using CallbackHandle = std::list<Callback>::iterator;

    /**
     * Register a callback to receive log entries.
     * @param[in] callback  Invoked for each log entry.
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

    /**
     * Format message and dispatch to all registered callbacks.
     * @param[in] level             Severity level.
     * @param[in] category          Opaque category identifier.
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
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (const tinyformat::format_error& fmterr) {
            log_msg = "Error \"" + std::string{fmterr.what()} + "\" while formatting log message: " + fmt.fmt;
        }

        Entry entry{
            .category = category,
            .message = std::move(log_msg),
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
};

} // namespace util::log

#endif // BITCOIN_UTIL_LOG_H
