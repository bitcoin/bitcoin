// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/x11/dispatch.h>

#include <cstdint>

namespace sapphire {
namespace soft_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
} // namespace soft_aes
} // namespace sapphire

extern sapphire::dispatch::AESRoundFn aes_round;
extern sapphire::dispatch::AESRoundFnNk aes_round_nk;

void SapphireAutoDetect()
{
    aes_round = sapphire::soft_aes::Round;
    aes_round_nk = sapphire::soft_aes::RoundKeyless;
}
