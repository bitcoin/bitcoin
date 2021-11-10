// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_SQUARE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_SQUARE_HPP

#include <cstddef>

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{

/*!
\brief Create a squared form buffer around a point
\ingroup strategies
\details This strategy can be used as PointStrategy for the buffer algorithm.
    It creates a square from each point, where the point lies in the center.
    It can be applied for points and multi_points, but also for a linestring (if it is degenerate,
    so consisting of only one point) and for polygons (if it is degenerate).
    This strategy is only applicable for Cartesian coordinate systems.

\qbk{
[heading Example]
[buffer_point_square]
[heading Output]
[$img/strategies/buffer_point_square.png]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
\* [link geometry.reference.strategies.strategy_buffer_point_circle point_circle]
\* [link geometry.reference.strategies.strategy_buffer_geographic_point_circle geographic_point_circle]
}
 */
class point_square
{
    template
    <
        typename Point,
        typename DistanceType,
        typename MultiplierType,
        typename OutputRange
    >
    inline void add_point(Point const& point,
                DistanceType const& distance,
                MultiplierType const& x,
                MultiplierType const& y,
                OutputRange& output_range) const
    {
        typename boost::range_value<OutputRange>::type p;
        set<0>(p, get<0>(point) + x * distance);
        set<1>(p, get<1>(point) + y * distance);
        output_range.push_back(p);
    }

    template
    <
        typename Point,
        typename DistanceType,
        typename OutputRange
    >
    inline void add_points(Point const& point,
                DistanceType const& distance,
                OutputRange& output_range) const
    {
        add_point(point, distance, -1.0, -1.0, output_range);
        add_point(point, distance, -1.0, +1.0, output_range);
        add_point(point, distance, +1.0, +1.0, output_range);
        add_point(point, distance, +1.0, -1.0, output_range);

        // Close it:
        output_range.push_back(output_range.front());
    }

public :

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Fills output_range with a square around point using distance_strategy
    template
    <
        typename Point,
        typename DistanceStrategy,
        typename OutputRange
    >
    inline void apply(Point const& point,
                DistanceStrategy const& distance_strategy,
                OutputRange& output_range) const
    {
        add_points(point, distance_strategy.apply(point, point,
                        strategy::buffer::buffer_side_left), output_range);
    }
#endif // DOXYGEN_SHOULD_SKIP_THIS

};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_SQUARE_HPP
