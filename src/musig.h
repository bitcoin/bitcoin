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
//! uint256 will byteswap the hex
constexpr uint256 MUSIG_CHAINCODE{"6589e367712c6200e367717145cb322d76576bc3248959c474f9a602ca878086"};

//! Create a secp56k1_musig_keyagg_cache from the pubkeys in their current order. This is necessary for most MuSig2 operations
bool GetMuSig2KeyAggCache(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache);
//! Retrieve the full aggregate pubkey from the secp256k1_musig_keyagg_cache
std::optional<CPubKey> GetCPubKeyFromMuSig2KeyAggCache(secp256k1_musig_keyagg_cache& cache);
//! Compute the full aggregate pubkey from the given participant pubkeys in their current order
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys);

#endif // BITCOIN_MUSIG_H
