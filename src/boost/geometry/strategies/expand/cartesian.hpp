// Boost.Geometry

// Copyright (c) 2020, Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_EXPAND_CARTESIAN_HPP
#define BOOST_GEOMETRY_STRATEGIES_EXPAND_CARTESIAN_HPP


#include <type_traits>

#include <boost/geometry/strategy/cartesian/expand_box.hpp>
#include <boost/geometry/strategy/cartesian/expand_point.hpp>
#include <boost/geometry/strategy/cartesian/expand_segment.hpp>

#include <boost/geometry/strategies/detail.hpp>
#include <boost/geometry/strategies/expand/services.hpp>


namespace boost { namespace geometry
{


namespace strategies { namespace expand
{


template <typename CalculationType = void>
struct cartesian
    : strategies::detail::cartesian_base
{
    template <typename Box, typename Geometry>
    static auto expand(Box const&, Geometry const&,
                       typename util::enable_if_point_t<Geometry> * = nullptr)
    {
        return strategy::expand::cartesian_point();
    }

    template <typename Box, typename Geometry>
    static auto expand(Box const&, Geometry const&,
                       typename util::enable_if_box_t<Geometry> * = nullptr)
    {
        return strategy::expand::cartesian_box();
    }

    template <typename Box, typename Geometry>
    static auto expand(Box const&, Geometry const&,
                       typename util::enable_if_segment_t<Geometry> * = nullptr)
    {
        return strategy::expand::cartesian_segment();
    }
};


namespace services
{

template <typename Box, typename Geometry>
struct default_strategy<Box, Geometry, cartesian_tag>
{
    using type = strategies::expand::cartesian<>;
};


template <>
struct strategy_converter<strategy::expand::cartesian_point>
{
    static auto get(strategy::expand::cartesian_point const& )
    {
        return strategies::expand::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::expand::cartesian_box>
{
    static auto get(strategy::expand::cartesian_box const& )
    {
        return strategies::expand::cartesian<>();
    }
};

template <>
struct strategy_converter<strategy::expand::cartesian_segment>
{
    static auto get(strategy::expand::cartesian_segment const&)
    {
        return strategies::expand::cartesian<>();
    }
};


} // namespace services

}} // namespace strategies::envelope

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_EXPAND_CARTESIAN_HPP
