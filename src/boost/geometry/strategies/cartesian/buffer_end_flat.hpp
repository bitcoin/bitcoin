// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2012-2014 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2018.
// Modifications copyright (c) 2018, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_END_FLAT_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_END_FLAT_HPP

#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/strategies/tags.hpp>
#include <boost/geometry/strategies/side.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{


namespace strategy { namespace buffer
{


/*!
\brief Let the buffer create flat ends
\ingroup strategies
\details This strategy can be used as EndStrategy for the buffer algorithm.
    It creates a flat end for each linestring-end. It can be applied
    for (multi)linestrings. Also it is applicable for spikes in (multi)polygons.
    This strategy is only applicable for Cartesian coordinate systems.

\qbk{
[heading Example]
[buffer_end_flat]
[heading Output]
[$img/strategies/buffer_end_flat.png]
[heading See also]
\* [link geometry.reference.algorithms.buffer.buffer_7_with_strategies buffer (with strategies)]
\* [link geometry.reference.strategies.strategy_buffer_end_round end_round]
}
 */
class end_flat
{

public :

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    //! Fills output_range with a flat end
    template <typename Point, typename RangeOut, typename DistanceStrategy>
    inline void apply(Point const& penultimate_point,
                Point const& perp_left_point,
                Point const& ultimate_point,
                Point const& perp_right_point,
                buffer_side_selector side,
                DistanceStrategy const& distance,
                RangeOut& range_out) const
    {
        typedef typename coordinate_type<Point>::type coordinate_type;

        typedef typename geometry::select_most_precise
        <
            coordinate_type,
            double
        >::type promoted_type;

        promoted_type const dist_left = distance.apply(penultimate_point, ultimate_point, buffer_side_left);
        promoted_type const dist_right = distance.apply(penultimate_point, ultimate_point, buffer_side_right);

        bool reversed = (side == buffer_side_left && dist_right < 0 && -dist_right > dist_left)
                    || (side == buffer_side_right && dist_left < 0 && -dist_left > dist_right)
                    ;
        if (reversed)
        {
            range_out.push_back(perp_right_point);
            range_out.push_back(perp_left_point);
        }
        else
        {
            range_out.push_back(perp_left_point);
            range_out.push_back(perp_right_point);
        }
        // Don't add the ultimate_point (endpoint of the linestring).
        // The buffer might be generated completely at one side.
        // In other cases it does no harm but is further useless
    }

    template <typename NumericType>
    static inline NumericType max_distance(NumericType const& distance)
    {
        return distance;
    }

    //! Returns the piece_type (flat end)
    static inline piece_type get_piece_type()
    {
        return buffered_flat_end;
    }
#endif // DOXYGEN_SHOULD_SKIP_THIS
};


}} // namespace strategy::buffer

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_BUFFER_END_FLAT_HPP
