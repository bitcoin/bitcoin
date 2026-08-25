// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#ifdef ENABLE_CHACHA20_VEC
#include <crypto/chacha20_vec_128impl.h>

#include <cassert>

#if defined(__x86_64__) || defined(__amd64__)
static constexpr bool target_x86_64 = true;
#else
static constexpr bool target_x86_64 = false;
#endif

#if defined(__aarch64__)
static constexpr bool target_arm64 = true;
#else
static constexpr bool target_arm64 = false;
#endif

static constexpr bool use_vectorized = target_x86_64 || target_arm64;

namespace chacha20_vec {

static_assert(BLOCKLEN == chacha20_vec128::BLOCKLEN);

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
    if constexpr (!use_vectorized) {
        return;
    }
    assert(in_bytes.size() == out_bytes.size());
    chacha20_vec128::ChaCha20Vectorized crypter(input);

    while(in_bytes.size() >= BLOCKLEN) {
        size_t blocks = out_bytes.size() / BLOCKLEN;
        if constexpr(target_x86_64) {
                // 4 is faster than 3 + 1
                // 4 + 4 is faster than 3 + 3 + 2
                // etc.
            if  (blocks == 8 || blocks == 4) {
                crypter.CryptStates<4>(in_bytes, out_bytes);
            } else if (blocks == 2) {
                crypter.CryptStates<2>(in_bytes, out_bytes);
            } else if (blocks == 1) {
                crypter.CryptStates<1>(in_bytes, out_bytes);
            } else {
                crypter.CryptStates<3>(in_bytes, out_bytes);
            }
        } else if constexpr (target_arm64) {
                // 15 is faster than 8 + 4 + 1 + 1 + 1
                // 12 is faster than 8 + 4
                // etc.
            if (blocks == 15 ) {
                crypter.CryptStates<15>(in_bytes, out_bytes);
            } else if (blocks == 14) {
                crypter.CryptStates<14>(in_bytes, out_bytes);
            } else if (blocks == 13) {
                crypter.CryptStates<13>(in_bytes, out_bytes);
            } else if (blocks == 12) {
                crypter.CryptStates<12>(in_bytes, out_bytes);
            } else if (blocks == 11) {
                crypter.CryptStates<11>(in_bytes, out_bytes);
            } else if (blocks == 10) {
                crypter.CryptStates<10>(in_bytes, out_bytes);
            } else if (blocks == 9) {
                crypter.CryptStates<9>(in_bytes, out_bytes);
            } else if (blocks == 7) {
                crypter.CryptStates<7>(in_bytes, out_bytes);
            } else if (blocks == 6) {
                crypter.CryptStates<6>(in_bytes, out_bytes);
            } else if (blocks == 5) {
                crypter.CryptStates<5>(in_bytes, out_bytes);
            } else if (blocks == 4) {
                crypter.CryptStates<4>(in_bytes, out_bytes);
            } else if  (blocks >= 8 ) {
                crypter.CryptStates<8>(in_bytes, out_bytes);
            } else {
                break;
            }
        }
    }
}

} // namespace chacha20_vec

#endif // ENABLE_CHACHA20_VEC
