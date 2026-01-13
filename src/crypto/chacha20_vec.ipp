// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#include <bit>
#include <cassert>
#include <cstring>
#include <limits>

#if defined(ENABLE_CHACHA20_VEC)

#if defined(CHACHA20_VEC_DISABLE_STATES_16) && \
    defined(CHACHA20_VEC_DISABLE_STATES_8) && \
    defined(CHACHA20_VEC_DISABLE_STATES_6) && \
    defined(CHACHA20_VEC_DISABLE_STATES_4) && \
    defined(CHACHA20_VEC_DISABLE_STATES_2)
#define CHACHA20_VEC_ALL_MULTI_STATES_DISABLED
#endif


#if !defined(CHACHA20_VEC_ALL_MULTI_STATES_DISABLED)

#if defined(__has_attribute)
#  if __has_attribute(always_inline)
#    define ALWAYS_INLINE __attribute__ ((always_inline)) inline
#  endif
#endif

#if !defined(ALWAYS_INLINE)
#  define ALWAYS_INLINE inline
#endif


namespace {

using vec256 = uint32_t __attribute__((__vector_size__(32)));

/** Endian-conversion for big-endian */
ALWAYS_INLINE void vec_byteswap(vec256& vec)
{
    if constexpr (std::endian::native == std::endian::big)
    {
        vec256 ret;
        ret[0] = __builtin_bswap32(vec[0]);
        ret[1] = __builtin_bswap32(vec[1]);
        ret[2] = __builtin_bswap32(vec[2]);
        ret[3] = __builtin_bswap32(vec[3]);
        ret[4] = __builtin_bswap32(vec[4]);
        ret[5] = __builtin_bswap32(vec[5]);
        ret[6] = __builtin_bswap32(vec[6]);
        ret[7] = __builtin_bswap32(vec[7]);
        vec = ret;
    }
}

/** Left-rotate vector */
template <size_t BITS>
ALWAYS_INLINE void vec_rotl(vec256& vec)
{
    vec = (vec << BITS) | (vec >> (32 - BITS));
}

/** Store a vector in all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_set_vec256(std::array<vec256, I>& arr, const vec256& vec)
{
    std::get<ITER>(arr) = vec;
    if constexpr(ITER + 1 < I ) arr_set_vec256<I, ITER + 1>(arr, vec);
}

/** Add a vector to all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_vec256(std::array<vec256, I>& arr, const vec256& vec)
{
    std::get<ITER>(arr) += vec;
    if constexpr(ITER + 1 < I ) arr_add_vec256<I, ITER + 1>(arr, vec);
}

/** Add corresponding vectors in arr1 to arr0 */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_arr(std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1)
{
    std::get<ITER>(arr0) += std::get<ITER>(arr1);
    if constexpr(ITER + 1 < I ) arr_add_arr<I, ITER + 1>(arr0, arr1);
}

/** Perform add/xor/rotate for the round function */
template <size_t BITS, size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_xor_rot(std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1, std::array<vec256, I>& arr2)
{
    vec256& x = std::get<ITER>(arr0);
    const vec256& y = std::get<ITER>(arr1);
    vec256& z = std::get<ITER>(arr2);

    x += y;
    z ^= x;
    vec_rotl<BITS>(z);

    if constexpr(ITER + 1 < I ) arr_add_xor_rot<BITS, I, ITER + 1>(arr0, arr1, arr2);
}

/*
The first round:
            QUARTERROUND( x0, x4, x8,x12);
            QUARTERROUND( x1, x5, x9,x13);
            QUARTERROUND( x2, x6,x10,x14);
            QUARTERROUND( x3, x7,x11,x15);

The second round:
            QUARTERROUND( x0, x5,x10,x15);
            QUARTERROUND( x1, x6,x11,x12);
            QUARTERROUND( x2, x7, x8,x13);
            QUARTERROUND( x3, x4, x9,x14);

After the first round, arr_shuf0, arr_shuf1, and arr_shuf2 are used to shuffle
the layout to prepare for the second round.

After the second round, they are used (in reverse) to restore the original
layout.

*/
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf0(std::array<vec256, I>& arr)
{
    vec256& x = std::get<ITER>(arr);
    x = __builtin_shufflevector(x, x, 1, 2, 3, 0, 5, 6, 7, 4);
    if constexpr(ITER + 1 < I ) arr_shuf0<I, ITER + 1>(arr);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf1(std::array<vec256, I>& arr)
{
    vec256& x = std::get<ITER>(arr);
    x = __builtin_shufflevector(x, x, 2, 3, 0, 1, 6, 7, 4, 5);
    if constexpr(ITER + 1 < I ) arr_shuf1<I, ITER + 1>(arr);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf2(std::array<vec256, I>& arr)
{
    vec256& x = std::get<ITER>(arr);
    x = __builtin_shufflevector(x, x, 3, 0, 1, 2, 7, 4, 5, 6);
    if constexpr(ITER + 1 < I ) arr_shuf2<I, ITER + 1>(arr);
}

/* Main round function. */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void doubleround(std::array<vec256, I>& arr0, std::array<vec256, I>& arr1, std::array<vec256, I>&arr2, std::array<vec256, I>&arr3)
{
    arr_add_xor_rot<16>(arr0, arr1, arr3);
    arr_add_xor_rot<12>(arr2, arr3, arr1);
    arr_add_xor_rot<8>(arr0, arr1, arr3);
    arr_add_xor_rot<7>(arr2, arr3, arr1);
    arr_shuf0(arr1);
    arr_shuf1(arr2);
    arr_shuf2(arr3);
    arr_add_xor_rot<16>(arr0, arr1, arr3);
    arr_add_xor_rot<12>(arr2, arr3, arr1);
    arr_add_xor_rot<8>(arr0, arr1, arr3);
    arr_add_xor_rot<7>(arr2, arr3, arr1);
    arr_shuf2(arr1);
    arr_shuf1(arr2);
    arr_shuf0(arr3);
    if constexpr (ITER + 1 < 10) doubleround<I, ITER + 1>(arr0, arr1, arr2, arr3);
}

/* Read 32bytes of input, xor with calculated state, write to output. Assumes
   that input and output are unaligned, and makes no assumptions about the
   internal layout of vec256;
*/
ALWAYS_INLINE void vec_read_xor_write(std::span<const std::byte, 32> in_bytes, std::span<std::byte, 32> out_bytes, const vec256& vec)
{
    std::array<uint32_t, 8> temparr;
    memcpy(temparr.data(), in_bytes.data(), in_bytes.size());
    vec256 tempvec = vec;
    vec_byteswap(tempvec);
    tempvec ^= (vec256){temparr[0], temparr[1], temparr[2], temparr[3], temparr[4], temparr[5], temparr[6], temparr[7]};
    temparr = {tempvec[0], tempvec[1], tempvec[2], tempvec[3], tempvec[4], tempvec[5], tempvec[6], tempvec[7]};
    memcpy(out_bytes.data(), temparr.data(), out_bytes.size());
}

/* Merge the 128 bit lanes from 2 states to the proper order, then pass each vec_read_xor_write */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1, const std::array<vec256, I>& arr2, const std::array<vec256, I>& arr3)
{
    const vec256& w = std::get<ITER>(arr0);
    const vec256& x = std::get<ITER>(arr1);
    const vec256& y = std::get<ITER>(arr2);
    const vec256& z = std::get<ITER>(arr3);

    vec_read_xor_write(in_bytes.first<32>(), out_bytes.first<32>(), __builtin_shufflevector(w, x, 4, 5, 6, 7, 12, 13, 14, 15));
    vec_read_xor_write(in_bytes.subspan<32, 32>(), out_bytes.subspan<32, 32>(), __builtin_shufflevector(y, z, 4, 5, 6, 7, 12, 13, 14, 15));
    vec_read_xor_write(in_bytes.subspan<64, 32>(), out_bytes.subspan<64, 32>(), __builtin_shufflevector(w, x, 0, 1, 2, 3, 8, 9, 10, 11));
    vec_read_xor_write(in_bytes.subspan<96, 32>(), out_bytes.subspan<96, 32>(), __builtin_shufflevector(y, z, 0, 1, 2, 3, 8, 9, 10, 11));

    if constexpr(ITER + 1 < I ) arr_read_xor_write<I, ITER + 1>(in_bytes.subspan<128>(), out_bytes.subspan<128>(), arr0, arr1, arr2, arr3);
}

/* Compile-time helper to create addend vectors which used to increment the states

    Generates vectors of the pattern:
    1 0 0 0 0 0 0 0
    3 0 0 0 2 0 0 0
    5 0 0 0 4 0 0 0
    ...
*/
template <size_t SIZE>
consteval std::array<vec256, SIZE> generate_increments()
{
    std::array<vec256, SIZE> rows;
    for (uint32_t i = 0; i < SIZE; i ++)
    {
        rows[i] = (i * (vec256){2, 0, 0, 0, 2, 0, 0, 0}) + (vec256){1, 0, 0, 0, 0, 0, 0, 0};
    }
    return rows;
}

/* Main crypt function. Calculates up to 16 states.

    Each array contains one or more vectors, with each array representing a
    quarter of a state. Initially, the high and low parts of each vector are
    duplicated. They each contain a portion of the current and next state.

    arr0[0]    arr1[0]    arr2[0]    arr3[0]   increment
    ----------|---------|----------|----------|---------
    0x61707865 input[0]   input[4]   input[8]   [1]
    0x3320646e input[1]   input[5]   input[9]   [0]
    0x79622d32 input[2]   input[6]   input[10]  [0]
    0x6b206574 input[3]   input[7]   input[11]  [0]

    0x61707865 input[0]   input[4]   input[8]   [0]
    0x3320646e input[1]   input[5]   input[9]   [0]
    0x79622d32 input[2]   input[6]   input[10]  [0]
    0x6b206574 input[3]   input[7]   input[11]  [0]

    After loading the states, arr3's vectors are incremented as-necessary to
    contain the correct counter values.

    This way, operations like "arr0[0] += arr1[0]" can perform all 8 operations
    in parallel, taking advantage of 256bit registers where available.

    arrX[0] represents states 0 and 1.
    arrX[1] represents states 2 and 3 (if present)
    etc.

    After the doublerounds have been run and the initial state has been mixed
    back in, the high and low portions of the vectors in each array are
    shuffled in order to prepare them for mixing with the input bytes. Finally,
    each state is xor'd with its corresponding input, byteswapped if necessary,
    and written to its output.
*/
template <size_t STATES>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& state0, const vec256& state1, const vec256& state2)
{
    static constexpr size_t HALF_STATES = STATES / 2;
    static constexpr vec256 nums256 = (vec256){0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    static constinit std::array<vec256, HALF_STATES> increments = generate_increments<HALF_STATES>();

    std::array<vec256, HALF_STATES> arr0, arr1, arr2, arr3;

    arr_set_vec256(arr0, nums256);
    arr_set_vec256(arr1, state0);
    arr_set_vec256(arr2, state1);
    arr_set_vec256(arr3, state2);

    arr_add_arr(arr3, increments);

    doubleround(arr0, arr1, arr2, arr3);

    arr_add_vec256(arr0, nums256);
    arr_add_vec256(arr1, state0);
    arr_add_vec256(arr2, state1);
    arr_add_vec256(arr3, state2);

    arr_add_arr(arr3, increments);

    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3);
}

} // anonymous namespace
#endif // CHACHA20_VEC_ALL_MULTI_STATES_DISABLED

#if defined(CHACHA20_NAMESPACE)
namespace CHACHA20_NAMESPACE {
#endif

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
#if !defined(CHACHA20_VEC_ALL_MULTI_STATES_DISABLED)
    assert(in_bytes.size() == out_bytes.size());
    const vec256 state0 =  (vec256){input[0], input[1], input[2], input[3], input[0], input[1], input[2], input[3]};
    const vec256 state1 =  (vec256){input[4], input[5], input[6], input[7], input[4], input[5], input[6], input[7]};
    vec256 state2 =  (vec256){input[8], input[9], input[10], input[11], input[8], input[9], input[10], input[11]};
#if !defined(CHACHA20_VEC_DISABLE_STATES_16)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 16) {
        multi_block_crypt<16>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){16, 0, 0, 0, 16, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_8)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 8) {
        multi_block_crypt<8>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){8, 0, 0, 0, 8, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_6)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 6) {
        multi_block_crypt<6>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){6, 0, 0, 0, 6, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_4)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 4) {
        multi_block_crypt<4>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){4, 0, 0, 0, 4, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_2)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 2) {
        multi_block_crypt<2>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){2, 0, 0, 0, 2, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
    }
#endif
#endif // CHACHA20_VEC_ALL_MULTI_STATES_DISABLED
}

#if defined(CHACHA20_NAMESPACE)
}
#endif

#endif // ENABLE_CHACHA20_VEC
