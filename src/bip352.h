// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BIP352_H
#define BITCOIN_BIP352_H

#include <key.h>
#include <pubkey.h>
#include <primitives/transaction.h>
#include <secp256k1.h>
#include <uint256.h>

#include <array>
#include <vector>

namespace BIP352 {

using InputHash = std::array<unsigned char, 32>;
using SecKey = std::array<unsigned char, 32>;
using PrivTweakData = std::pair<SecKey, InputHash>;
using PubTweakData = std::pair<secp256k1_pubkey, InputHash>;
using SharedSecret = std::array<unsigned char, 33>;

struct ScanningResult {
    bool    found;
    uint256 t_k;
    CPubKey label;
    CPubKey label_negated;
};

// Silent Payments sender interface
PrivTweakData CreateInputPrivkeysTweak(
    const std::vector<CKey>& plain_keys,
    const std::vector<CKey>& taproot_keys,
    const COutPoint& smallest_outpoint);

// Silent Payments receiver interface
PubTweakData CreateInputPubkeysTweak(
    const std::vector<CPubKey>& plain_pubkeys,
    const std::vector<XOnlyPubKey>& taproot_pubkeys,
    const COutPoint& smallest_outpoint);

// Silent Payments common interface
SharedSecret CreateSharedSecret(const PubTweakData& public_tweak_data, const CKey& receiver_scan_privkey);
SharedSecret CreateSharedSecret(const PrivTweakData& priv_tweak_data, const CPubKey& receiver_scan_pubkey);

XOnlyPubKey CreateOutput(
    const SharedSecret& shared_secret,
    const CPubKey& receiver_spend_pubkey,
    uint32_t output_index);

ScanningResult CheckOutput(
    const SharedSecret& shared_secret,
    const CPubKey& receiver_spend_pubkey,
    const XOnlyPubKey& tx_output,
    uint32_t output_index);

uint256 CombineTweaks(const uint256& t_k, const uint256& label_tweak);
std::pair<CPubKey, uint256> CreateLabelTweak(const CKey& scan_key, const int m);
CPubKey CreateLabeledSpendPubKey(const CPubKey& spend_pubkey, const CPubKey& label);
CKey CreateFullKey(const CKey& spend_key, const uint256& tweak);
}; // namespace BIP352
#endif // BITCOIN_BIP352_H
