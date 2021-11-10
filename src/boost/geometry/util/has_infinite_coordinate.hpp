// Boost.Geometry

// Copyright (c) 2015-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_HAS_INFINITE_COORDINATE_HPP
#define BOOST_GEOMETRY_UTIL_HAS_INFINITE_COORDINATE_HPP

#include <type_traits>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/util/has_nan_coordinate.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

namespace boost { namespace geometry
{
    
#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

struct isinf
{
    template <typename T>
    static inline bool apply(T const& t)
    {
        return boost::math::isinf(t);
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

template <typename Point>
bool has_infinite_coordinate(Point const& point)
{
    return detail::has_coordinate_with_property
        <
            Point,
            detail::isinf,
            std::is_floating_point
                <
                    typename coordinate_type<Point>::type
                >::value
        >::apply(point);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_HAS_INFINITE_COORDINATE_HPP
