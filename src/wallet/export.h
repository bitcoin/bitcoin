// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXPORT_H
#define BITCOIN_WALLET_EXPORT_H

#include <threadsafety.h>
#include <util/expected.h>
#include <wallet/wallet.h>

#include <optional>
#include <vector>

namespace wallet {
// Struct containing all of the info from WalletDescriptor, except with the descriptor as a string,
// and without its ID or cache.
// Used when exporting descriptors from the wallet.
struct WalletDescInfo {
    std::string descriptor;
    uint64_t creation_time;
    bool active;
    std::optional<bool> internal;
    std::optional<std::pair<int64_t,int64_t>> range;
    int64_t next_index;
};

//! Export the descriptors from a wallet so that they can be imported elsewhere
util::Expected<std::vector<WalletDescInfo>, std::string> ExportDescriptors(const CWallet& wallet, bool export_private) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

//! Make a new watchonly wallet file containing the public descriptors from this wallet
//! The exported watchonly wallet file will be named and placed at the path specified in 'destination'
util::Result<std::string> ExportWatchOnlyWallet(const CWallet& wallet, const fs::path& destination, WalletContext& context) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
} // namespace wallet

#endif // BITCOIN_WALLET_EXPORT_H
