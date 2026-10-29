// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_EXPORT_H
#define BITCOIN_WALLET_EXPORT_H

#include <key.h>
#include <threadsafety.h>
#include <util/expected.h>
#include <wallet/types.h>
#include <wallet/wallet.h>

#include <map>
#include <optional>
#include <set>
#include <vector>

class SigningProvider;
class XOnlyPubKey;
struct TaprootSpendData;

namespace wallet {
enum class AddressKeyType { WALLET, TAPROOT_OUTPUT };

//! Ordered by the corresponding RPC role strings, never by secret material.
enum class AddressKeyRole { MUSIG2_PARTICIPANT, SCRIPT, TAPROOT_INTERNAL, TAPROOT_OUTPUT, TAPROOT_SCRIPT };

struct AddressKey {
    CKey key;
    std::set<AddressKeyRole> roles;
};

struct AddressKeyResult {
    //! Sorted by full serialized public key, with roles merged for identical public keys.
    std::vector<AddressKey> keys;
    //! Only original P2PKH/P2WPKH wallet-mode successes have compatibility aliases.
    bool legacy_aliases{false};
};

//! Verify public Taproot spend data and collect only recognized internal/leaf key associations.
util::Expected<std::map<XOnlyPubKey, std::set<AddressKeyRole>>, AddressKeyError> GetTaprootKeyCandidates(const XOnlyPubKey& output, const TaprootSpendData& spenddata, const SigningProvider& provider);

//! Caller must authorize local private-key export and retain cs_wallet through serialization.
util::Expected<AddressKeyResult, AddressKeyError> ExportAddressKeys(const CWallet& wallet, const CTxDestination& destination, AddressKeyType type) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

// Struct containing all of the info from WalletDescriptor, except with the descriptor as a string,
// and without its ID.
// Used when exporting descriptors from the wallet.
struct WalletDescInfo {
    std::string descriptor;
    uint64_t creation_time;
    bool active;
    std::optional<bool> internal;
    std::optional<std::pair<int64_t,int64_t>> range;
    int64_t next_index;
    DescriptorCache cache;
};

//! Export the descriptors from a wallet so that they can be imported elsewhere
util::Expected<std::vector<WalletDescInfo>, std::string> ExportDescriptors(const CWallet& wallet, bool export_private) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);

//! Make a new watchonly wallet file containing the public descriptors from this wallet
//! The exported watchonly wallet file will be named and placed at the path specified in 'destination'
util::Result<std::string> ExportWatchOnlyWallet(const CWallet& wallet, const fs::path& destination, WalletContext& context) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet);
} // namespace wallet

#endif // BITCOIN_WALLET_EXPORT_H
