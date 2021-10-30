// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/logging.h>

#include <logging.h>
#include <noui.h>
#include <tinyformat.h>
#include <util/memory.h>

#include <stdexcept>

DebugLogHelper::DebugLogHelper(std::string message)
    : m_message{std::move(message)}
{
    m_print_connection = LogInstance().PushBackCallback(
        [this](const std::string& s) {
            if (m_found) return;
            m_found = s.find(m_message) != std::string::npos;
        });
    noui_test_redirect();
}

void DebugLogHelper::check_found()
{
    noui_reconnect();
    LogInstance().DeleteCallback(m_print_connection);
    if (!m_found) {
        throw std::runtime_error(strprintf("'%s' not found in debug log\n", m_message));
    }
}
