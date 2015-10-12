// Copyright (c) 2015 The Novacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

void copy_swap_hashes(uint32_t *blocks, uint32_t *state)
{
    blocks[0] = __builtin_bswap32(state[0]);
    blocks[1] = __builtin_bswap32(state[1]);
    blocks[2] = __builtin_bswap32(state[2]);
    blocks[3] = __builtin_bswap32(state[3]);
    blocks[4] = __builtin_bswap32(state[4]);
    blocks[5] = __builtin_bswap32(state[5]);
    blocks[6] = __builtin_bswap32(state[6]);
    blocks[7] = __builtin_bswap32(state[7]);
    blocks[8] = __builtin_bswap32(state[8]);
    blocks[9] = __builtin_bswap32(state[9]);
    blocks[10] = __builtin_bswap32(state[10]);
    blocks[11] = __builtin_bswap32(state[11]);
    blocks[12] = __builtin_bswap32(state[12]);
    blocks[13] = __builtin_bswap32(state[13]);
    blocks[14] = __builtin_bswap32(state[14]);
    blocks[15] = __builtin_bswap32(state[15]);
    blocks[16] = __builtin_bswap32(state[16]);
    blocks[17] = __builtin_bswap32(state[17]);
    blocks[18] = __builtin_bswap32(state[18]);
    blocks[19] = __builtin_bswap32(state[19]);
    blocks[20] = __builtin_bswap32(state[20]);
    blocks[21] = __builtin_bswap32(state[21]);
    blocks[22] = __builtin_bswap32(state[22]);
    blocks[23] = __builtin_bswap32(state[23]);
    blocks[24] = __builtin_bswap32(state[24]);
    blocks[25] = __builtin_bswap32(state[25]);
    blocks[26] = __builtin_bswap32(state[26]);
    blocks[27] = __builtin_bswap32(state[27]);
    blocks[28] = __builtin_bswap32(state[28]);
    blocks[29] = __builtin_bswap32(state[29]);
    blocks[30] = __builtin_bswap32(state[30]);
    blocks[31] = __builtin_bswap32(state[31]);
}

#ifdef USE_SSSE3
#include <immintrin.h>

void copy_swap_hashes_ssse3(uint32_t *blocks, uint32_t *state) 
{
    __m128i mask = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    _mm_storeu_si128((__m128i *)&blocks[0], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[0]), mask));
    _mm_storeu_si128((__m128i *)&blocks[4], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[4]), mask));
    _mm_storeu_si128((__m128i *)&blocks[8], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[8]), mask));
    _mm_storeu_si128((__m128i *)&blocks[12], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[12]), mask));
    _mm_storeu_si128((__m128i *)&blocks[16], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[16]), mask));
    _mm_storeu_si128((__m128i *)&blocks[20], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[20]), mask));
    _mm_storeu_si128((__m128i *)&blocks[24], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[24]), mask));
    _mm_storeu_si128((__m128i *)&blocks[28], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&state[28]), mask));
}
#endif
