/*
 *            Copyright Andrey Semashev 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   uuid/detail/uuid_x86.ipp
 *
 * \brief  This header contains optimized SSE implementation of \c boost::uuid operations.
 */

#ifndef BOOST_UUID_DETAIL_UUID_X86_IPP_INCLUDED_
#define BOOST_UUID_DETAIL_UUID_X86_IPP_INCLUDED_

// MSVC does not always have immintrin.h (at least, not up to MSVC 10), so include the appropriate header for each instruction set
#if defined(BOOST_UUID_USE_SSE41)
#include <smmintrin.h>
#elif defined(BOOST_UUID_USE_SSE3)
#include <pmmintrin.h>
#else
#include <emmintrin.h>
#endif

#if defined(BOOST_MSVC) && defined(_M_X64) && !defined(BOOST_UUID_USE_SSE3) && (BOOST_MSVC < 1900 /* Fixed in Visual Studio 2015 */ )
// At least MSVC 9 (VS2008) and 12 (VS2013) have an optimizer bug that sometimes results in incorrect SIMD code
// generated in Release x64 mode. In particular, it affects operator==, where the compiler sometimes generates
// pcmpeqd with a memory opereand instead of movdqu followed by pcmpeqd. The problem is that uuid can be
// not aligned to 16 bytes and pcmpeqd causes alignment violation in this case. We cannot be sure that other
// MSVC versions are not affected so we apply the workaround for all versions, except VS2015 on up where
// the bug has been fixed.
//
// https://svn.boost.org/trac/boost/ticket/8509#comment:3
// https://connect.microsoft.com/VisualStudio/feedbackdetail/view/981648#tabs
#define BOOST_UUID_DETAIL_MSVC_BUG981648
#if BOOST_MSVC >= 1600
extern "C" void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#endif
#endif

namespace boost {
namespace uuids {
namespace detail {

BOOST_FORCEINLINE __m128i load_unaligned_si128(const uint8_t* p) BOOST_NOEXCEPT
{
#if defined(BOOST_UUID_USE_SSE3)
    return _mm_lddqu_si128(reinterpret_cast< const __m128i* >(p));
#elif !defined(BOOST_UUID_DETAIL_MSVC_BUG981648)
    return _mm_loadu_si128(reinterpret_cast< const __m128i* >(p));
#elif defined(BOOST_MSVC) && BOOST_MSVC >= 1600
    __m128i mm = _mm_loadu_si128(reinterpret_cast< const __m128i* >(p));
    // Make sure this load doesn't get merged with the subsequent instructions
    _ReadWriteBarrier();
    return mm;
#else
    // VS2008 x64 doesn't respect _ReadWriteBarrier above, so we have to generate this crippled code to load unaligned data
    return _mm_unpacklo_epi64(_mm_loadl_epi64(reinterpret_cast< const __m128i* >(p)), _mm_loadl_epi64(reinterpret_cast< const __m128i* >(p + 8)));
#endif
}

} // namespace detail

inline bool uuid::is_nil() const BOOST_NOEXCEPT
{
    __m128i mm = uuids::detail::load_unaligned_si128(data);
#if defined(BOOST_UUID_USE_SSE41)
    return _mm_test_all_zeros(mm, mm) != 0;
#else
    mm = _mm_cmpeq_epi32(mm, _mm_setzero_si128());
    return _mm_movemask_epi8(mm) == 0xFFFF;
#endif
}

inline void uuid::swap(uuid& rhs) BOOST_NOEXCEPT
{
    __m128i mm_this = uuids::detail::load_unaligned_si128(data);
    __m128i mm_rhs = uuids::detail::load_unaligned_si128(rhs.data);
    _mm_storeu_si128(reinterpret_cast< __m128i* >(rhs.data), mm_this);
    _mm_storeu_si128(reinterpret_cast< __m128i* >(data), mm_rhs);
}

inline bool operator== (uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    __m128i mm_left = uuids::detail::load_unaligned_si128(lhs.data);
    __m128i mm_right = uuids::detail::load_unaligned_si128(rhs.data);

#if defined(BOOST_UUID_USE_SSE41)
    __m128i mm = _mm_xor_si128(mm_left, mm_right);
    return _mm_test_all_zeros(mm, mm) != 0;
#else
    __m128i mm_cmp = _mm_cmpeq_epi32(mm_left, mm_right);
    return _mm_movemask_epi8(mm_cmp) == 0xFFFF;
#endif
}

inline bool operator< (uuid const& lhs, uuid const& rhs) BOOST_NOEXCEPT
{
    __m128i mm_left = uuids::detail::load_unaligned_si128(lhs.data);
    __m128i mm_right = uuids::detail::load_unaligned_si128(rhs.data);

    // To emulate lexicographical_compare behavior we have to perform two comparisons - the forward and reverse one.
    // Then we know which bytes are equivalent and which ones are different, and for those different the comparison results
    // will be opposite. Then we'll be able to find the first differing comparison result (for both forward and reverse ways),
    // and depending on which way it is for, this will be the result of the operation. There are a few notes to consider:
    //
    // 1. Due to little endian byte order the first bytes go into the lower part of the xmm registers,
    //    so the comparison results in the least significant bits will actually be the most signigicant for the final operation result.
    //    This means we have to determine which of the comparison results have the least significant bit on, and this is achieved with
    //    the "(x - 1) ^ x" trick.
    // 2. Because there is only signed comparison in SSE/AVX, we have to invert byte comparison results whenever signs of the corresponding
    //    bytes are different. I.e. in signed comparison it's -1 < 1, but in unsigned it is the opposite (255 > 1). To do that we XOR left and right,
    //    making the most significant bit of each byte 1 if the signs are different, and later apply this mask with another XOR to the comparison results.
    // 3. pcmpgtw compares for "greater" relation, so we swap the arguments to get what we need.

    const __m128i mm_signs_mask = _mm_xor_si128(mm_left, mm_right);

    __m128i mm_cmp = _mm_cmpgt_epi8(mm_right, mm_left), mm_rcmp = _mm_cmpgt_epi8(mm_left, mm_right);

    mm_cmp = _mm_xor_si128(mm_signs_mask, mm_cmp);
    mm_rcmp = _mm_xor_si128(mm_signs_mask, mm_rcmp);

    uint32_t cmp = static_cast< uint32_t >(_mm_movemask_epi8(mm_cmp)), rcmp = static_cast< uint32_t >(_mm_movemask_epi8(mm_rcmp));

    cmp = (cmp - 1u) ^ cmp;
    rcmp = (rcmp - 1u) ^ rcmp;

    return cmp < rcmp;
}

} // namespace uuids
} // namespace boost

#endif // BOOST_UUID_DETAIL_UUID_X86_IPP_INCLUDED_
