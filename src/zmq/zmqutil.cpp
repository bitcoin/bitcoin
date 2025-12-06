// Copyright (c) 2014-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <zmq/zmqutil.h>

#include <logging.h>
#include <zmq.h>

#include <cerrno>
#include <string>

void zmqError(BCLog::Level level, const std::string& str)
{
    LogPrintLevel(BCLog::ZMQ, level, "Error: %s, msg: %s\n", str, zmq_strerror(errno));
}

void zmqErrorDebug(const std::string& str)
{
    zmqError(BCLog::Level::Debug, str);
}
