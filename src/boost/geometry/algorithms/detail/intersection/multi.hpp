// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014, 2020.
// Modifications copyright (c) 2014-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_MULTI_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_MULTI_HPP

#include <type_traits>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

// TODO: those headers probably may be removed
#include <boost/geometry/algorithms/detail/overlay/get_ring.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segments.hpp>
#include <boost/geometry/algorithms/detail/overlay/copy_segment_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>
#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>

#include <boost/geometry/algorithms/detail/intersection/interface.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/num_points.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersection
{


template <typename PointOut>
struct intersection_multi_linestring_multi_linestring_point
{
    template
    <
        typename MultiLinestring1, typename MultiLinestring2,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(MultiLinestring1 const& ml1,
            MultiLinestring2 const& ml2,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        // Note, this loop is quadratic w.r.t. number of linestrings per input.
        // Future Enhancement: first do the sections of each, then intersect.
        for (typename boost::range_iterator
                <
                    MultiLinestring1 const
                >::type it1 = boost::begin(ml1);
            it1 != boost::end(ml1);
            ++it1)
        {
            for (typename boost::range_iterator
                    <
                        MultiLinestring2 const
                    >::type it2 = boost::begin(ml2);
                it2 != boost::end(ml2);
                ++it2)
            {
                out = intersection_linestring_linestring_point<PointOut>
                      ::apply(*it1, *it2, robust_policy, out, strategy);
            }
        }

        return out;
    }
};


template <typename PointOut>
struct intersection_linestring_multi_linestring_point
{
    template
    <
        typename Linestring, typename MultiLinestring,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Linestring const& linestring,
            MultiLinestring const& ml,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        for (typename boost::range_iterator
                <
                    MultiLinestring const
                >::type it = boost::begin(ml);
            it != boost::end(ml);
            ++it)
        {
            out = intersection_linestring_linestring_point<PointOut>
                  ::apply(linestring, *it, robust_policy, out, strategy);
        }

        return out;
    }
};


// This loop is quite similar to the loop above, but beacuse the iterator
// is second (above) or first (below) argument, it is not trivial to merge them.
template
<
    bool ReverseAreal,
    typename LineStringOut,
    overlay_type OverlayType,
    bool FollowIsolatedPoints
>
struct intersection_of_multi_linestring_with_areal
{
    template
    <
        typename MultiLinestring, typename Areal,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(MultiLinestring const& ml, Areal const& areal,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        for (typename boost::range_iterator
                <
                    MultiLinestring const
                >::type it = boost::begin(ml);
            it != boost::end(ml);
            ++it)
        {
            out = intersection_of_linestring_with_areal
                <
                    ReverseAreal, LineStringOut, OverlayType, FollowIsolatedPoints
                >::apply(*it, areal, robust_policy, out, strategy);
        }

        return out;

    }
};

// This one calls the one above with reversed arguments
template
<
    bool ReverseAreal,
    typename LineStringOut,
    overlay_type OverlayType,
    bool FollowIsolatedPoints
>
struct intersection_of_areal_with_multi_linestring
{
    template
    <
        typename Areal, typename MultiLinestring,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(Areal const& areal, MultiLinestring const& ml,
            RobustPolicy const& robust_policy,
            OutputIterator out,
            Strategy const& strategy)
    {
        return intersection_of_multi_linestring_with_areal
            <
                ReverseAreal, LineStringOut, OverlayType, FollowIsolatedPoints
            >::apply(ml, areal, robust_policy, out, strategy);
    }
};



template <typename LinestringOut>
struct clip_multi_linestring
{
    template
    <
        typename MultiLinestring, typename Box,
        typename RobustPolicy,
        typename OutputIterator, typename Strategy
    >
    static inline OutputIterator apply(MultiLinestring const& multi_linestring,
            Box const& box,
            RobustPolicy const& robust_policy,
            OutputIterator out, Strategy const& )
    {
        typedef typename point_type<LinestringOut>::type point_type;
        strategy::intersection::liang_barsky<Box, point_type> lb_strategy;
        for (typename boost::range_iterator<MultiLinestring const>::type it
            = boost::begin(multi_linestring);
            it != boost::end(multi_linestring); ++it)
        {
            out = detail::intersection::clip_range_with_box
                <LinestringOut>(box, *it, robust_policy, out, lb_strategy);
        }
        return out;
    }
};


}} // namespace detail::intersection
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// Linear
template
<
    typename MultiLinestring1, typename MultiLinestring2,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        MultiLinestring1, MultiLinestring2,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        multi_linestring_tag, multi_linestring_tag, point_tag,
        linear_tag, linear_tag, pointlike_tag
    > : detail::intersection::intersection_multi_linestring_multi_linestring_point
            <
                GeometryOut
            >
{};


template
<
    typename Linestring, typename MultiLinestring,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        Linestring, MultiLinestring,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        linestring_tag, multi_linestring_tag, point_tag,
        linear_tag, linear_tag, pointlike_tag
    > : detail::intersection::intersection_linestring_multi_linestring_point
            <
                GeometryOut
            >
{};


template
<
    typename MultiLinestring, typename Box,
    typename GeometryOut,
    overlay_type OverlayType,
    bool Reverse1, bool Reverse2
>
struct intersection_insert
    <
        MultiLinestring, Box,
        GeometryOut,
        OverlayType,
        Reverse1, Reverse2,
        multi_linestring_tag, box_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::clip_multi_linestring
            <
                GeometryOut
            >
{};


template
<
    typename Linestring, typename MultiPolygon,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseLinestring, bool ReverseMultiPolygon
>
struct intersection_insert
    <
        Linestring, MultiPolygon,
        GeometryOut,
        OverlayType,
        ReverseLinestring, ReverseMultiPolygon,
        linestring_tag, multi_polygon_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_linestring_with_areal
            <
                ReverseMultiPolygon,
                GeometryOut,
                OverlayType,
                false
            >
{};


// Derives from areal/mls because runtime arguments are in that order.
// areal/mls reverses it itself to mls/areal
template
<
    typename Polygon, typename MultiLinestring,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReversePolygon, bool ReverseMultiLinestring
>
struct intersection_insert
    <
        Polygon, MultiLinestring,
        GeometryOut,
        OverlayType,
        ReversePolygon, ReverseMultiLinestring,
        polygon_tag, multi_linestring_tag, linestring_tag,
        areal_tag, linear_tag, linear_tag
    > : detail::intersection::intersection_of_areal_with_multi_linestring
            <
                ReversePolygon,
                GeometryOut,
                OverlayType,
                false
            >
{};


template
<
    typename MultiLinestring, typename Ring,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReverseRing
>
struct intersection_insert
    <
        MultiLinestring, Ring,
        GeometryOut,
        OverlayType,
        ReverseMultiLinestring, ReverseRing,
        multi_linestring_tag, ring_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReverseRing,
                GeometryOut,
                OverlayType,
                false
            >
{};

template
<
    typename MultiLinestring, typename Polygon,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReversePolygon
>
struct intersection_insert
    <
        MultiLinestring, Polygon,
        GeometryOut,
        OverlayType,
        ReverseMultiLinestring, ReversePolygon,
        multi_linestring_tag, polygon_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReversePolygon,
                GeometryOut,
                OverlayType,
                false
            >
{};



template
<
    typename MultiLinestring, typename MultiPolygon,
    typename GeometryOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReverseMultiPolygon
>
struct intersection_insert
    <
        MultiLinestring, MultiPolygon,
        GeometryOut,
        OverlayType,
        ReverseMultiLinestring, ReverseMultiPolygon,
        multi_linestring_tag, multi_polygon_tag, linestring_tag,
        linear_tag, areal_tag, linear_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReverseMultiPolygon,
                GeometryOut,
                OverlayType,
                false
            >
{};



template
<
    typename MultiLinestring, typename Ring,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReverseRing
>
struct intersection_insert
    <
        MultiLinestring, Ring,
        TupledOut,
        OverlayType,
        ReverseMultiLinestring, ReverseRing,
        multi_linestring_tag, ring_tag, detail::tupled_output_tag,
        linear_tag, areal_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReverseRing,
                TupledOut,
                OverlayType,
                true
            >
      , detail::expect_output
            <
                MultiLinestring, Ring, TupledOut,
                // NOTE: points can be the result only in case of intersection.
                // TODO: union should require L and A
                std::conditional_t
                    <
                        (OverlayType == overlay_intersection),
                        point_tag,
                        void
                    >,
                linestring_tag
            >
{};


template
<
    typename MultiLinestring, typename Polygon,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReversePolygon
>
struct intersection_insert
    <
        MultiLinestring, Polygon,
        TupledOut,
        OverlayType,
        ReverseMultiLinestring, ReversePolygon,
        multi_linestring_tag, polygon_tag, detail::tupled_output_tag,
        linear_tag, areal_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReversePolygon,
                TupledOut,
                OverlayType,
                true
            >
      , detail::expect_output
            <
                MultiLinestring, Polygon, TupledOut,
                // NOTE: points can be the result only in case of intersection.
                // TODO: union should require L and A
                std::conditional_t
                    <
                        (OverlayType == overlay_intersection),
                        point_tag,
                        void
                    >,
                linestring_tag
            >
{};

template
<
    typename Polygon, typename MultiLinestring,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReversePolygon, bool ReverseMultiLinestring
>
struct intersection_insert
    <
        Polygon, MultiLinestring,
        TupledOut,
        OverlayType,
        ReversePolygon, ReverseMultiLinestring,
        polygon_tag, multi_linestring_tag, detail::tupled_output_tag,
        areal_tag, linear_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_areal_with_multi_linestring
            <
                ReversePolygon,
                TupledOut,
                OverlayType,
                true
            >
      , detail::expect_output
            <
                Polygon, MultiLinestring, TupledOut,
                // NOTE: points can be the result only in case of intersection.
                // TODO: union should require L and A
                // TODO: in general the result of difference should depend on the first argument
                //       but this specialization calls L/A in reality so the first argument is linear.
                //       So expect only L for difference?
                std::conditional_t
                    <
                        (OverlayType == overlay_intersection),
                        point_tag,
                        void
                    >,
                linestring_tag
            >
{};

template
<
    typename MultiLinestring, typename MultiPolygon,
    typename TupledOut,
    overlay_type OverlayType,
    bool ReverseMultiLinestring, bool ReverseMultiPolygon
>
struct intersection_insert
    <
        MultiLinestring, MultiPolygon,
        TupledOut,
        OverlayType,
        ReverseMultiLinestring, ReverseMultiPolygon,
        multi_linestring_tag, multi_polygon_tag, detail::tupled_output_tag,
        linear_tag, areal_tag, detail::tupled_output_tag
    > : detail::intersection::intersection_of_multi_linestring_with_areal
            <
                ReverseMultiPolygon,
                TupledOut,
                OverlayType,
                true
            >
      , detail::expect_output
            <
                MultiLinestring, MultiPolygon, TupledOut,
                // NOTE: points can be the result only in case of intersection.
                // TODO: union should require L and A
                std::conditional_t
                    <
                        (OverlayType == overlay_intersection),
                        point_tag,
                        void
                    >,
                linestring_tag
            >
{};


} // namespace dispatch
#endif

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTION_MULTI_HPP

