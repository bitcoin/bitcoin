// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014-2020.
// Modifications copyright (c) 2014-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP


#include <cstddef>
#include <deque>
#include <type_traits>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/check_iterator_range.hpp>
#include <boost/geometry/algorithms/detail/point_on_border.hpp>
#include <boost/geometry/algorithms/detail/overlay/clip_linestring.hpp>
#include <boost/geometry/algorithms/detail/overlay/follow.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/linear_linear.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_areal.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_linear.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_pointlike.hpp>
#include <boost/geometry/algorithms/detail/overlay/range_in_geometry.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_as_subrange.hpp>

#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/core/static_assert.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/policies/robustness/rescale_policy_tags.hpp>
#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>
#include <boost/geometry/policies/robustness/get_rescale_policy.hpp>

#include <boost/geometry/strategies/default_strategy.hpp>
#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/relate/services.hpp>

#include <boost/geometry/views/segment_view.hpp>
#include <boost/geometry/views/detail/boundary_view.hpp>

#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#include <boost/geometry/io/wkt/wkt.hpp>
#endif

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{

template <typename PointOut>
struct intersection_segment_segment_point
{
    template
    <
        typename Segment1, typename Segment2,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Segment1 const& segment1,
            Segment2 const& segment2,
            RobustPolicy const& ,
            OutputIterator out,
            Strategy const& strategy)
    {
        // Make sure this is only called with no rescaling
        BOOST_STATIC_ASSERT((std::is_same
           <
               no_rescale_policy_tag,
               typename rescale_policy_type<RobustPolicy>::type
           >::value));

        typedef typename point_type<PointOut>::type point_type;

        // Get the intersection point (or two points)
        typedef segment_intersection_points<point_type> intersection_return_type;

        typedef policies::relate::segments_intersection_points
            <
                intersection_return_type
            > policy_type;

        detail::segment_as_subrange<Segment1> sub_range1(segment1);
        detail::segment_as_subrange<Segment2> sub_range2(segment2);

        intersection_return_type
            is = strategy.relate().apply(sub_range1, sub_range2, policy_type());

        for (std::size_t i = 0; i < is.count; i++)
        {
            PointOut p;
            geometry::convert(is.intersections[i], p);
            *out++ = p;
        }
        return out;
    }
};

template <typename PointOut>
struct intersection_linestring_linestring_point
{
    template
    <
        typename Linestring1, typename Linestring2,
        typename RobustPolicy,
        typename OutputIterator,
        typename Strategy
    >
    static inline OutputIterator apply(Linestring1 const& linestring1,
            Linestring2 const& linestring2,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        // Make sure this is only called with no rescaling
        BOOST_STATIC_ASSERT((std::is_same
           <
               no_rescale_policy_tag,
               typename rescale_policy_type<RobustPolicy>::type
           >::value));

        typedef detail::overlay::turn_info<PointOut> turn_info;
        std::deque<turn_info> turns;

        geometry::get_intersection_points(linestring1, linestring2,
                                          robust_policy, turns, strategy);

        for (typename boost::range_iterator<std::deque<turn_info> const>::type
            it = boost::begin(turns); it != boost::end(turns); ++it)
        {
            PointOut p;
            geometry::convert(it->point, p);
            *out++ = p;
        }
        return out;
    }
};

/*!
\brief Version of linestring with an areal feature (polygon or multipolygon)
*/
template
<
    bool ReverseAreal,
    typename GeometryOut,
    overlay_type OverlayType,
    bool FollowIsolatedPoints
>
struct intersection_of_linestring_with_areal
{
#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
    template <typename Turn, typename Operation>
    static inline void debug_follow(Turn const& turn, Operation op,
                                    int index)
    {
        std::cout << index
                  << " at " << op.seg_id
                  << " meth: " << method_char(turn.method)
                  << " op: " << operation_char(op.operation)
                  << " vis: " << visited_char(op.visited)
                  << " of:  " << operation_char(turn.operations[0].operation)
                  << operation_char(turn.operations[1].operation)
                  << " " << geometry::wkt(turn.point)
                  << std::endl;
    }

    template <typename Turn>
    static inline void debug_turn(Turn const& t, bool non_crossing)
    {
        std::cout << "checking turn @"
                  << geometry::wkt(t.point)
                  << "; " << method_char(t.method)
                  << ":" << operation_char(t.operations[0].operation)
                  << "/" << operation_char(t.operations[1].operation)
                  << "; non-crossing? "
                  << std::boolalpha << non_crossing << std::noboolalpha
                  << std::endl;
    }
#endif

    template <typename Linestring, typename Areal, typename Strategy, typename Turns>
    static inline bool simple_turns_analysis(Linestring const& linestring,
                                             Areal const& areal,
                                             Strategy const& strategy,
                                             Turns const& turns,
                                             int & inside_value)
    {
        using namespace overlay;

        bool found_continue = false;
        bool found_intersection = false;
        bool found_union = false;
        bool found_front = false;

        for (typename Turns::const_iterator it = turns.begin();
                it != turns.end(); ++it)
        {
            method_type const method = it->method;
            operation_type const op = it->operations[0].operation;

            if (method == method_crosses)
            {
                return false;
            }
            else if (op == operation_intersection)
            {
                found_intersection = true;
            }
            else if (op == operation_union)
            {
                found_union = true;
            }
            else if (op == operation_continue)
            {
                found_continue = true;
            }

            if ((found_intersection || found_continue) && found_union)
            {
                return false;
            }

            if (it->operations[0].position == position_front)
            {
                found_front = true;
            }
        }

        if (found_front)
        {
            if (found_intersection)
            {
                inside_value = 1; // inside
            }
            else if (found_union)
            {
                inside_value = -1; // outside
            }
            else // continue and blocked
            {
                inside_value = 0;
            }
            return true;
        }

        // if needed analyse points of a linestring
        // NOTE: range_in_geometry checks points of a linestring
        // until a point inside/outside areal is found
        // TODO: Could be replaced with point_in_geometry() because found_front is false
        inside_value = range_in_geometry(linestring, areal, strategy);

        if ( (found_intersection && inside_value == -1) // going in from outside
          || (found_continue && inside_value == -1) // going on boundary from outside
          || (found_union && inside_value == 1) ) // going out from inside
        {
            return false;
        }

        return true;
    }

    template
    <
        typename LineString, typename Areal,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(LineString const& linestring, Areal const& areal,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        // Make sure this is only called with no rescaling
        BOOST_STATIC_ASSERT((std::is_same
           <
               no_rescale_policy_tag,
               typename rescale_policy_type<RobustPolicy>::type
           >::value));

        if (boost::size(linestring) == 0)
        {
            return out;
        }

        typedef detail::overlay::follow
                <
                    GeometryOut,
                    LineString,
                    Areal,
                    OverlayType,
                    false, // do not remove spikes for linear geometries
                    FollowIsolatedPoints
                > follower;

        typedef typename geometry::detail::output_geometry_access
            <
                GeometryOut, linestring_tag, linestring_tag
            > linear;

        typedef typename point_type
            <
                typename linear::type
            >::type point_type;

        typedef geometry::segment_ratio
            <
                typename coordinate_type<point_type>::type
            > ratio_type;

        typedef detail::overlay::turn_info
            <
                point_type,
                ratio_type,
                detail::overlay::turn_operation_linear
                    <
                        point_type,
                        ratio_type
                    >
            > turn_info;

        std::deque<turn_info> turns;

        detail::get_turns::no_interrupt_policy policy;

        typedef detail::overlay::get_turn_info_linear_areal
            <
                detail::overlay::assign_null_policy
            > turn_policy;

        dispatch::get_turns
            <
                typename geometry::tag<LineString>::type,
                typename geometry::tag<Areal>::type,
                LineString,
                Areal,
                false,
                (OverlayType == overlay_intersection ? ReverseAreal : !ReverseAreal),
                turn_policy
            >::apply(0, linestring, 1, areal,
                     strategy, robust_policy,
                     turns, policy);

        int inside_value = 0;
        if (simple_turns_analysis(linestring, areal, strategy, turns, inside_value))
        {
            // No crossing the boundary, it is either
            // inside (interior + borders)
            // or outside (exterior + borders)
            // or on boundary

            // add linestring to the output if conditions are met
            if (follower::included(inside_value))
            {
                typename linear::type copy;
                geometry::convert(linestring, copy);
                *linear::get(out)++ = copy;
            }

            return out;
        }
        
#if defined(BOOST_GEOMETRY_DEBUG_FOLLOW)
        int index = 0;
        for(typename std::deque<turn_info>::const_iterator
            it = turns.begin(); it != turns.end(); ++it)
        {
            debug_follow(*it, it->operations[0], index++);
        }
#endif

        return follower::apply
                (
                    linestring, areal,
                    geometry::detail::overlay::operation_intersection,
                    turns, robust_policy, out, strategy
                );
    }
};


template <typename Turns, typename OutputIterator>
inline OutputIterator intersection_output_turn_points(Turns const& turns,
                                                      OutputIterator out)
{
    for (typename Turns::const_iterator
            it = turns.begin(); it != turns.end(); ++it)
    {
        *out++ = it->point;
    }

    return out;
}

template <typename PointOut>
struct intersection_areal_areal_point
{
    template
    <
        typename Geometry1, typename Geometry2,
        typename RobustPolicy,
        typename OutputIterator,
        typename Strategy
    >
    static inline OutputIterator apply(Geometry1 const& geometry1,
                                       Geometry2 const& geometry2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        typedef detail::overlay::turn_info
            <
                PointOut,
                typename segment_ratio_type<PointOut, RobustPolicy>::type
            > turn_info;
        std::vector<turn_info> turns;

        detail::get_turns::no_interrupt_policy policy;

        geometry::get_turns
            <
                false, false, detail::overlay::assign_null_policy
            >(geometry1, geometry2, strategy, robust_policy, turns, policy);

        return intersection_output_turn_points(turns, out);
    }
};

template <typename PointOut>
struct intersection_linear_areal_point
{
    template
    <
        typename Geometry1, typename Geometry2,
        typename RobustPolicy,
        typename OutputIterator,
        typename Strategy
    >
    static inline OutputIterator apply(Geometry1 const& geometry1,
                                       Geometry2 const& geometry2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        // Make sure this is only called with no rescaling
        BOOST_STATIC_ASSERT((std::is_same
           <
               no_rescale_policy_tag,
               typename rescale_policy_type<RobustPolicy>::type
           >::value));

        typedef geometry::segment_ratio<typename geometry::coordinate_type<PointOut>::type> ratio_type;

        typedef detail::overlay::turn_info
            <
                PointOut,
                ratio_type,
                detail::overlay::turn_operation_linear
                    <
                        PointOut,
                        ratio_type
                    >
            > turn_info;

        typedef detail::overlay::get_turn_info_linear_areal
            <
                detail::overlay::assign_null_policy
            > turn_policy;

        std::vector<turn_info> turns;

        detail::get_turns::no_interrupt_policy interrupt_policy;

        dispatch::get_turns
            <
                typename geometry::tag<Geometry1>::type,
                typename geometry::tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                false,
                false,
                turn_policy
            >::apply(0, geometry1, 1, geometry2,
                     strategy, robust_policy,
                     turns, interrupt_policy);

        return intersection_output_turn_points(turns, out);
    }
};

template <typename PointOut>
struct intersection_areal_linear_point
{
    template
    <
        typename Geometry1, typename Geometry2,
        typename RobustPolicy,
        typename OutputIterator,
        typename Strategy
    >
    static inline OutputIterator apply(Geometry1 const& geometry1,
                                       Geometry2 const& geometry2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return intersection_linear_areal_point
            <
                PointOut
            >::apply(geometry2, geometry1, robust_policy, out, strategy);
    }
};


}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    // real types
    typename Geometry1,
    typename Geometry2,
    typename GeometryOut,
    overlay_type OverlayType,
    // orientation
    bool Reverse1 = detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
    bool Reverse2 = detail::overlay::do_reverse<geometry::point_order<Geometry2>::value>::value,
    // tag dispatching:
    typename TagIn1 = typename geometry::tag<Geometry1>::type,
    typename TagIn2 = typename geometry::tag<Geometry2>::type,
    typename TagOut = typename detail::setop_insert_output_tag<GeometryOut>::type,
    // metafunction finetuning helpers:
    typename CastedTagIn1 = typename geometry::tag_cast<TagIn1, areal_tag, linear_tag, pointlike_tag>::type,
    typename CastedTagIn2 = typename geometry::tag_cast<TagIn2, areal_tag, linear_tag, pointlike_tag>::type,
    typename CastedTagOut = typename geometry::tag_cast<TagOut, areal_tag, linear_tag, pointlike_tag>::type
>
struct intersection_insert
{
    BOOST_GEOMETRY_STATIC_ASSERT_FALSE(
        "Not or not yet implemented for these Geometry types or their order.",
        Geometry1, Geometry2, GeometryOut,
        std::integral_constant<overlay_type, OverlayType>);
};


template
<
    typename Geometry1, typename Geometry2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2, typename TagOut
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, TagOut,
        areal_tag, areal_tag, areal_tag
    > : detail::overlay::overlay
        <
            Geometry1, Geometry2, Reverse1, Reverse2,
            detail::overlay::do_reverse<geometry::point_order<GeometryOut>::value>::value,
            GeometryOut, OverlayType
        >
{};


// Any areal type with box:
template
<
    typename Geometry, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn, typename TagOut
>
struct intersection_insert
    <
        Geometry, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        TagIn, box_tag, TagOut,
        areal_tag, areal_tag, areal_tag
    > : detail::overlay::overlay
        <
            Geometry, Box, Reverse1, Reverse2,
            detail::overlay::do_reverse<geometry::point_order<GeometryOut>::value>::value,
            GeometryOut, OverlayType
        >
{};


template
<
    typename Segment1, typename Segment2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Segment1, Segment2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        segment_tag, segment_tag, point_tag,
        linear_tag, linear_tag, pointlike_tag
    > : detail::intersection::intersection_segment_segment_point<GeometryOut>
{};


template
<
    typename Linestring1, typename Linestring2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Linestring1, Linestring2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        linestring_tag, linestring_tag, point_tag,
        linear_tag, linear_tag, pointlike_tag
    > : detail::intersection::intersection_linestring_linestring_point<GeometryOut>
{};


template
<
    typename Linestring, typename Box,
    typename GeometryOut,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Linestring, Box,
        GeometryOut,
        overlay_intersection,
        Reverse1, Reverse2,
        linestring_tag, box_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Linestring const& linestring,
            Box const& box,
            RobustPolicy const& robust_policy,
            OutputIterator out, Strategy const& )
    {
        typedef typename point_type<GeometryOut>::type point_type;
        strategy::intersection::liang_barsky<Box, point_type> lb_strategy;
        return detail::intersection::clip_range_with_box
            <GeometryOut>(box, linestring, robust_policy, out, lb_strategy);
    }
};


template
<
    typename Linestring, typename Polygon,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReversePolygon
>
struct intersection_insert
    <
        Linestring, Polygon,
        GeometryOut,
        OverlayType,
        ReverseLinestring, ReversePolygon,
        linestring_tag, polygon_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReversePolygon,
                GeometryOut,
                OverlayType,
                false
            >
{};


template
<
    typename Linestring, typename Ring,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReverseRing
>
struct intersection_insert
    <
        Linestring, Ring,
        GeometryOut,
        OverlayType,
        ReverseLinestring, ReverseRing,
        linestring_tag, ring_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReverseRing,
                GeometryOut,
                OverlayType,
                false
            >
{};

template
<
    typename Segment, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Segment, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        segment_tag, box_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Segment const& segment,
            Box const& box,
            RobustPolicy const& robust_policy,
            OutputIterator out, Strategy const& )
    {
        geometry::segment_view<Segment> range(segment);

        typedef typename point_type<GeometryOut>::type point_type;
        strategy::intersection::liang_barsky<Box, point_type> lb_strategy;
        return detail::intersection::clip_range_with_box
            <GeometryOut>(box, range, robust_policy, out, lb_strategy);
    }
};

template
<
    typename Geometry1, typename Geometry2,
    typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename Tag1, typename Tag2
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        PointOut,
        OverlayType,
        Reverse1, Reverse2,
        Tag1, Tag2, point_tag,
        areal_tag, areal_tag, pointlike_tag
    >
    : public detail::intersection::intersection_areal_areal_point
        <
            PointOut
        >
{};

template
<
    typename Geometry1, typename Geometry2,
    typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename Tag1, typename Tag2
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        PointOut,
        OverlayType,
        Reverse1, Reverse2,
        Tag1, Tag2, point_tag,
        linear_tag, areal_tag, pointlike_tag
    >
    : public detail::intersection::intersection_linear_areal_point
        <
            PointOut
        >
{};

template
<
    typename Geometry1, typename Geometry2,
    typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename Tag1, typename Tag2
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        PointOut,
        OverlayType,
        Reverse1, Reverse2,
        Tag1, Tag2, point_tag,
        areal_tag, linear_tag, pointlike_tag
    >
    : public detail::intersection::intersection_areal_linear_point
        <
            PointOut
        >
{};

template
<
    typename Geometry1, typename Geometry2, typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert_reversed
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Geometry1 const& g1,
                Geometry2 const& g2,
                RobustPolicy const& robust_policy,
                OutputIterator out,
                Strategy const& strategy)
    {
        return intersection_insert
            <
                Geometry2, Geometry1, GeometryOut,
                OverlayType,
                Reverse2, Reverse1
            >::apply(g2, g1, robust_policy, out, strategy);
    }
};


// dispatch for intersection(areal, areal, linear)
template
<
    typename Geometry1, typename Geometry2,
    typename LinestringOut,
    bool Reverse1, bool Reverse2,
    typename Tag1, typename Tag2
>
struct intersection_insert
    <
        Geometry1, Geometry2,
        LinestringOut,
        overlay_intersection,
        Reverse1, Reverse2,
        Tag1, Tag2, linestring_tag,
        areal_tag, areal_tag, linear_tag
    >
{
    template
    <
        typename RobustPolicy, typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Geometry1 const& geometry1,
                                       Geometry2 const& geometry2,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        detail::boundary_view<Geometry1 const> view1(geometry1);
        detail::boundary_view<Geometry2 const> view2(geometry2);

        return detail::overlay::linear_linear_linestring
            <
                detail::boundary_view<Geometry1 const>,
                detail::boundary_view<Geometry2 const>,
                LinestringOut,
                overlay_intersection
            >::apply(view1, view2, robust_policy, oit, strategy);
    }
};

// dispatch for difference/intersection of linear geometries
template
<
    typename Linear1, typename Linear2, typename LineStringOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2
>
struct intersection_insert
    <
        Linear1, Linear2, LineStringOut, OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, linestring_tag,
        linear_tag, linear_tag, linear_tag
    > : detail::overlay::linear_linear_linestring
        <
            Linear1, Linear2, LineStringOut, OverlayType
        >
{};

template
<
    typename Linear1, typename Linear2, typename TupledOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2
>
struct intersection_insert
    <
        Linear1, Linear2, TupledOut, OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, detail::tupled_output_tag,
        linear_tag, linear_tag, detail::tupled_output_tag
    >
    : detail::expect_output
        <
            Linear1, Linear2, TupledOut,
            // NOTE: points can be the result only in case of intersection.
            std::conditional_t
                <
                    (OverlayType == overlay_intersection),
                    point_tag,
                    void
                >,
            linestring_tag
        >
{
    // NOTE: The order of geometries in TupledOut tuple/pair must correspond to the order
    // iterators in OutputIterators tuple/pair.
    template
    <
        typename RobustPolicy, typename OutputIterators, typename Strategy
    >
    static inline OutputIterators apply(Linear1 const& linear1,
                                        Linear2 const& linear2,
                                        RobustPolicy const& robust_policy,
                                        OutputIterators oit,
                                        Strategy const& strategy)
    {
        return detail::overlay::linear_linear_linestring
            <
                Linear1, Linear2, TupledOut, OverlayType
            >::apply(linear1, linear2, robust_policy, oit, strategy);
    }
};


// dispatch for difference/intersection of point-like geometries

template
<
    typename Point1, typename Point2, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Point1, Point2, PointOut, OverlayType,
        Reverse1, Reverse2,
        point_tag, point_tag, point_tag,
        pointlike_tag, pointlike_tag, pointlike_tag
    > : detail::overlay::point_point_point
        <
            Point1, Point2, PointOut, OverlayType
        >
{};


template
<
    typename MultiPoint, typename Point, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        MultiPoint, Point, PointOut, OverlayType,
        Reverse1, Reverse2,
        multi_point_tag, point_tag, point_tag,
        pointlike_tag, pointlike_tag, pointlike_tag
    > : detail::overlay::multipoint_point_point
        <
            MultiPoint, Point, PointOut, OverlayType
        >
{};


template
<
    typename Point, typename MultiPoint, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Point, MultiPoint, PointOut, OverlayType,
        Reverse1, Reverse2,
        point_tag, multi_point_tag, point_tag,
        pointlike_tag, pointlike_tag, pointlike_tag
    > : detail::overlay::point_multipoint_point
        <
            Point, MultiPoint, PointOut, OverlayType
        >
{};


template
<
    typename MultiPoint1, typename MultiPoint2, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        MultiPoint1, MultiPoint2, PointOut, OverlayType,
        Reverse1, Reverse2,
        multi_point_tag, multi_point_tag, point_tag,
        pointlike_tag, pointlike_tag, pointlike_tag
    > : detail::overlay::multipoint_multipoint_point
        <
            MultiPoint1, MultiPoint2, PointOut, OverlayType
        >
{};


template
<
    typename PointLike1, typename PointLike2, typename TupledOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2
>
struct intersection_insert
    <
        PointLike1, PointLike2, TupledOut, OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, detail::tupled_output_tag,
        pointlike_tag, pointlike_tag, detail::tupled_output_tag
    >
    : detail::expect_output<PointLike1, PointLike2, TupledOut, point_tag>
{
    // NOTE: The order of geometries in TupledOut tuple/pair must correspond to the order
    // of iterators in OutputIterators tuple/pair.
    template
    <
        typename RobustPolicy, typename OutputIterators, typename Strategy
    >
    static inline OutputIterators apply(PointLike1 const& pointlike1,
                                        PointLike2 const& pointlike2,
                                        RobustPolicy const& robust_policy,
                                        OutputIterators oits,
                                        Strategy const& strategy)
    {
        namespace bgt = boost::geometry::tuples;

        static const bool out_point_index = bgt::find_index_if
            <
                TupledOut, geometry::detail::is_tag_same_as_pred<point_tag>::template pred
            >::value;

        bgt::get<out_point_index>(oits) = intersection_insert
            <
                PointLike1, PointLike2,
                typename bgt::element
                    <
                        out_point_index, TupledOut
                    >::type,
                OverlayType
            >::apply(pointlike1, pointlike2, robust_policy,
                     bgt::get<out_point_index>(oits),
                     strategy);

        return oits;
    }
};


// dispatch for difference/intersection of pointlike-linear geometries
template
<
    typename Point, typename Linear, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename Tag
>
struct intersection_insert
    <
        Point, Linear, PointOut, OverlayType,
        Reverse1, Reverse2,
        point_tag, Tag, point_tag,
        pointlike_tag, linear_tag, pointlike_tag
    > : detail_dispatch::overlay::pointlike_linear_point
        <
            Point, Linear, PointOut, OverlayType,
            point_tag, typename tag_cast<Tag, segment_tag, linear_tag>::type
        >
{};


template
<
    typename MultiPoint, typename Linear, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename Tag
>
struct intersection_insert
    <
        MultiPoint, Linear, PointOut, OverlayType,
        Reverse1, Reverse2,
        multi_point_tag, Tag, point_tag,
        pointlike_tag, linear_tag, pointlike_tag
    > : detail_dispatch::overlay::pointlike_linear_point
        <
            MultiPoint, Linear, PointOut, OverlayType,
            multi_point_tag,
            typename tag_cast<Tag, segment_tag, linear_tag>::type
        >
{};


// This specialization is needed because intersection() reverses the arguments
// for MultiPoint/Linestring combination.
template
<
    typename Linestring, typename MultiPoint, typename PointOut,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Linestring, MultiPoint, PointOut, overlay_intersection,
        Reverse1, Reverse2,
        linestring_tag, multi_point_tag, point_tag,
        linear_tag, pointlike_tag, pointlike_tag
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Linestring const& linestring,
                                       MultiPoint const& multipoint,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return detail_dispatch::overlay::pointlike_linear_point
            <
                MultiPoint, Linestring, PointOut, overlay_intersection,
                multi_point_tag, linear_tag
            >::apply(multipoint, linestring, robust_policy, out, strategy);
    }
};


template
<
    typename PointLike, typename Linear, typename TupledOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2
>
struct intersection_insert
    <
        PointLike, Linear, TupledOut, OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, detail::tupled_output_tag,
        pointlike_tag, linear_tag, detail::tupled_output_tag
    >
    // Reuse the implementation for PointLike/PointLike.
    : intersection_insert
        <
            PointLike, Linear, TupledOut, OverlayType,
            Reverse1, Reverse2,
            TagIn1, TagIn2, detail::tupled_output_tag,
            pointlike_tag, pointlike_tag, detail::tupled_output_tag
        >
{};


// This specialization is needed because intersection() reverses the arguments
// for MultiPoint/Linestring combination.
template
<
    typename Linestring, typename MultiPoint, typename TupledOut,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Linestring, MultiPoint, TupledOut, overlay_intersection,
        Reverse1, Reverse2,
        linestring_tag, multi_point_tag, detail::tupled_output_tag,
        linear_tag, pointlike_tag, detail::tupled_output_tag
    >
{
    template <typename RobustPolicy, typename OutputIterators, typename Strategy>
    static inline OutputIterators apply(Linestring const& linestring,
                                        MultiPoint const& multipoint,
                                        RobustPolicy const& robust_policy,
                                        OutputIterators out,
                                        Strategy const& strategy)
    {
        return intersection_insert
            <
                MultiPoint, Linestring, TupledOut, overlay_intersection
            >::apply(multipoint, linestring, robust_policy, out, strategy);
    }
};


// dispatch for difference/intersection of pointlike-areal geometries
template
<
    typename Point, typename Areal, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename ArealTag
>
struct intersection_insert
    <
        Point, Areal, PointOut, OverlayType,
        Reverse1, Reverse2,
        point_tag, ArealTag, point_tag,
        pointlike_tag, areal_tag, pointlike_tag
    > : detail_dispatch::overlay::pointlike_areal_point
        <
            Point, Areal, PointOut, OverlayType,
            point_tag, ArealTag
        >
{};

template
<
    typename MultiPoint, typename Areal, typename PointOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename ArealTag
>
struct intersection_insert
    <
        MultiPoint, Areal, PointOut, OverlayType,
        Reverse1, Reverse2,
        multi_point_tag, ArealTag, point_tag,
        pointlike_tag, areal_tag, pointlike_tag
    > : detail_dispatch::overlay::pointlike_areal_point
        <
            MultiPoint, Areal, PointOut, OverlayType,
            multi_point_tag, ArealTag
        >
{};

// This specialization is needed because intersection() reverses the arguments
// for MultiPoint/Ring and MultiPoint/Polygon combinations.
template
<
    typename Areal, typename MultiPoint, typename PointOut,
    bool Reverse1, bool Reverse2,
    typename ArealTag
>
struct intersection_insert
    <
        Areal, MultiPoint, PointOut, overlay_intersection,
        Reverse1, Reverse2,
        ArealTag, multi_point_tag, point_tag,
        areal_tag, pointlike_tag, pointlike_tag
    >
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Areal const& areal,
                                       MultiPoint const& multipoint,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator out,
                                       Strategy const& strategy)
    {
        return detail_dispatch::overlay::pointlike_areal_point
            <
                MultiPoint, Areal, PointOut, overlay_intersection,
                multi_point_tag, ArealTag
            >::apply(multipoint, areal, robust_policy, out, strategy);
    }
};


template
<
    typename PointLike, typename Areal, typename TupledOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2,
    typename TagIn1, typename TagIn2
>
struct intersection_insert
    <
        PointLike, Areal, TupledOut, OverlayType,
        Reverse1, Reverse2,
        TagIn1, TagIn2, detail::tupled_output_tag,
        pointlike_tag, areal_tag, detail::tupled_output_tag
    >
    // Reuse the implementation for PointLike/PointLike.
    : intersection_insert
        <
            PointLike, Areal, TupledOut, OverlayType,
            Reverse1, Reverse2,
            TagIn1, TagIn2, detail::tupled_output_tag,
            pointlike_tag, pointlike_tag, detail::tupled_output_tag
        >
{};


// This specialization is needed because intersection() reverses the arguments
// for MultiPoint/Ring and MultiPoint/Polygon combinations.
template
<
    typename Areal, typename MultiPoint, typename TupledOut,
    bool Reverse1, bool Reverse2,
    typename TagIn1
>
struct intersection_insert
    <
        Areal, MultiPoint, TupledOut, overlay_intersection,
        Reverse1, Reverse2,
        TagIn1, multi_point_tag, detail::tupled_output_tag,
        areal_tag, pointlike_tag, detail::tupled_output_tag
    >
{
    template <typename RobustPolicy, typename OutputIterators, typename Strategy>
    static inline OutputIterators apply(Areal const& areal,
                                        MultiPoint const& multipoint,
                                        RobustPolicy const& robust_policy,
                                        OutputIterators out,
                                        Strategy const& strategy)
    {
        return intersection_insert
            <
                MultiPoint, Areal, TupledOut, overlay_intersection
            >::apply(multipoint, areal, robust_policy, out, strategy);
    }
};


template
<
    typename Linestring, typename Polygon,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReversePolygon
>
struct intersection_insert
    <
        Linestring, Polygon,
        TupledOut,
        OverlayType,
        ReverseLinestring, ReversePolygon,
        linestring_tag, polygon_tag, detail::tupled_output_tag,
        linear_tag, areal_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReversePolygon,
                TupledOut,
                OverlayType,
                true
            >
{};

template
<
    typename Linestring, typename Ring,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReverseRing
>
struct intersection_insert
    <
        Linestring, Ring,
        TupledOut,
        OverlayType,
        ReverseLinestring, ReverseRing,
        linestring_tag, ring_tag, detail::tupled_output_tag,
        linear_tag, areal_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReverseRing,
                TupledOut,
                OverlayType,
                true
            >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{


template
<
    typename GeometryOut,
    bool ReverseSecond,
    overlay_type OverlayType,
    typename Geometry1, typename Geometry2,
    typename RobustPolicy,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            RobustPolicy robust_policy,
            OutputIterator out,
            Strategy const& strategy)
{
    return std::conditional_t
        <
            geometry::reverse_dispatch<Geometry1, Geometry2>::type::value,
            geometry::dispatch::intersection_insert_reversed
            <
                Geometry1, Geometry2,
                GeometryOut,
                OverlayType,
                overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
                overlay::do_reverse<geometry::point_order<Geometry2>::value, ReverseSecond>::value
            >,
            geometry::dispatch::intersection_insert
            <
                Geometry1, Geometry2,
                GeometryOut,
                OverlayType,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value,
                geometry::detail::overlay::do_reverse<geometry::point_order<Geometry2>::value, ReverseSecond>::value
            >
        >::apply(geometry1, geometry2, robust_policy, out, strategy);
}


/*!
\brief \brief_calc2{intersection} \brief_strategy
\ingroup intersection
\details \details_calc2{intersection_insert, spatial set theoretic intersection}
    \brief_strategy. \details_insert{intersection}
\tparam GeometryOut \tparam_geometry{\p_l_or_c}
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam OutputIterator \tparam_out{\p_l_or_c}
\tparam Strategy \tparam_strategy_overlay
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{intersection}
\param strategy \param_strategy{intersection}
\return \return_out

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename OutputIterator,
    typename Strategy
>
inline OutputIterator intersection_insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out,
            Strategy const& strategy)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    typedef typename geometry::rescale_overlay_policy_type
        <
            Geometry1,
            Geometry2,
            typename Strategy::cs_tag
        >::type rescale_policy_type;

    rescale_policy_type robust_policy
            = geometry::get_rescale_policy<rescale_policy_type>(
                geometry1, geometry2, strategy);

    return detail::intersection::insert
        <
            GeometryOut, false, overlay_intersection
        >(geometry1, geometry2, robust_policy, out, strategy);
}


/*!
\brief \brief_calc2{intersection}
\ingroup intersection
\details \details_calc2{intersection_insert, spatial set theoretic intersection}.
    \details_insert{intersection}
\tparam GeometryOut \tparam_geometry{\p_l_or_c}
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\tparam OutputIterator \tparam_out{\p_l_or_c}
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\param out \param_out{intersection}
\return \return_out

\qbk{[include reference/algorithms/intersection.qbk]}
*/
template
<
    typename GeometryOut,
    typename Geometry1,
    typename Geometry2,
    typename OutputIterator
>
inline OutputIterator intersection_insert(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out)
{
    concepts::check<Geometry1 const>();
    concepts::check<Geometry2 const>();

    typedef typename strategies::relate::services::default_strategy
        <
            Geometry1, Geometry2
        >::type strategy_type;
    
    return intersection_insert<GeometryOut>(geometry1, geometry2, out,
                                            strategy_type());
}

}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_INTERSECTION_INSERT_HPP
