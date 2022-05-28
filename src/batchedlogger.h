// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_BATCHEDLOGGER_H
#define SYSCOIN_BATCHEDLOGGER_H

#include <tinyformat.h>
#include <logging.h>

class CBatchedLogger
{
private:
    bool accept;
    std::string header;
    std::string msg;
    BCLog::LogFlags category;
public:
    CBatchedLogger(BCLog::LogFlags _category, const std::string& _header);
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

#endif // SYSCOIN_BATCHEDLOGGER_H
