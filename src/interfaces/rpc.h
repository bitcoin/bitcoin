// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_RPC_H
#define BITCOIN_INTERFACES_RPC_H

#include <memory>
#include <string>

class UniValue;
enum HTTPStatusCode : int;

namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {
//! Interface giving clients ability to emulate RPC calls.
class Rpc
{
public:
    virtual ~Rpc() = default;
    virtual UniValue executeRpc(const UniValue& request, const std::string& url, const std::string& user, HTTPStatusCode& status) = 0;
};

//! Return implementation of Rpc interface.
std::unique_ptr<Rpc> MakeRpc(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_RPC_H
