// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server_util.h>

#include <chain.h>
#include <common/args.h>
#include <net_processing.h>
#include <node/context.h>
#include <node/miner.h>
#include <policy/fees/block_policy_estimator.h>
#include <pow.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <txmempool.h>
#include <util/any.h>
#include <validation.h>

#include <any>

using node::NodeContext;
using node::UpdateTime;

NodeContext& EnsureAnyNodeContext(const std::any& context)
{
    auto node_context = util::AnyPtr<NodeContext>(context);
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

CTxMemPool& EnsureAnyMemPool(const std::any& context)
{
    return EnsureMemPool(EnsureAnyNodeContext(context));
}


BanMan& EnsureBanman(const NodeContext& node)
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

ArgsManager& EnsureArgsman(const NodeContext& node)
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

ChainstateManager& EnsureChainman(const NodeContext& node)
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

CBlockPolicyEstimator& EnsureFeeEstimator(const NodeContext& node)
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

CConnman& EnsureConnman(const NodeContext& node)
{
    if (!node.connman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.connman;
}

interfaces::Mining& EnsureMining(const NodeContext& node)
{
    if (!node.mining) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Node miner not found");
    }
    return *node.mining;
}

PeerManager& EnsurePeerman(const NodeContext& node)
{
    if (!node.peerman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    }
    return *node.peerman;
}

AddrMan& EnsureAddrman(const NodeContext& node)
{
    if (!node.addrman) {
        throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Address manager functionality missing or disabled");
    }
    return *node.addrman;
}

AddrMan& EnsureAnyAddrman(const std::any& context)
{
    return EnsureAddrman(EnsureAnyNodeContext(context));
}

void NextEmptyBlockIndex(CBlockIndex& tip, const Consensus::Params& consensusParams, CBlockIndex& next_index)
{
    CBlockHeader next_header{};
    next_header.hashPrevBlock  = tip.GetBlockHash();
    UpdateTime(&next_header, consensusParams, &tip);
    next_header.nBits = GetNextWorkRequired(&tip, &next_header, consensusParams);
    next_header.nNonce = 0;

    next_index.pprev = &tip;
    next_index.nTime = next_header.nTime;
    next_index.nBits = next_header.nBits;
    next_index.nNonce = next_header.nNonce;
    next_index.nHeight = tip.nHeight + 1;
}
