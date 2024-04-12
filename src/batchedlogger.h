// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BATCHEDLOGGER_H
#define BITCOIN_BATCHEDLOGGER_H

#include <logging.h>

class CBatchedLogger
{
private:
    bool accept;
    std::string m_logging_function;;
    std::string m_source_file;
    const int m_source_line;
    std::string msg;
public:
    CBatchedLogger(BCLog::LogFlags _category, const std::string& logging_function, const std::string& m_source_file, int m_source_line);
    virtual ~CBatchedLogger();

    template<typename... Args>
    void Batch(const std::string& fmt, const Args&... args)
    {
        if (!accept) {
            return;
        }
        msg += "    " + strprintf(fmt, args...) + "\n";
    }

    void Flush();
};

#endif//BITCOIN_BATCHEDLOGGER_H
