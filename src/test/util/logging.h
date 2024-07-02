// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_LOGGING_H
#define BITCOIN_TEST_UTIL_LOGGING_H

#include <sync.h>
#include <util/macros.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <optional>
#include <string>

class DebugLogHelper
{
public:
    using MatchFn = std::function<bool(const std::string* line)>;

    static bool MatchFnDefault(const std::string*)
    {
        return true;
    }

    explicit DebugLogHelper(
        std::string message,
        MatchFn match = MatchFnDefault,
        std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    ~DebugLogHelper();

private:
    const std::string m_message;
    const std::optional<std::chrono::milliseconds> m_timeout;
    // Mutex + LOCK() is not usable here because LOCK() may print to the log
    // itself (see DEBUG_LOCKCONTENTION) causing a deadlock between this mutex
    // and BCLog::Logger::m_cs which is acquired when logging a message.
    StdMutex m_mutex;
    std::condition_variable_any m_cv;
    bool m_found GUARDED_BY(m_mutex){false};
    std::list<std::function<void(const std::string&)>>::iterator m_print_connection;

    //! Custom match checking function.
    //!
    //! Invoked with pointers to lines containing matching strings, and with
    //! nullptr if ~DebugLogHelper() is called without any successful match.
    //!
    //! Can return true to enable default DebugLogHelper behavior of:
    //! (1) ending search after first successful match, and
    //! (2) raising an error in ~DebugLogHelper() if no match was found (will be called with nullptr then)
    //! Can return false to do the opposite in either case.
    MatchFn m_match;

    bool m_receiving_log;
    void StopReceivingLog();
};

#define ASSERT_DEBUG_LOG(message) DebugLogHelper UNIQUE_NAME(debugloghelper)(message)

#define ASSERT_DEBUG_LOG_WAIT(message, timeout) \
    DebugLogHelper UNIQUE_NAME(debugloghelper)(message, DebugLogHelper::MatchFnDefault, timeout)

#endif // BITCOIN_TEST_UTIL_LOGGING_H
