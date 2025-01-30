// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchedlogger.h>

CBatchedLogger::CBatchedLogger(BCLog::LogFlags category, BCLog::Level level, const std::string& logging_function,
                               const std::string& source_file, int source_line) :
    m_accept{LogAcceptCategory(category, level)},
    m_category{category},
    m_level{level},
    m_logging_function{logging_function},
    m_source_file{source_file},
    m_source_line{source_line}
{
}

CBatchedLogger::~CBatchedLogger()
{
    Flush();
}

void CBatchedLogger::Flush()
{
    if (!m_accept || m_msg.empty()) {
        return;
    }
    LogInstance().LogPrintStr(m_msg, m_logging_function, m_source_file, m_source_line, m_category, m_level);
    m_msg.clear();
}
