// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2015-2020.
// Modifications copyright (c) 2015-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLIP_LINESTRING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLIP_LINESTRING_HPP

#include <boost/range/begin.hpp>
#include <boost/range/empty.hpp>
#include <boost/range/end.hpp>

#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/algorithms/detail/overlay/append_no_duplicates.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/strategies/cartesian/point_in_point.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace intersection
{

/*!
    \brief Strategy: line clipping algorithm after Liang Barsky
    \ingroup overlay
    \details The Liang-Barsky line clipping algorithm clips a line with a clipping box.
    It is slightly adapted in the sense that it returns which points are clipped
    \tparam B input box type of clipping box
    \tparam P input/output point-type of segments to be clipped
    \note The algorithm is currently only implemented for 2D Cartesian points
    \note Though it is implemented in namespace strategy, and theoretically another
        strategy could be used, it is not (yet) updated to the general strategy concepts,
        and not (yet) splitted into a file in folder strategies
    \author Barend Gehrels, and the following recourses
    - A tutorial: http://www.skytopia.com/project/articles/compsci/clipping.html
    - a German applet (link broken): http://ls7-www.cs.uni-dortmund.de/students/projectgroups/acit/lineclip.shtml
*/
template<typename Box, typename Point>
class liang_barsky
{
private:
    typedef model::referring_segment<Point> segment_type;

    template <typename CoordinateType, typename CalcType>
    inline bool check_edge(CoordinateType const& p, CoordinateType const& q, CalcType& t1, CalcType& t2) const
    {
        bool visible = true;

        if(p < 0)
        {
            CalcType const r = static_cast<CalcType>(q) / p;
            if (r > t2)
                visible = false;
            else if (r > t1)
                t1 = r;
        }
        else if(p > 0)
        {
            CalcType const r = static_cast<CalcType>(q) / p;
            if (r < t1)
                visible = false;
            else if (r < t2)
                t2 = r;
        }
        else
        {
            if (q < 0)
                visible = false;
        }

        return visible;
    }

public:

// TODO: Temporary, this strategy should be moved, it is cartesian-only

    typedef strategy::within::cartesian_point_point equals_point_point_strategy_type;

    static inline equals_point_point_strategy_type get_equals_point_point_strategy()
    {
        return equals_point_point_strategy_type();
    }

    inline bool clip_segment(Box const& b, segment_type& s, bool& sp1_clipped, bool& sp2_clipped) const
    {
        typedef typename select_coordinate_type<Box, Point>::type coordinate_type;
        typedef typename select_most_precise<coordinate_type, double>::type calc_type;

        calc_type t1 = 0;
        calc_type t2 = 1;

        coordinate_type const dx = get<1, 0>(s) - get<0, 0>(s);
        coordinate_type const dy = get<1, 1>(s) - get<0, 1>(s);

        coordinate_type const p1 = -dx;
        coordinate_type const p2 = dx;
        coordinate_type const p3 = -dy;
        coordinate_type const p4 = dy;

        coordinate_type const q1 = get<0, 0>(s) - get<min_corner, 0>(b);
        coordinate_type const q2 = get<max_corner, 0>(b) - get<0, 0>(s);
        coordinate_type const q3 = get<0, 1>(s) - get<min_corner, 1>(b);
        coordinate_type const q4 = get<max_corner, 1>(b) - get<0, 1>(s);

        if (check_edge(p1, q1, t1, t2)      // left
            && check_edge(p2, q2, t1, t2)   // right
            && check_edge(p3, q3, t1, t2)   // bottom
            && check_edge(p4, q4, t1, t2))   // top
        {
            sp1_clipped = t1 > 0;
            sp2_clipped = t2 < 1;

            if (sp2_clipped)
            {
                set<1, 0>(s, get<0, 0>(s) + t2 * dx);
                set<1, 1>(s, get<0, 1>(s) + t2 * dy);
            }

            if(sp1_clipped)
            {
                set<0, 0>(s, get<0, 0>(s) + t1 * dx);
                set<0, 1>(s, get<0, 1>(s) + t1 * dy);
            }

            return true;
        }

        return false;
    }

    template<typename Linestring, typename OutputIterator>
    inline void apply(Linestring& line_out, OutputIterator out) const
    {
        if (!boost::empty(line_out))
        {
            *out = line_out;
            ++out;
            geometry::clear(line_out);
        }
    }
};


}} // namespace strategy::intersection


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{

/*!
    \brief Clips a linestring with a box
    \details A linestring is intersected (clipped) by the specified box
    and the resulting linestring, or pieces of linestrings, are sent to the specified output operator.
    \tparam OutputLinestring type of the output linestrings
    \tparam OutputIterator an output iterator which outputs linestrings
    \tparam Linestring linestring-type, for example a vector of points, matching the output-iterator type,
         the points should also match the input-iterator type
    \tparam Box box type
    \tparam Strategy strategy, a clipping strategy which should implement the methods "clip_segment" and "apply"
*/
template
<
    typename OutputLinestring,
    typename OutputIterator,
    typename Range,
    typename RobustPolicy,
    typename Box,
    typename Strategy
>
OutputIterator clip_range_with_box(Box const& b, Range const& range,
            RobustPolicy const&,
            OutputIterator out, Strategy const& strategy)
{
    if (boost::begin(range) == boost::end(range))
    {
        return out;
    }

    typedef typename point_type<OutputLinestring>::type point_type;

    OutputLinestring line_out;

    typedef typename boost::range_iterator<Range const>::type iterator_type;
    iterator_type vertex = boost::begin(range);
    for(iterator_type previous = vertex++;
            vertex != boost::end(range);
            ++previous, ++vertex)
    {
        point_type p1, p2;
        geometry::convert(*previous, p1);
        geometry::convert(*vertex, p2);

        // Clip the segment. Five situations:
        // 1. Segment is invisible, finish line if any (shouldn't occur)
        // 2. Segment is completely visible. Add (p1)-p2 to line
        // 3. Point 1 is invisible (clipped), point 2 is visible. Start new line from p1-p2...
        // 4. Point 1 is visible, point 2 is invisible (clipped). End the line with ...p2
        // 5. Point 1 and point 2 are both invisible (clipped). Start/finish an independant line p1-p2
        //
        // This results in:
        // a. if p1 is clipped, start new line
        // b. if segment is partly or completely visible, add the segment
        // c. if p2 is clipped, end the line

        bool c1 = false;
        bool c2 = false;
        model::referring_segment<point_type> s(p1, p2);

        if (!strategy.clip_segment(b, s, c1, c2))
        {
            strategy.apply(line_out, out);
        }
        else
        {
            // a. If necessary, finish the line and add a start a new one
            if (c1)
            {
                strategy.apply(line_out, out);
            }

            // b. Add p1 only if it is the first point, then add p2
            if (boost::empty(line_out))
            {
                detail::overlay::append_with_duplicates(line_out, p1);
            }
            detail::overlay::append_no_duplicates(line_out, p2,
                                                  strategy.get_equals_point_point_strategy());

            // c. If c2 is clipped, finish the line
            if (c2)
            {
                strategy.apply(line_out, out);
            }
        }

    }

    // Add last part
    strategy.apply(line_out, out);
    return out;
}

}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_CLIP_LINESTRING_HPP
