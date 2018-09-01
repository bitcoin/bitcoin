// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RPCCATEGORY_H
#define BITCOIN_RPC_RPCCATEGORY_H

#include <string>

enum class RPCCategory
{
    BLOCKCHAIN,
    CONTROL,
    GENERATING,
    HIDDEN,
    MINING,
    NETWORK,
    RAWTRANSACTIONS,
    UTIL,
    WALLET,
    ZMQ,
    TEST
};

namespace rpccategory
{

std::string Label(const RPCCategory category);

} // namespace rpccategory

#endif // BITCOIN_RPC_RPCCATEGORY_H
