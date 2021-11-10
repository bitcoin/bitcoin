// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020 Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_PROMOTE_FLOATING_POINT_HPP
#define BOOST_GEOMETRY_UTIL_PROMOTE_FLOATING_POINT_HPP


#include <type_traits>


namespace boost { namespace geometry
{


/*!
    \brief Meta-function converting, if necessary, to "a floating point" type
    \details
        - if input type is integer, type is double
        - else type is input type
    \ingroup utility
 */

template <typename T, typename PromoteIntegerTo = double>
struct promote_floating_point
{
    typedef std::conditional_t
        <
            std::is_integral<T>::value,
            PromoteIntegerTo,
            T
        > type;
};


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_UTIL_PROMOTE_FLOATING_POINT_HPP
