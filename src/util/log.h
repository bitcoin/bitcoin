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

#endif // BITCOIN_UTIL_LOG_H
