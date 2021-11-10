// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle
// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_LINEAR_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_LINEAR_HPP

#include <cstddef>
#include <deque>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/segment_as_subrange.hpp>

#include <boost/geometry/policies/disjoint_interrupt_policy.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>

#include <boost/geometry/algorithms/dispatch/disjoint.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{

template <typename Segment1, typename Segment2>
struct disjoint_segment
{
    template <typename Strategy>
    static inline bool apply(Segment1 const& segment1, Segment2 const& segment2,
                             Strategy const& strategy)
    {
        typedef typename point_type<Segment1>::type point_type;

        typedef segment_intersection_points<point_type> intersection_return_type;

        typedef policies::relate::segments_intersection_points
            <
                intersection_return_type
            > intersection_policy;

        detail::segment_as_subrange<Segment1> sub_range1(segment1);
        detail::segment_as_subrange<Segment2> sub_range2(segment2);
        intersection_return_type is = strategy.relate().apply(sub_range1, sub_range2,
                                                              intersection_policy());

        return is.count == 0;
    }
};


struct assign_disjoint_policy
{
    // We want to include all points:
    static bool const include_no_turn = true;
    static bool const include_degenerate = true;
    static bool const include_opposite = true;
    static bool const include_start_turn = false;
};


template <typename Geometry1, typename Geometry2>
struct disjoint_linear
{
    template <typename Strategy>
    static inline bool apply(Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             Strategy const& strategy)
    {
        typedef typename geometry::point_type<Geometry1>::type point_type;
        typedef geometry::segment_ratio
            <
                typename coordinate_type<point_type>::type
            > ratio_type;
        typedef overlay::turn_info
            <
                point_type,
                ratio_type,
                typename detail::get_turns::turn_operation_type
                        <
                            Geometry1, Geometry2, ratio_type
                        >::type
            > turn_info_type;

        std::deque<turn_info_type> turns;

        // Specify two policies:
        // 1) Stop at any intersection
        // 2) In assignment, include also degenerate points (which are normally skipped)
        disjoint_interrupt_policy interrupt_policy;
        dispatch::get_turns
            <
                typename geometry::tag<Geometry1>::type,
                typename geometry::tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                overlay::do_reverse<geometry::point_order<Geometry1>::value>::value, // should be false
                overlay::do_reverse<geometry::point_order<Geometry2>::value>::value, // should be false
                detail::get_turns::get_turn_info_type
                    <
                        Geometry1, Geometry2, assign_disjoint_policy
                    >
            >::apply(0, geometry1, 1, geometry2,
                     strategy, detail::no_rescale_policy(), turns, interrupt_policy);

        return !interrupt_policy.has_intersections;
    }
};


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Linear1, typename Linear2>
struct disjoint<Linear1, Linear2, 2, linear_tag, linear_tag, false>
    : detail::disjoint::disjoint_linear<Linear1, Linear2>
{};


template <typename Segment1, typename Segment2>
struct disjoint<Segment1, Segment2, 2, segment_tag, segment_tag, false>
    : detail::disjoint::disjoint_segment<Segment1, Segment2>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_LINEAR_LINEAR_HPP
