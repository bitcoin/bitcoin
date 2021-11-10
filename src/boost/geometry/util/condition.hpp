// Boost.Geometry

// Copyright (c) 2015 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_CONDITION_HPP
#define BOOST_GEOMETRY_UTIL_CONDITION_HPP


#include <boost/config.hpp>


// The macro defined in this file allows to suppress the MSVC
// compiler warning C4127: conditional expression is constant

#ifdef BOOST_MSVC

// NOTE: The code commented out below contains an alternative implementation
// of a macro using a free function. It was left here in case if in the future
// version of MSVC for the code currently used in the macro implementation
// the warning was generated.

//#ifndef DOXYGEN_NO_DETAIL
//namespace boost { namespace geometry { namespace detail {
//BOOST_FORCEINLINE bool condition(bool const b) { return b; }
//}}} // boost::geometry::detail
//#endif // DOXYGEN_NO_DETAIL
//#define BOOST_GEOMETRY_CONDITION(CONDITION) boost::geometry::detail::condition(CONDITION)

#define BOOST_GEOMETRY_CONDITION(CONDITION) ((void)0, (CONDITION))

#else

#define BOOST_GEOMETRY_CONDITION(CONDITION) (CONDITION)

#endif


#endif // BOOST_GEOMETRY_UTIL_CONDITION_HPP
