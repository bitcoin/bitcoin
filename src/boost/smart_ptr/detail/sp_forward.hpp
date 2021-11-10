#ifndef BOOST_SMART_PTR_DETAIL_SP_FORWARD_HPP_INCLUDED
#define BOOST_SMART_PTR_DETAIL_SP_FORWARD_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

//  detail/sp_forward.hpp
//
//  Copyright 2008,2012 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

namespace boost
{

namespace detail
{

#if !defined( BOOST_NO_CXX11_RVALUE_REFERENCES )

#if defined( BOOST_GCC ) && __GNUC__ * 100 + __GNUC_MINOR__ <= 404

// GCC 4.4 supports an outdated version of rvalue references and creates a copy of the forwarded object.
// This results in warnings 'returning reference to temporary'. Therefore we use a special version similar to std::forward.
template< class T > T&& sp_forward( T && t ) BOOST_NOEXCEPT
{
    return t;
}

#else

template< class T > T&& sp_forward( T & t ) BOOST_NOEXCEPT
{
    return static_cast< T&& >( t );
}

#endif

#endif

} // namespace detail

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_DETAIL_SP_FORWARD_HPP_INCLUDED
