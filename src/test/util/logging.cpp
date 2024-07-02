// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/logging.h>

#include <logging.h>
#include <noui.h>
#include <tinyformat.h>
#include <util/check.h>

#include <iostream>
#include <stdexcept>

DebugLogHelper::DebugLogHelper(std::string message, MatchFn match, std::optional<std::chrono::milliseconds> timeout)
    : m_message{std::move(message)}, m_timeout{timeout}, m_match(std::move(match))
{
    m_print_connection = LogInstance().PushBackCallback(
        [this](const std::string& s) {
            StdLockGuard lock{m_mutex};
            if (!m_found) {
                if (s.find(m_message) != std::string::npos && m_match(&s)) {
                    m_found = true;
                    m_cv.notify_all();
                }
            }
        });
    noui_test_redirect();
    m_receiving_log = true;
}

DebugLogHelper::~DebugLogHelper()
{
    {
        StdUniqueLock lock{m_mutex};
        if (m_timeout.has_value()) {
            m_cv.wait_for(lock, m_timeout.value(), [this]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_found; });
        }
    }
    StopReceivingLog();
    if (!m_found && m_match(nullptr)) {
        std::cerr << "Fatal error: expected message not found in the debug log: " << m_message << "\n";
        std::abort();
    }
}

void DebugLogHelper::StopReceivingLog()
{
    if (m_receiving_log) {
        noui_reconnect();
        LogInstance().DeleteCallback(m_print_connection);
        m_receiving_log = false;
    }
}
