// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MUSIG_H
#define BITCOIN_MUSIG_H

#include <pubkey.h>
#include <secp256k1_musig.h>

#include <vector>

bool GetMuSig2KeyAggCache(const std::vector<CPubKey>& pubkeys, secp256k1_musig_keyagg_cache& keyagg_cache);
std::optional<CPubKey> GetCPubKeyFromMuSig2KeyAggCache(secp256k1_musig_keyagg_cache& cache);
std::optional<CPubKey> MuSig2AggregatePubkeys(const std::vector<CPubKey>& pubkeys);

#endif // BITCOIN_MUSIG_H
