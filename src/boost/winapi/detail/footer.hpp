/*
 *             Copyright Andrey Semashev 2020.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

#if !defined(BOOST_WINAPI_ENABLE_WARNINGS)

#if defined(_MSC_VER) && !(defined(__INTEL_COMPILER) || defined(__clang__))

#pragma warning(pop)

#elif defined(__GNUC__) && !(defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)) \
    && (__GNUC__ * 100 + __GNUC_MINOR__) >= 406

#pragma GCC diagnostic pop

#endif

#endif // !defined(BOOST_WINAPI_ENABLE_WARNINGS)
