/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#if !defined(BOOST_WINAPI_ENABLE_WARNINGS)

#if defined(_MSC_VER) && !(defined(__INTEL_COMPILER) || defined(__clang__))

#pragma warning(push, 3)
// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)
// Inconsistent annotation for 'X'
#pragma warning(disable: 28251)

#elif defined(__GNUC__) && !(defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)) \
    && (__GNUC__ * 100 + __GNUC_MINOR__) >= 406

#pragma GCC diagnostic push
// ISO C++ 1998 does not support 'long long'
#pragma GCC diagnostic ignored "-Wlong-long"

#endif

#endif // !defined(BOOST_WINAPI_ENABLE_WARNINGS)
