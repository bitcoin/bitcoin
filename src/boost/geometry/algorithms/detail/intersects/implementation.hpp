// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2013-2020.
// Modifications copyright (c) 2013-2020, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_IMPLEMENTATION_HPP


#include <deque>

#include <boost/geometry/algorithms/detail/intersects/interface.hpp>
#include <boost/geometry/algorithms/detail/disjoint/implementation.hpp>

#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>
#include <boost/geometry/policies/disjoint_interrupt_policy.hpp>
#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>

#include <boost/geometry/strategies/relate/services.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace intersects
{

template <typename Geometry>
struct self_intersects
{
    static bool apply(Geometry const& geometry)
    {
        concepts::check<Geometry const>();

        typedef typename geometry::point_type<Geometry>::type point_type;
        typedef typename strategies::relate::services::default_strategy
                <
                    Geometry, Geometry
                >::type strategy_type;

        typedef detail::overlay::turn_info<point_type> turn_info;

        std::deque<turn_info> turns;

        typedef detail::overlay::get_turn_info
            <
                detail::overlay::assign_null_policy
            > turn_policy;

        strategy_type strategy;

        detail::disjoint::disjoint_interrupt_policy policy;
    // TODO: skip_adjacent should be set to false
        detail::self_get_turn_points::get_turns
            <
                false, turn_policy
            >::apply(geometry, strategy, detail::no_rescale_policy(), turns, policy, 0, true);
        return policy.has_intersections;
    }
};

}} // namespace detail::intersects
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_INTERSECTS_IMPLEMENTATION_HPP
