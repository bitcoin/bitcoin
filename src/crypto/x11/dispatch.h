// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_X11_DISPATCH_H
#define BITCOIN_CRYPTO_X11_DISPATCH_H

#include <cstdint>

namespace sapphire {
namespace dispatch {
typedef void (*AESRoundFn)(uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t, uint32_t, uint32_t, uint32_t,
                           uint32_t&, uint32_t&, uint32_t&, uint32_t&);
typedef void (*AESRoundFnNk)(uint32_t, uint32_t, uint32_t, uint32_t,
                             uint32_t&, uint32_t&, uint32_t&, uint32_t&);
} // namespace dispatch
} // namespace sapphire

void SapphireAutoDetect();

#endif // BITCOIN_CRYPTO_X11_DISPATCH_H
