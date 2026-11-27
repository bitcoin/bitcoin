// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_H

#include <array>
#include <cstdint>
#include <cstddef>
#include <span>

#ifdef __has_attribute
  #if __has_attribute(vector_size)
    #define ENABLE_CHACHA20_VEC 1
  #endif
#endif

#ifdef ENABLE_CHACHA20_VEC

namespace chacha20_vec
{
    static constexpr size_t BLOCKLEN = 64;
    void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept;
}

#endif // ENABLE_CHACHA20_VEC

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_H
