// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_SERVER_UTIL_H
#define BITCOIN_RPC_SERVER_UTIL_H

#include <context.h>

class CBlockPolicyEstimator;
class CConnman;
class ChainstateManager;
class CTxMemPool;
class PeerManager;
struct NodeContext;
struct LLMQContext;

NodeContext& EnsureAnyNodeContext(const CoreContext& context);
CTxMemPool& EnsureMemPool(const NodeContext& node);
CTxMemPool& EnsureAnyMemPool(const CoreContext& context);
ChainstateManager& EnsureChainman(const NodeContext& node);
ChainstateManager& EnsureAnyChainman(const CoreContext& context);
CBlockPolicyEstimator& EnsureFeeEstimator(const NodeContext& node);
CBlockPolicyEstimator& EnsureAnyFeeEstimator(const CoreContext& context);
LLMQContext& EnsureLLMQContext(const NodeContext& node);
LLMQContext& EnsureAnyLLMQContext(const CoreContext& context);
CConnman& EnsureConnman(const NodeContext& node);
PeerManager& EnsurePeerman(const NodeContext& node);

#endif // BITCOIN_RPC_SERVER_UTIL_H
