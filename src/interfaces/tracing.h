// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_TRACING_H
#define BITCOIN_INTERFACES_TRACING_H

#include <interfaces/handler.h>
#include <util/transaction_identifier.h>

#include <memory>

namespace node {
struct NodeContext;
} // namespace node

namespace interfaces {
struct UtxoInfo {
    Txid outpoint_hash;
    uint32_t outpoint_n;
    uint32_t height;
    int64_t value;
    bool is_coinbase;
};

class UtxoCacheTrace
{
public:
    virtual ~UtxoCacheTrace() = default;
    virtual void add(const UtxoInfo& coin) {}
    virtual void spend(const UtxoInfo& coin) {}
    virtual void uncache(const UtxoInfo& coin) {}
};

class Tracing
{
public:
    virtual ~Tracing() = default;
    virtual std::unique_ptr<Handler> traceUtxoCache(std::unique_ptr<UtxoCacheTrace> callback) = 0;
};

//! Return implementation of Tracing interface.
std::unique_ptr<Tracing> MakeTracing(node::NodeContext& node);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_TRACING_H
