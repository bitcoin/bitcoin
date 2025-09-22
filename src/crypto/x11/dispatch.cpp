// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <crypto/x11/dispatch.h>

#if !defined(DISABLE_OPTIMIZED_SHA256)
#include <compat/cpuid.h>
#endif // !DISABLE_OPTIMIZED_SHA256

#include <cstdint>

namespace sapphire {
#if !defined(DISABLE_OPTIMIZED_SHA256)
#if defined(ENABLE_SSSE3)
namespace ssse3_echo {
void MixColumns(uint64_t W[16][2]);
} // namespace ssse3_echo
#endif // ENABLE_SSSE3

#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
namespace x86_aesni_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
} // namespace x86_aesni_aes
namespace x86_aesni_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3);
} // namespace x86_aesni_echo
#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
#endif // !DISABLE_OPTIMIZED_SHA256

namespace soft_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
} // namespace soft_aes
namespace soft_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3);
void MixColumns(uint64_t W[16][2]);
} // namespace soft_echo
} // namespace sapphire

extern sapphire::dispatch::AESRoundFn aes_round;
extern sapphire::dispatch::AESRoundFnNk aes_round_nk;
extern sapphire::dispatch::EchoMixCols echo_mix_columns;
extern sapphire::dispatch::EchoRoundFn echo_round;

void SapphireAutoDetect()
{
    aes_round = sapphire::soft_aes::Round;
    aes_round_nk = sapphire::soft_aes::RoundKeyless;
    echo_round = sapphire::soft_echo::FullStateRound;
    echo_mix_columns = sapphire::soft_echo::MixColumns;

#if !defined(DISABLE_OPTIMIZED_SHA256)
#if defined(HAVE_GETCPUID)
    uint32_t eax, ebx, ecx, edx;
    GetCPUID(1, 0, eax, ebx, ecx, edx);
#if defined(ENABLE_SSE41) && defined(ENABLE_X86_AESNI)
    const bool use_sse_4_1 = ((ecx >> 19) & 1);
    const bool use_aes_ni = ((ecx >> 25) & 1);
    if (use_sse_4_1 && use_aes_ni) {
        aes_round = sapphire::x86_aesni_aes::Round;
        aes_round_nk = sapphire::x86_aesni_aes::RoundKeyless;
        echo_round = sapphire::x86_aesni_echo::FullStateRound;
    }
#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
#if defined(ENABLE_SSSE3)
    const bool use_ssse3 = ((ecx >> 9) & 1);
    if (use_ssse3) {
        echo_mix_columns = sapphire::ssse3_echo::MixColumns;
    }
#endif // ENABLE_SSSE3
#endif // HAVE_GETCPUID
#endif // !DISABLE_OPTIMIZED_SHA256
}
