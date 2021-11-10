// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_HPP

#include <algorithm>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/policies/compare.hpp>
#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
#include <iostream>
#include <boost/geometry/io/wkt/wkt.hpp>
#endif


namespace boost { namespace geometry
{


namespace strategy { namespace buffer
{

/*!
\brief Let the buffer create rounded corners
\ingroup strategies
\details This strategy can be used as JoinStrategy for the buffer algorithm.
    It creates a rounded corners around each convex vertex. It can be applied
    for (multi)linestrings and (multi)polygons.
    This strategy is only applicable for Cartesian coordinate systems.

\qbk{
[heading Example]
[buffer_join_round]
[heading Output]
[$img/strategies/buffer_join_round.png]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
\* [link geometry.reference.strategies.strategy_buffer_join_miter join_miter]
}
 */
class join_round
{
public :

    //! \brief Constructs the strategy
    //! \param points_per_circle points which would be used for a full circle
    explicit inline join_round(std::size_t points_per_circle = 90)
        : m_points_per_circle(points_per_circle)
    {}

private :
    template
    <
        typename PromotedType,
        typename Point,
        typename DistanceType,
        typename RangeOut
    >
    inline void generate_points(Point const& vertex,
                Point const& perp1, Point const& perp2,
                DistanceType const& buffer_distance,
                RangeOut& range_out) const
    {
        PromotedType const dx1 = get<0>(perp1) - get<0>(vertex);
        PromotedType const dy1 = get<1>(perp1) - get<1>(vertex);
        PromotedType const dx2 = get<0>(perp2) - get<0>(vertex);
        PromotedType const dy2 = get<1>(perp2) - get<1>(vertex);

        PromotedType const two_pi = geometry::math::two_pi<PromotedType>();

        PromotedType const angle1 = atan2(dy1, dx1);
        PromotedType angle2 = atan2(dy2, dx2);
        while (angle2 > angle1)
        {
            angle2 -= two_pi;
        }
        PromotedType const angle_diff = angle1 - angle2;

        // Divide the angle into an integer amount of steps to make it
        // visually correct also for a low number of points / circle

        // If a full circle is divided into 3 parts (e.g. angle is 125),
        // the one point in between must still be generated
        // The calculation below:
        // - generates 1 point  in between for an angle of 125 based on 3 points
        // - generates 0 points in between for an angle of 90  based on 4 points

        std::size_t const n = (std::max)(static_cast<std::size_t>(
            ceil(m_points_per_circle * angle_diff / two_pi)), std::size_t(1));

        PromotedType const diff = angle_diff / static_cast<PromotedType>(n);
        PromotedType a = angle1 - diff;

        // Walk to n - 1 to avoid generating the last point
        for (std::size_t i = 0; i < n - 1; i++, a -= diff)
        {
            Point p;
            set<0>(p, get<0>(vertex) + buffer_distance * cos(a));
            set<1>(p, get<1>(vertex) + buffer_distance * sin(a));
            range_out.push_back(p);
        }
    }

public :


#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Fills output_range with a rounded shape around a vertex
    template <typename Point, typename DistanceType, typename RangeOut>
    inline bool apply(Point const& ip, Point const& vertex,
                Point const& perp1, Point const& perp2,
                DistanceType const& buffer_distance,
                RangeOut& range_out) const
    {
        typedef typename coordinate_type<Point>::type coordinate_type;
        typedef typename boost::range_value<RangeOut>::type output_point_type;

        typedef typename geometry::select_most_precise
            <
                typename geometry::select_most_precise
                    <
                        coordinate_type,
                        typename geometry::coordinate_type<output_point_type>::type
                    >::type,
                double
            >::type promoted_type;

        geometry::equal_to<Point> equals;
        if (equals(perp1, perp2))
        {
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
            std::cout << "Corner for equal points " << geometry::wkt(ip) << " " << geometry::wkt(perp1) << std::endl;
#endif
            return false;
        }

        // Generate 'vectors'
        coordinate_type vix = (get<0>(ip) - get<0>(vertex));
        coordinate_type viy = (get<1>(ip) - get<1>(vertex));

        promoted_type length_i = geometry::math::sqrt(vix * vix + viy * viy);
        DistanceType const bd = geometry::math::abs(buffer_distance);
        promoted_type prop = bd / length_i;

        Point bp;
        set<0>(bp, get<0>(vertex) + vix * prop);
        set<1>(bp, get<1>(vertex) + viy * prop);

        range_out.push_back(perp1);
        generate_points<promoted_type>(vertex, perp1, perp2, bd, range_out);
        range_out.push_back(perp2);
        return true;
    }

    template <typename NumericType>
    static inline NumericType max_distance(NumericType const& distance)
    {
        return distance;
    }

#endif // DOXYGEN_SHOULD_SKIP_THIS

private :
    std::size_t m_points_per_circle;
};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_JOIN_ROUND_HPP
