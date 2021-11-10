///////////////////////////////////////////////////////////////
//  Copyright 2020 Madhur Chauhan.
//  Copyright 2020 John Maddock. 
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_MP_INTEL_INTRINSICS_HPP
#define BOOST_MP_INTEL_INTRINSICS_HPP
//
// Select which actual implementation header to use:
//
#ifdef __has_include
#if __has_include(<immintrin.h>)
#define BOOST_MP_HAS_IMMINTRIN_H
#endif
#endif
//
// If this is GCC/clang, then check that the actual intrinsic exists:
//
#if defined(__has_builtin) && defined(__GNUC__)
#if !__has_builtin(__builtin_ia32_addcarryx_u64) && defined(BOOST_MP_HAS_IMMINTRIN_H) && !(defined(BOOST_GCC) && (__GNUC__ >= 9))
#undef BOOST_MP_HAS_IMMINTRIN_H
#endif
#elif defined(BOOST_MP_HAS_IMMINTRIN_H) && defined(__GNUC__) && !(defined(BOOST_GCC) && (__GNUC__ >= 9))
#undef BOOST_MP_HAS_IMMINTRIN_H
#endif

#if defined(__clang__) && (__clang__ < 9)
// We appear to crash the compiler if we try to use these intrinsics?
#undef BOOST_MP_HAS_IMMINTRIN_H
#endif

#if defined(BOOST_MSVC) && !defined(_M_IX86) && !defined(_M_ARM64) && !defined(_M_X64)
//
// When targeting platforms such as ARM, msvc still has the INtel headers in it's include path
// even though they're not usable.  See https://github.com/boostorg/multiprecision/issues/321
//
#undef BOOST_MP_HAS_IMMINTRIN_H
#endif


//
// If the compiler supports the intrinsics used by GCC internally
// inside <immintrin.h> then we'll use them directly.
// This is a bit of defensive programming, mostly for a modern clang
// sitting on top of an older GCC header install.
//
#if defined(__has_builtin) && !defined(BOOST_INTEL)

# if __has_builtin(__builtin_ia32_addcarryx_u64)
#  define BOOST_MP_ADDC __builtin_ia32_addcarryx_u
# endif

# if __has_builtin(__builtin_ia32_subborrow_u64)
#  define BOOST_MP_SUBB __builtin_ia32_subborrow_u
# elif __has_builtin(__builtin_ia32_sbb_u64)
#  define BOOST_MP_SUBB __builtin_ia32_sbb_u
# endif

#endif

#ifndef BOOST_MP_ADDC
#define BOOST_MP_ADDC _addcarry_u
#endif
#ifndef BOOST_MP_SUBB
#define BOOST_MP_SUBB _subborrow_u
#endif

#ifdef BOOST_MP_HAS_IMMINTRIN_H

#ifdef BOOST_MSVC
//
// This is a subset of the full <immintrin.h> :
//
#include <intrin.h>
#else
#include <immintrin.h>
#endif

#if defined(BOOST_HAS_INT128)

namespace boost { namespace multiprecision { namespace detail {

BOOST_MP_FORCEINLINE unsigned char addcarry_limb(unsigned char carry, limb_type a, limb_type b, limb_type* p_result)
{
#ifdef BOOST_INTEL
   using cast_type = unsigned __int64;
#else
   using cast_type = unsigned long long;
#endif
   return BOOST_JOIN(BOOST_MP_ADDC, 64)(carry, a, b, reinterpret_cast<cast_type*>(p_result));
}

BOOST_MP_FORCEINLINE unsigned char subborrow_limb(unsigned char carry, limb_type a, limb_type b, limb_type* p_result)
{
#ifdef BOOST_INTEL
   using cast_type = unsigned __int64;
#else
   using cast_type = unsigned long long;
#endif
   return BOOST_JOIN(BOOST_MP_SUBB, 64)(carry, a, b, reinterpret_cast<cast_type*>(p_result));
}

}}} // namespace boost::multiprecision::detail

#else

namespace boost { namespace multiprecision { namespace detail {

BOOST_MP_FORCEINLINE unsigned char addcarry_limb(unsigned char carry, limb_type a, limb_type b, limb_type* p_result)
{
   return BOOST_JOIN(BOOST_MP_ADDC, 32)(carry, a, b, reinterpret_cast<unsigned int*>(p_result));
}

BOOST_MP_FORCEINLINE unsigned char subborrow_limb(unsigned char carry, limb_type a, limb_type b, limb_type* p_result)
{
   return BOOST_JOIN(BOOST_MP_SUBB, 32)(carry, a, b, reinterpret_cast<unsigned int*>(p_result));
}

}}} // namespace boost::multiprecision::detail

#endif

#endif

#endif
