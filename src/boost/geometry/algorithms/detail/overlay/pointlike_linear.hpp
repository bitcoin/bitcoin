// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2017 Adam Wulkiewicz, Lodz, Poland.

// Copyright (c) 2015-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_LINEAR_HPP

#include <iterator>
#include <vector>

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/envelope.hpp>
#include <boost/geometry/algorithms/expand.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/algorithms/detail/not.hpp>
#include <boost/geometry/algorithms/detail/partition.hpp>
#include <boost/geometry/algorithms/detail/disjoint/point_geometry.hpp>
#include <boost/geometry/algorithms/detail/equals/point_point.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/pointlike_pointlike.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/iterators/segment_iterator.hpp>

// TEMP
#include <boost/geometry/strategies/envelope/cartesian.hpp>
#include <boost/geometry/strategies/envelope/geographic.hpp>
#include <boost/geometry/strategies/envelope/spherical.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


// difference/intersection of point-linear
template
<
    typename Point,
    typename Geometry,
    typename PointOut,
    overlay_type OverlayType,
    typename Policy
>
struct point_single_point
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(Point const& point,
                                       Geometry const& geometry,
                                       RobustPolicy const&,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        action_selector_pl
            <
                PointOut, OverlayType
            >::apply(point, Policy::apply(point, geometry, strategy), oit);
        return oit;
    }
};

// difference/intersection of multipoint-segment
template
<
    typename MultiPoint,
    typename Geometry,
    typename PointOut,
    overlay_type OverlayType,
    typename Policy
>
struct multipoint_single_point
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(MultiPoint const& multipoint,
                                       Geometry const& geometry,
                                       RobustPolicy const&,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        for (typename boost::range_iterator<MultiPoint const>::type
                 it = boost::begin(multipoint);
             it != boost::end(multipoint);
             ++it)
        {
            action_selector_pl
                <
                    PointOut, OverlayType
                >::apply(*it, Policy::apply(*it, geometry, strategy), oit);
        }

        return oit;
    }
};


// difference/intersection of multipoint-linear
template
<
    typename MultiPoint,
    typename Linear,
    typename PointOut,
    overlay_type OverlayType,
    typename Policy
>
class multipoint_linear_point
{
private:
    // structs for partition -- start
    template <typename Strategy>
    struct expand_box_point
    {
        expand_box_point(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Box, typename Point>
        inline void apply(Box& total, Point const& point) const
        {
            geometry::expand(total, point, m_strategy);
        }

        Strategy const& m_strategy;
    };

    template <typename Strategy>
    struct expand_box_segment
    {
        explicit expand_box_segment(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Box, typename Segment>
        inline void apply(Box& total, Segment const& segment) const
        {
            geometry::expand(total,
                             geometry::return_envelope<Box>(segment, m_strategy),
                             m_strategy);
        }

        Strategy const& m_strategy;
    };

    template <typename Strategy>
    struct overlaps_box_point
    {
        explicit overlaps_box_point(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Box, typename Point>
        inline bool apply(Box const& box, Point const& point) const
        {
            return ! geometry::disjoint(point, box, m_strategy);
        }

        Strategy const& m_strategy;
    };

    template <typename Strategy>
    struct overlaps_box_segment
    {
        explicit overlaps_box_segment(Strategy const& strategy)
            : m_strategy(strategy)
        {}

        template <typename Box, typename Segment>
        inline bool apply(Box const& box, Segment const& segment) const
        {
            return ! geometry::disjoint(segment, box, m_strategy);
        }

        Strategy const& m_strategy;
    };

    template <typename OutputIterator, typename Strategy>
    class item_visitor_type
    {
    public:
        item_visitor_type(OutputIterator& oit, Strategy const& strategy)
            : m_oit(oit)
            , m_strategy(strategy)
        {}

        template <typename Item1, typename Item2>
        inline bool apply(Item1 const& item1, Item2 const& item2)
        {
            action_selector_pl
                <
                    PointOut, overlay_intersection
                >::apply(item1, Policy::apply(item1, item2, m_strategy), m_oit);

            return true;
        }

    private:
        OutputIterator& m_oit;
        Strategy const& m_strategy;
    };
    // structs for partition -- end

    class segment_range
    {
    public:
        typedef geometry::segment_iterator<Linear const> const_iterator;
        typedef const_iterator iterator;

        explicit segment_range(Linear const& linear)
            : m_linear(linear)
        {}

        const_iterator begin() const
        {
            return geometry::segments_begin(m_linear);
        }

        const_iterator end() const
        {
            return geometry::segments_end(m_linear);
        }

    private:
        Linear const& m_linear;
    };

    template <typename OutputIterator, typename Strategy>
    static inline OutputIterator get_common_points(MultiPoint const& multipoint,
                                                   Linear const& linear,
                                                   OutputIterator oit,
                                                   Strategy const& strategy)
    {
        item_visitor_type<OutputIterator, Strategy> item_visitor(oit, strategy);

        // TODO: disjoint Segment/Box may be called in partition multiple times
        // possibly for non-cartesian segments which could be slow. We should consider
        // passing a range of bounding boxes of segments after calculating them once.
        // Alternatively instead of a range of segments a range of Segment/Envelope pairs
        // should be passed, where envelope would be lazily calculated when needed the first time
        geometry::partition
            <
                geometry::model::box
                    <
                        typename boost::range_value<MultiPoint>::type
                    >
            >::apply(multipoint, segment_range(linear), item_visitor,
                     expand_box_point<Strategy>(strategy),
                     overlaps_box_point<Strategy>(strategy),
                     expand_box_segment<Strategy>(strategy),
                     overlaps_box_segment<Strategy>(strategy));

        return oit;
    }

public:
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(MultiPoint const& multipoint,
                                       Linear const& linear,
                                       RobustPolicy const& robust_policy,
                                       OutputIterator oit,
                                       Strategy const& strategy)
    {
        typedef std::vector
            <
                typename boost::range_value<MultiPoint>::type
            > point_vector_type;

        point_vector_type common_points;

        // compute the common points
        get_common_points(multipoint, linear,
                          std::back_inserter(common_points),
                          strategy);

        return multipoint_multipoint_point
            <
                MultiPoint, point_vector_type, PointOut, OverlayType
            >::apply(multipoint, common_points, robust_policy, oit, strategy);
    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace detail_dispatch { namespace overlay
{

// dispatch struct for pointlike-linear difference/intersection computation
template
<
    typename PointLike,
    typename Linear,
    typename PointOut,
    overlay_type OverlayType,
    typename Tag1,
    typename Tag2
>
struct pointlike_linear_point
    : not_implemented<PointLike, Linear, PointOut>
{};


template
<
    typename Point,
    typename Linear,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_linear_point
    <
        Point, Linear, PointOut, OverlayType, point_tag, linear_tag
    > : detail::overlay::point_single_point
        <
            Point, Linear, PointOut, OverlayType,
            detail::not_<detail::disjoint::reverse_covered_by>
        >
{};


template
<
    typename Point,
    typename Segment,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_linear_point
    <
        Point, Segment, PointOut, OverlayType, point_tag, segment_tag
    > : detail::overlay::point_single_point
        <
            Point, Segment, PointOut, OverlayType,
            detail::not_<detail::disjoint::reverse_covered_by>
        >
{};


template
<
    typename MultiPoint,
    typename Linear,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_linear_point
    <
        MultiPoint, Linear, PointOut, OverlayType, multi_point_tag, linear_tag
    > : detail::overlay::multipoint_linear_point
        <
            MultiPoint, Linear, PointOut, OverlayType,
            detail::not_<detail::disjoint::reverse_covered_by>
        >
{};


template
<
    typename MultiPoint,
    typename Segment,
    typename PointOut,
    overlay_type OverlayType
>
struct pointlike_linear_point
    <
        MultiPoint, Segment, PointOut, OverlayType, multi_point_tag, segment_tag
    > : detail::overlay::multipoint_single_point
        <
            MultiPoint, Segment, PointOut, OverlayType,
            detail::not_<detail::disjoint::reverse_covered_by>
        >
{};


}} // namespace detail_dispatch::overlay
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_POINTLIKE_LINEAR_HPP
