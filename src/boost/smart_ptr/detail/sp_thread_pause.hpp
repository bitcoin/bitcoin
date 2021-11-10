#ifndef BOOST_SMART_PTR_DETAIL_SP_THREAD_PAUSE_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_THREAD_PAUSE_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// boost/smart_ptr/detail/sp_thread_pause.hpp
//
// inline void bost::detail::sp_thread_pause();
//
//   Emits a "pause" instruction.
//
// Copyright 2008, 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0
// https://www.boost.org/LICENSE_1_0.txt

#if defined(_MSC_VER) && _MSC_VER >= 1310 && ( defined(_M_IX86) || defined(_M_X64) ) && !defined(__c2__)

extern "C" void _mm_pause();

#define BOOST_SP_PAUSE _mm_pause();

#elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )

#define BOOST_SP_PAUSE __asm__ __volatile__( "rep; nop" : : : "memory" );

#else

#define BOOST_SP_PAUSE

#endif

namespace boost
{
namespace detail
{

inline void sp_thread_pause()
{
    BOOST_SP_PAUSE
}

} // namespace detail
} // namespace boost

#undef BOOST_SP_PAUSE

#endif // #ifndef BOOST_SMART_PTR_DETAIL_SP_THREAD_PAUSE_HPP_INCLUDED
