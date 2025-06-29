// Copyright (c) 2022-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONTEXT_H
#define BITCOIN_CONTEXT_H

#include <functional>
#include <variant>

class ArgsManager;
class ChainstateManager;
class CTxMemPool;
class CBlockPolicyEstimator;
struct LLMQContext;
namespace node {
struct NodeContext;
} // namespace node
namespace wallet {
struct WalletContext;
} // namespace wallet

using CoreContext = std::variant<std::monostate,
                                 std::reference_wrapper<ArgsManager>,
                                 std::reference_wrapper<node::NodeContext>,
                                 std::reference_wrapper<wallet::WalletContext>,
                                 std::reference_wrapper<CTxMemPool>,
                                 std::reference_wrapper<ChainstateManager>,
                                 std::reference_wrapper<CBlockPolicyEstimator>,
                                 std::reference_wrapper<LLMQContext>>;

template<typename T>
T* GetContext(const CoreContext& context) noexcept
{
    return std::holds_alternative<std::reference_wrapper<T>>(context)
                ? &std::get<std::reference_wrapper<T>>(context).get()
                : nullptr;
}

#endif // BITCOIN_CONTEXT_H
