// Copyright (c) 2021 The Bitcoin Core developers
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

NodeContext& EnsureAnyNodeContext(const CoreContext& context)
{
    auto* const node_context = GetContext<NodeContext>(context);
    if (!node_context) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node context not found");
    }
    return *node_context;
}

CTxMemPool& EnsureMemPool(const NodeContext& node)
{
    if (!node.mempool) {
        throw JSONRPCError(RPC_CLIENT_MEMPOOL_DISABLED, "Mempool disabled or instance not found");
    }
    return *node.mempool;
}

CTxMemPool& EnsureAnyMemPool(const CoreContext& context)
{
    return EnsureMemPool(EnsureAnyNodeContext(context));
}

ArgsManager& EnsureArgsman(const NodeContext& node)
{
    if (!node.args) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node args not found");
    }
    return *node.args;
}

ArgsManager& EnsureAnyArgsman(const CoreContext& context)
{
    return EnsureArgsman(EnsureAnyNodeContext(context));
}

ChainstateManager& EnsureChainman(const NodeContext& node)
{
    if (!node.chainman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node chainman not found");
    }
    return *node.chainman;
}

ChainstateManager& EnsureAnyChainman(const CoreContext& context)
{
    return EnsureChainman(EnsureAnyNodeContext(context));
}

CBlockPolicyEstimator& EnsureFeeEstimator(const NodeContext& node)
{
    if (!node.fee_estimator) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Fee estimation disabled");
    }
    return *node.fee_estimator;
}

CBlockPolicyEstimator& EnsureAnyFeeEstimator(const CoreContext& context)
{
    return EnsureFeeEstimator(EnsureAnyNodeContext(context));
}

LLMQContext& EnsureLLMQContext(const NodeContext& node)
{
    if (!node.llmq_ctx) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node LLMQ context not found");
    }
    return *node.llmq_ctx;
}

LLMQContext& EnsureAnyLLMQContext(const CoreContext& context)
{
    return EnsureLLMQContext(EnsureAnyNodeContext(context));
}

CConnman& EnsureConnman(const NodeContext& node)
{
    if (!node.connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.connman;
}

PeerManager& EnsurePeerman(const NodeContext& node)
{
    if (!node.peerman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.peerman;
}
