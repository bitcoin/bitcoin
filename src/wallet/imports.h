// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_IMPORTS_H
#define BITCOIN_WALLET_IMPORTS_H

#include <wallet/wallet.h>

namespace wallet{

    struct ImportDescriptorResult {
        enum class FailureReason {
            NONE,
            INVALID_DESCRIPTOR,
            INVALID_PARAMETER,
            WALLET_ERROR,
            MISC_ERROR
        };
        bool success{false};
        bool used_default_range{false};
        FailureReason reason{FailureReason::NONE};
        std::string error;
        std::vector<std::string> warnings;

        ImportDescriptorResult& Error(FailureReason r, std::string e) {
            reason = r;
            error = std::move(e);
            return *this;
        }
    };

    //! Information about a descriptor to be imported.
    struct ImportDescriptorRequest {
        std::string descriptor;
        std::optional<std::string> label;
        std::optional<int64_t> timestamp;
        std::optional<bool> active;
        std::optional<bool> internal;
        std::optional<std::pair<int64_t,int64_t>> range;
        std::optional<int64_t> next_index;
    };

    ImportDescriptorResult ImportDescriptor(CWallet& wallet,
        const std::string& descriptor,
        bool active,
        const std::optional<bool>& internal,
        const std::optional<std::string>& label,
        int64_t timestamp,
        const std::optional<std::pair<int64_t, int64_t>>& range,
        const std::optional<int64_t>& next_idx) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

    std::vector<ImportDescriptorResult> ProcessDescriptorsImport(CWallet& wallet,
        std::vector<ImportDescriptorRequest> requests);
}

#endif // BITCOIN_WALLET_IMPORTS_H
