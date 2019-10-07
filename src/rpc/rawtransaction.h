// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RAWTRANSACTION_H
#define BITCOIN_RPC_RAWTRANSACTION_H

#include <univalue.h>
#include <rpc/server.h>

UniValue createrawtransaction(const JSONRPCRequest& request);

UniValue signrawtransaction(const JSONRPCRequest& request);


#endif
