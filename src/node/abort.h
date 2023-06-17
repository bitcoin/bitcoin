// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_ABORT_H
#define BITCOIN_NODE_ABORT_H

#include <kernel/fatal_error.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>

#include <atomic>
#include <optional>
#include <string>

namespace util {
class SignalInterrupt;
} // namespace util

namespace node {
void AbortNode(util::SignalInterrupt* shutdown, std::atomic<int>& exit_status, const bilingual_str& user_message);

template <typename T>
std::optional<T> HandleFatalError(const util::Result<T, kernel::FatalError> result, util::SignalInterrupt* shutdown, std::atomic<int>& exit_status)
{
    if (!result.GetErrors().empty() || !result) {
        AbortNode(shutdown, exit_status, ErrorString(result));
    }
    if (!result) return std::nullopt;
    return result.value();
}

template <typename T>
[[nodiscard]] T CheckFatal(const util::Result<T, kernel::FatalError> result, util::SignalInterrupt* shutdown, std::atomic<int>& exit_status)
{
    if (IsFatal(result)) {
        AbortNode(shutdown, exit_status, ErrorString(result));
    }
    if constexpr (std::is_same_v<T, bool>) {
        if (!result) {
            return false;
        }
        return result.value();
    }
}

} // namespace node

#endif // BITCOIN_NODE_ABORT_H
