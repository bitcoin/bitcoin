/*
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 *
 * (C) Copyright 2013 Tim Blechmann
 * (C) Copyright 2013, 2020 Andrey Semashev
 */

#ifndef BOOST_ATOMIC_DETAIL_PAUSE_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_PAUSE_HPP_INCLUDED_

#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(_MSC_VER)
#if defined(_M_AMD64) || defined(_M_IX86)
extern "C" void _mm_pause(void);
#if defined(BOOST_MSVC)
#pragma intrinsic(_mm_pause)
#endif
#elif defined(_M_ARM64) || defined(_M_ARM)
extern "C" void __yield(void);
#if defined(BOOST_MSVC)
#pragma intrinsic(__yield)
#endif
#endif
#endif

namespace boost {
namespace atomics {
namespace detail {

BOOST_FORCEINLINE void pause() BOOST_NOEXCEPT
{
#if defined(_MSC_VER)
#if defined(_M_AMD64) || defined(_M_IX86)
    _mm_pause();
#elif defined(_M_ARM64) || defined(_M_ARM)
    __yield();
#endif
#elif defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("pause;" : : : "memory");
#elif (defined(__ARM_ARCH) && __ARM_ARCH >= 8) || defined(__ARM_ARCH_8A__) || defined(__aarch64__)
    __asm__ __volatile__("yield;" : : : "memory");
#endif
#endif
}

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_PAUSE_HPP_INCLUDED_
