// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MUSIG_H
#define BITCOIN_MUSIG_H

#include <pubkey.h>

#include <optional>
#include <vector>

struct secp256k1_musig_keyagg_cache;

//! MuSig2 chaincode as defined by BIP 328
using namespace util::hex_literals;
constexpr uint256 MUSIG_CHAINCODE{
    // Use immediate lambda to work around GCC-14 bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=117966
    []() consteval { return uint256{"868087ca02a6f974c4598924c36b57762d32cb45717167e300622c7167e38965"_hex_u8}; }(),
};



//! Create a secp256k1_musig_keyagg_cache from the pubkeys in their current order. This is necessary for most MuSig2 operations
bool GetMuSig2KeyAggCache(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache);
//! Retrieve the full aggregate pubkey from the secp256k1_musig_keyagg_cache
std::optional<CPubKey> GetCPubKeyFromMuSig2KeyAggCache(secp256k1_musig_keyagg_cache& cache);
//! Compute the full aggregate pubkey from the given participant pubkeys in their current order
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys);

#endif // BITCOIN_MUSIG_H
