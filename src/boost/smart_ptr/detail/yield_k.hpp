#ifndef BOOST_SMART_PTR_DETAIL_YIELD_K_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_YIELD_K_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// boost/smart_ptr/detail/yield_k.hpp
//
// Copyright 2008, 2020 Peter Dimov
//
// inline void boost::detail::yield( unsigned k );
//
//   Typical use:
//   for( unsigned k = 0; !try_lock(); ++k ) yield( k );
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/smart_ptr/detail/sp_thread_pause.hpp>
#include <boost/smart_ptr/detail/sp_thread_sleep.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace detail
{

inline void yield( unsigned k )
{
    // Experiments on Windows and Fedora 32 show that a single pause,
    // followed by an immediate sp_thread_sleep(), is best.

    if( k == 0 )
    {
        sp_thread_pause();
    }
    else
    {
        sp_thread_sleep();
    }
}

} // namespace detail

} // namespace boost

#endif // #ifndef BOOST_SMART_PTR_DETAIL_YIELD_K_HPP_INCLUDED
