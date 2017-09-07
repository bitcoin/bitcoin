// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCUTIL_H
#define BITCOIN_RPCUTIL_H

#include "univalue.h"
#include <string>


UniValue CallRPC(std::string args, std::string wallet="");

class JSONRPCRequest;
extern UniValue gettransactionsummary(const JSONRPCRequest &request);

#endif // BITCOIN_RPCUTIL_H

