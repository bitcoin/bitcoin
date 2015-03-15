#include <stdlib.h>

#include "scrypt.h"
#include "pbkdf2.h"

#include "util.h"
#include "net.h"

#ifdef USE_SSE2
#ifdef _MSC_VER
// MSVC 64bit is unable to use inline asm
#include <intrin.h>
#else
// GCC Linux or i686-w64-mingw32
#include <cpuid.h>
#endif
#endif

extern "C" void scrypt_core(uint32_t *X, uint32_t *V);
#ifdef USE_SSE2
extern  uint256 scrypt_blockhash__sse2(const uint8_t* input);
#endif
/* cpu and memory intensive function to transform a 80 byte buffer into a 32 byte output
   scratchpad size needs to be at least 63 + (128 * r * p) + (256 * r + 64) + (128 * r * N) bytes
   r = 1, p = 1, N = 1024
 */

uint256 scrypt_blockhash_generic(const uint8_t* input)
{
    uint8_t scratchpad[SCRYPT_BUFFER_SIZE];
    uint32_t X[32];
    uint256 result = 0;

    uint32_t *V = (uint32_t *)(((uintptr_t)(scratchpad) + 63) & ~ (uintptr_t)(63));

    PBKDF2_SHA256(input, 80, input, 80, 1, (uint8_t *)X, 128);
    scrypt_core(X, V);
    PBKDF2_SHA256(input, 80, (uint8_t *)X, 128, 1, (uint8_t*)&result, 32);

    return result;
}

// By default, set to generic scrypt function. This will prevent crash in case when scrypt_detect_sse2() wasn't called
uint256 (*scrypt_blockhash_detected)(const uint8_t* input) = &scrypt_blockhash_generic;

#ifdef USE_SSE2
void scrypt_detect_sse2()
{
    // 32bit x86 Linux or Windows, detect cpuid features
    unsigned int cpuid_edx=0;
#if defined(_MSC_VER)
     // MSVC
     int x86cpuid[4];
     __cpuid(x86cpuid, 1);
     cpuid_edx = (unsigned int)x86cpuid[3];
#else // _MSC_VER
    // Linux or i686-w64-mingw32 (gcc-4.6.3)
    unsigned int eax, ebx, ecx;
    __get_cpuid(1, &eax, &ebx, &ecx, &cpuid_edx);
#endif // _MSC_VER

    if (cpuid_edx & 1<<26)
    {
        scrypt_blockhash_detected = &scrypt_blockhash__sse2;
        printf("scrypt: using scrypt-sse2 as detected.\n");
    }
    else
    {
        scrypt_blockhash_detected = &scrypt_blockhash_generic;
        printf("scrypt: using scrypt-generic, SSE2 unavailable.\n");
    }
}
#endif

uint256 scrypt_blockhash(const uint8_t* input)
{
    return scrypt_blockhash_detected(input);
}