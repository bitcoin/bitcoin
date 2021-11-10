// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2016, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ARITHMETIC_NORMALIZE_HPP
#define BOOST_GEOMETRY_ARITHMETIC_NORMALIZE_HPP


#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/arithmetic/dot_product.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template <typename Point>
inline typename coordinate_type<Point>::type vec_length_sqr(Point const& pt)
{
    return dot_product(pt, pt);
}

template <typename Point>
inline typename coordinate_type<Point>::type vec_length(Point const& pt)
{
    // NOTE: hypot() could be used instead of sqrt()
    return math::sqrt(dot_product(pt, pt));
}

template <typename Point>
inline bool vec_normalize(Point & pt, typename coordinate_type<Point>::type & len)
{
    typedef typename coordinate_type<Point>::type coord_t;

    coord_t const c0 = 0;
    len = vec_length(pt);
    
    if (math::equals(len, c0))
    {
        return false;
    }

    divide_value(pt, len);
    return true;
}

template <typename Point>
inline bool vec_normalize(Point & pt)
{
    typedef typename coordinate_type<Point>::type coord_t;
    coord_t len;
    return vec_normalize(pt, len);
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ARITHMETIC_NORMALIZE_HPP
