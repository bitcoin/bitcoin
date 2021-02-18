// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchedlogger.h>

CBatchedLogger::CBatchedLogger(BCLog::LogFlags _category, const std::string& logging_function, const std::string& source_file, int source_line) :
    accept(LogAcceptCategory(_category)),
    m_logging_function(logging_function),
    m_source_file(source_file),
    m_source_line(source_line)
{
}

CBatchedLogger::~CBatchedLogger()
{
    Flush();
}

void CBatchedLogger::Flush()
{
    if (!accept || msg.empty()) {
        return;
    }
    LogInstance().LogPrintStr(msg, m_logging_function, m_source_file, m_source_line);
    msg.clear();
}
