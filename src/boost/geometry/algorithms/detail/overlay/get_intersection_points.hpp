// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2017-2020.
// Modifications copyright (c) 2017-2020 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP


#include <cstddef>
#include <type_traits>

#include <boost/range/value_type.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/policies/robustness/rescale_policy_tags.hpp>

#include <boost/geometry/geometries/segment.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace get_intersection_points
{


template
<
    typename Point1,
    typename Point2,
    typename TurnInfo
>
struct get_turn_without_info
{
    template
    <
        typename UniqueSubRange1,
        typename UniqueSubRange2,
        typename Strategy,
        typename RobustPolicy,
        typename OutputIterator
    >
    static inline OutputIterator apply(UniqueSubRange1 const& range_p,
                UniqueSubRange2 const& range_q,
                TurnInfo const& ,
                Strategy const& strategy,
                RobustPolicy const& ,
                OutputIterator out)
    {
        // Make sure this is only called with no rescaling
        BOOST_STATIC_ASSERT((std::is_same
           <
               no_rescale_policy_tag,
               typename rescale_policy_type<RobustPolicy>::type
           >::value));

        typedef typename TurnInfo::point_type turn_point_type;

        typedef policies::relate::segments_intersection_points
            <
                segment_intersection_points<turn_point_type>
            > policy_type;

        typename policy_type::return_type const result
            = strategy.relate().apply(range_p, range_q, policy_type());

        for (std::size_t i = 0; i < result.count; i++)
        {
            TurnInfo tp;
            geometry::convert(result.intersections[i], tp.point);
            *out++ = tp;
        }

        return out;
    }
};

}} // namespace detail::get_intersection_points
#endif // DOXYGEN_NO_DETAIL




template
<
    typename Geometry1,
    typename Geometry2,
    typename RobustPolicy,
    typename Turns,
    typename Strategy
>
inline void get_intersection_points(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            RobustPolicy const& robust_policy,
            Turns& turns,
            Strategy const& strategy)
{
    concepts::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2 const>();

    typedef detail::get_intersection_points::get_turn_without_info
                        <
                            typename point_type<Geometry1>::type,
                            typename point_type<Geometry2>::type,
                            typename boost::range_value<Turns>::type
                        > TurnPolicy;

    detail::get_turns::no_interrupt_policy interrupt_policy;

    std::conditional_t
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::get_turns_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                false, false,
                TurnPolicy
            >,
            dispatch::get_turns
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1, Geometry2,
                false, false,
                TurnPolicy
            >
        >::apply(0, geometry1,
                 1, geometry2,
                 strategy,
                 robust_policy,
                 turns, interrupt_policy);
}




}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_INTERSECTION_POINTS_HPP
