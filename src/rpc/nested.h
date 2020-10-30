// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_NESTED_H
#define BITCOIN_RPC_NESTED_H

#include <functional>
#include <string>

#include <univalue.h>

namespace RPCNested {
    bool ParseCommandLine(std::function<UniValue(const std::string&, const UniValue&, const std::string&, bool &stop_parsing)> rpc_exec_func, std::function<bool(const std::string &)> filter_func, std::string &strResult, UniValue &output, const std::string &strCommand, const bool fExecute, std::string * const pstrFilteredOut, const std::string &uri_prefix);
}

#endif // BITCOIN_RPC_NESTED_H
