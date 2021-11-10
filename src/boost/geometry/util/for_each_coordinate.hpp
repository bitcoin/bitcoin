// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2020.
// Modifications copyright (c) 2020, Oracle and/or its affiliates.
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_FOR_EACH_COORDINATE_HPP
#define BOOST_GEOMETRY_UTIL_FOR_EACH_COORDINATE_HPP

#include <boost/concept/requires.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

template
<
    typename Point,
    int Dimension = 0,
    int DimensionCount = dimension<Point>::value
>
struct coordinates_scanner
{
    template <typename Op>
    static inline Op apply(Point& point, Op operation)
    {
        operation.template apply<Point, Dimension>(point);
        return coordinates_scanner
            <
                Point,
                Dimension + 1
            >::apply(point, operation);
    }
};

template <typename Point, int DimensionCount>
struct coordinates_scanner<Point, DimensionCount, DimensionCount>
{
    template <typename Op>
    static inline Op apply(Point& , Op operation)
    {
        return operation;
    }
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

template <typename Point, typename Op>
inline void for_each_coordinate(Point& point, Op operation)
{
    BOOST_CONCEPT_ASSERT( (concepts::Point<Point>) );

    detail::coordinates_scanner<Point>::apply(point, operation);
}

template <typename Point, typename Op>
inline Op for_each_coordinate(Point const& point, Op operation)
{
    BOOST_CONCEPT_ASSERT( (concepts::ConstPoint<Point>) );

    return detail::coordinates_scanner<Point const>::apply(point, operation);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_FOR_EACH_COORDINATE_HPP
