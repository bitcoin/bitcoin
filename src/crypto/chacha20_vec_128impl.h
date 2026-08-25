// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>

#if defined(__has_attribute)
#  if __has_attribute(always_inline)
#    define ALWAYS_INLINE __attribute__ ((always_inline)) inline
#  endif
#endif

#if !defined(ALWAYS_INLINE)
#  define ALWAYS_INLINE inline
#endif

namespace  chacha20_vec128 {

static constexpr size_t BLOCKLEN = 64;

using vec128 = uint32_t __attribute__((__vector_size__(16)));

/** Endian-conversion for big-endian */
ALWAYS_INLINE void vec_byteswap(vec128& vec)
{
    if constexpr (std::endian::native == std::endian::big)
    {
        vec128 ret;
        ret[0] = __builtin_bswap32(vec[0]);
        ret[1] = __builtin_bswap32(vec[1]);
        ret[2] = __builtin_bswap32(vec[2]);
        ret[3] = __builtin_bswap32(vec[3]);
        vec = ret;
    }
}

/** Left-rotate vector */
template <size_t BITS>
ALWAYS_INLINE void vec_rotl(vec128& vec)
{
    vec = (vec << BITS) | (vec >> (32 - BITS));
}

#ifdef __SSE2__
template <>
ALWAYS_INLINE
void vec_rotl<16>(vec128& vec)
{
    using vec128_u16 = uint16_t __attribute__((__vector_size__(16)));
    vec = (vec128)__builtin_shufflevector(reinterpret_cast<vec128_u16>(vec), vec128_u16{}, 1, 0, 3, 2, 5, 4, 7, 6);
}
#endif

#if defined(__SSSE3__)
template <>
ALWAYS_INLINE
void vec_rotl<8>(vec128& vec)
{
    using vec128_u8 = uint8_t __attribute__((__vector_size__(16)));
    vec = (vec128)__builtin_shufflevector(reinterpret_cast<vec128_u8>(vec), vec128_u8{}, 3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14);
}
#endif


/** Store a vector in all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_set_vec128(std::array<vec128, I>& arr, const vec128& vec)
{
    std::get<ITER>(arr) = vec;
    if constexpr(ITER + 1 < I ) arr_set_vec128<I, ITER + 1>(arr, vec);
}

/** Add a vector to all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_vec128(std::array<vec128, I>& arr, const vec128& vec)
{
    std::get<ITER>(arr) += vec;
    if constexpr(ITER + 1 < I ) arr_add_vec128<I, ITER + 1>(arr, vec);
}

/** Add corresponding vectors in arr1 to arr0 */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_arr(std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1)
{
    std::get<ITER>(arr0) += std::get<ITER>(arr1);
    if constexpr(ITER + 1 < I ) arr_add_arr<I, ITER + 1>(arr0, arr1);
}

template <size_t BITS>
ALWAYS_INLINE void vec_add_xor_rot(vec128& x, const vec128& y, vec128& z)
{
    x += y;
    z ^= x;
    vec_rotl<BITS>(z);
}

/** Perform add/xor/rotate for the round function */
template <size_t BITS, size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_xor_rot(std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1, std::array<vec128, I>& arr2)
{
    vec128& x = std::get<ITER>(arr0);
    const vec128& y = std::get<ITER>(arr1);
    vec128& z = std::get<ITER>(arr2);

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
ALWAYS_INLINE void arr_shuf0(std::array<vec128, I>& arr)
{
    vec128& x = std::get<ITER>(arr);
    x = vec128{x[1], x[2], x[3], x[0]};

    if constexpr(ITER + 1 < I ) arr_shuf0<I, ITER + 1>(arr);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf1(std::array<vec128, I>& arr)
{
    vec128& x = std::get<ITER>(arr);
    x = vec128{x[2], x[3], x[0], x[1]};

    if constexpr(ITER + 1 < I ) arr_shuf1<I, ITER + 1>(arr);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf2(std::array<vec128, I>& arr)
{
    vec128& x = std::get<ITER>(arr);
    x = vec128{x[3], x[0], x[1], x[2]};

    if constexpr(ITER + 1 < I ) arr_shuf2<I, ITER + 1>(arr);
}

/* Main round function. */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void doubleround(std::array<vec128, I>& arr0, std::array<vec128, I>& arr1, std::array<vec128, I>&arr2, std::array<vec128, I>&arr3)
{
    for(unsigned i = 0; i < 10; i++) {
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
    }
}

/* Read 32bytes of input, xor with calculated state, write to output. Assumes
   that input and output are unaligned, and makes no assumptions about the
   internal layout of vec128;
*/
ALWAYS_INLINE void vec_read_xor_write(std::span<const std::byte, 16> in_bytes, std::span<std::byte, 16> out_bytes, const vec128& vec)
{
    std::array<uint32_t, 4> temparr;
    memcpy(temparr.data(), in_bytes.data(), in_bytes.size());
    vec128 tempvec = vec;
    vec_byteswap(tempvec);
    tempvec ^= (vec128){temparr[0], temparr[1], temparr[2], temparr[3]};
    temparr = {tempvec[0], tempvec[1], tempvec[2], tempvec[3]};
    memcpy(out_bytes.data(), temparr.data(), out_bytes.size());
}

/* Merge the 128 bit lanes from 2 states to the proper order, then pass each vec_read_xor_write */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1, const std::array<vec128, I>& arr2, const std::array<vec128, I>& arr3)
{
    const vec128& w = std::get<ITER>(arr0);
    const vec128& x = std::get<ITER>(arr1);
    const vec128& y = std::get<ITER>(arr2);
    const vec128& z = std::get<ITER>(arr3);

    vec_read_xor_write(in_bytes.first<16>(), out_bytes.first<16>(), w);
    vec_read_xor_write(in_bytes.subspan<16, 16>(), out_bytes.subspan<16, 16>(), x);
    vec_read_xor_write(in_bytes.subspan<32, 16>(), out_bytes.subspan<32, 16>(), y);
    vec_read_xor_write(in_bytes.subspan<48, 16>(), out_bytes.subspan<48, 16>(), z);

    if constexpr(ITER + 1 < I ) arr_read_xor_write<I, ITER + 1>(in_bytes.subspan<64>(), out_bytes.subspan<64>(), arr0, arr1, arr2, arr3);
}
template <size_t SIZE>
consteval std::array<vec128, SIZE> generate_increments()
{
    std::array<vec128, SIZE> rows;
    for (uint32_t i = 0; i < SIZE; i ++)
    {
        rows[i] = (vec128){i, 0, 0, 0};
    }
    return rows;
}

template <size_t STATES>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec128& state0, const vec128& state1, const vec128& state2)
{
    static constexpr vec128 nums256 = (vec128){0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    static constinit std::array<vec128, STATES> increments = generate_increments<STATES>();
    std::array<vec128, STATES> arr0, arr1, arr2, arr3;

    arr_set_vec128(arr0, nums256);
    arr_set_vec128(arr1, state0);
    arr_set_vec128(arr2, state1);
    arr_set_vec128(arr3, state2);
    arr_add_arr(arr3, increments);

    doubleround(arr0, arr1, arr2, arr3);

    arr_add_vec128(arr0, nums256);
    arr_add_vec128(arr1, state0);
    arr_add_vec128(arr2, state1);
    arr_add_vec128(arr3, state2);
    arr_add_arr(arr3, increments);

    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3);
}

class ChaCha20Vectorized
{
    const vec128 state0;
    const vec128 state1;
    vec128 state2;
public:
    ALWAYS_INLINE ChaCha20Vectorized(const std::array<uint32_t, 12>& input) noexcept
        : state0((vec128){input[0], input[1], input[2], input[3]})
        , state1((vec128){input[4], input[5], input[6], input[7]})
        , state2((vec128){input[8], input[9], input[10], input[11]})
    {
    }
    template <size_t STATES>
    constexpr ALWAYS_INLINE void CryptStates(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes)
    {
        multi_block_crypt<STATES>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec128){STATES, 0, 0, 0};
        in_bytes = in_bytes.subspan(BLOCKLEN * STATES);
        out_bytes = out_bytes.subspan(BLOCKLEN * STATES);
    }
};

} // namespace chacha20_vec128

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
