// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_LOG_H
#define BITCOIN_UTIL_LOG_H

#include <crypto/siphash.h>
#include <span.h>

#include <cstdint>
#include <source_location>
#include <string_view>

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

struct SourceLocationEqual {
    bool operator()(const SourceLocation& lhs, const SourceLocation& rhs) const noexcept
    {
        return lhs.line() == rhs.line() && std::string_view(lhs.file_name()) == std::string_view(rhs.file_name());
    }
};

struct SourceLocationHasher {
    size_t operator()(const SourceLocation& s) const noexcept
    {
        // Use CSipHasher(0, 0) as a simple way to get uniform distribution.
        return size_t(CSipHasher(0, 0)
                          .Write(s.line())
                          .Write(MakeUCharSpan(std::string_view{s.file_name()}))
                          .Finalize());
    }
};

namespace util::log {
enum class Level {
    Trace = 0, // High-volume or detailed logging for development/debugging
    Debug,     // Reasonably noisy logging, but still usable in production
    Info,      // Default
    Warning,
    Error,
};
} // namespace util::log

// Allow __func__ to be used in any context without warnings:
// NOLINTNEXTLINE(bugprone-lambda-function-name)
#define LogPrintLevel_(category, level, should_ratelimit, ...) LogPrintFormatInternal(SourceLocation{__func__}, category, level, should_ratelimit, __VA_ARGS__)

// Log unconditionally. Uses basic rate limiting to mitigate disk filling attacks.
// Be conservative when using functions that unconditionally log to debug.log!
// It should not be the case that an inbound peer can fill up a user's storage
// with debug.log entries.
#define LogInfo(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Info, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogWarning(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Warning, /*should_ratelimit=*/true, __VA_ARGS__)
#define LogError(...) LogPrintLevel_(BCLog::LogFlags::ALL, BCLog::Level::Error, /*should_ratelimit=*/true, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.

// Log by prefixing the output with the passed category name and severity level. This logs conditionally if
// the category is allowed. No rate limiting is applied, because users specifying -debug are assumed to be
// developers or power users who are aware that -debug may cause excessive disk usage due to logging.
#define detail_LogIfCategoryAndLevelEnabled(category, level, ...)      \
    do {                                                               \
        if (LogAcceptCategory((category), (level))) {                  \
            bool rate_limit{level >= BCLog::Level::Info};              \
            Assume(!rate_limit); /*Only called with the levels below*/ \
            LogPrintLevel_(category, level, rate_limit, __VA_ARGS__);  \
        }                                                              \
    } while (0)

// Log conditionally, prefixing the output with the passed category name.
#define LogDebug(category, ...) detail_LogIfCategoryAndLevelEnabled(category, BCLog::Level::Debug, __VA_ARGS__)
#define LogTrace(category, ...) detail_LogIfCategoryAndLevelEnabled(category, BCLog::Level::Trace, __VA_ARGS__)

#endif // BITCOIN_UTIL_LOG_H
