// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DASH_BATCHEDLOGGER_H
#define DASH_BATCHEDLOGGER_H

#include "tinyformat.h"

class CBatchedLogger
{
private:
    std::string header;
    std::string msg;
public:
    CBatchedLogger(const std::string& _header);
    virtual ~CBatchedLogger();

    template<typename... Args>
    void Batch(const std::string& fmt, const Args&... args)
    {
        msg += "    " + strprintf(fmt, args...) + "\n";
    }

    void Flush();
};

#endif//DASH_BATCHEDLOGGER_H
