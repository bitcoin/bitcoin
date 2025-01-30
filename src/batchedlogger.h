// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BATCHEDLOGGER_H
#define BITCOIN_BATCHEDLOGGER_H

#include <logging.h>

class CBatchedLogger
{
private:
    bool m_accept;
    BCLog::LogFlags m_category;
    BCLog::Level m_level;
    std::string m_logging_function;
    std::string m_source_file;
    const int m_source_line;
    std::string m_msg;

public:
    CBatchedLogger(BCLog::LogFlags category, BCLog::Level level, const std::string& logging_function,
                   const std::string& m_source_file, int m_source_line);
    virtual ~CBatchedLogger();

    template<typename... Args>
    void Batch(const std::string& fmt, const Args&... args)
    {
        if (!m_accept) {
            return;
        }
        m_msg += "    " + strprintf(fmt, args...) + "\n";
    }

    void Flush();
};

#endif // BITCOIN_BATCHEDLOGGER_H
