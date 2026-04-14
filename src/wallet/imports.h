// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_IMPORTS_H
#define BITCOIN_WALLET_IMPORTS_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <util/translation.h>
#include <wallet/types.h>

namespace wallet {

//! Information about a descriptor to be imported.
struct ImportDescriptorRequest {
    std::string descriptor;
    std::string label;
    std::optional<int64_t> timestamp;
    bool active{false};
    std::optional<bool> internal;
    std::optional<std::pair<int64_t, int64_t>> range;
    std::optional<int64_t> next_index;
};

} // namespace wallet

#endif // BITCOIN_WALLET_IMPORTS_H
