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

struct ImportError {
    WalletError wallet_error;
    //! Set to true when a wallet-wide precondition failed before any descriptor was
    //! processed (e.g. wallet is already rescanning, or wallet is locked).
    //! Callers that support top-level errors should surface this as a
    //! top-level / call-wide error rather than a per-descriptor failure.
    bool is_general_error;

    ImportError(WalletErrorCode r, bilingual_str e, bool is_wallet_error)
        : wallet_error{r, std::move(e)},
        is_general_error{is_wallet_error}
    {};
};

struct ImportResult {
    std::vector<std::string> warnings;
    std::optional<ImportError> error;

    bool has_error() const {
        return error.has_value();
    }
};

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
