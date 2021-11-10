// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2015, 2018.
// Modifications copyright (c) 2015, 2018, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_CIRCLE_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_CIRCLE_HPP

#include <cstddef>

#include <boost/range/value_type.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

#include <boost/geometry/strategies/buffer.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{

/*!
\brief Create a circular buffer around a point
\ingroup strategies
\details This strategy can be used as PointStrategy for the buffer algorithm.
    It creates a circular buffer around a point. It can be applied
    for points and multi_points, but also for a linestring (if it is degenerate,
    so consisting of only one point) and for polygons (if it is degenerate).
    This strategy is only applicable for Cartesian coordinate systems.

\qbk{
[heading Example]
[buffer_point_circle]
[heading Output]
[$img/strategies/buffer_point_circle.png]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
\* [link geometry.reference.strategies.strategy_buffer_point_square point_square]
\* [link geometry.reference.strategies.strategy_buffer_geographic_point_circle geographic_point_circle]
}
 */
class point_circle
{
public :
    //! \brief Constructs the strategy
    //! \param count number of points for the created circle (if count
    //! is smaller than 3, count is internally set to 3)
    explicit point_circle(std::size_t count = 90)
        : m_count((count < 3u) ? 3u : count)
    {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Fills output_range with a circle around point using distance_strategy
    template
    <
        typename Point,
        typename OutputRange,
        typename DistanceStrategy
    >
    inline void apply(Point const& point,
                DistanceStrategy const& distance_strategy,
                OutputRange& output_range) const
    {
        typedef typename boost::range_value<OutputRange>::type output_point_type;

        typedef typename geometry::select_most_precise
            <
                typename geometry::select_most_precise
                    <
                        typename geometry::coordinate_type<Point>::type,
                        typename geometry::coordinate_type<output_point_type>::type
                    >::type,
                double
            >::type promoted_type;

        promoted_type const buffer_distance = distance_strategy.apply(point, point,
                        strategy::buffer::buffer_side_left);

        promoted_type const two_pi = geometry::math::two_pi<promoted_type>();

        promoted_type const diff = two_pi / promoted_type(m_count);
        promoted_type a = 0;

        for (std::size_t i = 0; i < m_count; i++, a -= diff)
        {
            output_point_type p;
            set<0>(p, get<0>(point) + buffer_distance * cos(a));
            set<1>(p, get<1>(point) + buffer_distance * sin(a));
            output_range.push_back(p);
        }

        // Close it:
        output_range.push_back(output_range.front());
    }
#endif // DOXYGEN_SHOULD_SKIP_THIS

private :
    std::size_t m_count;
};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_POINT_CIRCLE_HPP
