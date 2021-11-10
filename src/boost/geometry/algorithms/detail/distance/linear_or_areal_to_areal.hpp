// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014-2021, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_OR_AREAL_TO_AREAL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_OR_AREAL_TO_AREAL_HPP

#include <boost/geometry/algorithms/detail/distance/linear_to_linear.hpp>
#include <boost/geometry/algorithms/detail/distance/strategy_utils.hpp>
#include <boost/geometry/algorithms/intersects.hpp>

#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/strategies/distance.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename Linear, typename Areal, typename Strategies>
struct linear_to_areal
{
    typedef distance::return_t<Linear, Areal, Strategies> return_type;

    static inline return_type apply(Linear const& linear,
                                    Areal const& areal,
                                    Strategies const& strategies)
    {
        if ( geometry::intersects(linear, areal, strategies) )
        {
            return return_type(0);
        }

        return linear_to_linear
            <
                Linear, Areal, Strategies
            >::apply(linear, areal, strategies, false);
    }


    static inline return_type apply(Areal const& areal,
                                    Linear const& linear,
                                    Strategies const& strategies)
    {
        return apply(linear, areal, strategies);
    }
};

template <typename Areal1, typename Areal2, typename Strategies>
struct areal_to_areal
{
    typedef distance::return_t<Areal1, Areal2, Strategies> return_type;

    static inline return_type apply(Areal1 const& areal1,
                                    Areal2 const& areal2,
                                    Strategies const& strategies)
    {
        if ( geometry::intersects(areal1, areal2, strategies) )
        {
            return return_type(0);
        }

        return linear_to_linear
            <
                Areal1, Areal2, Strategies
            >::apply(areal1, areal2, strategies, false);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Linear, typename Areal, typename Strategy>
struct distance
    <
        Linear, Areal, Strategy,
        linear_tag, areal_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::linear_to_areal
        <
            Linear, Areal, Strategy
        >
{};

template <typename Areal, typename Linear, typename Strategy>
struct distance
    <
        Areal, Linear, Strategy,
        areal_tag, linear_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::linear_to_areal
        <
            Linear, Areal, Strategy
        >
{};


template <typename Areal1, typename Areal2, typename Strategy>
struct distance
    <
        Areal1, Areal2, Strategy,
        areal_tag, areal_tag, 
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::areal_to_areal
        <
            Areal1, Areal2, Strategy
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_LINEAR_OR_AREAL_TO_AREAL_HPP
