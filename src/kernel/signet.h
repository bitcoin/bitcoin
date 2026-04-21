// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_SIGNET_H
#define BITCOIN_KERNEL_SIGNET_H

#include <hash.h>
#include <kernel/messagestartchars.h>
#include <uint256.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace kernel {
using namespace util::hex_literals;

inline const std::vector<uint8_t> SIGNET_DEFAULT_CHALLENGE{
    "512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae"_hex_v_u8};

/**
 * Return the message start bytes for a signet: the first four bytes of the
 * double-SHA256 hash of the serialized signet challenge script (BIP325)
 */
inline MessageStartChars GetSignetMessageStart(const std::vector<uint8_t>& signet_challenge)
{
    HashWriter h{};
    h << signet_challenge;
    const uint256 hash = h.GetHash();
    MessageStartChars msg_start;
    std::copy_n(hash.begin(), 4, msg_start.begin());
    return msg_start;
}
} // namespace kernel

#endif // BITCOIN_KERNEL_SIGNET_H
