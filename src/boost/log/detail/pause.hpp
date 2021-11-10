/*
 *             Copyright Andrey Semashev 2016.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   pause.hpp
 * \author Andrey Semashev
 * \date   06.01.2016
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_PAUSE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_PAUSE_HPP_INCLUDED_

#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if defined(__INTEL_COMPILER) || defined(_MSC_VER)
#    if defined(_M_IX86)
#        define BOOST_LOG_AUX_PAUSE __asm { pause }
#    elif defined(_M_AMD64)
extern "C" void _mm_pause(void);
#        if defined(BOOST_MSVC)
#            pragma intrinsic(_mm_pause)
#        endif
#        define BOOST_LOG_AUX_PAUSE _mm_pause()
#    endif
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#    define BOOST_LOG_AUX_PAUSE __asm__ __volatile__("pause;")
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

BOOST_FORCEINLINE void pause() BOOST_NOEXCEPT
{
#if defined(BOOST_LOG_AUX_PAUSE)
    BOOST_LOG_AUX_PAUSE;
#endif
}

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_PAUSE_HPP_INCLUDED_
