// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP

#include <cstddef>
#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/detail/overlay/append_no_duplicates.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/relate/turns.hpp>
#include <boost/geometry/algorithms/detail/tupled_output.hpp>
#include <boost/geometry/core/static_assert.hpp>
#include <boost/geometry/util/condition.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{

namespace following
{

template <typename Turn, typename Operation>
inline bool is_entering(Turn const& /* TODO remove this parameter */, Operation const& op)
{
    // (Blocked means: blocked for polygon/polygon intersection, because
    // they are reversed. But for polygon/line it is similar to continue)
    return op.operation == operation_intersection
        || op.operation == operation_continue
        || op.operation == operation_blocked
        ;
}

template
<
    typename Turn,
    typename Operation,
    typename LineString,
    typename Polygon,
    typename Strategy
>
inline bool last_covered_by(Turn const& /*turn*/, Operation const& op,
                LineString const& linestring, Polygon const& polygon,
                Strategy const& strategy)
{
    return geometry::covered_by(range::at(linestring, op.seg_id.segment_index), polygon, strategy);
}


template
<
    typename Turn,
    typename Operation,
    typename LineString,
    typename Polygon,
    typename Strategy
>
inline bool is_leaving(Turn const& turn, Operation const& op,
                bool entered, bool first,
                LineString const& linestring, Polygon const& polygon,
                Strategy const& strategy)
{
    if (op.operation == operation_union)
    {
        return entered
            || turn.method == method_crosses
            || (first
                && op.position != position_front
                && last_covered_by(turn, op, linestring, polygon, strategy))
            ;
    }
    return false;
}


template
<
    typename Turn,
    typename Operation,
    typename LineString,
    typename Polygon,
    typename Strategy
>
inline bool is_staying_inside(Turn const& turn, Operation const& op,
                bool entered, bool first,
                LineString const& linestring, Polygon const& polygon,
                Strategy const& strategy)
{
    if (turn.method == method_crosses)
    {
        // The normal case, this is completely covered with entering/leaving
        // so stay out of this time consuming "covered_by"
        return false;
    }

    if (is_entering(turn, op))
    {
        return entered || (first && last_covered_by(turn, op, linestring, polygon, strategy));
    }

    return false;
}

template
<
    typename Turn,
    typename Operation,
    typename Linestring,
    typename Polygon,
    typename Strategy
>
inline bool was_entered(Turn const& turn, Operation const& op, bool first,
                Linestring const& linestring, Polygon const& polygon,
                Strategy const& strategy)
{
    if (first && (turn.method == method_collinear || turn.method == method_equal))
    {
        return last_covered_by(turn, op, linestring, polygon, strategy);
    }
    return false;
}

template
<
    typename Turn,
    typename Operation
>
inline bool is_touching(Turn const& turn, Operation const& op,
                        bool entered)
{
    return (op.operation == operation_union || op.operation == operation_blocked)
        && (turn.method == method_touch || turn.method == method_touch_interior)
        && !entered
        && !op.is_collinear;
}


template
<
    typename GeometryOut,
    typename Tag = typename geometry::tag<GeometryOut>::type
>
struct add_isolated_point
{};

template <typename LineStringOut>
struct add_isolated_point<LineStringOut, linestring_tag>
{
    template <typename Point, typename OutputIterator>
    static inline void apply(Point const& point, OutputIterator& out)
    {
        LineStringOut isolated_point_ls;
        geometry::append(isolated_point_ls, point);

#ifndef BOOST_GEOMETRY_ALLOW_ONE_POINT_LINESTRINGS
        geometry::append(isolated_point_ls, point);
#endif // BOOST_GEOMETRY_ALLOW_ONE_POINT_LINESTRINGS

        *out++ = isolated_point_ls;
    }
};

template <typename PointOut>
struct add_isolated_point<PointOut, point_tag>
{
    template <typename Point, typename OutputIterator>
    static inline void apply(Point const& point, OutputIterator& out)
    {
        PointOut isolated_point;

        geometry::detail::conversion::convert_point_to_point(point, isolated_point);

        *out++ = isolated_point;
    }
};


// Template specialization structure to call the right actions for the right type
template <overlay_type OverlayType, bool RemoveSpikes = true>
struct action_selector
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "If you get here the overlay type is not intersection or difference.",
        std::integral_constant<overlay_type, OverlayType>);
};

// Specialization for intersection, containing the implementation
template <bool RemoveSpikes>
struct action_selector<overlay_intersection, RemoveSpikes>
{
    template
    <
        typename OutputIterator,
        typename LineStringOut,
        typename LineString,
        typename Point,
        typename Operation,
        typename Strategy,
        typename RobustPolicy
    >
    static inline void enter(LineStringOut& current_piece,
                LineString const& ,
                segment_identifier& segment_id,
                signed_size_type , Point const& point,
                Operation const& operation,
                Strategy const& strategy,
                RobustPolicy const& ,
                OutputIterator& )
    {
        // On enter, append the intersection point and remember starting point
        // TODO: we don't check on spikes for linestrings (?). Consider this.
        detail::overlay::append_no_duplicates(current_piece, point, strategy);
        segment_id = operation.seg_id;
    }

    template
    <
        typename OutputIterator,
        typename LineStringOut,
        typename LineString,
        typename Point,
        typename Operation,
        typename Strategy,
        typename RobustPolicy
    >
    static inline void leave(LineStringOut& current_piece,
                LineString const& linestring,
                segment_identifier& segment_id,
                signed_size_type index, Point const& point,
                Operation const& ,
                Strategy const& strategy,
                RobustPolicy const& robust_policy,
                OutputIterator& out)
    {
        // On leave, copy all segments from starting point, append the intersection point
        // and add the output piece
        detail::copy_segments::copy_segments_linestring
            <
                false, RemoveSpikes
            >::apply(linestring, segment_id, index, strategy, robust_policy, current_piece);
        detail::overlay::append_no_duplicates(current_piece, point, strategy);
        if (::boost::size(current_piece) > 1)
        {
            *out++ = current_piece;
        }

        geometry::clear(current_piece);
    }

    template
    <
        typename LineStringOrPointOut,
        typename Point,
        typename OutputIterator
    >
    static inline void isolated_point(Point const& point,
                                      OutputIterator& out)
    {
        add_isolated_point<LineStringOrPointOut>::apply(point, out);
    }

    static inline bool is_entered(bool entered)
    {
        return entered;
    }

    static inline bool included(int inside_value)
    {
        return inside_value >= 0; // covered_by
    }

};

// Specialization for difference, which reverses these actions
template <bool RemoveSpikes>
struct action_selector<overlay_difference, RemoveSpikes>
{
    typedef action_selector<overlay_intersection, RemoveSpikes> normal_action;

    template
    <
        typename OutputIterator,
        typename LineStringOut,
        typename LineString,
        typename Point,
        typename Operation,
        typename Strategy,
        typename RobustPolicy
    >
    static inline void enter(LineStringOut& current_piece,
                LineString const& linestring,
                segment_identifier& segment_id,
                signed_size_type index, Point const& point,
                Operation const& operation,
                Strategy const& strategy,
                RobustPolicy const& robust_policy,
                OutputIterator& out)
    {
        normal_action::leave(current_piece, linestring, segment_id, index,
                    point, operation, strategy, robust_policy, out);
    }

    template
    <
        typename OutputIterator,
        typename LineStringOut,
        typename LineString,
        typename Point,
        typename Operation,
        typename Strategy,
        typename RobustPolicy
    >
    static inline void leave(LineStringOut& current_piece,
                LineString const& linestring,
                segment_identifier& segment_id,
                signed_size_type index, Point const& point,
                Operation const& operation,
                Strategy const& strategy,
                RobustPolicy const& robust_policy,
                OutputIterator& out)
    {
        normal_action::enter(current_piece, linestring, segment_id, index,
                    point, operation, strategy, robust_policy, out);
    }

    template
    <
        typename LineStringOrPointOut,
        typename Point,
        typename OutputIterator
    >
    static inline void isolated_point(Point const&,
                                      OutputIterator const&)
    {
    }

    static inline bool is_entered(bool entered)
    {
        return ! normal_action::is_entered(entered);
    }

    static inline bool included(int inside_value)
    {
        return ! normal_action::included(inside_value);
    }

};

} // namespace following

/*!
\brief Follows a linestring from intersection point to intersection point, outputting which
    is inside, or outside, a ring or polygon
\ingroup overlay
 */
template
<
    typename GeometryOut,
    typename LineString,
    typename Polygon,
    overlay_type OverlayType,
    bool RemoveSpikes,
    bool FollowIsolatedPoints
>
class follow
{
    typedef geometry::detail::output_geometry_access
        <
            GeometryOut, linestring_tag, linestring_tag
        > linear;
    typedef geometry::detail::output_geometry_access
        <
            GeometryOut, point_tag, linestring_tag
        > pointlike;

public :

    static inline bool included(int inside_value)
    {
        return following::action_selector
            <
                OverlayType, RemoveSpikes
            >::included(inside_value);
    }

    template
    <
        typename Turns,
        typename OutputIterator,
        typename RobustPolicy,
        typename Strategy
    >
    static inline OutputIterator apply(LineString const& linestring, Polygon const& polygon,
                detail::overlay::operation_type ,  // TODO: this parameter might be redundant
                Turns& turns,
                RobustPolicy const& robust_policy,
                OutputIterator out,
                Strategy const& strategy)
    {
        typedef typename boost::range_iterator<Turns>::type turn_iterator;
        typedef typename boost::range_value<Turns>::type turn_type;
        typedef typename boost::range_iterator
            <
                typename turn_type::container_type
            >::type turn_operation_iterator_type;

        typedef following::action_selector<OverlayType, RemoveSpikes> action;

        // Sort intersection points on segments-along-linestring, and distance
        // (like in enrich is done for poly/poly)
        // sort turns by Linear seg_id, then by fraction, then
        // for same ring id: x, u, i, c
        // for different ring id: c, i, u, x
        typedef relate::turns::less
            <
                0, relate::turns::less_op_linear_areal_single<0>,
                typename Strategy::cs_tag
            > turn_less;
        std::sort(boost::begin(turns), boost::end(turns), turn_less());

        typename linear::type current_piece;
        geometry::segment_identifier current_segment_id(0, -1, -1, -1);

        // Iterate through all intersection points (they are ordered along the line)
        bool entered = false;
        bool first = true;
        for (turn_iterator it = boost::begin(turns); it != boost::end(turns); ++it)
        {
            turn_operation_iterator_type iit = boost::begin(it->operations);

            if (following::was_entered(*it, *iit, first, linestring, polygon, strategy))
            {
                debug_traverse(*it, *iit, "-> Was entered");
                entered = true;
            }

            if (following::is_staying_inside(*it, *iit, entered, first, linestring, polygon, strategy))
            {
                debug_traverse(*it, *iit, "-> Staying inside");

                entered = true;
            }
            else if (following::is_entering(*it, *iit))
            {
                debug_traverse(*it, *iit, "-> Entering");

                entered = true;
                action::enter(current_piece, linestring, current_segment_id,
                    iit->seg_id.segment_index, it->point, *iit,
                    strategy, robust_policy,
                    linear::get(out));
            }
            else if (following::is_leaving(*it, *iit, entered, first, linestring, polygon, strategy))
            {
                debug_traverse(*it, *iit, "-> Leaving");

                entered = false;
                action::leave(current_piece, linestring, current_segment_id,
                    iit->seg_id.segment_index, it->point, *iit,
                    strategy, robust_policy,
                    linear::get(out));
            }
            else if (BOOST_GEOMETRY_CONDITION(FollowIsolatedPoints)
                  && following::is_touching(*it, *iit, entered))
            {
                debug_traverse(*it, *iit, "-> Isolated point");

                action::template isolated_point
                    <
                        typename pointlike::type
                    >(it->point, pointlike::get(out));
            }

            first = false;
        }

        if (action::is_entered(entered))
        {
            detail::copy_segments::copy_segments_linestring
                <
                    false, RemoveSpikes
                >::apply(linestring,
                         current_segment_id,
                         static_cast<signed_size_type>(boost::size(linestring) - 1),
                         strategy, robust_policy,
                         current_piece);
        }

        // Output the last one, if applicable
        std::size_t current_piece_size = ::boost::size(current_piece);
        if (current_piece_size > 1)
        {
            *linear::get(out)++ = current_piece;
        }
        else if (BOOST_GEOMETRY_CONDITION(FollowIsolatedPoints)
              && current_piece_size == 1)
        {
            action::template isolated_point
                <
                    typename pointlike::type
                >(range::front(current_piece), pointlike::get(out));
        }

        return out;
    }

};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_FOLLOW_HPP
