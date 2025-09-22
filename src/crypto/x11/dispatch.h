// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_X11_DISPATCH_H
#define BITCOIN_CRYPTO_X11_DISPATCH_H

#include <crypto/x11/sph_shavite.h>

#include <cstdint>

namespace sapphire {
namespace dispatch {
typedef void (*AESRoundFn)(uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t&, uint32_t&, uint32_t&, uint32_t&);
typedef void (*AESRoundFnNk)(uint32_t, uint32_t, uint32_t, uint32_t,
                             uint32_t&, uint32_t&, uint32_t&, uint32_t&);

typedef void (*EchoRoundFn)(uint64_t[16][2], uint32_t&, uint32_t&, uint32_t&, uint32_t&);
typedef void (*EchoShiftMix)(uint64_t[16][2]);

typedef void (*ShaviteCompressFn)(sph_shavite_big_context*, const void *);
} // namespace dispatch
} // namespace sapphire

void SapphireAutoDetect();

#endif // BITCOIN_CRYPTO_X11_DISPATCH_H
