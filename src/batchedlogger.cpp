// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <batchedlogger.h>

CBatchedLogger::CBatchedLogger(BCLog::LogFlags _category, const std::string& _header) :
    accept(LogAcceptCategory(_category, BCLog::Level::Debug)), header(_header), category(_category)
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
    LogInstance().LogPrintStr(strprintf("%s:\n%s", header, msg), "", "", 0, category, BCLog::Level::Debug);
    msg.clear();
}
