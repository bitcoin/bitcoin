// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_TIMER_H
#define BITCOIN_LOGGING_TIMER_H

#include <logging.h>
#include <util/macros.h>
#include <util/time.h>
#include <util/types.h>

#include <chrono>
#include <optional>
#include <string>


namespace BCLog {

//! RAII-style object that outputs timing information to logs.
template <typename TimeType>
class Timer
{
public:
    //! If log_category is left as the default, end_msg will log unconditionally
    //! (instead of being filtered by category).
    Timer(
        std::string prefix,
        std::string end_msg,
        BCLog::LogFlags log_category = BCLog::LogFlags::ALL,
        bool msg_on_completion = true)
        : m_prefix(std::move(prefix)),
          m_title(std::move(end_msg)),
          m_log_category(log_category),
          m_message_on_completion(msg_on_completion)
    {
        this->Log(strprintf("%s started", m_title));
        m_start_t = std::chrono::steady_clock::now();
    }

    ~Timer()
    {
        if (m_message_on_completion) {
            this->Log(strprintf("%s completed", m_title));
        } else {
            this->Log("completed");
        }
    }

    void Log(const std::string& msg)
    {
        const std::string full_msg = this->LogMsg(msg);

        if (m_log_category == BCLog::LogFlags::ALL) {
            LogPrintf("%s\n", full_msg);
        } else {
            LogDebug(m_log_category, "%s\n", full_msg);
        }
    }

    std::string LogMsg(const std::string& msg)
    {
        const auto end_time{std::chrono::steady_clock::now()};
        if (!m_start_t) {
            return strprintf("%s: %s", m_prefix, msg);
        }
        const auto duration{end_time - *m_start_t};

        if constexpr (std::is_same<TimeType, std::chrono::microseconds>::value) {
            return strprintf("%s: %s (%iμs)", m_prefix, msg, Ticks<std::chrono::microseconds>(duration));
        } else if constexpr (std::is_same<TimeType, std::chrono::milliseconds>::value) {
            return strprintf("%s: %s (%.2fms)", m_prefix, msg, Ticks<MillisecondsDouble>(duration));
        } else if constexpr (std::is_same<TimeType, std::chrono::seconds>::value) {
            return strprintf("%s: %s (%.2fs)", m_prefix, msg, Ticks<SecondsDouble>(duration));
        } else {
            static_assert(ALWAYS_FALSE<TimeType>, "Error: unexpected time type");
        }
    }

private:
    std::optional<std::chrono::steady_clock::time_point> m_start_t{};

    //! Log prefix; usually the name of the function this was created in.
    const std::string m_prefix;

    //! A descriptive message of what is being timed.
    const std::string m_title;

    //! Forwarded on to LogDebug if specified - has the effect of only
    //! outputting the timing log when a particular debug= category is specified.
    const BCLog::LogFlags m_log_category;

    //! Whether to output the message again on completion.
    const bool m_message_on_completion;
};

} // namespace BCLog


#define LOG_TIME_MICROS_WITH_CATEGORY(end_msg, log_category) \
    BCLog::Timer<std::chrono::microseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category)
#define LOG_TIME_MILLIS_WITH_CATEGORY(end_msg, log_category) \
    BCLog::Timer<std::chrono::milliseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category)
#define LOG_TIME_MILLIS_WITH_CATEGORY_MSG_ONCE(end_msg, log_category) \
    BCLog::Timer<std::chrono::milliseconds> UNIQUE_NAME(logging_timer)(__func__, end_msg, log_category, /* msg_on_completion=*/false)
#define LOG_TIME_SECONDS(end_msg) \
    BCLog::Timer<std::chrono::seconds> UNIQUE_NAME(logging_timer)(__func__, end_msg)


#endif // BITCOIN_LOGGING_TIMER_H
