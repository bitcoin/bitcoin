// Boost.Geometry (aka GGL, Generic Geometry Library)
// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP

#include <cstddef>

#include <boost/math/special_functions/fpclassify.hpp>

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/strategies/side.hpp>



namespace boost { namespace geometry
{

namespace strategy { namespace buffer
{



/*!
\brief Let the buffer use straight sides along segments (the default)
\ingroup strategies
\details This strategy can be used as SideStrategy for the buffer algorithm.
    It is currently the only provided strategy for this purpose

\qbk{
[heading Example]
See the examples for other buffer strategies\, for example
[link geometry.reference.strategies.strategy_buffer_join_round join_round]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
}
 */
class side_straight
{
public :
#ifndef DOXYGEN_SHOULD_SKIP_THIS

    // Returns true if the buffer distance is always the same
    static inline bool equidistant()
    {
        return true;
    }

    template
    <
        typename Point,
        typename OutputRange,
        typename DistanceStrategy
    >
    static inline result_code apply(
                Point const& input_p1, Point const& input_p2,
                buffer_side_selector side,
                DistanceStrategy const& distance,
                OutputRange& output_range)
    {
        typedef typename coordinate_type<Point>::type coordinate_type;
        typedef typename geometry::select_most_precise
        <
            coordinate_type,
            double
        >::type promoted_type;

        // Generate a block along (left or right of) the segment

        // Simulate a vector d (dx,dy)
        coordinate_type const dx = get<0>(input_p2) - get<0>(input_p1);
        coordinate_type const dy = get<1>(input_p2) - get<1>(input_p1);

        // For normalization [0,1] (=dot product d.d, sqrt)
        promoted_type const length = geometry::math::sqrt(dx * dx + dy * dy);

        if (! boost::math::isfinite(length))
        {
            // In case of coordinates differences of e.g. 1e300, length
            // will overflow and we should not generate output
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
            std::cout << "Error in length calculation for points "
                << geometry::wkt(input_p1) << " " << geometry::wkt(input_p2)
                << " length: " << length << std::endl;
#endif
            return result_error_numerical;
        }

        if (geometry::math::equals(length, 0))
        {
            // Coordinates are simplified and therefore most often not equal.
            // But if simplify is skipped, or for lines with two
            // equal points, length is 0 and we cannot generate output.
            return result_no_output;
        }

        promoted_type const d = distance.apply(input_p1, input_p2, side);

        // Generate the normalized perpendicular p, to the left (ccw)
        promoted_type const px = -dy / length;
        promoted_type const py = dx / length;

        if (geometry::math::equals(px, 0)
            && geometry::math::equals(py, 0))
        {
            // This basically should not occur - because of the checks above.
            // There are no unit tests triggering this condition
#ifdef BOOST_GEOMETRY_DEBUG_BUFFER_WARN
            std::cout << "Error in perpendicular calculation for points "
                << geometry::wkt(input_p1) << " " << geometry::wkt(input_p2)
                << " length: " << length
                << " distance: " << d
                << std::endl;
#endif
            return result_no_output;
        }

        output_range.resize(2);

        set<0>(output_range.front(), get<0>(input_p1) + px * d);
        set<1>(output_range.front(), get<1>(input_p1) + py * d);
        set<0>(output_range.back(), get<0>(input_p2) + px * d);
        set<1>(output_range.back(), get<1>(input_p2) + py * d);

        return result_normal;
    }
#endif // DOXYGEN_SHOULD_SKIP_THIS
};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_SIDE_STRAIGHT_HPP
