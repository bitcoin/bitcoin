// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_SERVER_UTIL_H
#define BITCOIN_RPC_SERVER_UTIL_H

#include <context.h>

class ArgsManager;
class CBlockPolicyEstimator;
class CConnman;
class CTxMemPool;
class ChainstateManager;
class PeerManager;
struct LLMQContext;
namespace node {
struct NodeContext;
} // namespace node

node::NodeContext& EnsureAnyNodeContext(const CoreContext& context);
CTxMemPool& EnsureMemPool(const node::NodeContext& node);
CTxMemPool& EnsureAnyMemPool(const CoreContext& context);
ArgsManager& EnsureArgsman(const node::NodeContext& node);
ArgsManager& EnsureAnyArgsman(const CoreContext& context);
ChainstateManager& EnsureChainman(const node::NodeContext& node);
ChainstateManager& EnsureAnyChainman(const CoreContext& context);
CBlockPolicyEstimator& EnsureFeeEstimator(const node::NodeContext& node);
CBlockPolicyEstimator& EnsureAnyFeeEstimator(const CoreContext& context);
LLMQContext& EnsureLLMQContext(const node::NodeContext& node);
LLMQContext& EnsureAnyLLMQContext(const CoreContext& context);
CConnman& EnsureConnman(const node::NodeContext& node);
PeerManager& EnsurePeerman(const node::NodeContext& node);

#endif // BITCOIN_RPC_SERVER_UTIL_H
