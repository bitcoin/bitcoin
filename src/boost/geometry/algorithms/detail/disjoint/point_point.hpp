// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2015 Adam Wulkiewicz, Lodz, Poland

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_POINT_POINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_POINT_POINT_HPP

#include <cstddef>
#include <type_traits>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/dispatch/disjoint.hpp>

#include <boost/geometry/strategies/detail.hpp>

// For backward compatibility
#include <boost/geometry/strategies/disjoint.hpp>
#include <boost/geometry/strategies/cartesian/point_in_point.hpp>
#include <boost/geometry/strategies/spherical/point_in_point.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


/*!
    \brief Internal utility function to detect of points are disjoint
    \note To avoid circular references
 */
template
<
    typename Point1, typename Point2, typename Strategy,
    std::enable_if_t<strategies::detail::is_umbrella_strategy<Strategy>::value, int> = 0
>
inline bool disjoint_point_point(Point1 const& point1, Point2 const& point2,
                                 Strategy const& strategy)
{
    typedef decltype(strategy.relate(point1, point2)) strategy_type;
    // ! within(point1, point2)
    return ! strategy_type::apply(point1, point2);
}

template
<
    typename Point1, typename Point2, typename Strategy,
    std::enable_if_t<! strategies::detail::is_umbrella_strategy<Strategy>::value, int> = 0
>
inline bool disjoint_point_point(Point1 const& point1, Point2 const& point2,
                                 Strategy const& )
{
    // ! within(point1, point2)
    return ! Strategy::apply(point1, point2);
}


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Point1, typename Point2, std::size_t DimensionCount>
struct disjoint<Point1, Point2, DimensionCount, point_tag, point_tag, false>
{
    template <typename Strategy>
    static inline bool apply(Point1 const& point1, Point2 const& point2,
                             Strategy const& strategy)
    {
        typedef decltype(strategy.relate(point1, point2)) strategy_type;
        // ! within(point1, point2)
        return ! strategy_type::apply(point1, point2);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_POINT_POINT_HPP
