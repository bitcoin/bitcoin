#include "aes.h"

#ifdef __x86_64__

#include <stdint.h>
#include <x86intrin.h>

static inline void ExpandAESKey256_sub1(__m128i *tmp1, __m128i *tmp2)
{
    __m128i tmp4;
    *tmp2 = _mm_shuffle_epi32(*tmp2, 0xFF);
    tmp4 = _mm_slli_si128(*tmp1, 0x04);
    *tmp1 = _mm_xor_si128(*tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    *tmp1 = _mm_xor_si128(*tmp1, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    *tmp1 = _mm_xor_si128(*tmp1, tmp4);
    *tmp1 = _mm_xor_si128(*tmp1, *tmp2);
}

static inline void ExpandAESKey256_sub2(__m128i *tmp1, __m128i *tmp3)
{
    __m128i tmp2, tmp4;
    tmp4 = _mm_aeskeygenassist_si128(*tmp1, 0x00);
    tmp2 = _mm_shuffle_epi32(tmp4, 0xAA);
    tmp4 = _mm_slli_si128(*tmp3, 0x04);
    *tmp3 = _mm_xor_si128(*tmp3, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    *tmp3 = _mm_xor_si128(*tmp3, tmp4);
    tmp4 = _mm_slli_si128(tmp4, 0x04);
    *tmp3 = _mm_xor_si128(*tmp3, tmp4);
    *tmp3 = _mm_xor_si128(*tmp3, tmp2);
}

// Special thanks to Intel for helping me
// with ExpandAESKey256() and its subroutines
static void ExpandAESKey256(__m128i *keys, const __m128i *KeyBuf)
{
    __m128i tmp1, tmp2, tmp3;
    tmp1 = _mm_loadu_si128(&KeyBuf[0]);
    tmp3 = _mm_loadu_si128(&KeyBuf[1]);
    keys[0] = tmp1;
    keys[1] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x01);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[2] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[3] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x02);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[4] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[5] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x04);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[6] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[7] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x08);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[8] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[9] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x10);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[10] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[11] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x20);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[12] = tmp1;
    ExpandAESKey256_sub2(&tmp1, &tmp3);
    keys[13] = tmp3;

    tmp2 = _mm_aeskeygenassist_si128(tmp3, 0x40);
    ExpandAESKey256_sub1(&tmp1, &tmp2);
    keys[14] = tmp1;
}

#define AESENC(i,j) \
    State[j] = _mm_aesenc_si128(State[j], ExpandedKey[(j*16)+i]);

#define AESENC_N(i) \
    AESENC(i,0) \
    AESENC(i,1) \
    AESENC(i,2) \
    AESENC(i,3) \
    AESENC(i,4) \
    AESENC(i,5) \
    AESENC(i,6) \
    AESENC(i,7) \


static inline void AES256Core(__m128i* State, const __m128i* ExpandedKey)
{
    const uint32_t N = AES_PARALLEL_N;

    for(unsigned int j=0; j<N; ++j)
        State[j] = _mm_xor_si128(State[j], ExpandedKey[j*16+0]);

    AESENC_N(1)
    AESENC_N(2)
    AESENC_N(3)
    AESENC_N(4)
    AESENC_N(5)
    AESENC_N(6)
    AESENC_N(7)
    AESENC_N(8)
    AESENC_N(9)
    AESENC_N(10)
    AESENC_N(11)
    AESENC_N(12)
    AESENC_N(13)

    for(unsigned int j=0; j<N; ++j)
        State[j] = _mm_aesenclast_si128(State[j], ExpandedKey[j*16+14]);
}

static void AES256CBC(__m128i** data, const __m128i** next, const __m128i* ExpandedKey, __m128i* IV)
{
    const uint32_t N = AES_PARALLEL_N;
    __m128i State[N];
    for(unsigned int j=0; j<N; ++j) {
        State[j] = _mm_xor_si128( _mm_xor_si128(data[j][0], next[j][0]), IV[j]);
    }

    AES256Core(State, ExpandedKey);
    for(unsigned int j=0; j<N; ++j)
        data[j][0] = State[j];

    for(int i = 1; i < BLOCK_COUNT; ++i)
    {
        for(unsigned int j=0; j<N; ++j) {
            State[j] = _mm_xor_si128( _mm_xor_si128(data[j][i], next[j][i]), data[j][i - 1]);
        }
        AES256Core(State, ExpandedKey);
        for(unsigned int j=0; j<N; ++j) {
            data[j][i] = State[j];
        }
    }
}

void ExpandAESKey256_int(uint32_t *keys, const uint32_t *KeyBuf) {
	return ExpandAESKey256((__m128i*)keys, (const __m128i*)KeyBuf);
}

void AES256CBC_int(uint32_t** data, const uint32_t** next, const uint32_t* ExpandedKey, uint32_t* IV) {
	return AES256CBC((__m128i**)data, (const __m128i**)next, (const __m128i*)ExpandedKey, (__m128i*)IV);
}

#else // __AES__

#include <stdlib.h>

// Not supported on this platform. These functions should never be called.
void ExpandAESKey256_int(uint32_t *keys, const uint32_t *KeyBuf) {
	exit(1);
}
void AES256CBC_int(uint32_t** data, const uint32_t** next, const uint32_t* ExpandedKey, uint32_t* IV) {
	exit(1);
}

#endif // __AES__
