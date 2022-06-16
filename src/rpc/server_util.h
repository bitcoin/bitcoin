// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_SERVER_UTIL_H
#define BITCOIN_RPC_SERVER_UTIL_H

#include <any>

class ArgsManager;
class CBlockPolicyEstimator;
class CConnman;
class CTxMemPool;
class ChainstateManager;
class PeerManager;
namespace node {
struct NodeContext;
} // namespace node

node::NodeContext& EnsureAnyNodeContext(const std::any& context);
CTxMemPool& EnsureMemPool(const node::NodeContext& node);
CTxMemPool& EnsureAnyMemPool(const std::any& context);
ArgsManager& EnsureArgsman(const node::NodeContext& node);
ArgsManager& EnsureAnyArgsman(const std::any& context);
ChainstateManager& EnsureChainman(const node::NodeContext& node);
ChainstateManager& EnsureAnyChainman(const std::any& context);
CBlockPolicyEstimator& EnsureFeeEstimator(const node::NodeContext& node);
CBlockPolicyEstimator& EnsureAnyFeeEstimator(const std::any& context);
CConnman& EnsureConnman(const node::NodeContext& node);
PeerManager& EnsurePeerman(const node::NodeContext& node);

#endif // BITCOIN_RPC_SERVER_UTIL_H
