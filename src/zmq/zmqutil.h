// Copyright (c) 2014-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_ZMQUTIL_H
#define BITCOIN_ZMQ_ZMQUTIL_H

#include <logging.h>

#include <string>

/** Log ZMQ error at the specified level */
void zmqError(BCLog::Level level, const std::string& str);

/** Log ZMQ error at Debug level */
void zmqErrorDebug(const std::string& str);

/** Prefix for unix domain socket addresses (which are local filesystem paths) */
const std::string ADDR_PREFIX_IPC = "ipc://"; // used by libzmq, example "ipc:///root/path/to/file"

#endif // BITCOIN_ZMQ_ZMQUTIL_H
