// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <crypto/x11/dispatch.h>

#if !defined(DISABLE_OPTIMIZED_SHA256)
#include <compat/cpuid.h>

#if defined(ENABLE_ARM_AES)
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif // __APPLE__

#if defined(__linux__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#endif // __linux__

#if defined(__FreeBSD__)
#include <machine/elf.h>
#include <sys/auxv.h>
#endif // __FreeBSD__

#if defined(_WIN32)
#include <processthreadsapi.h>
#include <winnt.h>
#endif // _WIN32
#endif // ENABLE_ARM_AES
#endif // !DISABLE_OPTIMIZED_SHA256

#include <cstddef>

namespace sapphire {
#if !defined(DISABLE_OPTIMIZED_SHA256)
#if defined(ENABLE_ARM_AES)
namespace arm_crypto_aes {
void Round(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t k0, uint32_t k1, uint32_t k2, uint32_t k3,
           uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
void RoundKeyless(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t& y0, uint32_t& y1, uint32_t& y2, uint32_t& y3);
} // namespace arm_crypto_aes
namespace arm_crypto_echo {
void FullStateRound(uint64_t W[16][2], uint32_t& k0, uint32_t& k1, uint32_t& k2, uint32_t& k3);
} // namespace arm_crypto_echo
#endif // ENABLE_ARM_AES

#if defined(ENABLE_SSSE3)
namespace ssse3_echo {
void ShiftAndMix(uint64_t W[16][2]);
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
namespace x86_aesni_shavite {
void CompressElement(uint32_t& l0, uint32_t& l1, uint32_t& l2, uint32_t& l3,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, const uint32_t* rk);
} // namespace x86_aesni_shavite
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
void ShiftAndMix(uint64_t W[16][2]);
} // namespace soft_echo
namespace soft_shavite {
void CompressElement(uint32_t& l0, uint32_t& l1, uint32_t& l2, uint32_t& l3,
                     uint32_t r0, uint32_t r1, uint32_t r2, uint32_t r3, const uint32_t* rk);
} // namespace soft_shavite
} // namespace sapphire

extern sapphire::dispatch::AESRoundFn aes_round;
extern sapphire::dispatch::AESRoundFnNk aes_round_nk;
extern sapphire::dispatch::EchoShiftMix echo_shift_mix;
extern sapphire::dispatch::EchoRoundFn echo_round;
extern sapphire::dispatch::ShaviteCompressFn shavite_c512e;

void SapphireAutoDetect()
{
    aes_round = sapphire::soft_aes::Round;
    aes_round_nk = sapphire::soft_aes::RoundKeyless;
    echo_round = sapphire::soft_echo::FullStateRound;
    echo_shift_mix = sapphire::soft_echo::ShiftAndMix;
    shavite_c512e = sapphire::soft_shavite::CompressElement;

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
        shavite_c512e = sapphire::x86_aesni_shavite::CompressElement;
    }
#endif // ENABLE_SSE41 && ENABLE_X86_AESNI
#if defined(ENABLE_SSSE3)
    const bool use_ssse3 = ((ecx >> 9) & 1);
    if (use_ssse3) {
        echo_shift_mix = sapphire::ssse3_echo::ShiftAndMix;
    }
#endif // ENABLE_SSSE3
#endif // HAVE_GETCPUID

#if defined(ENABLE_ARM_AES)
    bool have_arm_aes = false;
#if defined(__APPLE__)
    int val = 0;
    size_t len = sizeof(val);
    if (::sysctlbyname("hw.optional.arm.FEAT_AES", &val, &len, nullptr, 0) == 0) {
        have_arm_aes = val != 0;
    }
#endif // __APPLE__

#if defined(__linux__)
#if defined(__arm__)
    have_arm_aes = (::getauxval(AT_HWCAP2) & HWCAP2_AES);
#endif // __arm__
#if defined(__aarch64__)
    have_arm_aes = (::getauxval(AT_HWCAP) & HWCAP_AES);
#endif // __aarch64__
#endif // __linux__

#if defined(__FreeBSD__)
    [[maybe_unused]] unsigned long hwcap{0};
#if defined(__arm__)
    have_arm_aes = ((::elf_aux_info(AT_HWCAP2, &hwcap, sizeof(hwcap)) == 0) && ((hwcap & HWCAP2_AES) != 0));
#endif // __arm__
#if defined(__aarch64__)
    have_arm_aes = ((::elf_aux_info(AT_HWCAP, &hwcap, sizeof(hwcap)) == 0) && ((hwcap & HWCAP_AES) != 0));
#endif // __aarch64__
#endif // __FreeBSD__

#if defined(_WIN32)
    have_arm_aes = ::IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
#endif // _WIN32

    if (have_arm_aes) {
        aes_round = sapphire::arm_crypto_aes::Round;
        aes_round_nk = sapphire::arm_crypto_aes::RoundKeyless;
        echo_round = sapphire::arm_crypto_echo::FullStateRound;
    }
#endif // ENABLE_ARM_AES
#endif // !DISABLE_OPTIMIZED_SHA256
}
