// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/x11/dispatch.h>
#include <crypto/x11/util/consts_aes.hpp>

#include <cstdint>

namespace sapphire {
namespace soft_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3)
{
    using namespace consts;
    y0 = aes_tbox_le[0][(x0) & 0xff] ^ aes_tbox_le[1][((x1) >> 8) & 0xff] ^ aes_tbox_le[2][((x2) >> 16) & 0xff] ^ aes_tbox_le[3][((x3) >> 24) & 0xff] ^ (k0);
    y1 = aes_tbox_le[0][(x1) & 0xff] ^ aes_tbox_le[1][((x2) >> 8) & 0xff] ^ aes_tbox_le[2][((x3) >> 16) & 0xff] ^ aes_tbox_le[3][((x0) >> 24) & 0xff] ^ (k1);
    y2 = aes_tbox_le[0][(x2) & 0xff] ^ aes_tbox_le[1][((x3) >> 8) & 0xff] ^ aes_tbox_le[2][((x0) >> 16) & 0xff] ^ aes_tbox_le[3][((x1) >> 24) & 0xff] ^ (k2);
    y3 = aes_tbox_le[0][(x3) & 0xff] ^ aes_tbox_le[1][((x0) >> 8) & 0xff] ^ aes_tbox_le[2][((x1) >> 16) & 0xff] ^ aes_tbox_le[3][((x2) >> 24) & 0xff] ^ (k3);
}

void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3)
{
    Round(x0, x1, x2, x3, /*k0=*/0, /*k1=*/0, /*k2=*/0, /*k3=*/0, y0, y1, y2, y3);
}
} // namespace soft_aes
} // namespace sapphire

sapphire::dispatch::AESRoundFn aes_round = sapphire::soft_aes::Round;
sapphire::dispatch::AESRoundFnNk aes_round_nk = sapphire::soft_aes::RoundKeyless;
