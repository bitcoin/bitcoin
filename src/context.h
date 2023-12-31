// Copyright (c) 2022-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONTEXT_H
#define BITCOIN_CONTEXT_H

#include <variant>
#include <optional>

class ChainstateManager;
class CTxMemPool;
class CBlockPolicyEstimator;
struct LLMQContext;
struct NodeContext;
struct WalletContext;

using CoreContext = std::variant<std::nullopt_t,
                                 std::reference_wrapper<NodeContext>,
                                 std::reference_wrapper<WalletContext>,
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

#endif // BITCOIN_CONTEXT_VARIANT_H
