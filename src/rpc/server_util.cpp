// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server_util.h>

#include <net_processing.h>
#include <node/context.h>
#include <policy/fees.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <txmempool.h>
#include <util/system.h>
#include <validation.h>

#include <any>

node::NodeContext& EnsureAnyNodeContext(const std::any& context)
{
    auto node_context = util::AnyPtr<node::NodeContext>(context);
    if (!node_context) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node context not found");
    }
    return *node_context;
}

CTxMemPool& EnsureMemPool(const node::NodeContext& node)
{
    if (!node.mempool) {
        throw JSONRPCError(RPC_CLIENT_MEMPOOL_DISABLED, "Mempool disabled or instance not found");
    }
    return *node.mempool;
}

CTxMemPool& EnsureAnyMemPool(const std::any& context)
{
    return EnsureMemPool(EnsureAnyNodeContext(context));
}


BanMan& EnsureBanman(const node::NodeContext& node)
{
    if (!node.banman) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Error: Ban database not loaded");
    }
    return *node.banman;
}

BanMan& EnsureAnyBanman(const std::any& context)
{
    return EnsureBanman(EnsureAnyNodeContext(context));
}

ArgsManager& EnsureArgsman(const node::NodeContext& node)
{
    if (!node.args) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node args not found");
    }
    return *node.args;
}

ArgsManager& EnsureAnyArgsman(const std::any& context)
{
    return EnsureArgsman(EnsureAnyNodeContext(context));
}

ChainstateManager& EnsureChainman(const node::NodeContext& node)
{
    if (!node.chainman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node chainman not found");
    }
    return *node.chainman;
}

ChainstateManager& EnsureAnyChainman(const std::any& context)
{
    return EnsureChainman(EnsureAnyNodeContext(context));
}

CBlockPolicyEstimator& EnsureFeeEstimator(const node::NodeContext& node)
{
    if (!node.fee_estimator) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Fee estimation disabled");
    }
    return *node.fee_estimator;
}

CBlockPolicyEstimator& EnsureAnyFeeEstimator(const std::any& context)
{
    return EnsureFeeEstimator(EnsureAnyNodeContext(context));
}

CConnman& EnsureConnman(const node::NodeContext& node)
{
    if (!node.connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.connman;
}

PeerManager& EnsurePeerman(const node::NodeContext& node)
{
    if (!node.peerman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.peerman;
}
