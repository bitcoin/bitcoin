// Boost.Range library
//
//  Copyright Thorsten Ottosen 2003-2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef BOOST_RANGE_DETAIL_SIZER_HPP
#define BOOST_RANGE_DETAIL_SIZER_HPP

#if defined(_MSC_VER)
# pragma once
#endif

#include <boost/range/config.hpp>
#include <cstddef>

namespace boost 
{
    //////////////////////////////////////////////////////////////////////
    // constant array size
    //////////////////////////////////////////////////////////////////////
    
    template< typename T, std::size_t sz >
    char (& sizer( const T BOOST_RANGE_ARRAY_REF()[sz] ) )[sz];
    
    template< typename T, std::size_t sz >
    char (& sizer( T BOOST_RANGE_ARRAY_REF()[sz] ) )[sz];

} // namespace 'boost'

#endif
